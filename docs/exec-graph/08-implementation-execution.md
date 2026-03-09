# execGraph Implementation Execution

## Purpose

This document records the first real `Implementation execution` passes for execGraph.

The goal of this pass was not broad subsystem completion.
The goal was to prove that implementation has actually started and can already run a small but real Linux-process workflow end to end.

## Active Packet

Packet:

- `product_bootstrap`

Scope:

- create the top-level `exec_graph/` product tree
- add a native `C++17` build entry point
- establish the first runnable smoke path
- add a small benchmark-capable demo loop

Do not cross:

- no redesign of graph contracts or public APIs
- no premature subsystem explosion
- no framework or persistence layering beyond what is required for the first executable proof

## Proof Obligations

This pass had to prove:

1. the repo now contains a real `exec_graph/` implementation tree
2. the code builds through `CMake`
3. a toy workflow can run four common Linux processes in sequence
4. the workflow output is deterministic enough for a smoke test
5. the executable can run in a simple benchmark mode so later performance comparisons have a baseline

Deferred from this pass:

- subsystem libraries beyond the single demo executable
- explicit scheduler/runtime separation
- contract fixtures beyond the toy workflow
- sanitizer and static-analysis profiles

## Implemented Surface

Added:

- `exec_graph/CMakeLists.txt`
- `exec_graph/include/exec_graph/runtime/process_runtime.hpp`
- `exec_graph/src/runtime/process_runtime.cpp`
- `exec_graph/src/app/demo_pipeline.cpp`
- `exec_graph/examples/fruit.txt`
- `exec_graph/examples/toy_linux.workflow`
- `exec_graph/examples/unique_count.workflow`
- `exec_graph/tests/smoke/run_toy_workflow.sh`

The current demo workflows prove two different pipeline shapes.

Workflow A runs these four common Linux programs:

1. `cat`
2. `tr`
3. `sort`
4. `uniq -c`

Workflow B runs these four common Linux programs:

1. `cat`
2. `sort`
3. `uniq`
4. `wc -l`

The example input is transformed into the deterministic output:

```text
      2 APPLE
      1 BANANA
      2 PEAR
```

## Ownership And Resource Notes

- process ownership is local to the demo runner
- each process receives explicit stdin/stdout pipe setup
- the parent closes pipe ends deterministically after fork
- stdout is captured and passed as stdin to the next process
- stderr is now captured explicitly and preserved in surfaced failure diagnostics

This is intentionally small but already exercises:

- process creation
- pipe wiring
- sequential dataflow
- output capture
- exit-code enforcement
- file-local command execution for file-based workflows

The second pass also moved the process execution logic out of `main` and into reusable runtime functions:

- `exec_graph::runtime::load_workflow`
- `exec_graph::runtime::run_process`
- `exec_graph::runtime::run_workflow`

The next pass introduced the first real graph-shaped product slice:

- `exec_graph::graph::load_graph_document`
- `exec_graph::graph::topological_order`
- `exec_graph::graph_core::build_snapshot`
- `exec_graph::graph_core::execute_snapshot_outputs`

The current graph document supports:

- explicit `node` declarations
- explicit `edge` declarations
- duplicate-node rejection
- unknown-node rejection
- cycle rejection
- topological execution of DAG-shaped process graphs
- file-local execution for file-based graphs so example command paths resolve relative to the graph file
- per-process stderr capture for failing nodes and commands

A real graph example now exists in:

- `exec_graph/examples/toy_process.graph`

A failing graph example now exists in:

- `exec_graph/examples/failing_stderr.graph`

The current runtime path no longer executes directly from the parsed document.
It now builds an explicit in-memory `graph_core` snapshot with:

- stable numeric `NodeId` values
- dense node records
- contiguous argv storage
- contiguous incoming and outgoing adjacency storage
- a revision-local monotonic arena backing the snapshot lifetime

The newest pass also introduced the first persisted graph seam:

- SQLite-backed graph snapshot storage
- save/load through `graph_core::GraphRepositorySqlite`
- optimistic concurrency checks through `expected_revision`
- execution of stored graphs through the same `graph_core` snapshot path

The current pass replaced repository-local schema setup with a real migration path:

- append-only SQL migration files under `exec_graph/migrations/`
- `exec_graph::infra::sqlite::MigrationRunner`
- `exec_graph::infra::sqlite::Transaction` for rollback-safe write scopes
- a dedicated `eg_migrate` executable
- stored-graph save/load flows now apply migrations before touching repository code
- migration execution now reports discovered/applied/skipped counts so idempotency is visible in verification
- the default `eg_migrate` migration path now resolves correctly from either the repo root or the `exec_graph/` product root

The latest pass hardened the verification surface around that implementation:

