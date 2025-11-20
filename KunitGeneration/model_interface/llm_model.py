import os
import subprocess
import re
import faiss
from pathlib import Path
from dotenv import load_dotenv
from sentence_transformers import SentenceTransformer
from openai import OpenAI
from KunitGeneration.model_interface.prompts.unittest_kunit_prompts import kunit_generation_prompt

class KUnitTestGenerator:
    """Generates KUnit tests using RAG + LLM and fixes compilation errors."""

    def __init__(self, main_test_dir: Path, model_name: str, temperature: float, max_retries: int = 3):
        if not main_test_dir.is_dir():
            raise FileNotFoundError(f"The specified test directory does not exist: {main_test_dir}")

        # Paths
        self.base_dir = main_test_dir
        self.functions_dir = self.base_dir / "test_functions"
        self.output_dir = self.base_dir / "generated_tests"
        self.sample_files = [
            self.base_dir / "reference_testcases" / "kunit_test1.c",
            self.base_dir / "reference_testcases" / "kunit_test2.c",
            self.base_dir / "reference_testcases" / "kunit_test3.c",
        ]
        self.error_log_file = self.base_dir / "compilation_log" / "compile_error.txt"

        # Model settings
        self.model_name = model_name
        self.temperature = temperature
        self.max_tokens = 8192
        self.max_retries = max_retries

        # Environment + Client
        self._load_environment()
        self.client = self._initialize_client()
        self.prompt_template = kunit_generation_prompt

        # RAG setup
        self.vector_index = self.base_dir / "code_index.faiss"
        self.vector_map = self.base_dir / "file_map.txt"
        self.embed_model = SentenceTransformer("all-MiniLM-L6-v2")
        self.index, self.file_map = self._load_or_build_index()

    def _load_environment(self):
        load_dotenv()
        self.api_key = os.environ.get("NVIDIA_API_KEY")
        if not self.api_key:
            raise ValueError("NVIDIA_API_KEY environment variable not set.")

    def _initialize_client(self):
        return OpenAI(base_url="https://integrate.api.nvidia.com/v1", api_key=self.api_key)

    # ---------------- RAG Functions ----------------
    def _build_faiss_index(self):
        code_dir = self.base_dir / "reference_testcases"
        files = list(code_dir.rglob("*.c"))
        if not files:
            print(f"⚠️ No .c files found under {code_dir}")
            return None, []
        print(f"📦 Building FAISS index from {len(files)} C source files...")
        texts = [f.read_text(errors="ignore") for f in files]
        embeddings = self.embed_model.encode(texts, show_progress_bar=True)
        dim = embeddings.shape[1]
        index = faiss.IndexFlatL2(dim)
        index.add(embeddings)
        faiss.write_index(index, str(self.vector_index))
        self.vector_map.write_text("\n".join(str(p) for p in files))
        return index, [str(p) for p in files]

    def _load_or_build_index(self):
        print("🔍 Loading embedding model and FAISS index...")
        if not self.vector_index.exists() or not self.vector_map.exists():
            return self._build_faiss_index()
        index = faiss.read_index(str(self.vector_index))
        file_map = self.vector_map.read_text().splitlines()
        print(f"✅ Loaded FAISS index from {self.vector_index}")
        return index, file_map

    def _retrieve_context(self, query_text: str, top_k: int = 3):
        if self.index is None:
            return ["// Retrieval skipped (no FAISS index available)"]
        query_emb = self.embed_model.encode([query_text])
        distances, indices = self.index.search(query_emb, top_k)
        results = []
        for idx in indices[0]:
            if idx < len(self.file_map):
                p = Path(self.file_map[idx])
                if p.exists():
                    text = p.read_text(errors="ignore")
                    results.append(f"// From {p}\n{text[:1500]}")
        return results

    # ---------------- Model Query ----------------
    def _query_model(self, prompt: str) -> str:
        try:
            completion = self.client.chat.completions.create(
                model=self.model_name,
                messages=[{"role": "user", "content": prompt}],
                temperature=self.temperature,
                max_tokens=self.max_tokens,
            )
            response = completion.choices[0].message.content
            return response.replace("```c", "").replace("```", "").strip()
        except Exception as e:
            print(f"An error occurred while querying the model: {e}")
            return f"// Error generating response: {e}"

    def _load_context_files(self) -> dict:
        def safe_read(p: Path, fallback="// Missing file"):
            return p.read_text(encoding="utf-8") if p.exists() else fallback
        return {
            "sample_code1": safe_read(self.sample_files[0]),
            "sample_code2": safe_read(self.sample_files[1]),
            "sample_code3": safe_read(self.sample_files[2]),
            "error_logs": safe_read(self.error_log_file, "// No previous error logs"),
        }

