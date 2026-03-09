#!/usr/bin/env python3
"""SQLite-backed source-of-truth CLI for long-lived software design projects."""

from __future__ import annotations

import argparse
import ast
import datetime as dt
import json
import os
import sqlite3
import sys
import time
import uuid
from pathlib import Path
from typing import Any


DEFAULT_DB = ".design/design.db"
PROJECT_STATUSES = {"active", "archived"}
NODE_STATUSES = {"todo", "in_progress", "blocked", "done"}
ARTIFACT_STATUSES = {"draft", "approved", "superseded"}
QUESTION_STATUSES = {"open", "resolved"}
DECISION_STATUSES = {"proposed", "accepted", "rejected", "superseded"}
PACKAGE_STATUSES = {"draft", "approved", "superseded"}
DOWNSTREAM_IMPLEMENTATION_STAGES = {
    "implementation_design",
    "implementation_execution",
    "implementation_verification_execution",
}
SOFTWARE_DESIGN_STAGE_SEQUENCE = [
    ("problem_definition", "Problem definition"),
    ("requirements_definition", "Requirements definition"),
    ("solution_framing", "Solution framing"),
    ("domain_and_data_modeling", "Domain and data modeling"),
    ("behavior_and_workflow_design", "Behavior and workflow design"),
    ("system_decomposition", "System decomposition"),
    ("contract_design", "Contract design"),
    ("internal_subsystem_design", "Internal subsystem design"),
    ("security_and_trust_boundary_design", "Security and trust-boundary design"),
    ("operational_design", "Operational design"),
    ("verification_design", "Verification design"),
    ("implementation_handoff", "Implementation handoff"),
]


SCHEMA_SQL = """
PRAGMA foreign_keys = ON;

CREATE TABLE IF NOT EXISTS projects (
    id TEXT PRIMARY KEY,
    name TEXT NOT NULL UNIQUE,
    goal TEXT NOT NULL,
    scope TEXT NOT NULL DEFAULT '',
    constraints_json TEXT NOT NULL DEFAULT '[]',
    non_goals_json TEXT NOT NULL DEFAULT '[]',
    status TEXT NOT NULL DEFAULT 'active',
    created_at TEXT NOT NULL,
    updated_at TEXT NOT NULL
);

CREATE TABLE IF NOT EXISTS design_nodes (
    id TEXT PRIMARY KEY,
    project_id TEXT NOT NULL REFERENCES projects(id) ON DELETE CASCADE,
    parent_id TEXT REFERENCES design_nodes(id) ON DELETE SET NULL,
    title TEXT NOT NULL,
    stage TEXT NOT NULL,
    status TEXT NOT NULL,
    summary TEXT NOT NULL DEFAULT '',
    depends_on_json TEXT NOT NULL DEFAULT '[]',
    artifact_ids_json TEXT NOT NULL DEFAULT '[]',
    open_question_ids_json TEXT NOT NULL DEFAULT '[]',
    constraint_package_id TEXT REFERENCES constraint_packages(id) ON DELETE SET NULL,
    created_at TEXT NOT NULL,
    updated_at TEXT NOT NULL
);

CREATE INDEX IF NOT EXISTS idx_design_nodes_project ON design_nodes(project_id);
CREATE INDEX IF NOT EXISTS idx_design_nodes_parent ON design_nodes(parent_id);
CREATE INDEX IF NOT EXISTS idx_design_nodes_status ON design_nodes(project_id, status);

CREATE TABLE IF NOT EXISTS artifacts (
    id TEXT PRIMARY KEY,
    project_id TEXT NOT NULL REFERENCES projects(id) ON DELETE CASCADE,
    node_id TEXT REFERENCES design_nodes(id) ON DELETE SET NULL,
    type TEXT NOT NULL,
    title TEXT NOT NULL,
    content_json TEXT NOT NULL,
    status TEXT NOT NULL,
    version INTEGER NOT NULL,
    created_at TEXT NOT NULL,
    updated_at TEXT NOT NULL
);

CREATE INDEX IF NOT EXISTS idx_artifacts_project ON artifacts(project_id);
CREATE INDEX IF NOT EXISTS idx_artifacts_node ON artifacts(node_id);

CREATE TABLE IF NOT EXISTS decisions (
    id TEXT PRIMARY KEY,
    project_id TEXT NOT NULL REFERENCES projects(id) ON DELETE CASCADE,
    node_id TEXT REFERENCES design_nodes(id) ON DELETE SET NULL,
    decision TEXT NOT NULL,
    rationale TEXT NOT NULL DEFAULT '',
    alternatives_json TEXT NOT NULL DEFAULT '[]',
    impact TEXT NOT NULL DEFAULT '',
    status TEXT NOT NULL,
    created_at TEXT NOT NULL,
    updated_at TEXT NOT NULL
);

CREATE INDEX IF NOT EXISTS idx_decisions_project ON decisions(project_id);
CREATE INDEX IF NOT EXISTS idx_decisions_node ON decisions(node_id);

CREATE TABLE IF NOT EXISTS open_questions (
    id TEXT PRIMARY KEY,
    project_id TEXT NOT NULL REFERENCES projects(id) ON DELETE CASCADE,
    node_id TEXT REFERENCES design_nodes(id) ON DELETE SET NULL,
    question TEXT NOT NULL,
    context TEXT NOT NULL DEFAULT '',
    blocking INTEGER NOT NULL,
    status TEXT NOT NULL,
    resolution TEXT NOT NULL DEFAULT '',
    created_at TEXT NOT NULL,
    updated_at TEXT NOT NULL
);

CREATE INDEX IF NOT EXISTS idx_questions_project ON open_questions(project_id);
CREATE INDEX IF NOT EXISTS idx_questions_node ON open_questions(node_id);
CREATE INDEX IF NOT EXISTS idx_questions_status ON open_questions(project_id, status, blocking);

CREATE TABLE IF NOT EXISTS constraint_packages (
    id TEXT PRIMARY KEY,
    project_id TEXT NOT NULL REFERENCES projects(id) ON DELETE CASCADE,
    node_id TEXT NOT NULL UNIQUE REFERENCES design_nodes(id) ON DELETE CASCADE,
    module_name TEXT NOT NULL,
    package_json TEXT NOT NULL,
    status TEXT NOT NULL,
    created_at TEXT NOT NULL,
    updated_at TEXT NOT NULL
);

CREATE INDEX IF NOT EXISTS idx_packages_project ON constraint_packages(project_id);

CREATE TABLE IF NOT EXISTS events (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    project_id TEXT NOT NULL REFERENCES projects(id) ON DELETE CASCADE,
    entity_type TEXT NOT NULL,
    entity_id TEXT NOT NULL,
    action TEXT NOT NULL,
    payload_json TEXT NOT NULL,
    created_at TEXT NOT NULL
);

CREATE INDEX IF NOT EXISTS idx_events_project ON events(project_id, created_at);
CREATE INDEX IF NOT EXISTS idx_events_entity ON events(entity_type, entity_id, created_at);

CREATE TABLE IF NOT EXISTS code_index_runs (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    workspace_root TEXT NOT NULL,
    started_at TEXT NOT NULL,
    completed_at TEXT NOT NULL,
    duration_ms INTEGER NOT NULL,
    file_count INTEGER NOT NULL,
    symbol_count INTEGER NOT NULL,
    parser_backend TEXT NOT NULL,
    created_at TEXT NOT NULL
);

CREATE TABLE IF NOT EXISTS code_files (
    workspace_root TEXT NOT NULL,
    file_path TEXT NOT NULL,
    language TEXT NOT NULL,
    module_name TEXT NOT NULL,
    mtime_ns INTEGER NOT NULL,
    size_bytes INTEGER NOT NULL,
    index_run_id INTEGER NOT NULL REFERENCES code_index_runs(id) ON DELETE CASCADE,
    PRIMARY KEY (workspace_root, file_path)
);

CREATE INDEX IF NOT EXISTS idx_code_files_run ON code_files(index_run_id);

CREATE TABLE IF NOT EXISTS code_symbols (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    workspace_root TEXT NOT NULL,
    file_path TEXT NOT NULL,
    symbol_name TEXT NOT NULL,
    symbol_kind TEXT NOT NULL,
    parent_symbol TEXT,
    signature TEXT NOT NULL DEFAULT '',
    start_line INTEGER NOT NULL,
    end_line INTEGER NOT NULL,
    index_run_id INTEGER NOT NULL REFERENCES code_index_runs(id) ON DELETE CASCADE
);

CREATE INDEX IF NOT EXISTS idx_code_symbols_run ON code_symbols(index_run_id);
CREATE INDEX IF NOT EXISTS idx_code_symbols_file ON code_symbols(workspace_root, file_path);
"""


