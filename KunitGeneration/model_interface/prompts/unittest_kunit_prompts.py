
kunit_generation_prompt = f"""
You are an expert Linux kernel developer with deep experience in writing high-quality, coverage-focused KUnit tests.

Your task is to automatically generate a **complete, compilable KUnit test file** for the given C source file, using a reference source and an error log to guide the generation. The goal is to produce code that compiles without any errors.

## Inputs

1. Reference Source (`source_code`):
{source_code}

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
11. **Do NOT mock the functions being tested**. You may mock **dependencies** (helper functions, hardware calls, or internal struct accessors) if needed for compilation.

## Instructions

- Analyze all functions in `source_code`.
- Generate tests covering all branches, edge cases, and error paths.
- Only mock dependencies if required; never mock the function under test.
- Output only compilable KUnit C source code.
"""