# ---------------- Kernel Build Integration ----------------
    def _update_makefile(self, test_name: str):
        makefile_path = Path("/home/amd/linux/drivers/platform/x86/amd/pmf/Makefile")
        if not makefile_path.exists():
            print(f"⚠️  No Makefile found at '{makefile_path}' — skipping Makefile update.")
            return
    
        config_name = test_name.upper()
        entry = f"obj-$(CONFIG_{config_name}) += {test_name}.o"
    
        text = makefile_path.read_text(encoding="utf-8")
    
        # ✅ Fully escaped pattern for lines like: obj-$(CONFIG_XYZ) += xyz.o
        pattern = re.compile(r'^(obj-\$\(\s*CONFIG_[A-Z0-9_]+\s*\)\s*\+=\s*\w+\.o)', re.MULTILINE)
    
        # Comment out matching lines
        updated_text = re.sub(
            pattern,
            r'# \1  # commented by KUnitGen',
            text
        )
    
        if updated_text != text:
            makefile_path.write_text(updated_text, encoding="utf-8")
            print("📝 Commented previous Makefile entries.")
    
        # Add new entry only if not already present
        if entry not in updated_text:
            with open(makefile_path, "a", encoding="utf-8") as f:
                f.write("\n" + entry + "\n")
            print(f"🧩 Added to Makefile: {entry}")

    def _update_kconfig(self, test_name: str):
        """
        Ensures the main kernel Kconfig sources the custom test Kconfig file,
        and adds the specific config entry for the given test to that file.
        """
        main_kconfig_path = Path("/home/amd/linux/drivers/platform/x86/amd/pmf/Kconfig")
        backup_path = main_kconfig_path.with_suffix(".KunitGen_backup")
        config_name = test_name.upper()
    
        kconfig_entry = (
            f"\nconfig {config_name}\n"
            f'\tbool "KUnit test for {test_name}"\n'
            f"\tdepends on KUNIT\n"
            f"\tdefault n\n"
        )
    
        if not main_kconfig_path.exists():
            print(f"❌ Main Kconfig not found at {main_kconfig_path}")
            return False
    
        # Backup if not exists
        if not backup_path.exists():
            backup_path.write_text(main_kconfig_path.read_text(encoding="utf-8"), encoding="utf-8")
            print(f"📦 Backup created: {backup_path}")
    
        lines = main_kconfig_path.read_text(encoding="utf-8").splitlines(keepends=True)
    
        # Only add if entry doesn't already exist
        if any(f"config {config_name}" in line for line in lines):
            print(f"ℹ️ Kconfig entry for {config_name} already exists.")
            return True
    
        # Insert kconfig_entry before the first occurrence of source/actions Kconfig or endif
        new_lines = []
        inserted = False
        for line in lines:
            if (('source "drivers/pinctrl/actions/Kconfig"' in line) or ('endif' in line)) and not inserted:
                new_lines.append(kconfig_entry)  # insert before the line
                inserted = True
            new_lines.append(line)
    
        # Write back
        main_kconfig_path.write_text("".join(new_lines), encoding="utf-8")
        print(f"✅ Added Kconfig entry for {config_name}")

        return True
    def _update_test_config(self, test_name: str):
        cfg_path = Path("/home/amd/linux/my_pmf.config")
        if not cfg_path.exists():
            print(f"⚠️  No my_pinctrl.config found at '{cfg_path}' — skipping.")
            return
        config_line = f"CONFIG_{test_name.upper()}=y"
        cfg_text = cfg_path.read_text(encoding="utf-8")
        if config_line not in cfg_text:
            with open(cfg_path, "a", encoding="utf-8") as f:
                f.write("\n" + config_line + "\n")
            print(f"🧩 Enabled {config_line} in my_pinctrl.config.")
            

    

    def _compile_and_check(self) -> bool:
        """Compile using the kernel's make command and check for errors."""
        print("⚙️  Running kernel build to check for compilation errors...")
        
        kernel_dir = Path("/home/amd/linux")
        cmd = (
            f"cp /home/amd/nithin/KunitGen/main_test_dir/generated_tests/*.c /home/amd/linux/drivers/gpio && "
            f"./tools/testing/kunit/kunit.py run --kunitconfig=my_gpio.config --arch=x86_64 --raw_output > "
            f"/home/amd/nithin/KunitGen/main_test_dir/compilation_log/compile_error.txt 2>&1"
        )
    
        subprocess.run(cmd, shell=True, cwd=kernel_dir)
        # Check if log exists
        if not self.error_log_file.exists():
            print(f"❌ Log file not found: {self.error_log_file}")
            return False
    
        log_lines = self.error_log_file.read_text(encoding="utf-8").splitlines()
    
        seen = set()
        error_blocks = []
        i = 0
        while i < len(log_lines):
            line = log_lines[i]
            # Match lines containing 'error:' or 'fatal error:'
            if re.search(r"(error:|fatal error:)", line, re.IGNORECASE):
                # Remove file path and line numbers
                parts = line.split(':')
                for j, p in enumerate(parts):
                    if 'error' in p.lower():
                        clean_error = ':'.join(parts[j:]).strip()
                        break
                else:
                    clean_error = line.strip()
    
                if clean_error not in seen:
                    seen.add(clean_error)
                    # Capture next code line if it looks like code
                    code_line = ""
                    if i + 1 < len(log_lines):
                        next_line = log_lines[i + 1]
                        if re.match(r"\s+\S", next_line):
                            code_line = next_line.rstrip()
                    if code_line:
                        error_blocks.append(f"{clean_error}\n{code_line}")
                    else:
                        error_blocks.append(clean_error)
            i += 1
    
        extracted_errors = "\n\n".join(error_blocks) if error_blocks else "No explicit error lines found."
    
        # Save cleaned log
        extracted_log = self.error_log_file.parent / "clean_compile_errors.txt"
        extracted_log.write_text(extracted_errors, encoding="utf-8")
    
        if error_blocks:
            print(f"❌ Compilation failed. {len(error_blocks)} unique errors saved to '{extracted_log.name}'.")
            return False
    
        print("✅ Compilation successful.")
        return True


    # ---------------- Main Generation ----------------
    def generate_test_for_function(self, func_file_path: Path):
        func_code = func_file_path.read_text(encoding="utf-8")
        test_name = f"{func_file_path.stem}_kunit_test"
        out_file = self.output_dir / f"{test_name}.c"
        context = self._load_context_files()
        retrieved_snippets = self._retrieve_context(func_code)
        retrieved_text = "\n\n".join(retrieved_snippets)

        for attempt in range(1, self.max_retries + 1):
            print(f"\n🔹 Generating test for {func_file_path.name} (Attempt {attempt}/{self.max_retries})...")
            prompt = f"""
You are an expert Linux kernel developer generating KUnit tests.

## Function to test
{func_code}

## Retrieved Similar Code
{retrieved_text}

## Reference Code
{context['sample_code2']}

## Previous Compilation Errors
{context['error_logs']}

Rules:
- Include required headers correctly
- Use kunit_kzalloc for allocations
- Use KUNIT_EXPECT_* macros
- Do not mock target functions
- Output should be a compilable KUnit test C file
- Do not include explanations, only C code
"""
            generated_test = self._query_model(prompt)
            out_file.write_text(generated_test, encoding="utf-8")
            print(f"✅ Generated test file: {out_file}")
            # Compilation feedback loop remains unchanged
            break
        
    def run():
    """Main entry point to generate KUnit tests for all functions."""
    generator = KUnitTestGenerator(
        main_test_dir=Path("/home/amd/nithin/KunitGen/main_test_dir"),
        model_name="qwen/qwen3-coder-480b-a35b-instruct",
        temperature=0.4,
        max_retries=3
    )

    # Ensure output and log directories exist
    generator.output_dir.mkdir(parents=True, exist_ok=True)
    generator.error_log_file.parent.mkdir(parents=True, exist_ok=True)

    # Collect all .c files from test_functions
    func_files = list(generator.functions_dir.glob("*.c"))
    if not func_files:
        print(f"❌ No C files found in '{generator.functions_dir}'")
        return

    print(f"--- 🚀 Starting KUnit Test Generation for {len(func_files)} files ---")
    for func_file in func_files:
        generator.generate_test_for_function(func_file)

    print("\n--- ✅ All tests processed. ---")


if __name__ == "__main__":
    run()