def now() -> str:
    return dt.datetime.now(dt.timezone.utc).isoformat()


def new_id(prefix: str) -> str:
    return f"{prefix}_{uuid.uuid4().hex[:12]}"


def json_load(text: str | None, default: Any) -> Any:
    if text is None or text == "":
        return default
    return json.loads(text)


def json_dump(value: Any) -> str:
    return json.dumps(value, sort_keys=True)


def fail(message: str, exit_code: int = 1) -> None:
    print(json.dumps({"ok": False, "error": message}, indent=2), file=sys.stderr)
    raise SystemExit(exit_code)


def validate_choice(value: str, allowed: set[str], label: str) -> None:
    if value not in allowed:
        fail(f"invalid {label}: {value}. allowed: {sorted(allowed)}")


def parse_json_input(raw: str | None, file_path: str | None, default: Any) -> Any:
    if raw and file_path:
        fail("use only one of --json or --file")
    if file_path:
        return json.loads(Path(file_path).read_text())
    if raw:
        return json.loads(raw)
    return default


def infer_language(path: Path) -> str:
    suffix = path.suffix.lower()
    if suffix == ".py":
        return "python"
    if suffix in {".md", ".markdown"}:
        return "markdown"
    if suffix in {".json"}:
        return "json"
    if suffix in {".yaml", ".yml"}:
        return "yaml"
    if suffix in {".js", ".mjs", ".cjs"}:
        return "javascript"
    if suffix in {".ts", ".tsx"}:
        return "typescript"
    if suffix in {".c", ".h"}:
        return "c"
    if suffix in {".cc", ".cpp", ".cxx", ".hpp", ".hh", ".hxx"}:
        return "cpp"
    if suffix in {".rs"}:
        return "rust"
    if suffix in {".go"}:
        return "go"
    return "text"


def infer_module_name(path: Path, workspace_root: Path) -> str:
    rel = path.relative_to(workspace_root)
    return str(rel.with_suffix("")).replace(os.sep, ".")


def workspace_key(workspace_root: Path) -> str:
    rel = os.path.relpath(workspace_root.resolve(), Path.cwd().resolve())
    if rel == ".":
        return "."
    return rel.replace(os.sep, "/")


def should_skip_path(path: Path) -> bool:
    skipped_parts = {".git", "__pycache__", ".venv", "venv", "node_modules", ".mypy_cache", ".pytest_cache", "build"}
    return any(part in skipped_parts for part in path.parts)


def build_signature(node: ast.AST) -> str:
    if isinstance(node, (ast.FunctionDef, ast.AsyncFunctionDef)):
        arg_names = [arg.arg for arg in node.args.args]
        return f"({', '.join(arg_names)})"
    return ""


def extract_python_symbols(path: Path) -> list[dict[str, Any]]:
    try:
        source = path.read_text()
        tree = ast.parse(source)
    except (OSError, SyntaxError, UnicodeDecodeError):
        return []

    symbols: list[dict[str, Any]] = []

    class Visitor(ast.NodeVisitor):
        def __init__(self) -> None:
            self.parents: list[str] = []

        def visit_ClassDef(self, node: ast.ClassDef) -> None:
            parent = self.parents[-1] if self.parents else None
            symbols.append(
                {
                    "symbol_name": node.name,
                    "symbol_kind": "class",
                    "parent_symbol": parent,
                    "signature": "",
                    "start_line": getattr(node, "lineno", 1),
                    "end_line": getattr(node, "end_lineno", getattr(node, "lineno", 1)),
                }
            )
            self.parents.append(node.name)
            self.generic_visit(node)
            self.parents.pop()

        def visit_FunctionDef(self, node: ast.FunctionDef) -> None:
            parent = self.parents[-1] if self.parents else None
            kind = "method" if parent else "function"
            symbols.append(
                {
                    "symbol_name": node.name,
                    "symbol_kind": kind,
                    "parent_symbol": parent,
                    "signature": build_signature(node),
                    "start_line": getattr(node, "lineno", 1),
                    "end_line": getattr(node, "end_lineno", getattr(node, "lineno", 1)),
                }
            )
            self.generic_visit(node)

        def visit_AsyncFunctionDef(self, node: ast.AsyncFunctionDef) -> None:
            parent = self.parents[-1] if self.parents else None
            kind = "async_method" if parent else "async_function"
            symbols.append(
                {
                    "symbol_name": node.name,
                    "symbol_kind": kind,
                    "parent_symbol": parent,
                    "signature": build_signature(node),
                    "start_line": getattr(node, "lineno", 1),
                    "end_line": getattr(node, "end_lineno", getattr(node, "lineno", 1)),
                }
            )
            self.generic_visit(node)

    Visitor().visit(tree)
    return symbols


def generate_code_map(workspace_root: Path) -> tuple[list[dict[str, Any]], list[dict[str, Any]], str]:
    files: list[dict[str, Any]] = []
    symbols: list[dict[str, Any]] = []
    parser_backend = "python-ast+filesystem"

    for path in sorted(workspace_root.rglob("*")):
        if not path.is_file() or should_skip_path(path):
            continue
        if path.name.endswith(".db"):
            continue
        language = infer_language(path)
        rel = str(path.relative_to(workspace_root))
        stat = path.stat()
        files.append(
            {
                "file_path": rel,
                "language": language,
                "module_name": infer_module_name(path, workspace_root),
                "mtime_ns": stat.st_mtime_ns,
                "size_bytes": stat.st_size,
            }
        )
        if language == "python":
            for symbol in extract_python_symbols(path):
                symbols.append({"file_path": rel, **symbol})

    return files, symbols, parser_backend


def refresh_code_index(db: "Database", workspace_root: Path) -> dict[str, Any]:
    root_key = workspace_key(workspace_root)
    started_at = now()
    started = time.perf_counter()
    files, symbols, parser_backend = generate_code_map(workspace_root)
    duration_ms = int((time.perf_counter() - started) * 1000)
    completed_at = now()

    with db.conn:
        db.conn.execute(
            """
            INSERT INTO code_index_runs (
                workspace_root, started_at, completed_at, duration_ms,
                file_count, symbol_count, parser_backend, created_at
            ) VALUES (?, ?, ?, ?, ?, ?, ?, ?)
            """,
            (
                root_key,
                started_at,
                completed_at,
                duration_ms,
                len(files),
                len(symbols),
                parser_backend,
                completed_at,
            ),
        )
        run_id = db.conn.execute("SELECT last_insert_rowid()").fetchone()[0]
        db.conn.execute("DELETE FROM code_symbols WHERE workspace_root = ?", (root_key,))
        db.conn.execute("DELETE FROM code_files WHERE workspace_root = ?", (root_key,))
        db.conn.executemany(
            """
            INSERT INTO code_files (
                workspace_root, file_path, language, module_name, mtime_ns, size_bytes, index_run_id
            ) VALUES (?, ?, ?, ?, ?, ?, ?)
            """,
            [
                (
                    root_key,
                    file_info["file_path"],
                    file_info["language"],
                    file_info["module_name"],
                    file_info["mtime_ns"],
                    file_info["size_bytes"],
                    run_id,
                )
                for file_info in files
            ],
        )
        db.conn.executemany(
            """
            INSERT INTO code_symbols (
                workspace_root, file_path, symbol_name, symbol_kind, parent_symbol,
                signature, start_line, end_line, index_run_id
            ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)
            """,
            [
                (
                    root_key,
                    symbol["file_path"],
                    symbol["symbol_name"],
                    symbol["symbol_kind"],
                    symbol["parent_symbol"],
                    symbol["signature"],
                    symbol["start_line"],
                    symbol["end_line"],
                    run_id,
                )
                for symbol in symbols
            ],
        )

    return {
        "workspace_root": root_key,
        "duration_ms": duration_ms,
        "file_count": len(files),
        "symbol_count": len(symbols),
        "parser_backend": parser_backend,
    }


class Database:
    def __init__(self, db_path: Path):
        self.db_path = db_path
        self.db_path.parent.mkdir(parents=True, exist_ok=True)
        self.conn = sqlite3.connect(str(db_path))
        self.conn.row_factory = sqlite3.Row
        self.conn.execute("PRAGMA foreign_keys = ON")

    def close(self) -> None:
        self.conn.close()

    def init(self) -> None:
        self.conn.executescript(SCHEMA_SQL)
        self.conn.commit()

    def event(self, project_id: str, entity_type: str, entity_id: str, action: str, payload: dict[str, Any]) -> None:
        self.conn.execute(
            """
            INSERT INTO events (project_id, entity_type, entity_id, action, payload_json, created_at)
            VALUES (?, ?, ?, ?, ?, ?)
            """,
            (project_id, entity_type, entity_id, action, json_dump(payload), now()),
        )

    def one(self, query: str, params: tuple[Any, ...]) -> sqlite3.Row:
        row = self.conn.execute(query, params).fetchone()
        if row is None:
            fail("record not found", 2)
        return row


