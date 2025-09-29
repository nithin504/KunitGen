from pathlib import Path

# Paths (adjust to your tree)
GENERATED_TEST_DIR = Path("/home/amd/nithin/KunitGen/main_test_dir/generated_tests")
MAKEFILE_PATH = Path("/home/amd/linux/drivers/pinctrl/Makefile")
KCONFIG_PATH = Path("/home/amd/linux/drivers/pinctrl/Kconfig")
CONFIG_FILE_PATH = Path("/home/amd/linux/my_pinctrl.config")

def get_generated_tests():
    return [f.stem for f in GENERATED_TEST_DIR.glob("*.c")]

def fmt_config(test: str) -> str:
    return "CONFIG_" + test.upper()

def infer_parent_feature(test: str) -> str:
    # You may refine this logic based on your kernel tree
    if "amd" in test.lower():
        return "PINCTRL_AMD"
    return "PINCTRL"

def add_makefile(tests):
    if not MAKEFILE_PATH.exists():
        print("Makefile missing")
        return
    lines = MAKEFILE_PATH.read_text(encoding="utf-8").splitlines()
    existing = set(lines)
    new = []
    for t in tests:
        cfg = fmt_config(t)
        line = f"obj-$({cfg}) += {t}.o"
        if line not in existing:
            new.append(line)
    if new:
        with open(MAKEFILE_PATH, "a", encoding="utf-8") as f:
            f.write("\n# KUnit tests\n")
            f.write("\n".join(new) + "\n")
        print("Makefile updated.")

def add_kconfig(tests):
    if not KCONFIG_PATH.exists():
        print("Kconfig missing")
        return
    text = KCONFIG_PATH.read_text(encoding="utf-8")
    with open(KCONFIG_PATH, "a", encoding="utf-8") as f:
        f.write("\n# KUnit test entries\n")
        for t in tests:
            cfg = fmt_config(t)
            if f"config {cfg}" in text:
                continue
            parent = infer_parent_feature(t)
            f.write(f"""
config {cfg}
    bool "KUnit test for {t}"
    depends on KUNIT
    default n
    help
      Enables KUnit test for {t}.
""")
    print("Kconfig updated.")

def add_config_fragment(tests):
    # This fragment could act as your .kunitconfig or part of it
    if not CONFIG_FILE_PATH.exists():
        CONFIG_FILE_PATH.touch()
    lines = CONFIG_FILE_PATH.read_text(encoding="utf-8").splitlines()
    existing = set(lines)
    added = 0
    with open(CONFIG_FILE_PATH, "a", encoding="utf-8") as f:
        for t in tests:
            cfg = fmt_config(t)
            line = f"{cfg}=y"
            if line not in existing:
                f.write(line + "\n")
                added +=1
            # also add parent if missing
            parent = infer_parent_feature(t)
            parent_line = f"CONFIG_{parent}=y"
            if parent_line not in existing:
                f.write(parent_line + "\n")
                added += 1
    print("Config fragment updated with", added, "entries.")

def main():
    tests = get_generated_tests()
    if not tests:
        print("No tests found")
        return
    print("Found tests:", tests)
    add_makefile(tests)
    add_kconfig(tests)
    add_config_fragment(tests)
    print("Updates complete.")

if __name__ == "__main__":
    main()
