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
- stderr is left attached to the parent for now to keep the first proof loop simple

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

## Residual Risks

- the current toy runner is still a narrow demo, not yet a real graph engine
- stderr capture and richer diagnostics are still thin
- the workflow file format is intentionally simple and not yet a stable product contract
- no subsystem target split exists yet beyond the first executable

Mitigated in the second pass:

- ASan and UBSan build options are now wired into `CMake`
- a second toy workflow now proves the runner is not overfit to one output shape

## Next Execution Decision

The next pass should keep implementation execution on the same node and target the next lowest-risk, highest-value items:

1. introduce the first real graph-shaped workflow representation instead of a line-based toy format
2. add stderr capture and richer execution diagnostics
3. add valgrind and thread-safety-oriented verification where practical
4. continue splitting app code from reusable runtime pieces