def row_to_project(row: sqlite3.Row) -> dict[str, Any]:
    return {
        "id": row["id"],
        "name": row["name"],
        "goal": row["goal"],
        "scope": row["scope"],
        "constraints": json_load(row["constraints_json"], []),
        "non_goals": json_load(row["non_goals_json"], []),
        "status": row["status"],
        "created_at": row["created_at"],
        "updated_at": row["updated_at"],
    }


def row_to_node(row: sqlite3.Row) -> dict[str, Any]:
    return {
        "id": row["id"],
        "project_id": row["project_id"],
        "parent_id": row["parent_id"],
        "title": row["title"],
        "stage": row["stage"],
        "status": row["status"],
        "summary": row["summary"],
        "depends_on": json_load(row["depends_on_json"], []),
        "artifact_ids": json_load(row["artifact_ids_json"], []),
        "open_question_ids": json_load(row["open_question_ids_json"], []),
        "constraint_package_id": row["constraint_package_id"],
        "created_at": row["created_at"],
        "updated_at": row["updated_at"],
    }


def row_to_artifact(row: sqlite3.Row) -> dict[str, Any]:
    return {
        "id": row["id"],
        "project_id": row["project_id"],
        "node_id": row["node_id"],
        "type": row["type"],
        "title": row["title"],
        "content": json_load(row["content_json"], {}),
        "status": row["status"],
        "version": row["version"],
        "created_at": row["created_at"],
        "updated_at": row["updated_at"],
    }


def artifact_sort_key(artifact: dict[str, Any]) -> tuple[int, int, str, str, str]:
    content = artifact.get("content", {})
    ordered = content.get("ordered_output", {})
    sequence_index = ordered.get("sequence_index")
    has_sequence = isinstance(sequence_index, int) and sequence_index > 0
    document_path = content.get("document_path", "")
    return (
        0 if has_sequence else 1,
        sequence_index if has_sequence else sys.maxsize,
        ordered.get("folder", ""),
        document_path,
        artifact["title"],
    )


def row_to_decision(row: sqlite3.Row) -> dict[str, Any]:
    return {
        "id": row["id"],
        "project_id": row["project_id"],
        "node_id": row["node_id"],
        "decision": row["decision"],
        "rationale": row["rationale"],
        "alternatives": json_load(row["alternatives_json"], []),
        "impact": row["impact"],
        "status": row["status"],
        "created_at": row["created_at"],
        "updated_at": row["updated_at"],
    }


def row_to_question(row: sqlite3.Row) -> dict[str, Any]:
    return {
        "id": row["id"],
        "project_id": row["project_id"],
        "node_id": row["node_id"],
        "question": row["question"],
        "context": row["context"],
        "blocking": bool(row["blocking"]),
        "status": row["status"],
        "resolution": row["resolution"],
        "created_at": row["created_at"],
        "updated_at": row["updated_at"],
    }


def row_to_package(row: sqlite3.Row) -> dict[str, Any]:
    return {
        "id": row["id"],
        "project_id": row["project_id"],
        "node_id": row["node_id"],
        "module_name": row["module_name"],
        "package": json_load(row["package_json"], {}),
        "status": row["status"],
        "created_at": row["created_at"],
        "updated_at": row["updated_at"],
    }


def resolve_mandatory_implementation_context(db: Database, project_id: str) -> dict[str, Any] | None:
    rows = db.conn.execute(
        """
        SELECT *
        FROM artifacts
        WHERE project_id = ? AND type = 'implementation_discovery' AND status = 'approved'
        ORDER BY updated_at DESC, version DESC, created_at DESC
        """,
        (project_id,),
    ).fetchall()
    for row in rows:
        artifact = row_to_artifact(row)
        context = artifact["content"].get("mandatory_implementation_context")
        if not isinstance(context, dict):
            continue
        return {
            "artifact_id": artifact["id"],
            "node_id": artifact["node_id"],
            "selected_profile": artifact["content"].get("selected_profile", {}),
            "mandatory_implementation_context": context,
        }
    return None


def output(payload: dict[str, Any], index_result: dict[str, Any] | None = None) -> None:
    body = {"ok": True, **payload}
    if index_result is not None:
        body["code_index"] = index_result
    print(json.dumps(body, indent=2))


def workspace_defaults(workspace_root: Path) -> dict[str, Any]:
    config_path = workspace_root / ".designctl.json"
    if not config_path.exists():
        return {}
    try:
        return json.loads(config_path.read_text())
    except json.JSONDecodeError as exc:
        fail(f"invalid JSON in {config_path}: {exc}")
    return {}


def project_init(db: Database, args: argparse.Namespace) -> None:
    db.init()
    constraints = parse_json_input(args.constraints_json, args.constraints_file, [])
    non_goals = parse_json_input(args.non_goals_json, args.non_goals_file, [])
    project_id = args.project_id or new_id("proj")
    ts = now()
    db.conn.execute(
        """
        INSERT INTO projects (id, name, goal, scope, constraints_json, non_goals_json, status, created_at, updated_at)
        VALUES (?, ?, ?, ?, ?, ?, 'active', ?, ?)
        """,
        (project_id, args.name, args.goal, args.scope or "", json_dump(constraints), json_dump(non_goals), ts, ts),
    )
    db.event(project_id, "project", project_id, "project.init", {"name": args.name})
    db.conn.commit()
    row = db.one("SELECT * FROM projects WHERE id = ?", (project_id,))
    output({"project": row_to_project(row), "db": str(db.db_path)}, args.code_index_result)


def project_get(db: Database, args: argparse.Namespace) -> None:
    row = db.one("SELECT * FROM projects WHERE id = ? OR name = ?", (args.project, args.project))
    output({"project": row_to_project(row)}, args.code_index_result)


def project_list(db: Database, args: argparse.Namespace) -> None:
    status = args.status or "%"
    rows = db.conn.execute(
        """
        SELECT * FROM projects
        WHERE status LIKE ?
        ORDER BY created_at DESC, name
        """,
        (status,),
    ).fetchall()
    output({"projects": [row_to_project(row) for row in rows]}, args.code_index_result)


def project_update(db: Database, args: argparse.Namespace) -> None:
    project = require_project(db, args.project)
    validate_choice(args.status or project["status"], PROJECT_STATUSES, "project status")

    constraints = json_load(project["constraints_json"], [])
    if args.constraints_json or args.constraints_file:
        constraints = parse_json_input(args.constraints_json, args.constraints_file, [])
    elif args.append_constraint:
        for constraint in args.append_constraint:
            if constraint not in constraints:
                constraints.append(constraint)

    non_goals = json_load(project["non_goals_json"], [])
    if args.non_goals_json or args.non_goals_file:
        non_goals = parse_json_input(args.non_goals_json, args.non_goals_file, [])
    elif args.append_non_goal:
        for non_goal in args.append_non_goal:
            if non_goal not in non_goals:
                non_goals.append(non_goal)

    updates = {
        "name": args.name or project["name"],
        "goal": args.goal if args.goal is not None else project["goal"],
        "scope": args.scope if args.scope is not None else project["scope"],
        "constraints_json": json_dump(constraints),
        "non_goals_json": json_dump(non_goals),
        "status": args.status or project["status"],
        "updated_at": now(),
        "id": project["id"],
    }
    db.conn.execute(
        """
        UPDATE projects
        SET name = ?, goal = ?, scope = ?, constraints_json = ?, non_goals_json = ?, status = ?, updated_at = ?
        WHERE id = ?
        """,
        (
            updates["name"],
            updates["goal"],
            updates["scope"],
            updates["constraints_json"],
            updates["non_goals_json"],
            updates["status"],
            updates["updated_at"],
            updates["id"],
        ),
    )
    db.event(
        project["id"],
        "project",
        project["id"],
        "project.update",
        {
            "name_changed": updates["name"] != project["name"],
            "goal_changed": updates["goal"] != project["goal"],
            "scope_changed": updates["scope"] != project["scope"],
            "constraints_changed": updates["constraints_json"] != project["constraints_json"],
            "non_goals_changed": updates["non_goals_json"] != project["non_goals_json"],
            "status": updates["status"],
        },
    )
    db.conn.commit()
    output({"project": row_to_project(db.one("SELECT * FROM projects WHERE id = ?", (project["id"],)))}, args.code_index_result)


