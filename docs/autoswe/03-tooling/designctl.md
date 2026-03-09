# `designctl`

## Purpose

`designctl` is the V1 source-of-truth tool for a single-LLM design workflow.

It provides:

- one SQLite database file per project
- current-state tables
- append-only event history
- always-refresh code map indexing
- narrow write commands
- JSON output for agent consumption

The database is the canonical project memory.
Markdown docs are not the canonical source of truth.

## Why This Exists

The agent should not make progress by editing random files.

It should make progress by writing structured design state through a constrained interface.

This forces:

- explicit project state
- queryable long-term memory
- visible status transitions
- auditable design changes

## Current Model

Core entities:

- project
- design node
- artifact
- decision
- question record
- constraint package
- event
- code index run
- code file
- code symbol

Current-state tables store the latest approved or active view.
The `events` table stores append-only history.
The code map tables store the latest observed workspace structure.

## Code Map Refresh

By default, every `designctl` invocation refreshes the workspace code map before doing any other work.

Current backend:

- `python-ast+filesystem`

This is a V1 parser-backed index. It can be replaced with tree-sitter later without changing the SQLite role in the system.

The tool:

- prints code map generation time on every run
- includes code index metadata in JSON output
- fails the run if indexing exceeds the configured threshold

Default threshold:

- 5 seconds

Disable indexing explicitly with:

```bash
python3 -m autoswe --skip-index ...
```

Change the failure threshold with:

```bash
python3 -m autoswe --index-max-seconds 10 ...
```

## Command Surface

Project commands:

- `project init`
- `project get`
- `project list`
- `project update`

Node commands:

- `node create`
- `node update`
- `node update-status`
- `node get`
- `node list`

Artifact commands:

- `artifact add`
- `artifact get`
- `artifact list`
- `artifact template`
- `artifact update-status`

Decision commands:

- `decision record`
- `decision get`
- `decision update`
- `decision update-status`
- `decision list`

Question commands:

- `question record`
- `question get`
- `question resolve`
- `question list`

Questions are audit records of resolved escalations.
Durable `open` questions are disallowed.

Constraint package commands:

- `constraint-package create`
- `constraint-package get`
- `constraint-package list`
- `constraint-package update-status`

Query commands:

- `query tree`
- `query next-work`
- `query events`
- `query code-map`

Resume command:

- `resume`

`resume` is the high-level entry point. It selects the active project, reads current workflow state from the database, and returns the context needed to continue from the current stage.

## Usage

Initialize a project:

```bash
python3 -m autoswe --db example.db project init \
  --name demo \
  --goal "Design a durable LLM-led software project workflow"
```

Create a node:

```bash
python3 -m autoswe --db example.db node create \
  --project demo \
  --title "Problem definition" \
  --stage problem_definition
```

Add an artifact:

```bash
python3 -m autoswe --db example.db artifact add \
  --project demo \
  --node-id node_abc \
  --type problem_statement \
  --title "Initial problem statement" \
  --json '{"problem":"..."}'
```

Update project metadata without editing the database directly:

```bash
python3 -m autoswe --db example.db project update \
  --project demo \
  --scope "Updated scope" \
  --append-constraint "Primary implementation language is C++17"
```

Update a node summary or stage without raw SQL:

```bash
python3 -m autoswe --db example.db node update \
  --node-id node_abc \
  --summary "Revised summary" \
  --stage implementation_design
```

Supersede or reject a decision directly through the CLI:

```bash
python3 -m autoswe --db example.db decision update-status \
  --decision-id dec_abc \
  --status superseded
```

Edit decision text without raw SQL:

```bash
python3 -m autoswe --db example.db decision update \
  --decision-id dec_abc \
  --decision "Updated decision text" \
  --rationale "Updated rationale"
```

Get a built-in template for implementation-phase artifacts:

```bash
python3 -m autoswe artifact template --type implementation_design
```

Artifact lists honor `ordered_output.sequence_index` when present, so numbered follow-on docs appear in sequence instead of creation order.

List all artifacts attached to a node:

```bash
python3 -m autoswe --db example.db artifact list \
  --project demo \
  --node-id node_abc
```

Inspect one recorded decision:

```bash
python3 -m autoswe --db example.db decision get \
  --decision-id dec_abc
```

Create a constraint package:

```bash
python3 -m autoswe --db example.db constraint-package create \
  --project demo \
  --node-id node_leaf \
  --module-name leaf-module \
  --file package.json
```

Record a resolved question:

```bash
python3 -m autoswe --db example.db question record \
  --project demo \
  --node-id node_abc \
  --question "Should external integrations be in scope for V1?" \
  --resolution "No. External integrations are out of scope for V1."
```

Ask what to work on next:

```bash
python3 -m autoswe --db example.db query next-work --project demo
```

Resume the workflow from the database:

```bash
python3 -m autoswe --db example.db --workspace . resume
```

Inspect the current code map:

```bash
python3 -m autoswe --db example.db query code-map
```

## V1 Limitations

This tool intentionally targets one active LLM per project.

It does not yet implement:

- multi-agent claiming
- leases
- conflict resolution
- migrations beyond schema re-init
- rich artifact versioning
- real tree-sitter parsing

Those should be added only after the single-agent workflow is stable.
