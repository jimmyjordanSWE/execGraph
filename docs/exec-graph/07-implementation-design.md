# Exec Graph Implementation Design

## Purpose

This document runs the `Implementation design` workflow for exec-graph using the approved `C++17` implementation profile.

It maps the approved design and implementation-discovery decisions into a concrete repository, build, test, and packet plan without changing the approved boundaries.

## 1. Implementation Boundary Checklist

Already fixed by design:

- canonical type grammar and compatibility semantics
- graph truth ownership and optimistic concurrency rules
- node-definition schema and capability/sandbox model
- command/query/event public contract and session/concurrency rules
- artifact descriptor and retention semantics
- runtime-linux conformance requirements
- verification obligations and smoke-path expectations

Already fixed by implementation discovery:

- primary language is `C++17`
- main rejected alternative is `Go`
- networking stack is Boost.Asio + Boost.Beast
- JSON layer is Boost.JSON
- metadata persistence uses the SQLite C API with thin internal wrappers
- build/test toolchain uses CMake, CTest, GoogleTest, clang-tidy, Clang Static Analyzer, ASan, TSan, and UBSan
- packaging supports a native binary plus Docker build path

Implementation design owns:

- source tree and build target layout
- internal library boundaries below public subsystem contracts
- exact persistence library layout
- runtime and event-loop structure
- allocator, arena, and pool boundaries
- test target layout
- fixture layout
- repo migration/extraction strategy
- repo-local implementation packets

Escalate back to design if implementation requires changing:

- public API envelopes
- node-definition meaning
- capability or sandbox semantics
- artifact identity or retention semantics
- revision/conflict rules

## 2. Repository Reality Map

Current repo state:

- `autoswe/` holds the autoSWE workflow tooling
- `docs/` holds design and product documents
- `workflows/` holds workflow definitions
- there is no exec-graph product source tree yet
- there is no CMake project yet
- there are no product tests, fixtures, or migrations yet

Repository mismatch list:

- the repo currently mixes workflow-tooling code and product design artifacts
- the repo has no isolated home for the exec-graph `C++17` implementation
- the repo has no build-system boundary that distinguishes product code from tooling code

Decision:

- keep the repo for now
- create a dedicated top-level `exec_graph/` product tree
- do not split into a separate repo before the first proof spikes
- revisit repo extraction after runtime, API, and SQLite spikes are green

## 3. Subsystem-To-Codebase Realization Map

The approved subsystems map into the codebase as follows:

| Approved subsystem | Code home | Build target type |
| --- | --- | --- |
| `canon-types` | `exec_graph/src/canon_types/` and `exec_graph/include/exec_graph/canon_types/` | static library |
| `graph-core` | `exec_graph/src/graph_core/` and `exec_graph/include/exec_graph/graph_core/` | static library |
| `node-defs` | `exec_graph/src/node_defs/` and `exec_graph/include/exec_graph/node_defs/` | static library |
| `graph-validate` | `exec_graph/src/graph_validate/` and `exec_graph/include/exec_graph/graph_validate/` | static library |
| `runtime-api` | `exec_graph/src/runtime_api/` and `exec_graph/include/exec_graph/runtime_api/` | static library |
| `artifact-store` | `exec_graph/src/artifact_store/` and `exec_graph/include/exec_graph/artifact_store/` | static library |
| `scheduler` | `exec_graph/src/scheduler/` and `exec_graph/include/exec_graph/scheduler/` | static library |
| `runtime-linux` | `exec_graph/src/runtime_linux/` and `exec_graph/include/exec_graph/runtime_linux/` | static library |
| `api-server` | `exec_graph/src/api_server/` and `exec_graph/include/exec_graph/api_server/` | static library |

Supporting areas:

- `exec_graph/src/infra/` for narrow infrastructure helpers that do not own product meaning
- `exec_graph/src/app/` for process/bootstrap wiring only
- `exec_graph/tests/` for test targets
- `exec_graph/fixtures/` for JSON fixtures, sample graphs, and smoke-path data
- `exec_graph/migrations/` for SQLite schema evolution
- `exec_graph/examples/` for example node definitions and smoke graphs
- `exec_graph/cmake/` for build helpers

## 4. Source Tree Plan

Planned source tree:

```text
exec_graph/
  CMakeLists.txt
  CMakePresets.json
  cmake/
  include/exec_graph/
    canon_types/
    graph_core/
    node_defs/
    graph_validate/
    runtime_api/
    artifact_store/
    scheduler/
    runtime_linux/
    api_server/
    infra/
  src/
    canon_types/
    graph_core/
    node_defs/
    graph_validate/
    runtime_api/
    artifact_store/
    scheduler/
    runtime_linux/
    api_server/
    infra/
    app/
  tests/
    unit/
    contract/
    integration/
    smoke/
    support/
  fixtures/
    node_defs/
    contracts/
    events/
    graphs/
    artifacts/
  migrations/
  examples/
  docker/
```

Decision:

- product code lives only under `exec_graph/`
- repo root remains a workspace root, not the product source root
- no product `.cc` or `.h` files should be added outside `exec_graph/`