def require_project(db: Database, project: str) -> sqlite3.Row:
    return db.one("SELECT * FROM projects WHERE id = ? OR name = ?", (project, project))


def require_node(db: Database, node_id: str) -> sqlite3.Row:
    return db.one("SELECT * FROM design_nodes WHERE id = ?", (node_id,))


def update_node_links(
    db: Database,
    node_id: str,
    artifact_id: str | None = None,
    question_id: str | None = None,
    constraint_package_id: str | None = None,
) -> None:
    row = require_node(db, node_id)
    artifacts = json_load(row["artifact_ids_json"], [])
    questions = json_load(row["open_question_ids_json"], [])
    if artifact_id and artifact_id not in artifacts:
        artifacts.append(artifact_id)
    if question_id and question_id not in questions:
        questions.append(question_id)
    package_id = constraint_package_id or row["constraint_package_id"]
    db.conn.execute(
        """
        UPDATE design_nodes
        SET artifact_ids_json = ?, open_question_ids_json = ?, constraint_package_id = ?, updated_at = ?
        WHERE id = ?
        """,
        (json_dump(artifacts), json_dump(questions), package_id, now(), node_id),
    )


def node_create(db: Database, args: argparse.Namespace) -> None:
    project = require_project(db, args.project)
    if args.parent_id:
        require_node(db, args.parent_id)
    depends_on = parse_json_input(args.depends_on_json, args.depends_on_file, [])
    node_id = args.node_id or new_id("node")
    ts = now()
    db.conn.execute(
        """
        INSERT INTO design_nodes (
            id, project_id, parent_id, title, stage, status, summary,
            depends_on_json, artifact_ids_json, open_question_ids_json, created_at, updated_at
        ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, '[]', '[]', ?, ?)
        """,
        (node_id, project["id"], args.parent_id, args.title, args.stage, args.status, args.summary or "", json_dump(depends_on), ts, ts),
    )
    db.event(project["id"], "design_node", node_id, "node.create", {"title": args.title, "stage": args.stage})
    db.conn.commit()
    row = require_node(db, node_id)
    output({"node": row_to_node(row)}, args.code_index_result)


def node_update_status(db: Database, args: argparse.Namespace) -> None:
    validate_choice(args.status, NODE_STATUSES, "node status")
    node = require_node(db, args.node_id)
    db.conn.execute("UPDATE design_nodes SET status = ?, updated_at = ? WHERE id = ?", (args.status, now(), args.node_id))
    db.event(node["project_id"], "design_node", args.node_id, "node.update_status", {"status": args.status})
    db.conn.commit()
    output({"node": row_to_node(require_node(db, args.node_id))}, args.code_index_result)


def node_update(db: Database, args: argparse.Namespace) -> None:
    node = require_node(db, args.node_id)
    if args.parent_id and args.clear_parent:
        fail("use only one of --parent-id or --clear-parent")

    title = args.title if args.title is not None else node["title"]
    stage = args.stage if args.stage is not None else node["stage"]
    summary = args.summary if args.summary is not None else node["summary"]
    parent_id = None if args.clear_parent else (args.parent_id if args.parent_id is not None else node["parent_id"])
    if parent_id:
        require_node(db, parent_id)
    depends_on = (
        parse_json_input(args.depends_on_json, args.depends_on_file, [])
        if (args.depends_on_json or args.depends_on_file)
        else json_load(node["depends_on_json"], [])
    )

    db.conn.execute(
        """
        UPDATE design_nodes
        SET title = ?, stage = ?, summary = ?, parent_id = ?, depends_on_json = ?, updated_at = ?
        WHERE id = ?
        """,
        (title, stage, summary, parent_id, json_dump(depends_on), now(), args.node_id),
    )
    db.event(
        node["project_id"],
        "design_node",
        args.node_id,
        "node.update",
        {
            "title_changed": title != node["title"],
            "stage_changed": stage != node["stage"],
            "summary_changed": summary != node["summary"],
            "parent_changed": parent_id != node["parent_id"],
            "depends_on_changed": json_dump(depends_on) != node["depends_on_json"],
        },
    )
    db.conn.commit()
    output({"node": row_to_node(require_node(db, args.node_id))}, args.code_index_result)


def node_get(db: Database, args: argparse.Namespace) -> None:
    output({"node": row_to_node(require_node(db, args.node_id))}, args.code_index_result)


def node_list(db: Database, args: argparse.Namespace) -> None:
    project = require_project(db, args.project)
    rows = db.conn.execute(
        """
        SELECT * FROM design_nodes
        WHERE project_id = ?
        ORDER BY COALESCE(parent_id, ''), created_at, title
        """,
        (project["id"],),
    ).fetchall()
    output({"nodes": [row_to_node(row) for row in rows]}, args.code_index_result)


def artifact_add(db: Database, args: argparse.Namespace) -> None:
    project = require_project(db, args.project)
    if args.node_id:
        require_node(db, args.node_id)
    content = parse_json_input(args.json, args.file, {})
    artifact_id = args.artifact_id or new_id("art")
    ts = now()
    db.conn.execute(
        """
        INSERT INTO artifacts (id, project_id, node_id, type, title, content_json, status, version, created_at, updated_at)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
        """,
        (artifact_id, project["id"], args.node_id, args.type, args.title, json_dump(content), "draft", 1, ts, ts),
    )
    if args.node_id:
        update_node_links(db, args.node_id, artifact_id=artifact_id)
    db.event(project["id"], "artifact", artifact_id, "artifact.add", {"type": args.type, "title": args.title})
    db.conn.commit()
    output({"artifact": row_to_artifact(db.one("SELECT * FROM artifacts WHERE id = ?", (artifact_id,)))}, args.code_index_result)


def artifact_update(db: Database, args: argparse.Namespace) -> None:
    artifact = db.one("SELECT * FROM artifacts WHERE id = ?", (args.artifact_id,))
    title = args.title if args.title is not None else artifact["title"]
    content = (
        parse_json_input(args.json, args.file, json_load(artifact["content_json"], {}))
        if (args.json or args.file)
        else json_load(artifact["content_json"], {})
    )
    version = artifact["version"] + 1 if args.bump_version else artifact["version"]
    db.conn.execute(
        """
        UPDATE artifacts
        SET title = ?, content_json = ?, version = ?, updated_at = ?
        WHERE id = ?
        """,
        (title, json_dump(content), version, now(), args.artifact_id),
    )
    db.event(
        artifact["project_id"],
        "artifact",
        args.artifact_id,
        "artifact.update",
        {
            "title_changed": title != artifact["title"],
            "content_changed": json_dump(content) != artifact["content_json"],
            "version": version,
        },
    )
    db.conn.commit()
    output({"artifact": row_to_artifact(db.one("SELECT * FROM artifacts WHERE id = ?", (args.artifact_id,)))}, args.code_index_result)


def artifact_approve(db: Database, args: argparse.Namespace) -> None:
    validate_choice(args.status, ARTIFACT_STATUSES, "artifact status")
    artifact = db.one("SELECT * FROM artifacts WHERE id = ?", (args.artifact_id,))
    db.conn.execute("UPDATE artifacts SET status = ?, updated_at = ? WHERE id = ?", (args.status, now(), args.artifact_id))
    db.event(artifact["project_id"], "artifact", args.artifact_id, "artifact.update_status", {"status": args.status})
    db.conn.commit()
    output({"artifact": row_to_artifact(db.one("SELECT * FROM artifacts WHERE id = ?", (args.artifact_id,)))}, args.code_index_result)


def artifact_get(db: Database, args: argparse.Namespace) -> None:
    row = db.one("SELECT * FROM artifacts WHERE id = ?", (args.artifact_id,))
    output({"artifact": row_to_artifact(row)}, args.code_index_result)


def artifact_list(db: Database, args: argparse.Namespace) -> None:
    project = require_project(db, args.project)
    status = args.status or "%"
    if args.node_id:
        require_node(db, args.node_id)
        rows = db.conn.execute(
            """
            SELECT * FROM artifacts
            WHERE project_id = ? AND node_id = ? AND status LIKE ?
            ORDER BY created_at DESC, title
            """,
            (project["id"], args.node_id, status),
        ).fetchall()
    else:
        rows = db.conn.execute(
            """
            SELECT * FROM artifacts
            WHERE project_id = ? AND status LIKE ?
            ORDER BY created_at DESC, title
            """,
            (project["id"], status),
        ).fetchall()
    artifacts = [row_to_artifact(row) for row in rows]
    artifacts.sort(key=artifact_sort_key)
    output({"artifacts": artifacts}, args.code_index_result)