- `exec_graph/CMakePresets.json` now defines `debug`, `asan-ubsan`, and `tsan`
- all configure presets now export `compile_commands.json`
- workspace editor settings now point C/C++ tooling at `build/exec_graph/compile_commands.json`
- `exec_graph/tests/verification/run_tsan_matrix.sh`
- `exec_graph/tests/verification/run_valgrind_graph_smoke.sh`
- `exec_graph/tests/smoke/run_migration_idempotent.sh`
- `exec_graph/tests/smoke/run_workflow_events.sh`
- `exec_graph/tests/smoke/run_relative_example_paths.sh`

The current pass introduced the first structured runtime-event surface:

- `workflow.started`
- `workflow.step.started`
- `process.started`
- `process.output`
- `process.completed`
- `process.failed`
- `process.killed`
- `workflow.step.completed`
- `workflow.step.failed`
- `workflow.completed`
- `workflow.failed`
- `graph.started`
- `graph.node.started`
- `graph.node.completed`
- `graph.node.failed`
- `graph.completed`
- `graph.failed`
- JSONL event rendering from the runtime layer
- `--emit-events-jsonl` in `eg_demo_pipeline`
- workflow execution now emits workflow-run events, workflow-step progress events, and per-process terminal events on both success and failure
- graph execution now emits graph-run events, graph-layer per-node progress events, and per-process terminal events on both success and failure

The newest pass tightened runtime supervision and event serialization without changing the public runner shape:

- Boost.JSON now owns JSONL event serialization instead of handwritten string escaping
- process execution now creates an owned process group for each launched command
- workflow commands and graph nodes can now declare `timeout_ms=<n>` and `graceful_shutdown_ms=<n>` before the command argv
- timed-out commands now receive `SIGTERM` first and then `SIGKILL` if they outlive the grace window
- the runtime now emits `process.stop.requested` and `process.kill.sent` control events before the terminal process event
- a dedicated timeout smoke path now proves timeout-driven escalation and machine-readable `terminal_cause = timeout`

## Verification Evidence

Build:

```bash
cmake -S exec_graph -B build/exec_graph -G Ninja
cmake --build build/exec_graph
```

Smoke:

```bash
ctest --test-dir build/exec_graph --output-on-failure
build/exec_graph/eg_demo_pipeline --workflow exec_graph/examples/toy_linux.workflow
```

Benchmark baseline:

```bash
build/exec_graph/eg_demo_pipeline --workflow exec_graph/examples/toy_linux.workflow --benchmark 50
```

Observed benchmark result for the first baseline:

- `benchmark.iterations=50`
- `benchmark.total_ms=436`
- `benchmark.avg_ms=8.72`

Second-pass observations:

- normal-build workflow A benchmark:
  - `benchmark.total_ms=501`
  - `benchmark.avg_ms=10.02`
- ASan/UBSan workflow B benchmark:
  - `benchmark.total_ms=455`
  - `benchmark.avg_ms=9.1`

Graph-path observations:

- graph workflow, normal build:
  - `benchmark.total_ms=304`
  - `benchmark.avg_ms=6.08`
- graph workflow, ASan/UBSan build:
  - `benchmark.total_ms=478`
  - `benchmark.avg_ms=9.56`

Diagnostics-path observation:

- failing graph output now includes captured stderr, for example:
  - `node fail failed with exit code 1: cat exec_graph/examples/definitely_missing_input.txt | stderr: cat: exec_graph/examples/definitely_missing_input.txt: No such file or directory`

Persistence-path observations:

- saving a graph now reports:
  - `stored graph manual_graph at revision 1`
- loading a stored graph and executing it reproduces the expected sink output
- conflicting updates now fail with:
  - `revision_conflict: graph manual_graph is at revision 1, expected 7`

Migration-path observation:

- explicit migration execution now reports:
  - `applied migrations from exec_graph/migrations to /tmp/exec_graph_migrate.<id>.db (discovered=2, applied=2, skipped=0)`
- rerunning the same migration set now reports:
  - `applied migrations from exec_graph/migrations to /tmp/exec_graph_migrate.<id>.db (discovered=2, applied=0, skipped=2)`

Hardening-path observations:

- ThreadSanitizer build and smoke matrix now pass under Clang
- valgrind reports `0 errors from 0 contexts` on the graph smoke path
- the generated compile database now includes the fetched Boost header path used by runtime event serialization

Structured-events observations:

- successful workflow execution now emits JSONL entries such as:
  - `{"name":"workflow.started","subject":"workflow",...,"step_count":4,...}`
  - `{"name":"workflow.step.started","subject":"workflow.step.1",...,"related_subject":"workflow",...,"completed_step_count":0,...}`
  - `{"name":"process.started","subject":"workflow.step.1",...}`
  - `{"name":"process.completed","subject":"workflow.step.1",...,"terminal_cause":"exit_zero",...}`
  - `{"name":"workflow.step.completed","subject":"workflow.step.1",...,"byte_count":32,...}`
  - `{"name":"workflow.completed","subject":"workflow",...,"completed_step_count":4,...}`
