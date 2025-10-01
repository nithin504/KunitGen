from pathlib import Path

# --- CONFIGURE THESE PATHS ---
GENERATED_TEST_DIR = Path("/home/amd/KunitGen/main_test_dir/generated_tests")
MAKEFILE_PATH = Path("/home/amd/linux/drivers/pinctrl/Makefile")
KCONFIG_PATH = Path("/home/amd/linux/drivers/pinctrl/Kconfig")
CONFIG_FILE_PATH = Path("/home/amd/linux/my_pinctrl.config")

def get_generated_test_files():
    return [f.stem for f in GENERATED_TEST_DIR.glob("*.c")]

def format_config_symbol(test_name: str) -> str:
    return "CONFIG_" + test_name.upper()

def infer_dependency(test_name: str) -> str:
    if "amd" in test_name.lower():
        return "PINCTRL_AMD"
    return "PINCTRL"

def add_to_makefile(test_files):
    if not MAKEFILE_PATH.exists():
        print(f"❌ Makefile not found: {MAKEFILE_PATH}")
        return

    makefile_lines = MAKEFILE_PATH.read_text(encoding="utf-8").splitlines()
    existing_lines = set(makefile_lines)
    new_lines = []

    for test in test_files:
        config = format_config_symbol(test)
        line = f"obj-$({config}) += {test}.o"
        if line not in existing_lines:
            new_lines.append(line)

    if new_lines:
        with open(MAKEFILE_PATH, "a", encoding="utf-8") as f:
            f.write("\n# --- KUnit tests added by script ---\n")
            f.write("\n".join(new_lines) + "\n")
        print(f"✅ Added {len(new_lines)} entries to Makefile.")
    else:
        print("ℹ️  No new entries needed in Makefile.")

def add_to_kconfig(test_files):
    if not KCONFIG_PATH.exists():
        print(f"❌ Kconfig not found: {KCONFIG_PATH}")
        return

    kconfig_text = KCONFIG_PATH.read_text(encoding="utf-8")
    added = 0

    with open(KCONFIG_PATH, "a", encoding="utf-8") as f:
        f.write("\n# --- KUnit tests added by script ---\n")
        for test in test_files:
            config = format_config_symbol(test)
            if f"config {config}" in kconfig_text:
                continue

            dependency = infer_dependency(test)

            block = f"""
config {config}
    tristate "KUnit test for {test}"
    depends on KUNIT && {dependency}
    default KUNIT_ALL_TESTS
    help
      Enables the KUnit test for {test}.
"""
            f.write(block)
            added += 1

    print(f"✅ Added {added} entries to Kconfig.")

def add_to_config_file(test_files):
    if not CONFIG_FILE_PATH.exists():
        CONFIG_FILE_PATH.touch()

    config_lines = CONFIG_FILE_PATH.read_text(encoding="utf-8").splitlines()
    existing = set(config_lines)
    added = 0

    with open(CONFIG_FILE_PATH, "a", encoding="utf-8") as f:
        for test in test_files:
            config = format_config_symbol(test)
            line = f"{config}=y"
            if line not in existing:
                f.write(line + "\n")
                added += 1

    print(f"✅ Added {added} entries to {CONFIG_FILE_PATH.name}.")

def main():
    if not GENERATED_TEST_DIR.exists():
        print(f"❌ Test directory not found: {GENERATED_TEST_DIR}")
        return

    test_files = get_generated_test_files()
    if not test_files:
        print("❌ No generated test files found.")
        return

    print(f"🔍 Found {len(test_files)} test files to register.\n")

    add_to_makefile(test_files)
    add_to_kconfig(test_files)
    add_to_config_file(test_files)

    print("\n✅ All files updated successfully.")

if __name__ == "__main__":
    main()