def artifact_template(db: Database, args: argparse.Namespace) -> None:
    templates: dict[str, dict[str, Any]] = {
        "implementation_discovery": {
            "document_path": "",
            "constraint": "",
            "ordered_output": {
                "folder": "",
                "readme_path": "",
                "sequence_index": 0,
            },
            "selected_profile": {
                "language": "",
                "build_system": "",
                "api_layer": "",
                "metadata_store": "",
                "persistence_access": "",
                "runtime_model": "",
                "verification_toolchain": [],
            },
            "main_alternative": "",
            "major_tradeoffs": [],
            "mandatory_implementation_context": {
                "summary": [],
                "global": {
                    "required_dependencies": [],
                    "required_tools": [],
                    "forbidden_substitutions": [],
                    "escalate_if": [],
                },
                "per_step": {
                    "implementation_design": {
                        "required_dependencies": [],
                        "required_tools": [],
                        "forbidden_substitutions": [],
                        "escalate_if": [],
                    },
                    "implementation_execution": {
                        "required_dependencies": [],
                        "required_tools": [],
                        "forbidden_substitutions": [],
                        "escalate_if": [],
                    },
                    "implementation_verification_execution": {
                        "required_dependencies": [],
                        "required_tools": [],
                        "forbidden_substitutions": [],
                        "escalate_if": [],
                    },
                },
            },
            "spikes": [],
            "residual_risks": [],
            "escalate_if": [],
        },
        "implementation_design": {
            "document_path": "",
            "ordered_output": {
                "folder": "",
                "readme_path": "",
                "sequence_index": 0,
            },
            "implementation_boundary": {
                "design_owned": [],
                "discovery_owned": [],
                "implementation_design_owned": [],
            },
            "repository_reality": {
                "existing_layout": [],
                "mismatches": [],
                "extraction_or_split_needed": False,
            },
            "realization_map": [],
            "source_tree_plan": {
                "top_level_dirs": [],
                "build_targets": [],
                "test_targets": [],
                "fixture_locations": [],
            },
            "interface_map": [],
            "persistence_plan": {},
            "runtime_plan": {},
            "verification_mapping": [],
            "integration_plan": {
                "dependency_order": [],
                "parallelizable_packets": [],
            },
            "migration_or_extraction_plan": [],
            "implementation_packets": [],
            "operating_surface": {
                "standard_commands": [],
                "required_scripts_or_tasks": [],
            },
            "residual_risks": [],
            "escalate_if": [],
        },
        "implementation_execution": {
            "document_path": "",
            "ordered_output": {
                "folder": "",
                "readme_path": "",
                "sequence_index": 0,
            },
            "active_packet": {
                "name": "",
                "scope": [],
                "touches": [],
                "do_not_cross": [],
            },
            "proof_obligations": [],
            "deferred_proofs": [],
            "operating_loop": {
                "build_command": "",
                "fast_test_command": "",
                "diagnostic_commands": [],
                "benchmark_commands": [],
            },
            "vertical_slice": [],
            "ownership_and_lifetime_notes": [],
            "resource_safety_notes": [],
            "regression_additions": [],
            "benchmark_notes": [],
            "acceptance_results": [],
            "execution_evidence": [],
            "residual_risks": [],
            "next_execution_decision": "",
            "escalate_if": [],
        },
        "implementation_verification_execution": {
            "document_path": "",
            "ordered_output": {
                "folder": "",
                "readme_path": "",
                "sequence_index": 0,
            },
            "verification_scope": [],
            "verification_matrix": {
                "functional": [],
                "safety": [],
                "performance": [],
            },
            "functional_results": [],
            "safety_results": [],
            "benchmark_results": [],
            "regression_comparison": [],
            "failure_triage": [],
            "verification_verdict": "",
            "release_gate_status": "",
            "verification_evidence": [],
            "next_step_decision": "",
        },
    }
    if args.type not in templates:
        fail(f"no built-in template for artifact type: {args.type}")
    output({"artifact_template": {"type": args.type, "content": templates[args.type]}}, args.code_index_result)


def decision_record(db: Database, args: argparse.Namespace) -> None:
    project = require_project(db, args.project)
    if args.node_id:
        require_node(db, args.node_id)
    validate_choice(args.status, DECISION_STATUSES, "decision status")
    alternatives = parse_json_input(args.alternatives_json, args.alternatives_file, [])
    decision_id = args.decision_id or new_id("dec")
    ts = now()
    db.conn.execute(
        """
        INSERT INTO decisions (id, project_id, node_id, decision, rationale, alternatives_json, impact, status, created_at, updated_at)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
        """,
        (decision_id, project["id"], args.node_id, args.decision, args.rationale or "", json_dump(alternatives), args.impact or "", args.status, ts, ts),
    )
    db.event(project["id"], "decision", decision_id, "decision.record", {"status": args.status})
    db.conn.commit()
    output({"decision": row_to_decision(db.one("SELECT * FROM decisions WHERE id = ?", (decision_id,)))}, args.code_index_result)


def decision_update(db: Database, args: argparse.Namespace) -> None:
    decision = db.one("SELECT * FROM decisions WHERE id = ?", (args.decision_id,))
    text = args.decision if args.decision is not None else decision["decision"]
    rationale = args.rationale if args.rationale is not None else decision["rationale"]
    impact = args.impact if args.impact is not None else decision["impact"]
    alternatives = (
        parse_json_input(args.alternatives_json, args.alternatives_file, [])
        if (args.alternatives_json or args.alternatives_file)
        else json_load(decision["alternatives_json"], [])
    )
    db.conn.execute(
        """
        UPDATE decisions
        SET decision = ?, rationale = ?, alternatives_json = ?, impact = ?, updated_at = ?
        WHERE id = ?
        """,
        (text, rationale, json_dump(alternatives), impact, now(), args.decision_id),
    )
    db.event(
        decision["project_id"],
        "decision",
        args.decision_id,
        "decision.update",
        {
            "decision_changed": text != decision["decision"],
            "rationale_changed": rationale != decision["rationale"],
            "alternatives_changed": json_dump(alternatives) != decision["alternatives_json"],
            "impact_changed": impact != decision["impact"],
        },
    )
    db.conn.commit()
    output({"decision": row_to_decision(db.one("SELECT * FROM decisions WHERE id = ?", (args.decision_id,)))}, args.code_index_result)


def decision_update_status(db: Database, args: argparse.Namespace) -> None:
    validate_choice(args.status, DECISION_STATUSES, "decision status")
    decision = db.one("SELECT * FROM decisions WHERE id = ?", (args.decision_id,))
    db.conn.execute("UPDATE decisions SET status = ?, updated_at = ? WHERE id = ?", (args.status, now(), args.decision_id))
    db.event(decision["project_id"], "decision", args.decision_id, "decision.update_status", {"status": args.status})
    db.conn.commit()
    output({"decision": row_to_decision(db.one("SELECT * FROM decisions WHERE id = ?", (args.decision_id,)))}, args.code_index_result)


def decision_get(db: Database, args: argparse.Namespace) -> None:
    row = db.one("SELECT * FROM decisions WHERE id = ?", (args.decision_id,))
    output({"decision": row_to_decision(row)}, args.code_index_result)


def decision_list(db: Database, args: argparse.Namespace) -> None:
    project = require_project(db, args.project)
    status = args.status or "%"
    if args.node_id:
        require_node(db, args.node_id)
        rows = db.conn.execute(
            """
            SELECT * FROM decisions
            WHERE project_id = ? AND node_id = ? AND status LIKE ?
            ORDER BY created_at
            """,
            (project["id"], args.node_id, status),
        ).fetchall()
    else:
        rows = db.conn.execute(
            "SELECT * FROM decisions WHERE project_id = ? AND status LIKE ? ORDER BY created_at",
            (project["id"], status),
        ).fetchall()
    output({"decisions": [row_to_decision(row) for row in rows]}, args.code_index_result)


def question_record(db: Database, args: argparse.Namespace) -> None:
    project = require_project(db, args.project)
    if args.node_id:
        require_node(db, args.node_id)
    question_id = args.question_id or new_id("q")
    ts = now()
    db.conn.execute(
        """
        INSERT INTO open_questions (id, project_id, node_id, question, context, blocking, status, resolution, created_at, updated_at)
        VALUES (?, ?, ?, ?, ?, ?, 'resolved', ?, ?, ?)
        """,
        (question_id, project["id"], args.node_id, args.question, args.context or "", 1 if args.blocking else 0, args.resolution, ts, ts),
    )
    if args.node_id:
        update_node_links(db, args.node_id, question_id=question_id)
    db.event(project["id"], "open_question", question_id, "question.record", {"blocking": bool(args.blocking)})
    db.conn.commit()
    output({"question": row_to_question(db.one("SELECT * FROM open_questions WHERE id = ?", (question_id,)))}, args.code_index_result)


