import os
import re
import requests

class BaseFunctionExtractor:
    """
    A base class for extracting functions from source code.
    Provides shared logic for finding matching braces and saving files.
    """

    def __init__(self, source_code: str, base_dir: str = None):
        """Initializes the extractor with source code and a base directory."""
        if not source_code:
            raise ValueError("Source code cannot be empty.")
        self.source_code = source_code
        self.functions = []
        self.file_extension = ".txt"  # Default extension
        self.base_dir = base_dir or os.getcwd()  # Set base_dir, default to current working directory

    def extract_functions(self):
        """Placeholder for the extraction logic. Must be implemented by subclasses."""
        raise NotImplementedError("Subclasses must implement the extract_functions method.")

    def _find_matching_brace(self, start_pos: int) -> int:
        """Finds the matching closing brace for a function body."""
        brace_count = 0
        in_body = False
        # Start searching from the first '{'
        first_brace_pos = self.source_code.find('{', start_pos)
        if first_brace_pos == -1:
              raise ValueError("No opening brace found for the function body.")

        for i, char in enumerate(self.source_code[first_brace_pos:], start=first_brace_pos):
            if char == '{':
                brace_count += 1
                in_body = True
            elif char == '}':
                brace_count -= 1
            if in_body and brace_count == 0:
                return i + 1
        raise ValueError("Unmatched opening brace.")

    def save_to_files(self, output_dir: str):
        """Saves each extracted function to its own file."""
        # If output_dir is a relative path, join it with the base directory
        if not os.path.isabs(output_dir):
            output_dir = os.path.join(self.base_dir, output_dir)
            
        os.makedirs(output_dir, exist_ok=True)
        count = 0
        for func in self.functions:
            safe_name = re.sub(r'\W+', '_', func['name'])
            file_path = os.path.join(output_dir, f"{safe_name}{self.file_extension}")
            try:
                with open(file_path, 'w', encoding='utf-8') as f:
                    f.write(func['code'])
                count += 1
            except IOError as e:
                print(f"Error writing file {file_path}: {e}")
        print(f"Successfully wrote {count} functions to the '{output_dir}' directory.")

    def process_and_save(self, output_dir: str):
        """
        A new convenience method to run the entire process:
        1. Extracts all functions from the source code.
        2. Saves the extracted functions to the specified directory.
        """
        print(f"--- Starting Extraction for {self.file_extension} files ---")
        self.extract_functions()
        
        if not self.functions:
            print("--- No functions found to save. Process finished. ---")
            return self

        print(f"--- Extraction complete. Found {len(self.functions)} functions. ---")
        self.save_to_files(output_dir)
        print("--- File saving process complete. ---")
        return self # Allows for method chaining


class CFunctionExtractor(BaseFunctionExtractor):
    """Extracts functions from C source code."""

    def __init__(self, source_code: str, base_dir: str = None):
        super().__init__(source_code, base_dir)
        self.file_extension = ".c"

    def extract_functions(self):
        """Parses C source code to find and extract all function definitions."""
        # This regex looks for a function prototype followed by an opening brace '{'
        # It handles optional 'static', various return types (words, pointers), and parameters.
        prototype_pattern = re.compile(
            r'^\s*'                  # Optional leading whitespace
            r'(?P<prototype>'        # Start capturing the prototype
            r'(?:static\s+)?'        # Optional 'static' keyword
            r'[\w\s\*]+\s+'          # Return type (e.g., "int", "void *", "unsigned long")
            r'\w+\s*'                # Function name
            r'\([^)]*\)'             # Parameters inside parentheses
            r')'                     # End capturing the prototype
            r'\s*\{',                # Whitespace and the opening brace
            re.MULTILINE
        )

        for match in prototype_pattern.finditer(self.source_code):
            start_index = match.start()
            prototype = match.group('prototype').strip()
            
            # Extract function name (the word just before the opening parenthesis)
            name_match = re.search(r'(\w+)\s*\([^)]*\)$', prototype)
            func_name = name_match.group(1) if name_match else f"unknown_function_{start_index}"

            try:
                end_index = self._find_matching_brace(start_index)
                snippet = self.source_code[start_index:end_index]
                self.functions.append({'name': func_name, 'code': snippet})
            except ValueError as e:
                print(f"Warning: Could not parse C function '{func_name}'. Reason: {e}")

        print(f"Found {len(self.functions)} potential C functions.")
        return self


class CppFunctionExtractor(BaseFunctionExtractor):
    """Extracts functions and methods from C++ source code."""

    def __init__(self, source_code: str, base_dir: str = None):
        super().__init__(source_code, base_dir)
        self.file_extension = ".cpp"

    def extract_functions(self):
        """
        Parses C++ source code to find and extract functions and methods.
        This pattern is more complex to handle templates, namespaces, and class methods.
        """
        # This regex is designed for C++ complexity.
        prototype_pattern = re.compile(
            r"""
            ^\s* # Optional leading whitespace
            (?P<prototype>           # Start capturing the prototype
            (?:template\s*<[^>]*>\s*)? # Optional template declaration
            (?:[\w\s\*&:<>,]+?)     # Permissive return type (incl. templates, namespaces)
            \s+                     # Separator
            ((?:[\w_]+::)*[\w_~]+)   # Function/method name with scope (e.g., MyClass::myMethod, ~MyDestructor)
            \s*\([^)]*\)            # Parameter list
            (?:\s*const)?           # Optional 'const' qualifier
            (?:\s*override)?        # Optional 'override' qualifier
            (?:\s*final)?           # Optional 'final' qualifier
            )                       # End capturing the prototype
            \s*\{                   # Whitespace and the opening brace
            """,
            re.MULTILINE | re.VERBOSE
        )

        for match in prototype_pattern.finditer(self.source_code):
            start_index = match.start()
            prototype = match.group('prototype').strip()
            
            # Extract function/method name, which can include scope resolution (::) and be a destructor (~)
            name_match = re.search(r'((?:[\w_]+::)*[\w_~]+)\s*\([^)]*\)', prototype)
            func_name = name_match.group(1) if name_match else f"unknown_cpp_function_{start_index}"

            try:
                end_index = self._find_matching_brace(start_index)
                snippet = self.source_code[start_index:end_index]
                self.functions.append({'name': func_name, 'code': snippet})
            except ValueError as e:
                print(f"Warning: Could not parse C++ function '{func_name}'. Reason: {e}")

        print(f"Found {len(self.functions)} potential C++ functions/methods.")
        return self