- failing workflow execution now emits JSONL entries such as:
  - `{"name":"workflow.started","subject":"workflow",...,"step_count":1,...}`
  - `{"name":"workflow.step.started","subject":"workflow.step.1",...,"related_subject":"workflow",...,"completed_step_count":0,...}`
  - `{"name":"process.failed","subject":"workflow.step.1",...,"terminal_cause":"exit_non_zero",...}`
  - `{"name":"workflow.step.failed","subject":"workflow.step.1",...,"stream_name":"stderr",...}`
  - `{"name":"workflow.failed","subject":"workflow",...,"related_subject":"workflow.step.1",...,"completed_step_count":0,...}`
- timeout-driven workflow execution now emits JSONL entries such as:
  - `{"name":"workflow.started","subject":"workflow",...,"step_count":1,...}`
  - `{"name":"process.started","subject":"workflow.step.1",...}`
  - `{"name":"process.stop.requested","subject":"workflow.step.1",...,"signal_number":15,"terminal_cause":"timeout",...}`
  - `{"name":"process.kill.sent","subject":"workflow.step.1",...,"signal_number":9,"terminal_cause":"timeout",...}`
  - `{"name":"process.killed","subject":"workflow.step.1",...,"terminal_cause":"timeout",...}`
  - `{"name":"workflow.step.failed","subject":"workflow.step.1",...,"terminal_cause":"timeout",...}`
  - `{"name":"workflow.failed","subject":"workflow",...,"related_subject":"workflow.step.1",...,"terminal_cause":"timeout",...}`
- successful graph execution now emits JSONL entries such as:
  - `{"name":"graph.started","subject":"graph",...,"node_count":4,"sink_count":1,...}`
  - `{"name":"graph.node.started","subject":"count",...,"related_subject":"graph",...,"completed_node_count":3,...}`
  - `{"name":"process.started","subject":"count",...}`
  - `{"name":"process.output","subject":"count",...,"stream_name":"stdout",...}`
  - `{"name":"process.completed","subject":"count",...,"terminal_cause":"exit_zero",...}`
  - `{"name":"graph.node.completed","subject":"count",...,"byte_count":26,...}`
  - `{"name":"graph.completed","subject":"graph",...,"completed_node_count":4,...}`
- failing graph execution now emits JSONL entries such as:
  - `{"name":"graph.started","subject":"graph",...,"node_count":1,...}`
  - `{"name":"graph.node.started","subject":"fail",...,"related_subject":"graph",...,"completed_node_count":0,...}`
  - `{"name":"process.started","subject":"fail",...}`
  - `{"name":"process.output","subject":"fail",...,"stream_name":"stderr",...}`
  - `{"name":"process.failed","subject":"fail",...,"terminal_cause":"exit_non_zero",...}`
  - `{"name":"graph.node.failed","subject":"fail",...,"stream_name":"stderr",...}`
  - `{"name":"graph.failed","subject":"graph",...,"related_subject":"fail",...,"completed_node_count":0,...}`

Path-resolution observations:

- file-based workflow execution now resolves command paths relative to the workflow file directory
- file-based graph execution now resolves command paths relative to the graph file directory
- stored-graph save/load now persists the graph working directory so repository roundtrips preserve file-local command resolution
- the same toy workflow and toy graph now run correctly from both the repo root and the `exec_graph/` product root
- stored-graph save/load now works from both `eg_migrate` and `eg_demo_pipeline` when launched from either the repo root or the `exec_graph/` product root

Supervision-path observations:

- workflow files can now express per-command timeout and grace windows inline before the command argv
- graph documents can now express the same timeout metadata per node
- timeout enforcement now happens against the owned process group rather than only the direct child pid

## Residual Risks

- the current toy runner is still a narrow demo, not yet a real graph engine
- diagnostics are still text-first rather than structured runtime events
- the workflow file format is intentionally simple and not yet a stable product contract
- migration support exists, but still only as a minimal local runner with one schema file
- runtime diagnostics now cover workflow-run progress, graph-run progress, per-process lifecycle, and timeout-driven stop/kill supervision, but broader scheduler and reusable service surfaces still do not exist

Mitigated in the second pass:

- ASan and UBSan build options are now wired into `CMake`
- a second toy workflow now proves the runner is not overfit to one output shape

Mitigated in the latest pass:

- the product can now execute a real node/edge graph document instead of only a line-based linear workflow
- invalid cyclic graphs are now rejected explicitly
- process failures now retain stderr output in surfaced diagnostics
- graph execution now runs through an explicit `graph_core` snapshot instead of directly over parser-owned structures
- graphs can now be persisted and reloaded with revision guards instead of only living as raw files
- schema setup now runs through append-only SQL migrations and a dedicated migration executable

## Next Execution Decision

The next pass should keep implementation execution on the same node and target the next lowest-risk, highest-value items:

1. continue tightening SQLite infra and repository seams now that migrations are explicit
2. widen the structured runtime event model beyond workflow, graph, and timeout-supervision lifecycle toward scheduler and reusable service surfaces
3. continue separating graph parsing from stable graph-core contracts
4. widen runtime examples beyond the current small smoke set
