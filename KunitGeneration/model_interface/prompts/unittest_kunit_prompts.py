kunit_generation_prompt="""
You are an expert Linux kernel developer with deep experience in writing **high-quality, coverage-focused KUnit tests**.


Your task is to write a single, complete, and compilable KUnit test file named `generated_kunit_test.c`. This file must provide **maximum test coverage** for all functions in the input C source — including edge cases, all branches, error paths, and uncommon conditions.

1.  **Source Functions to Test (`func_code`)**: A block of C code containing one or more functions that require testing.
    ```c
    {func_code}
    ```

2.  **Reference KUnit Test (`sample_code`)**: An example KUnit test file to be used for style, structure, and formatting.
    ```c
    {sample_code1}
    ```

    **Reference 2:**
    ```c
    {sample_code2}
    ```

    **Reference 3:**
    ```c
    {sample_code3}
    ```

3.  **Previous Errors to Fix (`error_logs`)**: A log of previous compilation or warning errors. Your generated code must not repeat these errors.
    ```
    {error_logs}
    ```

## Instructions

### 1. Analyze and Plan
- Carefully analyze **every function** in `{func_code}`.
- Plan test cases that **cover all control paths**:
  - All `if`/`else` branches
  - All return paths
  - Invalid or unusual inputs
  - Edge conditions (e.g. `NULL`, `0`, `-1`, `INT_MAX`, etc.)
- Name each test case `test_<function_name>_<condition>` (e.g., `test_parse_header_invalid`).
- Eliminate dead code or untestable branches if present, and make a best effort to simulate them if needed.

### 2. Implement KUnit Test Cases
- For every identified function, implement multiple test cases as needed to **achieve full code coverage**.
- Ensure each test uses appropriate `KUNIT_EXPECT_*` macros and checks return values, outputs, or side effects.
- Strictly follow the **style and layout** of `{sample_code2}`.

### 3. Code Integration
- Copy all tested functions from `{func_code}` into the test file and mark them `static` to make them directly callable.
- Define only **minimal mock structs** or test helpers. Avoid unused code and suppress `-Wunused-*` warnings.
- Do not generate stub functions unless they are strictly required to make test cases compile.

### 4. Consolidation
- Place all test cases in a single `static struct kunit_case` array.
- Define one `static struct kunit_suite` that references this array.
- Use `kunit_test_suite()` to register the test suite.

## Output Requirements

- Output only the **pure C source code** of the generated file `generated_kunit_test.c`.
- Do **not** include:
  - Markdown (e.g., ```c)
  - Placeholders
  - Comments
  - Explanations
- The output must be **compilable**, **style-compliant**, and **warning-free**.
"""