def question_open(db: Database, args: argparse.Namespace) -> None:
    raise SystemExit(
        "question open is disabled: durable open questions are not allowed; use `question record --resolution ...` or resolve any legacy open question immediately"
    )


def question_resolve(db: Database, args: argparse.Namespace) -> None:
    question = db.one("SELECT * FROM open_questions WHERE id = ?", (args.question_id,))
    db.conn.execute(
        "UPDATE open_questions SET status = 'resolved', resolution = ?, updated_at = ? WHERE id = ?",
        (args.resolution, now(), args.question_id),
    )
    db.event(question["project_id"], "open_question", args.question_id, "question.resolve", {"resolution": args.resolution})
    db.conn.commit()
    output({"question": row_to_question(db.one("SELECT * FROM open_questions WHERE id = ?", (args.question_id,)))}, args.code_index_result)


def question_get(db: Database, args: argparse.Namespace) -> None:
    row = db.one("SELECT * FROM open_questions WHERE id = ?", (args.question_id,))
    output({"question": row_to_question(row)}, args.code_index_result)


def question_list(db: Database, args: argparse.Namespace) -> None:
    project = require_project(db, args.project)
    status = args.status or "%"
    rows = db.conn.execute(
        """
        SELECT * FROM open_questions
        WHERE project_id = ? AND status LIKE ?
        ORDER BY blocking DESC, created_at
        """,
        (project["id"], status),
    ).fetchall()
    output({"questions": [row_to_question(row) for row in rows]}, args.code_index_result)


def package_create(db: Database, args: argparse.Namespace) -> None:
    project = require_project(db, args.project)
    node = require_node(db, args.node_id)
    package = parse_json_input(args.json, args.file, {})
    package_id = args.package_id or new_id("pkg")
    ts = now()
    db.conn.execute(
        """
        INSERT INTO constraint_packages (id, project_id, node_id, module_name, package_json, status, created_at, updated_at)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?)
        """,
        (package_id, project["id"], args.node_id, args.module_name, json_dump(package), args.status, ts, ts),
    )
    update_node_links(db, node["id"], constraint_package_id=package_id)
    db.event(project["id"], "constraint_package", package_id, "constraint_package.create", {"module_name": args.module_name})
    db.conn.commit()
    output({"constraint_package": row_to_package(db.one("SELECT * FROM constraint_packages WHERE id = ?", (package_id,)))}, args.code_index_result)


def package_update_status(db: Database, args: argparse.Namespace) -> None:
    validate_choice(args.status, PACKAGE_STATUSES, "constraint package status")
    row = db.one("SELECT * FROM constraint_packages WHERE id = ?", (args.package_id,))
    db.conn.execute("UPDATE constraint_packages SET status = ?, updated_at = ? WHERE id = ?", (args.status, now(), args.package_id))
    db.event(row["project_id"], "constraint_package", args.package_id, "constraint_package.update_status", {"status": args.status})
    db.conn.commit()
    output({"constraint_package": row_to_package(db.one("SELECT * FROM constraint_packages WHERE id = ?", (args.package_id,)))}, args.code_index_result)


def package_get(db: Database, args: argparse.Namespace) -> None:
    row = db.one("SELECT * FROM constraint_packages WHERE id = ? OR node_id = ?", (args.id_or_node, args.id_or_node))
    output({"constraint_package": row_to_package(row)}, args.code_index_result)


def package_list(db: Database, args: argparse.Namespace) -> None:
    project = require_project(db, args.project)
    status = args.status or "%"
    rows = db.conn.execute(
        """
        SELECT * FROM constraint_packages
        WHERE project_id = ? AND status LIKE ?
        ORDER BY created_at, module_name
        """,
        (project["id"], status),
    ).fetchall()
    output({"constraint_packages": [row_to_package(row) for row in rows]}, args.code_index_result)


def next_work_payload(db: Database, project: sqlite3.Row) -> dict[str, Any]:
    mandatory_context = resolve_mandatory_implementation_context(db, project["id"])
    rows = db.conn.execute(
        """
        SELECT *
        FROM design_nodes
        WHERE project_id = ? AND status IN ('todo', 'in_progress', 'blocked')
        ORDER BY
            CASE status WHEN 'in_progress' THEN 0 WHEN 'todo' THEN 1 ELSE 2 END,
            created_at
        """,
        (project["id"],),
    ).fetchall()
    open_questions = {
        row["node_id"]: []
        for row in db.conn.execute(
            "SELECT node_id FROM open_questions WHERE project_id = ? AND status = 'open'",
            (project["id"],),
        ).fetchall()
        if row["node_id"]
    }
    for row in db.conn.execute(
        "SELECT * FROM open_questions WHERE project_id = ? AND status = 'open' ORDER BY blocking DESC, created_at",
        (project["id"],),
    ).fetchall():
        if row["node_id"]:
            open_questions.setdefault(row["node_id"], []).append(row_to_question(row))
    candidates = []
    for row in rows:
        node = row_to_node(row)
        dependencies = node["depends_on"]
        blocked_by_dependencies = []
        for dep_id in dependencies:
            dep = db.one("SELECT status FROM design_nodes WHERE id = ?", (dep_id,))
            if dep["status"] != "done":
                blocked_by_dependencies.append({"node_id": dep_id, "status": dep["status"]})
        blocking_questions = [q for q in open_questions.get(node["id"], []) if q["blocking"]]
        blocked_by_mandatory_context = []
        if node["stage"] in DOWNSTREAM_IMPLEMENTATION_STAGES:
            if mandatory_context is None:
                blocked_by_mandatory_context.append(
                    {
                        "reason": "approved implementation_discovery artifact with mandatory_implementation_context is required",
                    }
                )
            else:
                per_step = mandatory_context["mandatory_implementation_context"].get("per_step", {})
                if not isinstance(per_step, dict) or node["stage"] not in per_step:
                    blocked_by_mandatory_context.append(
                        {
                            "reason": f"mandatory_implementation_context is missing per-step guidance for {node['stage']}",
                            "artifact_id": mandatory_context["artifact_id"],
                        }
                    )
        candidates.append(
            {
                "node": node,
                "blocked_by_dependencies": blocked_by_dependencies,
                "blocking_questions": blocking_questions,
                "blocked_by_mandatory_context": blocked_by_mandatory_context,
                "ready": not blocked_by_dependencies
                and not blocking_questions
                and not blocked_by_mandatory_context
                and node["status"] != "blocked",
            }
        )
    ready = [candidate for candidate in candidates if candidate["ready"]]
    return {
        "next_work": ready[0] if ready else None,
        "candidates": candidates,
        "mandatory_implementation_context": mandatory_context,
    }


def missing_stage_payload(tree_rows: list[sqlite3.Row]) -> list[dict[str, str]]:
    existing_stages = {row["stage"] for row in tree_rows}
    missing = []
    for stage, title in SOFTWARE_DESIGN_STAGE_SEQUENCE:
        if stage not in existing_stages:
            missing.append(
                {
                    "stage": stage,
                    "title": title,
                    "reason": "canonical software-design stage has no design node",
                }
            )
    return missing


def query_tree(db: Database, args: argparse.Namespace) -> None:
    project = require_project(db, args.project)
    rows = db.conn.execute(
        """
        SELECT id, parent_id, title, stage, status, summary, depends_on_json, constraint_package_id
        FROM design_nodes
        WHERE project_id = ?
        ORDER BY created_at, title
        """,
        (project["id"],),
    ).fetchall()
    nodes = []
    for row in rows:
        nodes.append(
            {
                "id": row["id"],
                "parent_id": row["parent_id"],
                "title": row["title"],
                "stage": row["stage"],
                "status": row["status"],
                "summary": row["summary"],
                "depends_on": json_load(row["depends_on_json"], []),
                "constraint_package_id": row["constraint_package_id"],
            }
        )
    output({"project": row_to_project(project), "tree": nodes}, args.code_index_result)


def query_next_work(db: Database, args: argparse.Namespace) -> None:
    project = require_project(db, args.project)
    output(next_work_payload(db, project), args.code_index_result)


