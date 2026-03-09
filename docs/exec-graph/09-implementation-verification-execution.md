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
- the structured runtime-event JSONL path
- the two toy Linux workflow examples
- the real graph example and invalid-graph rejection path
- failing-graph stderr capture path
- workflow-run runtime-event success/failure path
- timeout-driven process supervision and control-event path
- normal-build and ASan/UBSan verification loops
- ThreadSanitizer verification loop
- valgrind graph smoke verification
- migration idempotency verification
- file-local example path verification
- compile-database generation for editor/tooling correctness
- initial benchmark evidence

Out of scope:

- broader subsystem contract fixtures
- reusable long-running service lifecycle semantics

## Verification Matrix

Functional:

- normal build
- smoke workflow A
- smoke workflow B
- smoke process-graph path
- invalid-graph rejection path
- failing-graph stderr diagnostics path
- graph repository save/load/revision-conflict roundtrip
- structured runtime-event success/failure path
- workflow-level runtime-event success/failure path
- graph-level runtime-event success/failure path
- timeout-supervision stop/kill escalation path

Safety:

- ASan/UBSan configure
- ASan/UBSan build
- ASan/UBSan smoke test run
- TSan configure/build/test run
- valgrind graph smoke run

Performance:

- normal-build benchmark for the process-graph path
- ASan/UBSan benchmark for the process-graph path

## Results

Functional results:

- `cmake --build build/exec_graph` passed
- `ctest --test-dir build/exec_graph --output-on-failure` passed with 6/6 tests
- `ctest --test-dir build/exec_graph --output-on-failure` now passes with 8/8 tests including migration idempotency coverage
- `ctest --test-dir build/exec_graph --output-on-failure` now passes with 9/9 tests including relative-path execution coverage
- `ctest --test-dir build/exec_graph --output-on-failure` now passes with 10/10 tests including workflow-event coverage
- `ctest --test-dir build/exec_graph --output-on-failure` now passes with 11/11 tests including timeout supervision coverage
- file-based workflows and graphs now execute correctly from the `exec_graph/` product root as well as the repo root
- the graph execution path now runs through `graph_core::GraphSnapshot`
- all configure presets now export `compile_commands.json`, and the workspace editor points at the debug compile database
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

- structured runtime events now emit machine-readable lifecycle and output entries on both success and failure:

```text
{"name":"workflow.started","subject":"workflow",...,"step_count":4,...}
{"name":"workflow.step.started","subject":"workflow.step.1",...,"related_subject":"workflow",...,"completed_step_count":0,...}
{"name":"process.started","subject":"workflow.step.1",...}
{"name":"process.completed","subject":"workflow.step.1",...,"terminal_cause":"exit_zero",...}
{"name":"workflow.step.completed","subject":"workflow.step.1",...,"byte_count":32,...}
{"name":"workflow.completed","subject":"workflow",...,"completed_step_count":4,...}
{"name":"workflow.started","subject":"workflow",...,"step_count":1,...}
{"name":"workflow.step.started","subject":"workflow.step.1",...,"related_subject":"workflow",...,"completed_step_count":0,...}
{"name":"process.failed","subject":"workflow.step.1",...,"terminal_cause":"exit_non_zero",...}
{"name":"workflow.step.failed","subject":"workflow.step.1",...,"stream_name":"stderr",...}
{"name":"workflow.failed","subject":"workflow",...,"related_subject":"workflow.step.1",...,"completed_step_count":0,...}
{"name":"workflow.started","subject":"workflow",...,"step_count":1,...}
{"name":"process.started","subject":"workflow.step.1",...}
{"name":"process.stop.requested","subject":"workflow.step.1",...,"signal_number":15,"terminal_cause":"timeout",...}
{"name":"process.kill.sent","subject":"workflow.step.1",...,"signal_number":9,"terminal_cause":"timeout",...}
{"name":"process.killed","subject":"workflow.step.1",...,"terminal_cause":"timeout",...}
{"name":"workflow.step.failed","subject":"workflow.step.1",...,"terminal_cause":"timeout",...}
{"name":"workflow.failed","subject":"workflow",...,"related_subject":"workflow.step.1",...,"terminal_cause":"timeout",...}
{"name":"graph.started","subject":"graph",...,"node_count":4,"sink_count":1,...}
{"name":"graph.node.started","subject":"count",...,"related_subject":"graph",...,"completed_node_count":3,...}
{"name":"process.started","subject":"count",...}
{"name":"process.output","subject":"count",...,"stream_name":"stdout",...}
{"name":"process.completed","subject":"count",...,"terminal_cause":"exit_zero",...}
{"name":"graph.node.completed","subject":"count",...,"byte_count":26,...}
{"name":"graph.completed","subject":"graph",...,"completed_node_count":4,...}
{"name":"graph.started","subject":"graph",...,"node_count":1,...}
{"name":"graph.node.started","subject":"fail",...,"related_subject":"graph",...,"completed_node_count":0,...}
{"name":"process.started","subject":"fail",...}
{"name":"process.output","subject":"fail",...,"stream_name":"stderr",...}
{"name":"process.failed","subject":"fail",...,"terminal_cause":"exit_non_zero",...}
{"name":"graph.node.failed","subject":"fail",...,"stream_name":"stderr",...}
{"name":"graph.failed","subject":"graph",...,"related_subject":"fail",...,"completed_node_count":0,...}
```

