# One-Day Experiment Report

## Question

Can a single AI-assisted development session move a systems project from problem framing through architecture, implementation discovery, bounded implementation, and verification while preserving its reasoning in a structured ledger?

The test project was ExecGraph: a backend-first platform concept for composing, executing, and observing programs and services as typed graphs.

## Time boundary

The repository's 11 original commits were created on March 9, 2026. This report describes the state reached during that experiment; later presentation and repository-hygiene work is not counted as part of the one-day implementation result.

## Method

The `autoSWE` workflow separated the work into explicit phases:

1. problem and requirements definition
2. solution framing
3. domain, data, and behavior design
4. system decomposition and contracts
5. security, operations, and verification design
6. implementation handoff and leaf constraint packages
7. implementation discovery
8. repository-level implementation design
9. implementation and verification execution

SQLite was the operational source of truth. Markdown files were emitted artifacts, not the only record of project state. The ledger recorded project nodes, decisions, artifacts, dependencies, constraint packages, and mutation events.

## Ledger receipt

The final working database contained:

| Record type | Count |
| --- | ---: |
| Projects | 2 |
| Design nodes | 40 |
| Artifacts | 29 |
| Decisions | 34 |
| Constraint packages | 13 |
| State-change events | 264 |
| Durable open questions | 0 |

The two projects were the ExecGraph product and the autoSWE workflow itself. All design stages through implementation handoff were marked complete. ExecGraph implementation discovery and implementation design were complete; implementation execution remained explicitly in progress.

A sanitized snapshot of the working database is included at `../.design/design.db`. Generated code-index rows and machine-specific paths were removed before publication, while the projects, decisions, artifacts, constraint packages, and audit events were retained. Its schema and all state-transition behavior remain inspectable in [`../autoswe/cli.py`](../autoswe/cli.py).

## Decisions that materially shaped the code

- use a layered backend architecture, keeping graph semantics separate from runtime adapters and clients
- use C++17 for the reference implementation, with Go considered but not selected
- keep product code under a dedicated `exec_graph/` tree
- represent execution graphs as immutable, arena-backed snapshots
- use SQLite behind narrow RAII infrastructure and repository seams
- require optimistic revision checks for persisted graph updates
- treat security and trust boundaries as an architecture phase rather than a late review
- require end-to-end execution, failure, persistence, and hardening evidence

## What was actually built

The C++ prototype can:

- parse sequential workflows and named directed graphs
- reject invalid and cyclic graphs
- calculate graph execution order
- execute Linux processes with captured standard streams
- retain graph-file working-directory context
- supervise owned process groups
- stop timed-out work with a configurable graceful period before forced termination
- emit structured JSONL lifecycle events
- persist and revision-check graph snapshots in SQLite
- apply idempotent database migrations

The CMake project defines 11 smoke tests covering successful workflows, graph execution, invalid graphs, failures, persistence round-trips, migration idempotency, event emission, timeout behavior, and relative path handling. Separate scripts provide sanitizer and Valgrind entry points.

## What was not built

The design is substantially larger than the prototype. There is no production sandbox, capability system, artifact store, scheduler, service runtime, API server, complete type system, or persistent agent-execution ledger. The proof of concept executes trusted local commands and must not be presented as a security boundary.

## Result

The experiment supports three modest conclusions:

1. A structured ledger can preserve architectural continuity across a long agentic development sequence.
2. Compact constraint packages can carry design choices into bounded implementation without specifying every local coding decision.
3. The method can produce working systems code and verification evidence, but it does not remove the need to control scope, validate claims, or distinguish architecture from implemented behavior.

The most valuable product insight emerged late: ExecGraph is more compelling as a controlled execution and provenance layer for autonomous agents than as a generic workflow engine.

## If resumed

The next useful experiment would be intentionally narrow:

- place a simple coding agent behind a single execution broker
- deny direct shell access outside that broker
- record commands, process ancestry, outputs, file diffs, artifacts, and policy decisions
- run each task in a disposable isolated environment
- demonstrate inspection and rollback of one complete agent session

That would test the core security-and-provenance thesis without first building the full platform described in the architecture documents.