def query_events(db: Database, args: argparse.Namespace) -> None:
    project = require_project(db, args.project)
    rows = db.conn.execute(
        """
        SELECT * FROM events
        WHERE project_id = ?
        ORDER BY id DESC
        LIMIT ?
        """,
        (project["id"], args.limit),
    ).fetchall()
    events = [
        {
            "id": row["id"],
            "entity_type": row["entity_type"],
            "entity_id": row["entity_id"],
            "action": row["action"],
            "payload": json_load(row["payload_json"], {}),
            "created_at": row["created_at"],
        }
        for row in rows
    ]
    output({"events": events}, args.code_index_result)


def query_code_map(db: Database, args: argparse.Namespace) -> None:
    workspace_root = workspace_key(Path(args.workspace))
    files = [
        {
            "file_path": row["file_path"],
            "language": row["language"],
            "module_name": row["module_name"],
            "mtime_ns": row["mtime_ns"],
            "size_bytes": row["size_bytes"],
        }
        for row in db.conn.execute(
            """
            SELECT file_path, language, module_name, mtime_ns, size_bytes
            FROM code_files
            WHERE workspace_root = ?
            ORDER BY file_path
            """,
            (workspace_root,),
        ).fetchall()
    ]
    symbols = [
        {
            "file_path": row["file_path"],
            "symbol_name": row["symbol_name"],
            "symbol_kind": row["symbol_kind"],
            "parent_symbol": row["parent_symbol"],
            "signature": row["signature"],
            "start_line": row["start_line"],
            "end_line": row["end_line"],
        }
        for row in db.conn.execute(
            """
            SELECT file_path, symbol_name, symbol_kind, parent_symbol, signature, start_line, end_line
            FROM code_symbols
            WHERE workspace_root = ?
            ORDER BY file_path, start_line, symbol_name
            """,
            (workspace_root,),
        ).fetchall()
    ]
    output({"code_map": {"workspace_root": workspace_root, "files": files, "symbols": symbols}}, args.code_index_result)


def select_resume_project(db: Database, args: argparse.Namespace) -> tuple[sqlite3.Row, str, list[dict[str, Any]]]:
    if args.project:
        project = require_project(db, args.project)
        return project, "explicit --project", []

    defaults = workspace_defaults(Path(args.workspace))
    default_project = defaults.get("default_project")
    if default_project:
        project = require_project(db, default_project)
        return project, f"workspace default_project={default_project}", []

    projects = db.conn.execute(
        """
        SELECT *
        FROM projects
        WHERE status = 'active'
        ORDER BY created_at DESC
        """
    ).fetchall()
    if not projects:
        fail("no active projects found")

    ranked = []
    for project in projects:
        next_payload = next_work_payload(db, project)
        unresolved_count = len(next_payload["candidates"])
        ready_count = sum(1 for candidate in next_payload["candidates"] if candidate["ready"])
        last_event = db.conn.execute(
            "SELECT created_at FROM events WHERE project_id = ? ORDER BY id DESC LIMIT 1",
            (project["id"],),
        ).fetchone()
        last_activity_at = last_event["created_at"] if last_event else project["created_at"]
        ranked.append(
            {
                "project": row_to_project(project),
                "ready_count": ready_count,
                "unresolved_count": unresolved_count,
                "last_activity_at": last_activity_at,
            }
        )

    ranked.sort(
        key=lambda item: (
            item["ready_count"] > 0,
            item["unresolved_count"] > 0,
            item["last_activity_at"],
            item["project"]["created_at"],
        ),
        reverse=True,
    )
    project = require_project(db, ranked[0]["project"]["id"])
    return project, "auto-selected most active project", ranked