Safety results:

- `cmake -S exec_graph -B build/exec_graph_asan -G Ninja -DEG_ENABLE_ASAN=ON -DEG_ENABLE_UBSAN=ON` passed
- `cmake --build build/exec_graph_asan` passed
- `ctest --test-dir build/exec_graph_asan --output-on-failure` passed with 11/11 tests
- `cmake --preset tsan` passed
- `cmake --build build/exec_graph_tsan` passed
- `ctest --test-dir build/exec_graph_tsan --output-on-failure` passed with 11/11 tests
- `valgrind --error-exitcode=99 --leak-check=full --track-origins=yes build/exec_graph/eg_demo_pipeline --graph exec_graph/examples/toy_process.graph` passed with `0 errors from 0 contexts`
- running `eg_migrate` twice against the same database cleanly reported `applied=2, skipped=0` and then `applied=0, skipped=2`
- running `eg_demo_pipeline --workflow ... --emit-events-jsonl` now emits workflow-run terminal outcomes and workflow-step progress signals on both success and failure
- running `eg_demo_pipeline --workflow exec_graph/examples/timed_out.workflow --emit-events-jsonl` now emits timeout-driven `process.stop.requested`, `process.kill.sent`, and terminal `process.killed` events with `terminal_cause = timeout`
- running `eg_demo_pipeline` from `exec_graph/` with `--workflow examples/toy_linux.workflow` and `--graph examples/toy_process.graph` both succeeded
- stored-graph repository roundtrip still succeeded after persisting the graph working directory alongside source text
- product-root stored-graph save/load now succeeds after `eg_demo_pipeline` adopted the same default migration-path resolution as `eg_migrate`
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
- thread-oriented and valgrind-oriented verification now exist, but they are still narrow
- graph execution is still limited to single-input DAG nodes and a simple text graph format
- file-based execution no longer depends on repo-root cwd assumptions
- structured runtime events now include workflow-run terminal outcomes, workflow-step progress, timeout-driven supervision, graph-run terminal outcomes, and graph-layer node progress, but not yet broader scheduler/runtime surfaces
- migration support now exists through `eg_migrate`, but the migration set is still minimal
- migration execution is now idempotent and visible in smoke verification, but the migration set is still minimal

## Verification Verdict

Current verdict:

- the first implementation packet is proven enough to continue
- the build is not release-ready
- the implementation is ready for a deeper execution pass

## Next-Step Decision

Return to `Implementation execution` and continue with:

1. continue tightening SQLite infra now that migrations are explicit
2. continue widening structured runtime events from workflow, graph, and timeout-supervision signals toward broader scheduler/runtime surfaces
3. additional runtime examples beyond the current two toy workflows
4. widen hardening coverage beyond the current smoke-oriented checks
