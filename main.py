# main.py

import requests
from pathlib import Path
from KunitGeneration.model_interface.llm_model import KUnitTestGenerator
from KunitGeneration.data_ingestion.function_extraction import CFunctionExtractor  # Or update import path if needed

def fetch_github_raw_file(url: str) -> str:
    """Download source code from a raw GitHub URL."""
    print(f"🌐 Downloading from: {url}")
    response = requests.get(url)
    if response.status_code == 200:
        print("✅ Download successful.\n")
        return response.text
    else:
        raise Exception(f"Failed to fetch file. HTTP {response.status_code}: {url}")

def main():
    # --- Configuration ---
    github_raw_url = "https://raw.githubusercontent.com/torvalds/linux/master/drivers/pinctrl/pinctrl-amd.c"  # ✅ Replace with your own
    main_test_dir = Path("main_test_dir")
    extracted_dir = Path("test_functions")

    model_name = "qwen/qwen3-coder-480b-a35b-instruct"  # Free model on OpenRouter
    temperature = 0.2
   
    # --- Step 1: Fetch source code from GitHub ---
    try:
        source_code = fetch_github_raw_file(github_raw_url)
    except Exception as e:
        print(f"❌ Error fetching source code: {e}")
        return

    # --- Step 2: Extract functions and save them ---
    try:
        extracted_dir.mkdir(parents=True, exist_ok=True)
        extractor = CFunctionExtractor(source_code=source_code, base_dir=str(main_test_dir))
        extractor.process_and_save(str(extracted_dir))
    except Exception as e:
        print(f"❌ Error during function extraction: {e}")
        return 
    # --- Step 3: Generate KUnit tests ---
    try:
        generator = KUnitTestGenerator(
            main_test_dir=main_test_dir,
            model_name=model_name,
            temperature=temperature
        )
        generator.run()
    except Exception as e:
        print(f"❌ Error during test generation: {e}")

if __name__ == "__main__":
    main()
