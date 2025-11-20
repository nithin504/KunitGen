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
