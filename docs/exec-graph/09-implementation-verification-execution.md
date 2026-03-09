# execGraph Implementation Verification Execution

## Purpose

This document records the first explicit `Implementation verification execution` pass for execGraph.

The verification scope was the current `product_bootstrap` packet and its immediate follow-up refactor.

## Verification Scope

In scope:

- the `exec_graph/` C++17 source tree
- the reusable process-runtime module
- the reusable graph-document module
- the first `graph_core` snapshot module
- the first SQLite-backed graph repository seam
- the `eg_demo_pipeline` executable
- the two toy Linux workflow examples
- the real graph example and invalid-graph rejection path
- failing-graph stderr capture path
- normal-build and ASan/UBSan verification loops
- initial benchmark evidence

Out of scope:

- valgrind
- thread-sanitizer
- broader subsystem contract fixtures
- long-running service semantics

## Verification Matrix

Functional:

- normal build
- smoke workflow A
- smoke workflow B
- smoke process-graph path
- invalid-graph rejection path
- failing-graph stderr diagnostics path
- graph repository save/load/revision-conflict roundtrip

Safety:

- ASan/UBSan configure
- ASan/UBSan build
- ASan/UBSan smoke test run

Performance:

- normal-build benchmark for the process-graph path
- ASan/UBSan benchmark for the process-graph path

## Results

Functional results:

- `cmake --build build/exec_graph` passed
- `ctest --test-dir build/exec_graph --output-on-failure` passed with 6/6 tests
- the graph execution path now runs through `graph_core::GraphSnapshot`
- workflow A produced:

```text
      2 APPLE
      1 BANANA
      2 PEAR
```

- workflow B produced:

```text
3
```

- failing graph output included captured stderr:

```text
eg_demo_pipeline error: node fail failed with exit code 1: cat exec_graph/examples/definitely_missing_input.txt | stderr: cat: exec_graph/examples/definitely_missing_input.txt: No such file or directory
```

- stored-graph roundtrip now works:

```text
stored graph manual_graph at revision 1
sink count:
      2 APPLE
      1 BANANA
      2 PEAR
```

Safety results:

- `cmake -S exec_graph -B build/exec_graph_asan -G Ninja -DEG_ENABLE_ASAN=ON -DEG_ENABLE_UBSAN=ON` passed
- `cmake --build build/exec_graph_asan` passed
- `ctest --test-dir build/exec_graph_asan --output-on-failure` passed with 6/6 tests
- no sanitizer failures were observed in this pass

Performance results:

- graph workflow, normal build, 50 iterations:
  - `benchmark.total_ms=304`
  - `benchmark.avg_ms=6.08`
- graph workflow, ASan/UBSan build, 50 iterations:
  - `benchmark.total_ms=478`
  - `benchmark.avg_ms=9.56`

These are early baseline measurements, not yet stable product benchmarks.

## Triage

No blocking failures were found in this verification pass.

Residual concerns:

- benchmark evidence is still narrow and noisy
- sanitizer coverage does not yet include thread-safety checks
- graph execution is still limited to single-input DAG nodes and a simple text graph format
- diagnostics are still emitted as text, not yet normalized runtime event records
- repository-local schema setup exists, but a proper migration runner does not
- migration support now exists through `eg_migrate`, but the migration set is still minimal

## Verification Verdict

Current verdict:

- the first implementation packet is proven enough to continue
- the build is not release-ready
- the implementation is ready for a deeper execution pass

## Next-Step Decision

Return to `Implementation execution` and continue with:

1. broader memory and concurrency verification
2. continue tightening SQLite infra now that migrations are explicit
3. additional runtime examples beyond the current two toy workflows
4. structured runtime diagnostics and event normalization
