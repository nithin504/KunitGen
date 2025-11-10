import os
import subprocess
from pathlib import Path
from openai import OpenAI
from dotenv import load_dotenv

import re
# Import your prompt template
# Ensure this path is correct relative to where you run the script
from KunitGeneration.model_interface.prompts.unittest_kunit_prompts import kunit_generation_prompt


class KUnitTestGenerator:
    """
    A class to generate, compile, and validate KUnit tests for C functions.
    Automatically regenerates tests until compilation passes successfully.
    """

    def __init__(self, main_test_dir: Path, model_name: str, temperature: float, max_retries: int = 3):
        if not main_test_dir.is_dir():
            raise FileNotFoundError(f"The specified test directory does not exist: {main_test_dir}")

        # --- Paths ---
        self.base_dir = main_test_dir
        self.functions_dir = self.base_dir / "test_functions"
        self.output_dir = self.base_dir / "generated_tests"
        self.sample_files = [
            self.base_dir / "reference_testcases" / "kunit_test1.c",
            self.base_dir / "reference_testcases" / "kunit_test2.c",
            self.base_dir / "reference_testcases" / "kunit_test3.c",
        ]
        self.error_log_file = self.base_dir / "compilation_log" / "compile_error.txt"

        # --- Model ---
        self.model_name = model_name
        self.temperature = temperature
        self.max_tokens = 8192
        self.max_retries = max_retries

        # --- Setup ---
        self._load_environment()
        self.client = self._initialize_client()
        self.prompt_template = kunit_generation_prompt

    # ---------------- Environment Setup ----------------
    def _load_environment(self):
        load_dotenv()
        self.api_key = os.environ.get("NVIDIA_API_KEY")
        if not self.api_key:
            raise ValueError("NVIDIA_API_KEY environment variable not set.")
        
        

    def _initialize_client(self):
        return OpenAI(base_url="https://integrate.api.nvidia.com/v1", api_key=self.api_key)
        
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
            # Clean up model output from code fences
            return response.replace("```c", "").replace("```", "").strip()
        except Exception as e:
            print(f"An error occurred while querying the model: {e}")
            return f"// Error generating response: {e}"

    # ---------------- Context Loader ----------------
    def _load_context_files(self) -> dict:
        print("Reading sample KUnit test files and error logs...")
        context = {}
        def safe_read(p: Path, fallback="// Missing file"):
            return p.read_text(encoding="utf-8") if p.exists() else fallback

        context["sample_code1"] = safe_read(self.sample_files[0])
        context["sample_code2"] = safe_read(self.sample_files[1])
        context["sample_code3"] = safe_read(self.sample_files[2])
        context["error_logs"] = safe_read(self.error_log_file, "// No previous error logs")

        return context

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


    # ---------------- Main Test Generation ----------------
    def generate_test_for_function(self, func_file_path: Path):
        func_code = func_file_path.read_text(encoding="utf-8")
        test_name = f"{func_file_path.stem}_kunit_test"
        out_file = self.output_dir / f"{test_name}.c"
        context = self._load_context_files()
        #self._compile_and_check=True
        for attempt in range(1, 6):
        #while self._compile_and_check()==True:
            print(f"\n🔹 Generating test for {func_file_path.name} (Attempt {attempt}/{self.max_retries})...")

            #final_prompt = self.prompt_template.format(func_code=func_code, **context)
            final_prompt=f"""
You are an expert Linux kernel developer with deep experience in writing high-quality, coverage-focused KUnit tests.

Your task is to automatically generate a **complete, compilable KUnit test file** for the given C source file, using a reference source and an error log to guide the generation. The goal is to produce code that compiles without any errors.

## Inputs

1. Reference Source (`source_code`):
{sample_code2}

2. Previous Compilation Errors (`error_logs`):
{error_logs}

## Critical Rules

1. Include all necessary kernel headers for structs, macros, and functions.
2. Use pointers for opaque structs and allocate with `kunit_kzalloc(test, sizeof(*obj), GFP_KERNEL)`.
3. Avoid modifying read-only or const members.
4. Ensure all functions are declared or included via headers; do not call undeclared functions.
5. Include required macros/constants via headers.
6. Place all test cases in a single `static struct kunit_case` array.
7. Define one `static struct kunit_suite` referencing this array with `.test_cases`.
8. Register the suite with `kunit_test_suite(my_suite)`.
9. Use `KUNIT_EXPECT_*` macros for assertions.
10. Avoid duplicating any errors from `error_logs`.
11. Hit the unhit branches such that include the source file obtain code coverage.
12. **Do NOT mock the functions being tested**. You may mock **dependencies** (helper functions, hardware calls, or internal struct accessors) if required using #define and cannot change the source file.
13. add #include "gpio-amdpt.c" for each testcase
## Instructions

- Analyze all functions in `source_code`.
- Generate tests covering all branches, edge cases, and error paths.
- Only mock dependencies if required; never mock the function under test.
- Output only compilable KUnit C source code.
"""
            generated_test = self._query_model(final_prompt)
            out_file.write_text(generated_test, encoding="utf-8")

            # Update kernel build metadata before compiling
            self._update_makefile(test_name)
            self._update_kconfig(test_name)
            self._update_test_config(test_name)

            if self._compile_and_check():
                print(f"✅ Test '{test_name}' built successfully on attempt {attempt}.")
                return
            else:
                if self.error_log_file.exists():
                    context["error_logs"] = self.error_log_file.read_text(encoding="utf-8")
                print("🔁 Retrying with compile error feedback...")

        print(f"❌ Failed to build a compilable test for {func_file_path.name} after {self.max_retries} attempts.")

    def run(self):
        print(f"--- Starting KUnit Test Generation in '{self.base_dir}' ---")
        self.output_dir.mkdir(parents=True, exist_ok=True)
        self.error_log_file.parent.mkdir(parents=True, exist_ok=True)

        func_files = list(self.functions_dir.glob("*.c"))
        if not func_files:
            print(f"❌ No C files found in '{self.functions_dir}'")
            return

        for func_file in func_files:
            self.generate_test_for_function(func_file)

        print("\n--- ✅ All tests processed. ---")