## 5. Build Target Map

Primary libraries:

- `eg_canon_types`
- `eg_graph_core`
- `eg_node_defs`
- `eg_graph_validate`
- `eg_runtime_api`
- `eg_artifact_store`
- `eg_scheduler`
- `eg_runtime_linux`
- `eg_api_server`
- `eg_infra_sqlite`
- `eg_infra_events`
- `eg_test_support`

Primary executables:

- `eg_control_plane`
- `eg_smoke_runner`
- `eg_migrate`

Build rules:

- each approved subsystem gets its own target
- subsystem targets export only their public headers under `include/exec_graph/...`
- the app target links subsystem targets but owns no domain logic
- tests link the real subsystem targets, not alternate toy versions

Dependency order:

1. `eg_canon_types`
2. `eg_graph_core`
3. `eg_node_defs`
4. `eg_runtime_api`
5. `eg_graph_validate`
6. `eg_artifact_store`
7. `eg_scheduler`
8. `eg_runtime_linux`
9. `eg_api_server`
10. `eg_control_plane`

## 6. Internal Interface Map

Internal seam rules:

- `graph_core` exposes graph snapshots, mutation commands, revision guards, and repository interfaces; it does not expose raw SQLite tables
- `node_defs` exposes parsed/validated definition objects; it does not expose JSON parser internals
- `graph_validate` consumes `graph_core`, `node_defs`, `runtime_api`, and `canon_types` through narrow service interfaces
- `runtime_api` defines abstract request, result, and event types; `runtime_linux` implements them
- `artifact_store` exposes descriptor/materialization interfaces; callers do not construct raw on-disk paths
- `scheduler` consumes graph reads, validation, runtime dispatch, and artifact lookup through explicit interfaces
- `api_server` talks to graph and scheduler services, not directly to persistence or process primitives

Below-boundary interface rules:

- internal DTOs may differ from public JSON envelopes
- SQLite row shapes are private to persistence adapters
- allocators and pools are internal to owning subsystems unless a shared allocator interface is explicitly justified

## 7. Persistence Realization Plan

Persistence is split into:

- subsystem domain logic
- thin SQLite repositories/adapters
- migration executor

Concrete layout:

- `src/infra/sqlite/connection_pool.cc`
- `src/infra/sqlite/statement_cache.cc`
- `src/infra/sqlite/tx_scope.cc`
- `src/graph_core/graph_repository_sqlite.cc`
- `src/runtime_api/run_repository_sqlite.cc`
- `src/artifact_store/artifact_repository_sqlite.cc`
- `src/api_server/session_repository_sqlite.cc`
- `migrations/*.sql`

Rules:

- use explicit SQL statements and prepared statement wrappers
- no ORM layer
- transaction boundaries are owned by use-case services, not leaked arbitrarily into handlers
- migrations are append-only SQL files executed by `eg_migrate`
- persistence adapters return domain objects or repository DTOs, never raw SQLite handles across subsystem seams

Prepared-statement and connection caching:

- statement caches live inside `eg_infra_sqlite`
- connection/thread ownership remains explicit
- SQLite busy handling is centralized, not reimplemented per subsystem

## 8. Runtime, Concurrency, And Resource Realization Plan

Control-plane runtime shape:

- one main process
- one primary event loop for network and coordination
- explicit supervisor objects for runtime-linux task/service lifecycles
- explicit event broker for normalized event fanout
- explicit scheduler coordinator for run planning and dispatch

Allocator and pool boundaries:

- `graph_core`: revision-local arenas for immutable graph snapshot assembly
- `scheduler`: per-run planning arenas or pools for plan nodes and dependency state
- `runtime_linux`: bounded event/buffer pools for stream capture and lifecycle event buffering
- `api_server`: standard allocator by default; no early custom pool unless profiling proves need
- `artifact_store` and SQLite adapters: no global arena strategy; favor straightforward ownership

Rules:

- allocator/pool strategy must remain subsystem-local
- pools may not blur ownership or lifetime across subsystem boundaries
- if a pool strategy complicates correctness more than it helps, spike results can narrow its use

Shutdown semantics:

- `eg_control_plane` owns ordered startup and shutdown
- `api_server` stops accepting new requests before runtime supervision drains
- scheduler drains or marks in-flight work explicitly
- runtime-linux enforces graceful stop then kill as already designed

## 9. Verification Mapping

Test layout:

- `tests/unit/`
  - canon-types normalization/compatibility
  - graph-core revision guards
  - node-defs parsing/validation
  - runtime-api lifecycle rules
  - artifact hashing/retention helpers
- `tests/contract/`
  - command envelope fixtures
  - event envelope fixtures
  - node-definition fixtures
  - artifact descriptor fixtures
- `tests/integration/`
  - graph-core + graph-validate
  - runtime-linux + artifact-store
  - scheduler + runtime-api
  - api-server + scheduler + SSE
- `tests/smoke/`
  - one-shot task smoke
  - long-lived service smoke
  - end-to-end API graph execution smoke
- `tests/support/`
  - fixture loaders
  - temp-dir helpers
  - fake process helpers

Verification commands:

