import os
import re
from pathlib import Path
from dotenv import load_dotenv

import chromadb
from chromadb.config import Settings
from sentence_transformers import SentenceTransformer
from openai import OpenAI


class KUnitTestGenerator:
    """Generate KUnit tests using RAG + ChromaDB + LLM"""

    def __init__(
        self,
        main_test_dir: Path,
        model_name: str,
        temperature: float = 0.2,
        max_retries: int = 3,
    ):
        if not main_test_dir.is_dir():
            raise FileNotFoundError(main_test_dir)

        # ---------------- Paths ----------------
        self.base_dir = main_test_dir
        self.reference_dir = self.base_dir / "reference_testcases"
        self.output_dir = self.base_dir / "generated_tests"
        self.logs_dir = self.base_dir / "compilation_log"
        self.chroma_dir = self.base_dir / "chroma_db"

        self.output_dir.mkdir(exist_ok=True)
        self.logs_dir.mkdir(exist_ok=True)
        self.chroma_dir.mkdir(exist_ok=True)

        # ---------------- LLM ----------------
        self.model_name = model_name
        self.temperature = temperature
        self.max_tokens = 8192
        self.max_retries = max_retries

        load_dotenv()
        self.client = OpenAI(
            base_url="https://integrate.api.nvidia.com/v1",
            api_key=os.environ["NVIDIA_API_KEY"]
        )

        # ---------------- Embeddings ----------------
        self.embed_model = SentenceTransformer("all-MiniLM-L6-v2")

        # ---------------- ChromaDB ----------------
        self.chroma = chromadb.Client(
            Settings(
                persist_directory=str(self.chroma_dir),
                anonymized_telemetry=False
            )
        )

        self.collection = self.chroma.get_or_create_collection(
            name="kunit_kernel_functions"
        )

        if self.collection.count() == 0:
            self._build_index()

    # =====================================================================
    # FUNCTION EXTRACTION (STRUCTURAL CHUNKING)
    # =====================================================================

    def _extract_functions(self, code: str):
        pattern = re.compile(
            r"(static\s+)?[\w\*\s]+\s+(\w+)\s*\([^)]*\)\s*\{",
            re.MULTILINE
        )

        functions = []
        matches = list(pattern.finditer(code))

        for i, match in enumerate(matches):
            start = match.start()
            end = matches[i + 1].start() if i + 1 < len(matches) else len(code)
            fn_body = code[start:end]

            functions.append({
                "name": match.group(2),
                "code": fn_body.strip()
            })

        return functions

    # =====================================================================
    # BUILD CHROMADB INDEX (ONCE)
    # =====================================================================

    def _build_index(self):
        print("📦 Building ChromaDB index (function-level kernel code)...")

        documents, metadatas, ids = [], [], []

        for file in self.reference_dir.rglob("*.c"):
            code = file.read_text(errors="ignore")
            functions = self._extract_functions(code)

            for idx, fn in enumerate(functions):
                documents.append(fn["code"])
                metadatas.append({
                    "function": fn["name"],
                    "file": str(file),
                    "subsystem": file.parent.name
                })
                ids.append(f"{file.stem}_{fn['name']}_{idx}")

        if not documents:
            raise RuntimeError("No kernel functions found")

        embeddings = self.embed_model.encode(
            documents,
            normalize_embeddings=True,
            show_progress_bar=True
        ).tolist()

        self.collection.add(
            documents=documents,
            embeddings=embeddings,
            metadatas=metadatas,
            ids=ids
        )

        self.chroma.persist()
        print(f"✅ Indexed {len(documents)} kernel functions")

    # =====================================================================
    # RETRIEVE CONTEXT
    # =====================================================================

    def retrieve_context(
        self,
        query: str,
        k: int = 5,
        subsystem: str | None = None
    ):
        query_embedding = self.embed_model.encode(
            query,
            normalize_embeddings=True
        ).tolist()

        where = {"subsystem": subsystem} if subsystem else None

        results = self.collection.query(
            query_embeddings=[query_embedding],
            n_results=k,
            where=where
        )

        return results["documents"][0], results["metadatas"][0]

    # =====================================================================
    # GENERATE KUNIT TEST (LLM)
    # =====================================================================

    def generate_kunit_test(self, function_name: str, subsystem: str | None = None):
        docs, metas = self.retrieve_context(
            f"Generate KUnit test for {function_name}",
            subsystem=subsystem
        )

        context = "\n\n".join(docs)

        prompt = f"""
You are a Linux kernel developer.

Write a correct and compilable KUnit test.
Follow kernel coding style.
Do not hallucinate APIs.

Target function:
{function_name}

Relevant kernel context:
{context}
"""

        response = self.client.chat.completions.create(
            model=self.model_name,
            messages=[{"role": "user", "content": prompt}],
            temperature=self.temperature,
            max_tokens=self.max_tokens
        )

        return response.choices[0].message.content.strip()
