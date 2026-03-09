# Repo Agent Conventions

## Chat Entry Point

When the user types `design` or `/design` in chat, interpret it as:

- resume the autoSWE workflow from `.design/design.db`
- use the workspace default project if configured, otherwise auto-select the active project from the database
- inspect current state before deciding the next step
- continue from the highest-priority unresolved work in the database

This chat convention exists because repo code can define agent behavior, but it cannot create native slash-command UI features in the chat client.

In clients like the Codex VS Code extension, custom repo slash commands may not appear in the UI.
If `/design` is not available there, type `design` as a normal chat message instead.

## Terminal Entry Point

The terminal equivalent is:

```bash
python3 -m autoswe resume
```

Override the project explicitly when needed:

```bash
python3 -m autoswe resume --project autoSWE
```

## Path Conventions

When editing repo files:

- use project-relative paths and relative markdown links by default
- do not hardcode machine-specific absolute filesystem paths
- use absolute paths only when a tool or protocol explicitly requires them

This repo may move between machines or into GitHub, so links and references must remain portable.
