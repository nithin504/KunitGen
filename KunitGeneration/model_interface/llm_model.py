import os
from pathlib import Path
from openai import OpenAI
from dotenv import load_dotenv

# Corrected import path to match our previous structure
from KunitGeneration.model_interface.prompts.unittest_kunit_prompts import kunit_generation_prompt

class KUnitTestGenerator:
    """
    A class to generate KUnit tests for C functions using an LLM.
    It loads a prompt template, reads C function files, queries a model,
    and saves the generated tests.
    """

    def __init__(self, main_test_dir: Path, model_name: str, temperature: float):
        """
        Initializes the generator by setting up paths, loading configuration,
        and preparing the API client.

        Args:
            main_test_dir: The root directory for the test suite.
            model_name: The name of the model to use for generation.
            temperature: The creativity/randomness setting for the model.
        """
        if not main_test_dir.is_dir():
            raise FileNotFoundError(f"The specified test directory does not exist: {main_test_dir}")

        # --- File and Directory Paths ---
        self.base_dir = main_test_dir
        self.functions_dir = self.base_dir / "test_functions"
        self.output_dir = self.base_dir / "generated_tests"
        self.sample_files = [
            self.base_dir / "reference_testcases" / "kunit_test1.c", # Look for global samples
            self.base_dir / "reference_testcases" / "kunit_test2.c",
            self.base_dir / "reference_testcases" / "kunit_test3.c",
        ]
        self.error_log_file = self.base_dir / "compilation_log" / "compile_error.txt"

        # --- Model Configuration ---
        self.model_name = model_name
        self.temperature = temperature
        self.max_tokens = 8192

        # --- Load Config and Initialize Client ---
        self._load_environment()
        self.client = self._initialize_client()
        self.prompt_template = kunit_generation_prompt

    def _load_environment(self):
        """Loads environment variables from a .env file."""
        load_dotenv()
        self.api_key = os.environ.get("NVIDIA_API_KEY")
        if not self.api_key:
            raise ValueError("NVIDIA_API_KEY environment variable not set.")

    def _initialize_client(self):
        """Initializes the OpenAI client for OpenRouter."""
        return OpenAI(
            base_url="https://integrate.api.nvidia.com/v1",
            api_key=self.api_key,
        )

    def _query_model(self, prompt: str) -> str:
        """Sends a prompt to the configured OpenRouter model and returns the response."""
        try:
            completion = self.client.chat.completions.create(
                model=self.model_name,
                messages=[{"role": "user", "content": prompt}],
                temperature=self.temperature,
                max_tokens=self.max_tokens,
            )
            return completion.choices[0].message.content
        except Exception as e:
            print(f"An error occurred while querying the model: {e}")
            return f"// Error generating response: {e}"

    def _load_context_files(self) -> dict:
        """
        Loads sample files and error logs to provide context to the model,
        replicating the logic from the user's provided snippet.
        """
        print("Reading sample KUnit test files and error logs...")
        context = {}

        # Read files into separate variables, with fallbacks, as in the snippet.
        sample_code1 = self.sample_files[0].read_text(encoding="utf-8") if self.sample_files[0].exists() else "// No sample reference available"
        sample_code2 = self.sample_files[1].read_text(encoding="utf-8") if self.sample_files[1].exists() else "// No sample reference available"
        sample_code3 = self.sample_files[2].read_text(encoding="utf-8") if self.sample_files[2].exists() else "// No sample reference available"
        error_logs = self.error_log_file.read_text(encoding="utf-8") if self.error_log_file.exists() else "// No error logs available"

        # Perform checks and print warnings based on the content, as requested.
        if "No sample" in sample_code1:
            print(f"⚠️  Warning: Sample file '{self.sample_files[0]}' not found or is empty.")
        if "No sample" in sample_code2:
            print(f"⚠️  Warning: Sample file '{self.sample_files[1]}' not found or is empty.")
        if "No sample" in sample_code3:
            print(f"⚠️  Warning: Sample file '{self.sample_files[2]}' not found or is empty.")
        if "No error logs" in error_logs:
            print(f"⚠️  Warning: Error log file '{self.error_log_file}' not found or is empty.")

        # Populate the context dictionary to be returned
        context['sample_code1'] = sample_code1
        context['sample_code2'] = sample_code2
        context['sample_code3'] = sample_code3
        context['error_logs'] = error_logs
        
        return context

    def generate_test_for_function(self, func_file_path: Path):
        """Generates and saves a KUnit test for a single C function file."""
        print(f"🔹 Generating test for {func_file_path.name}...")
        func_code = func_file_path.read_text(encoding="utf-8")
        context = self._load_context_files()

        final_prompt = self.prompt_template.format(
            func_code=func_code,
            **context
        )

        response_text = self._query_model(final_prompt)
        out_file = self.output_dir / f"{func_file_path.stem}_kunit_test.c"
        out_file.write_text(response_text, encoding="utf-8")
        print(f"✅ Test generated and saved to: {out_file}")

    def run(self):
        """Main execution loop to find all C function files and generate tests."""
        print(f"--- Starting KUnit Test Generation in '{self.base_dir}' ---")
        self.output_dir.mkdir(parents=True, exist_ok=True)

        function_files = list(self.functions_dir.glob("*.c"))
        if not function_files:
            print(f"❌ No C files found in '{self.functions_dir}'. Please run the extraction script first.")
            return

        for func_file in function_files:
            self.generate_test_for_function(func_file)

        print("\n--- All tests have been generated. ---")

