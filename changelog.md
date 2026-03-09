# Changelog

## 2026-03-09

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
