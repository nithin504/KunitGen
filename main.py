# main.py

from pathlib import Path
from KunitGeneration.model_interface.llm_model import KUnitTestGenerator

def main():
    # Set the base directory for your test suite (adjust as needed)
    main_test_dir = Path("main_test_dir")  # 🔁 Replace this with your actual path

    # Configuration
    model_name = "deepseek/deepseek-chat-v3.1:free"  # ✅ Free and open model on OpenRouter
    temperature = 0.2

    try:
        generator = KUnitTestGenerator(
            main_test_dir=main_test_dir,
            model_name=model_name,
            temperature=temperature
        )

        # Run the full test generation process
        generator.run()

    except Exception as e:
        print(f"❌ Error: {e}")

if __name__ == "__main__":
    main()
