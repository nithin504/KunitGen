import re
from pathlib import Path

def _compile_and_check(url: str) -> bool:
    """Extract unique compiler errors with the offending code line (like GCC style)."""
    error_log_file = Path(url)
    if not error_log_file.exists():
        print(f"❌ Log file not found: {url}")
        return False

    log_lines = error_log_file.read_text(encoding="utf-8").splitlines()

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

            # Skip duplicates
            if clean_error not in seen:
                seen.add(clean_error)
                # Attempt to get the next code line if it exists and looks like code (has indentation)
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
    extracted_log = error_log_file.parent / "clean_compile_errors.txt"
    extracted_log.write_text(extracted_errors, encoding="utf-8")

    if error_blocks:
        print(f"❌ Compilation failed. {len(error_blocks)} unique errors saved to '{extracted_log.name}'.")
        return False
    else:
        print("✅ No errors found in log.")
        return True


if __name__ == "__main__":
    url = "kunit_test.txt"
    _compile_and_check(url)