def resume(db: Database, args: argparse.Namespace) -> None:
    project, selection_reason, ranked_projects = select_resume_project(db, args)
    next_payload = next_work_payload(db, project)
    tree_rows = db.conn.execute(
        """
        SELECT id, parent_id, title, stage, status, summary, depends_on_json, constraint_package_id
        FROM design_nodes
        WHERE project_id = ?
        ORDER BY created_at, title
        """,
        (project["id"],),
    ).fetchall()
    tree = [
        {
            "id": row["id"],
            "parent_id": row["parent_id"],
            "title": row["title"],
            "stage": row["stage"],
            "status": row["status"],
            "summary": row["summary"],
            "depends_on": json_load(row["depends_on_json"], []),
            "constraint_package_id": row["constraint_package_id"],
        }
        for row in tree_rows
    ]
    missing_stages = missing_stage_payload(tree_rows)
    recent_decisions = [
        row_to_decision(row)
        for row in db.conn.execute(
            """
            SELECT *
            FROM decisions
            WHERE project_id = ?
            ORDER BY created_at DESC
            LIMIT ?
            """,
            (project["id"], args.decision_limit),
        ).fetchall()
    ]
    recent_events = [
        {
            "id": row["id"],
            "entity_type": row["entity_type"],
            "entity_id": row["entity_id"],
            "action": row["action"],
            "payload": json_load(row["payload_json"], {}),
            "created_at": row["created_at"],
        }
        for row in db.conn.execute(
            """
            SELECT *
            FROM events
            WHERE project_id = ?
            ORDER BY id DESC
            LIMIT ?
            """,
            (project["id"], args.event_limit),
        ).fetchall()
    ]
    unresolved = next_payload["candidates"]
    if next_payload["next_work"]:
        status = "ready"
    elif missing_stages:
        status = "incomplete"
    elif unresolved:
        status = "blocked"
    else:
        status = "complete"
    output(
        {
            "entrypoint": {
                "name": "design",
                "description": "single workflow resume entry point",
                "repo_command": "python3 -m autoswe resume",
                "tool_command": f"python3 -m autoswe --db {args.db} --workspace . resume",
            },
            "selected_project": row_to_project(project),
            "selection_reason": selection_reason,
            "project_candidates": ranked_projects,
            "status": status,
            "next_work": next_payload["next_work"],
            "candidates": next_payload["candidates"],
            "mandatory_implementation_context": next_payload["mandatory_implementation_context"],
            "missing_stages": missing_stages,
            "tree": tree,
            "recent_decisions": recent_decisions,
            "recent_events": recent_events,
        },
        args.code_index_result,
    )


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="Design source-of-truth CLI")
    parser.add_argument("--db", default=DEFAULT_DB, help="Path to SQLite database file")
    parser.add_argument("--workspace", default=".", help="Workspace root to index")
    parser.add_argument("--skip-index", action="store_true", help="Disable automatic code map refresh")
    parser.add_argument(
        "--index-max-seconds",
        type=float,
        default=5.0,
        help="Fail the run if code map refresh exceeds this duration",
    )
    subparsers = parser.add_subparsers(dest="command", required=True)

    project = subparsers.add_parser("project")
    project_sub = project.add_subparsers(dest="subcommand", required=True)
    p_init = project_sub.add_parser("init")
    p_init.add_argument("--name", required=True)
    p_init.add_argument("--goal", required=True)
    p_init.add_argument("--scope")
    p_init.add_argument("--constraints-json")
    p_init.add_argument("--constraints-file")
    p_init.add_argument("--non-goals-json")
    p_init.add_argument("--non-goals-file")
    p_init.add_argument("--project-id")
    p_init.set_defaults(func=project_init)
    p_get = project_sub.add_parser("get")
    p_get.add_argument("--project", required=True)
    p_get.set_defaults(func=project_get)
    p_list = project_sub.add_parser("list")
    p_list.add_argument("--status")
    p_list.set_defaults(func=project_list)
    p_update = project_sub.add_parser("update")
    p_update.add_argument("--project", required=True)
    p_update.add_argument("--name")
    p_update.add_argument("--goal")
    p_update.add_argument("--scope")
    p_update.add_argument("--constraints-json")
    p_update.add_argument("--constraints-file")
    p_update.add_argument("--append-constraint", action="append")
    p_update.add_argument("--non-goals-json")
    p_update.add_argument("--non-goals-file")
    p_update.add_argument("--append-non-goal", action="append")
    p_update.add_argument("--status")
    p_update.set_defaults(func=project_update)

    node = subparsers.add_parser("node")
    node_sub = node.add_subparsers(dest="subcommand", required=True)
    n_create = node_sub.add_parser("create")
    n_create.add_argument("--project", required=True)
    n_create.add_argument("--title", required=True)
    n_create.add_argument("--stage", required=True)
    n_create.add_argument("--status", default="todo")
    n_create.add_argument("--summary")
    n_create.add_argument("--parent-id")
    n_create.add_argument("--depends-on-json")
    n_create.add_argument("--depends-on-file")
    n_create.add_argument("--node-id")
    n_create.set_defaults(func=node_create)
    n_status = node_sub.add_parser("update-status")
    n_status.add_argument("--node-id", required=True)
    n_status.add_argument("--status", required=True)
    n_status.set_defaults(func=node_update_status)
    n_update = node_sub.add_parser("update")
    n_update.add_argument("--node-id", required=True)
    n_update.add_argument("--title")
    n_update.add_argument("--stage")
    n_update.add_argument("--summary")
    n_update.add_argument("--parent-id")
    n_update.add_argument("--clear-parent", action="store_true")
    n_update.add_argument("--depends-on-json")
    n_update.add_argument("--depends-on-file")
    n_update.set_defaults(func=node_update)
    n_get = node_sub.add_parser("get")
    n_get.add_argument("--node-id", required=True)
    n_get.set_defaults(func=node_get)
    n_list = node_sub.add_parser("list")
    n_list.add_argument("--project", required=True)
    n_list.set_defaults(func=node_list)

    artifact = subparsers.add_parser("artifact")
    artifact_sub = artifact.add_subparsers(dest="subcommand", required=True)
    a_add = artifact_sub.add_parser("add")
    a_add.add_argument("--project", required=True)
    a_add.add_argument("--node-id")
    a_add.add_argument("--type", required=True)
    a_add.add_argument("--title", required=True)
    a_add.add_argument("--json")
    a_add.add_argument("--file")
    a_add.add_argument("--artifact-id")
    a_add.set_defaults(func=artifact_add)
    a_update = artifact_sub.add_parser("update")
    a_update.add_argument("--artifact-id", required=True)
    a_update.add_argument("--title")
    a_update.add_argument("--json")
    a_update.add_argument("--file")
    a_update.add_argument("--bump-version", action="store_true")
    a_update.set_defaults(func=artifact_update)
    a_approve = artifact_sub.add_parser("update-status")
    a_approve.add_argument("--artifact-id", required=True)
    a_approve.add_argument("--status", required=True)
    a_approve.set_defaults(func=artifact_approve)
    a_get = artifact_sub.add_parser("get")
    a_get.add_argument("--artifact-id", required=True)
    a_get.set_defaults(func=artifact_get)
    a_list = artifact_sub.add_parser("list")
    a_list.add_argument("--project", required=True)
    a_list.add_argument("--node-id")
    a_list.add_argument("--status")
    a_list.set_defaults(func=artifact_list)
    a_template = artifact_sub.add_parser("template")
    a_template.add_argument("--type", required=True)
    a_template.set_defaults(func=artifact_template)

    decision = subparsers.add_parser("decision")
    decision_sub = decision.add_subparsers(dest="subcommand", required=True)
    d_record = decision_sub.add_parser("record")
    d_record.add_argument("--project", required=True)
    d_record.add_argument("--node-id")
    d_record.add_argument("--decision", required=True)
    d_record.add_argument("--rationale")
    d_record.add_argument("--alternatives-json")
    d_record.add_argument("--alternatives-file")
    d_record.add_argument("--impact")
    d_record.add_argument("--status", default="accepted")
    d_record.add_argument("--decision-id")
    d_record.set_defaults(func=decision_record)
    d_list = decision_sub.add_parser("list")
    d_list.add_argument("--project", required=True)
    d_list.add_argument("--node-id")
    d_list.add_argument("--status")
    d_list.set_defaults(func=decision_list)
    d_get = decision_sub.add_parser("get")
    d_get.add_argument("--decision-id", required=True)
    d_get.set_defaults(func=decision_get)
    d_update = decision_sub.add_parser("update")
    d_update.add_argument("--decision-id", required=True)
    d_update.add_argument("--decision")
    d_update.add_argument("--rationale")
    d_update.add_argument("--alternatives-json")
    d_update.add_argument("--alternatives-file")
    d_update.add_argument("--impact")
    d_update.set_defaults(func=decision_update)
    d_update_status = decision_sub.add_parser("update-status")
    d_update_status.add_argument("--decision-id", required=True)
    d_update_status.add_argument("--status", required=True)
    d_update_status.set_defaults(func=decision_update_status)

    question = subparsers.add_parser("question")
    question_sub = question.add_subparsers(dest="subcommand", required=True)
    q_record = question_sub.add_parser("record")
    q_record.add_argument("--project", required=True)
    q_record.add_argument("--node-id")
    q_record.add_argument("--question", required=True)
    q_record.add_argument("--resolution", required=True)
    q_record.add_argument("--context")
    q_record.add_argument("--blocking", action="store_true")
    q_record.add_argument("--question-id")
    q_record.set_defaults(func=question_record)
    q_open = question_sub.add_parser("open")
    q_open.add_argument("--project", required=True)
    q_open.add_argument("--node-id")
    q_open.add_argument("--question", required=True)
    q_open.add_argument("--context")
    q_open.add_argument("--blocking", action="store_true")
    q_open.add_argument("--question-id")
    q_open.set_defaults(func=question_open)
    q_resolve = question_sub.add_parser("resolve")
    q_resolve.add_argument("--question-id", required=True)
    q_resolve.add_argument("--resolution", required=True)
    q_resolve.set_defaults(func=question_resolve)
    q_get = question_sub.add_parser("get")
    q_get.add_argument("--question-id", required=True)
    q_get.set_defaults(func=question_get)
    q_list = question_sub.add_parser("list")
    q_list.add_argument("--project", required=True)
    q_list.add_argument("--status")
    q_list.set_defaults(func=question_list)

    package = subparsers.add_parser("constraint-package")
    package_sub = package.add_subparsers(dest="subcommand", required=True)
    c_create = package_sub.add_parser("create")
    c_create.add_argument("--project", required=True)
    c_create.add_argument("--node-id", required=True)
    c_create.add_argument("--module-name", required=True)
    c_create.add_argument("--json")
    c_create.add_argument("--file")
    c_create.add_argument("--status", default="draft")
    c_create.add_argument("--package-id")
    c_create.set_defaults(func=package_create)
    c_get = package_sub.add_parser("get")
    c_get.add_argument("--id-or-node", required=True)
    c_get.set_defaults(func=package_get)
    c_list = package_sub.add_parser("list")
    c_list.add_argument("--project", required=True)
    c_list.add_argument("--status")
    c_list.set_defaults(func=package_list)
    c_update = package_sub.add_parser("update-status")
    c_update.add_argument("--package-id", required=True)
    c_update.add_argument("--status", required=True)
    c_update.set_defaults(func=package_update_status)

    query = subparsers.add_parser("query")
    query_sub = query.add_subparsers(dest="subcommand", required=True)
    q_tree = query_sub.add_parser("tree")
    q_tree.add_argument("--project", required=True)
    q_tree.set_defaults(func=query_tree)
    q_next = query_sub.add_parser("next-work")
    q_next.add_argument("--project", required=True)
    q_next.set_defaults(func=query_next_work)
    q_events = query_sub.add_parser("events")
    q_events.add_argument("--project", required=True)
    q_events.add_argument("--limit", type=int, default=20)
    q_events.set_defaults(func=query_events)
    q_map = query_sub.add_parser("code-map")
    q_map.set_defaults(func=query_code_map)

    resume_cmd = subparsers.add_parser("resume")
    resume_cmd.add_argument("--project")
    resume_cmd.add_argument("--decision-limit", type=int, default=5)
    resume_cmd.add_argument("--event-limit", type=int, default=10)
    resume_cmd.set_defaults(func=resume)

    return parser


def main() -> None:
    parser = build_parser()
    args = parser.parse_args()
    db = Database(Path(args.db))
    try:
        db.init()
        args.code_index_result = None
        if not args.skip_index:
            workspace_root = Path(args.workspace)
            index_result = refresh_code_index(db, workspace_root)
            args.code_index_result = index_result
            print(
                f"code map refresh: {index_result['duration_ms']} ms "
                f"({index_result['file_count']} files, {index_result['symbol_count']} symbols, "
                f"backend={index_result['parser_backend']})",
                file=sys.stderr,
            )
            if index_result["duration_ms"] > int(args.index_max_seconds * 1000):
                fail(
                    "code map refresh exceeded time limit "
                    f"({index_result['duration_ms']} ms > {int(args.index_max_seconds * 1000)} ms). "
                    "Re-run with --skip-index to disable it."
                )
        if hasattr(args, "status") and args.command == "node" and args.subcommand in {"create", "update-status"}:
            validate_choice(args.status, NODE_STATUSES, "node status")
        if hasattr(args, "status") and args.command == "project" and args.subcommand == "update":
            validate_choice(getattr(args, "status", "active") or "active", PROJECT_STATUSES, "project status")
        if args.command == "constraint-package" and args.subcommand in {"create", "update-status"}:
            validate_choice(getattr(args, "status", "draft"), PACKAGE_STATUSES, "constraint package status")
        args.func(db, args)
    finally:
        db.close()


if __name__ == "__main__":
    main()
