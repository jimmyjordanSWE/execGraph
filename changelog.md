# Changelog

## 2026-08-17

- Reframed the repository as an honest public case study of the March 9 one-day experiment, separating the autoSWE development method from the ExecGraph product hypothesis.
- Documented the stronger agent-execution-ledger direction, its security boundary, and the difference between implemented runtime foundations and future sandboxing and provenance work.
- Added a reproducible Linux CI workflow and explicit prototype security warning.
- Added the MIT license for public release.
- Sanitized the committed design-ledger snapshot by removing generated code-index rows and machine-specific paths while retaining the experiment's decisions, artifacts, constraint packages, and audit events.
- Removed a redundant copy of the original AI-assisted software-design reference transcript.

## 2026-03-09

- Added owned process-group supervision and per-command `timeout_ms` / `graceful_shutdown_ms` controls to exec-graph workflows and graph nodes, with timeout-driven `SIGTERM` then `SIGKILL` behavior plus `process.stop.requested` and `process.kill.sent` runtime events and smoke coverage.
- Taught autoSWE code indexing to skip generated `build/` trees so workflow resume does not time out after CMake-generated compile databases or fetched third-party sources appear under build outputs.
- Enabled exec-graph CMake compile database generation in all presets and added workspace editor settings to point at the debug `compile_commands.json` output.
- Replaced exec-graph's handwritten JSONL event escaping with Boost.JSON-backed serialization, and taught the CMake build to bootstrap Boost 1.83 headers automatically when the host toolchain does not already provide Boost.JSON.
- Updated the autoSWE implementation workflow so implementation discovery must emit a mandatory implementation-context artifact, and downstream implementation design, execution, and verification steps must consume it.
- Widened exec-graph JSONL runtime events with `workflow.started`, `workflow.step.started`, `workflow.step.completed`, `workflow.step.failed`, `workflow.completed`, and `workflow.failed`, plus smoke coverage for workflow-layer progress and terminal outcomes.
- Fixed file-based workflow and graph execution so command paths resolve relative to the workflow or graph file directory, and added smoke coverage for running examples from the `exec_graph` product root.
- Persisted stored-graph working directories through SQLite so repository roundtrips keep file-local command resolution intact.
- Fixed `eg_demo_pipeline` default migration-path resolution so stored-graph save/load also works from the `exec_graph` product root.
- Widened exec-graph JSONL runtime events again with `graph.node.started`, `graph.node.completed`, and `graph.node.failed`, plus smoke coverage for graph-layer per-node progress.
- Hardened exec-graph SQLite infra with rollback-safe transaction guards, connection-level busy-timeout and foreign-key setup, and a migration-idempotency smoke test with discovered/applied/skipped reporting.
- Fixed `eg_migrate` default migration-path resolution so it works from both the repo root and the `exec_graph/` product root.
- Widened exec-graph JSONL runtime events with `graph.started`, `graph.completed`, and `graph.failed`, plus smoke coverage for graph-level terminal outcomes.
- Expanded the autoSWE design phase to include explicit `Requirements definition`, `Behavior and workflow design`, `Security and trust-boundary design`, and `Operational design` stages before `Implementation handoff`.
- Defined `Implementation discovery` as the first implementation-phase step after design, and updated the canonical workflow, CLI stage sequence, and the `autoswe` project graph in `.design/design.db` to match.
- Added canonical post-design workflow docs for `Implementation discovery` and `Implementation design` so the implementation phase now has explicit first steps after design completion.
- Packaged the workflow tool as the local Python package `autoswe` with a `pyproject.toml`, console scripts, and a direct module entrypoint.
- Replaced the repo-local launcher path with `python3 -m autoswe resume` and updated core docs to use the packaged CLI.
- Renamed the design-system track to autoSWE, moved its docs to `docs/autoswe/`, moved canonical workflow files to `workflows/autoswe/`, and renamed the design project in `.design/design.db` to `autoswe`.
- Cleaned the repo layout by moving live workflow state to `.design/design.db` and relocating the source skill reference under `docs/autoswe/99-reference/reference/`.
- Updated `designctl` defaults and repo docs to match the new layout and keep the project root focused on human-facing files.
- Created project skeleton under WSL.
- Added initial high-level design specification focused on contracts and APIs.
- Added analysis notes and next-spec sequencing.
- Added a separate autoSWE doc set covering orchestrator flow, stage gates, subskills, toolchain boundaries, and leaf constraint packages without modifying the original transcript draft.
- Added `designctl`, a Python CLI backed by SQLite with current-state tables, append-only event history, and structured commands for projects, design nodes, artifacts, decisions, questions, and constraint packages.
- Cleaned the repo structure in documentation so the node-harness spec is the primary track again and autoSWE is explicitly marked as a separate incubating track.
- Reorganized autoSWE for portability: moved the CLI to `tools/`, created canonical workflow files under `workflows/autoswe/`, and marked the older design-system docs as supporting reference material.
- Added a practical operator manual for how a human and a single LLM should use the autoSWE workflow together.
- Added a standalone autoSWE entry document so the workflow manual is separate from the repo root README.
- Changed the autoSWE workflow so ambiguity must be resolved or explicitly stopped rather than left as durable open questions, and documented `designctl question record` as the audit path for resolved escalations.
- Resolved the node-harness specification gaps that had been listed as open questions by defining canonical type grammar, coercion rules, node-definition serialization, service health and kill semantics, cache key derivation, artifact retention, command and event encoding, and session concurrency rules.
- Added a single repo entry point `./design` backed by `designctl resume` so workflow state can be resumed from `design.db` without the user choosing sub-workflows or low-level commands.
- Added a repo-local `AGENTS.md` convention so typing `design` or `/design` in chat means resume the workflow from `design.db`, even when the chat client has no native slash-command integration.
