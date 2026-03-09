# Exec Graph

This repo currently contains two related but distinct tracks.

## Primary Track: Exec Graph Platform

This is the main project in the repo.

Single workflow entry point:
- `python3 -m autoswe resume`
- If your client supports slash-command aliases, map `/design` to run `python3 -m autoswe resume`.
- In chat, use `design` or `/design` as the repo convention to mean "resume from the database".
- In the Codex VS Code extension, if custom slash commands do not appear, type `design` as a normal prompt instead.

Focus:
- define contracts and public APIs
- define canonical types and boundary semantics
- define runtime lifecycle and graph execution models
- keep implementation concerns out of scope until the contracts are stable

Primary documents:
- [docs/design.md](docs/design.md)
- [docs/exec-graph/README.md](docs/exec-graph/README.md)
- [analysis/.analysis.md](analysis/.analysis.md)

## Secondary Track: autoSWE

This is exploratory tooling for design-first, LLM-assisted software development. It is not the exec-graph spec itself.

Workflow entry point:
- [docs/autoswe/README.md](docs/autoswe/README.md)

The design-system docs describe how the design tool works.
The exec-graph docs describe the product being designed.

## Current Source Layout

- `pyproject.toml`: Python package definition for the local `autoswe` CLI
- `.design/design.db`: local design-state database
- `docs/design.md`: exec-graph architecture spec
- `docs/exec-graph/`: ordered follow-on exec-graph subsystem and handoff specs
- `exec_graph/`: C++17 product source tree and runnable toy workflow proof
- `workflows/autoswe/`: canonical autoSWE workflow definitions grouped by numbered phase folders
- `docs/autoswe/`: supporting autoSWE rationale and reference docs grouped by numbered section folders
- `autoswe/`: Python package for the workflow CLI and state management
- `docs/autoswe/99-reference/reference/software-design-skill.md`: source skill document that the workflow must continue to cover
- `changelog.md`: repo changes