- configure debug-sanitized build
- build all targets
- run unit + contract + integration + smoke via CTest
- run clang-tidy profile
- run static-analyzer profile

Fixture locations:

- `fixtures/node_defs/`
- `fixtures/contracts/`
- `fixtures/events/`
- `fixtures/graphs/`
- `fixtures/artifacts/`

## 10. Integration And Dependency Plan

Parallelizable work clusters:

- cluster A: `canon-types`, fixture support, build bootstrap
- cluster B: `graph-core`, `node-defs`, SQLite infra
- cluster C: `runtime-api`, `artifact-store`
- cluster D: `graph-validate`, `scheduler`
- cluster E: `runtime-linux`
- cluster F: `api-server`, sessions, SSE

Integration proving order:

1. compile and test `canon-types`
2. persist and reload graph snapshots
3. validate node definitions and invalid graphs
4. publish and materialize artifacts
5. drive runtime-linux one-shot and service flows
6. run scheduler against validated graphs
7. expose API and SSE
8. pass smoke paths and Docker packaging checks

Temporary fakes allowed:

- fake runtime adapter for early scheduler tests
- temp-dir artifact roots for tests
- in-memory event sink for some unit/integration tests

Not allowed:

- fake graph semantics
- alternate public API envelopes
- fake capability model that bypasses the designed contract

## 11. Repository Migration Or Extraction Plan

Near-term plan:

- keep `autoswe/`, `docs/`, and `workflows/` where they are
- add all product implementation code under `exec_graph/`
- do not create a root-level `CMakeLists.txt` for the whole mixed repo
- use `exec_graph/CMakeLists.txt` as the product entry point

Revisit repo extraction when:

- runtime-linux spike is green
- API/SSE spike is green
- SQLite/migration spike is green
- the product source tree is large enough that mixed-repo ergonomics become costly

## 12. Repo-Local Implementation Packets

Packet 1: product bootstrap

- create `exec_graph/` source tree
- add CMake, presets, and baseline test support
- add sanitizer and static-analysis build profiles

Packet 2: canon-types and contract fixtures

- implement `eg_canon_types`
- add normalization and compatibility fixtures
- establish contract fixture loading

Packet 3: SQLite infra and graph-core

- add `eg_infra_sqlite`
- add migration runner
- implement graph snapshot storage and revision guards

Packet 4: node-defs and graph-validate

- implement node-definition parsing/validation
- implement validation reports and deterministic diagnostics

Packet 5: runtime-api and artifact-store

- implement runtime request/result/event types
- implement descriptor hashing/materialization/retention metadata

Packet 6: runtime-linux spike and implementation

- prove subprocess/service supervision
- implement bounded capture, stop/kill/restart, and output publication

Packet 7: scheduler

- implement run planning, dependency ordering, cache-key use, and service reuse rules

Packet 8: api-server and sessions

- implement HTTP JSON commands/queries
- implement SSE event fanout
- implement session and idempotency handling

Packet 9: smoke paths and Docker packaging

- wire the control-plane executable
- run required smoke paths
- produce Docker build path and release evidence

## 13. Implementation Operating Surface

Standard commands:

```bash
cmake -S exec_graph -B build/exec_graph -G Ninja
cmake --build build/exec_graph
ctest --test-dir build/exec_graph --output-on-failure
```

Required build profiles:

- debug
- debug + ASan/UBSan
- debug + TSan
- release

Recommended command wrappers:

- configure preset for default debug
- configure preset for sanitizer build
- lint target for clang-tidy
- analyze target for Clang Static Analyzer
- smoke target for end-to-end proof runs

Diagnostics expectations:

- sanitizer failures must be first-class CI failures
- logs and test output should be written so humans and agents can localize subsystem ownership quickly

## 14. Mismatch Register

- the current repo is still mixed-purpose, so product code isolation must be enforced structurally
- the allocator/pool plan is approved directionally but still requires spike evidence before broad use
- Docker packaging is part of release engineering, but no product build files exist yet

## 15. Residual Implementation Risks

- early overuse of custom pools could make correctness worse instead of better
- Boost-based networking integration may still reveal SSE or shutdown edge cases
- SQLite busy-handling and recovery behavior must be proven under realistic concurrency
- runtime-linux ownership and shutdown order are easy places for subtle bugs

## 16. Escalation List

Escalate if implementation design uncovers a need to change:

- envelope or schema meaning
- revision/conflict semantics
- capability or sandbox semantics
- artifact identity or retention semantics
- the single-process V1 assumption for correctness rather than scaling

## 17. Implementation Blueprint

Exec-graph V1 should be implemented as a product-local `C++17` enclave inside this repo:

- `exec_graph/` becomes the only home for product source
- each approved subsystem becomes an explicit library target
- app/bootstrap code stays thin
- SQLite is accessed directly through narrow infra wrappers
- allocator and pool strategies are localized to `graph_core`, `scheduler`, and `runtime_linux` only after spike evidence
- tests, fixtures, migrations, and smoke paths are first-class from packet 1 onward

This is concrete enough to begin implementation without guessing the repo shape, target graph, test layout, or packet order.
