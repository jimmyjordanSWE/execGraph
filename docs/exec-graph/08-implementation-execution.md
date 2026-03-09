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

## Residual Risks

- the current toy runner is still a narrow demo, not yet a real graph engine
- diagnostics are still text-first rather than structured runtime events
- the workflow file format is intentionally simple and not yet a stable product contract
- no migration runner exists yet beyond repository-local schema initialization

Mitigated in the second pass:

- ASan and UBSan build options are now wired into `CMake`
- a second toy workflow now proves the runner is not overfit to one output shape

Mitigated in the latest pass:

- the product can now execute a real node/edge graph document instead of only a line-based linear workflow
- invalid cyclic graphs are now rejected explicitly
- process failures now retain stderr output in surfaced diagnostics
- graph execution now runs through an explicit `graph_core` snapshot instead of directly over parser-owned structures
- graphs can now be persisted and reloaded with revision guards instead of only living as raw files

## Next Execution Decision

The next pass should keep implementation execution on the same node and target the next lowest-risk, highest-value items:

1. add a proper migration runner and widen SQLite infra beyond repository-local schema setup
2. add valgrind and thread-safety-oriented verification where practical
3. continue separating graph parsing from stable graph-core contracts
4. move runtime diagnostics from freeform text toward structured execution events
