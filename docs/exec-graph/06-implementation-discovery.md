# Exec Graph Implementation Discovery

## Purpose

This document reruns the `Implementation discovery` workflow for exec-graph using an updated hard constraint:

- the primary V1 implementation language must be `C++17`

The goal is to select a concrete V1 implementation profile without changing the approved design boundary.

## 1. Non-Negotiables

The following remain design-owned and are not open to local implementation redesign:

- canonical type grammar and compatibility semantics
- graph truth, revision ownership, and optimistic concurrency semantics
- explicit node capability and sandbox declarations
- transport-agnostic command/query/event model with UTF-8 JSON encoding
- artifact descriptors, retention classes, and content-addressed artifact handling
- support for one-shot tasks and long-lived services
- security invariants around capability narrowing, path normalization, and auditability
- verification obligations already defined in the design artifacts

Additional implementation constraint for this rerun:

- the primary V1 control-plane and runtime implementation language is `C++17`

Implementation discovery is still allowed to choose libraries, build tooling, testing infrastructure, packaging, and buy-vs-build posture as long as those choices do not change the approved contracts or boundaries.

## 2. Environment And Repo Inventory

Current repo reality:

- the repository currently contains design artifacts, the `autoswe` autoSWE workflow tool, and exec-graph follow-on specs
- there is no exec-graph implementation package yet
- the current executable tooling in the repo is Python-based, but that is tooling reality rather than a product implementation constraint
- the repo already has clear subsystem boundaries and follow-on product specifications for runtime, validation, artifacts, and the SDK contract
- local design state is stored in `.design/design.db`
- the repo is not yet laid out as a product implementation repository for exec-graph itself

Inherited constraints from approved design:

- V1 is a modular monolith control plane, not a distributed rewrite
- V1 metadata storage is SQLite-backed
- V1 artifact bytes are stored in a local content-addressed filesystem
- the runtime target is Linux subprocess execution, with OCI container support optional
- the public API shape is HTTP JSON commands and queries plus SSE events
- cross-language support happens at the node-definition and runtime boundary, not by rewriting the control plane in every language

Practical delivery constraints inferred from the project:

- implementation should minimize confusion between autoSWE tooling and exec-graph product code
- the first implementation should run cleanly on Linux and fit containerized deployment if desired
- the stack should support explicit arenas, pools, and low-level runtime control where beneficial
- the implementation should not depend on managed-runtime GC behavior for core control-plane correctness

## 3. Discovery Questions

The concrete discovery questions are:

1. With `C++17` fixed, is `Go` still a materially better fit for V1 despite the new constraint?
2. What networking and HTTP stack should implement HTTP JSON plus SSE?
3. What JSON representation layer should implement public contracts?
4. What persistence layer should sit on top of SQLite?
5. What concurrency/runtime model should supervise scheduling and Linux process execution?
6. What build, lint, static-analysis, sanitizer, and test toolchain should be standard?
7. What should be built in-house versus adopted?
8. What implementation work should be postponed until after V1 proof?
9. What repo-local structure should implementation design assume?

Dependencies between discovery questions:

- the language constraint drives memory strategy, process supervision, and packaging
- HTTP and JSON choices should align with the public contract boundary
- persistence choice affects migration and verification design
- concurrency model affects scheduler and runtime-linux shape
- build and analysis tooling must be chosen with the language/runtime profile, not separately

## 4. Candidate Option Set

### 4.1 Primary Language

Serious candidates:

- `C++17`
- `Go`

### 4.2 Networking And API Layer Under `C++17`

Serious candidates:

- Boost.Asio + Boost.Beast
- a heavier full-stack C++ web framework

Eliminated for V1:

- framework-heavy C++ web stacks with large internal policy surfaces

### 4.3 JSON And Contract Layer Under `C++17`

Serious candidates:

- Boost.JSON
- a separate third-party JSON library plus custom adapters

### 4.4 Persistence Access Layer

Serious candidates:

- SQLite C API with thin internal RAII wrappers and prepared-statement caching
- a heavier ORM-like abstraction layer

Eliminated for V1:

- ORM-style mapping layers
- Postgres as the primary metadata store

### 4.5 Toolchain

Serious candidates:

- CMake + CTest + GoogleTest + clang-tidy + Clang Static Analyzer + ASan/TSan/UBSan
- Go toolchain with `go build`, `go test`, `pprof`, and race detection

## 5. C++17 Versus Go

### C++17

Strengths:

- best fit for explicit arenas, object pools, fixed-size node storage, and allocator-aware data structures
- strongest control over memory layout, lifetimes, and process/runtime boundaries
- direct fit for Linux subprocess, signal, file-descriptor, and sandbox-adjacent work
- better long-term substrate match if exec-graph is fundamental execution infrastructure
- easy to deploy in a Docker image as a native binary without bringing along a managed runtime

Weaknesses:

- higher implementation and review burden than Go
- lower baseline safety against LLM-generated memory and lifetime bugs
- more build-system and dependency complexity than Go

Assessment:

- best fit for the updated project priorities and explicit language constraint

### Go

Strengths:

- simpler build and test flow through the standard `go` toolchain
- strong built-in profiling and diagnostics
- race detector and good concurrency ergonomics
- lower incidental complexity for HTTP/server code

Weaknesses:

- weaker control over allocation patterns, layout, and object lifetime than `C++17`
- GC and runtime behavior remain part of the execution model even if pools are used
- worse fit for a design centered on explicit arenas and fixed-size pooled node structures
- less suitable if the control plane is treated as infrastructure substrate rather than mostly application server code

Assessment:

- best alternative, but not the winner under the new constraint set

## 6. Design-Fit Assessment

The approved design describes a system that is:

- control-plane heavy
- runtime and process heavy
- artifact and state heavy
- sensitive to execution semantics and resource control

Under the updated priorities, the following matter more than they did in the earlier pass:

- deterministic layout and allocation control
- low-level control of long-lived infrastructure behavior
- confidence in process supervision and runtime enforcement
- container-friendly native deployment

That pushes the selection toward:

- `C++17` for the control plane and runtime core
- thin, explicit libraries rather than framework-heavy abstractions

## 7. Delivery And Team-Fit Assessment

Assumed team model:

- implementation is primarily LLM-driven
- humans act as reviewers, approvers, and escalation points

This no longer dominates the language decision.
Instead, it constrains how the `C++17` implementation should be shaped:

- explicit ownership rules
- simple subsystem seams
- allocator and pool strategies documented and tested
- no template-heavy metaprogramming for its own sake
- no giant framework indirection
- no hidden ORM-style data model

Selected direction:

- `C++17` with a conservative, explicit style
- RAII-based internal wrappers
- subsystem-local ownership
- small, testable units around runtime, persistence, HTTP, and graph logic

Why this wins:

- it respects the infrastructure-first nature of the project
- it still leaves enough structural discipline for LLM-assisted implementation to be workable

## 8. Operational And Security Assessment

Selected operational direction:

- single deployable native control-plane process for V1
- SQLite metadata store on local disk
- local content-addressed artifact root
- Linux subprocess runtime first
- optional OCI container execution kept behind the runtime adapter boundary
- Docker-compatible deployment path for packaging and environment control

Operational strengths:

- low runtime overhead ceiling
- direct OS integration for subprocess supervision
- strong fit for long-lived infrastructure processes

Operational risks:

- memory/lifetime bugs are a real risk and must be actively controlled
- C++ dependency management and build reproducibility need discipline from the start
- framework sprawl would make the system harder to audit and change

Security posture:

- explicit sandbox and capability narrowing remain mandatory
- runtime-linux should start with honest Linux process confinement and only add OCI isolation when proven
- sanitizers and static analysis are part of the safety posture, not optional extras

## 9. Verification-Feasibility Assessment

The selected `C++17` stack supports the required verification model if the toolchain is chosen explicitly:

- unit and integration tests through GoogleTest and CTest
- static analysis through clang-tidy and the Clang Static Analyzer
- runtime bug detection through AddressSanitizer, ThreadSanitizer, and UndefinedBehaviorSanitizer
- end-to-end smoke tests through native test binaries and fixture-driven process execution

Verification benefits:

- allocator and lifetime bugs can be surfaced early with sanitizers
- SQLite usage can stay explicit and therefore easier to test
- process supervision paths can be exercised directly against Linux behavior

Verification risk:

- if ownership and allocator rules are under-specified, sanitizer coverage will catch bugs late rather than preventing them structurally

Mitigation:

- implementation design must make ownership, pools, and subsystem lifetimes explicit

## 10. Economic And Licensing Assessment

Selected stack economics:

- `C++17`, CMake, GoogleTest, Boost, Clang tools, and SQLite are viable for a private internal and product implementation path
- V1 avoids hosted control-plane dependencies
- local-first and container-friendly deployment keeps infrastructure cost low during proof

Economic conclusion:

- the chosen stack is economically safe for V1

## 11. Compatibility And Interoperability Assessment

Compatibility requirements already fixed by design:

- Linux runtime support is required
- JSON wire encoding is canonical
- wrapped nodes may target multiple languages

Selected compatibility posture:

- native `C++17` control plane on Linux
- HTTP JSON plus SSE for client interaction
- wrapped-node compatibility achieved through node-definition contracts and runtime bindings

Why this fits:

- `C++17` does not reduce node-language flexibility
- the implementation language of the control plane remains separate from the languages supported at the node boundary

## 12. Release-Engineering Assessment

Selected release-engineering direction:

- use CMake as the build system
- use Ninja as the default local/CI generator when available
- use CTest for test orchestration
- package deployable binaries and container images explicitly
- keep schema fixtures, node-definition fixtures, and sample graphs in-repo as release evidence

Release implications:

- release builds should be reproducible from a containerized build environment
- sanitizer-enabled builds should remain a standard verification profile even if release builds are optimized
- build configuration must be explicit and simple enough for CI and LLM operators to reason about

## 13. Ownership And Maintainability Assessment

Ownership model for V1:

- each approved subsystem becomes a real `C++17` library or module boundary
- persistence remains a thin infrastructure layer, not the center of the domain model
- runtime-linux owns low-level process and signal behavior
- graph-core owns truth and revisions
- API-server owns transport and session logic

Maintainability rules:

- avoid template metaprogramming unless it is clearly paying for itself
- avoid heavy macro-based frameworks
- keep allocator and pool usage local and documented
- keep public-contract models and internal storage models separate

Long-term concern:

- this repo still mixes product design artifacts with autoSWE tooling

Implication for implementation design:

- implementation design should either carve out a dedicated `exec_graph/` product tree or split the product into its own repo before heavy coding begins

## 14. Buy-Vs-Build Register

Build in-house:

- canon-types
- graph-core
- node-defs
- graph-validate
- runtime-api
- runtime-linux
- artifact-store
- scheduler
- exec-graph command/query/event contracts
- allocator, arena, and object-pool strategy specific to exec-graph

Adopt:

- Boost.Asio and Boost.Beast for async networking and HTTP
- Boost.JSON for JSON parsing and serialization
- SQLite C API for metadata persistence
- CMake and CTest for build and test orchestration
- GoogleTest for unit and integration tests
- clang-tidy, Clang Static Analyzer, ASan, TSan, and UBSan for quality gates

Wrap, do not own:

- wrapped executables and language runtimes
- optional OCI engine CLI integration

Postpone:

- distributed control plane
- Postgres-first deployment
- container-orchestrator integration
- browser/UI implementation

## 15. Evidence Policy And Spike Plan

Paper-analysis decisions:

- `C++17` as the primary implementation language
- Docker-compatible native deployment
- SQLite as the V1 metadata store
- CMake, GoogleTest, and Clang-based analysis as the quality toolchain

Spike-required decisions:

1. runtime supervision spike
   - prove the selected `C++17` event loop and subprocess model can handle one-shot tasks, long-lived services, bounded output capture, stop, kill, and restart
2. HTTP and SSE spike
   - prove the selected networking stack can serve command/query traffic and ordered SSE event streams without distorting the normalized event contract
3. allocator and pool spike
   - prove the proposed arena and object-pool strategy works cleanly across graph-core, scheduler plan state, and runtime event buffering without creating unacceptable lifetime complexity
4. SQLite contention and migration spike
   - prove prepared-statement caching, transaction scopes, and migration discipline are sufficient for local V1 load

Spike success criteria:

- each spike must end with runnable code, a short findings note, and an explicit accept/reject decision
- if a spike exposes design pressure, it must escalate back to design instead of being hidden in implementation code

## 16. Decision Scoring Summary

Relative weighting:

- design fit: very high
- systems and runtime control: very high
- performance feasibility: high
- verification feasibility: high
- operational simplicity: high
- team and LLM operability: medium
- economic impact: medium
- long-term changeability: high

Overall outcome:

| Decision | Winner | Why |
| --- | --- | --- |
| Primary language | `C++17` | Best fit for explicit arenas, pools, native runtime control, and infrastructure-first priorities |
| Main alternative | `Go` | Strong server/runtime alternative, but weaker fit for explicit memory strategy and substrate-style control |
| API layer | Boost.Asio + Boost.Beast | Thin, explicit async and HTTP stack without forcing a larger framework |
| JSON layer | Boost.JSON | Good allocator-aware JSON support inside the selected Boost-based stack |
| Persistence layer | SQLite C API + thin internal wrappers | Maximum control, explicit statements, no ORM drag |
| Toolchain | CMake, CTest, GoogleTest, clang-tidy, Clang Static Analyzer, ASan/TSan/UBSan | Strong native-systems build and verification posture |

## 17. Selected Implementation Profile

Primary implementation profile for exec-graph V1:

- language: `C++17`
- build system: CMake
- default builder: Ninja when available
- test runner: CTest
- unit and integration tests: GoogleTest
- static analysis: clang-tidy and Clang Static Analyzer
- runtime diagnostics: AddressSanitizer, ThreadSanitizer, and UndefinedBehaviorSanitizer
- API layer: Boost.Asio + Boost.Beast
- JSON and contract models: Boost.JSON plus explicit typed adapters
- metadata store: SQLite
- persistence access: SQLite C API with internal RAII wrappers, prepared statements, and explicit transactions
- concurrency and supervision: explicit event-loop and supervisor model in `C++17`
- artifact bytes: local content-addressed filesystem
- packaging: native binary plus Docker image path

Repo-local structure assumption for implementation design:

- create a dedicated `exec_graph/` product source tree separate from the `autoswe/` autoSWE package
- organize the code by approved subsystem boundaries, not by framework layers
- keep docs and JSON fixtures as first-class implementation inputs

## 18. Technology Selection Record

Accepted tradeoffs:

- choose `C++17` over `Go`, accepting higher implementation discipline costs in exchange for stronger runtime control and allocator freedom
- choose explicit native libraries over a heavier C++ web framework
- choose direct SQLite integration over a larger abstraction layer
- defer distributed deployment and deep container orchestration until the local substrate is proven

Rejected directions:

- reverting to a Python control plane for V1
- using a GC-managed runtime as the primary implementation substrate
- adopting a heavy ORM or large policy-heavy application framework

## Residual-Risk Register

- memory and lifetime bugs remain a real engineering risk and must be actively controlled through design discipline and sanitizers
- the exact arena and pool boundaries still need an implementation spike
- the current repo layout still mixes the product track with autoSWE tooling, which can confuse implementation boundaries
- ordered SSE event fanout performance still needs to be proven under realistic churn

## Escalation List

Escalate back to design if any of the following become necessary:

- replacing optimistic concurrency or revision semantics
- weakening or expanding the declared capability and sandbox model
- changing the normalized command/query/event envelope shape
- changing artifact identity or retention semantics
- requiring a distributed control plane to satisfy V1 correctness

## External Ecosystem References

These references informed the selected profile and should be checked again during implementation design:

- CMake docs: https://cmake.org/documentation/
- GoogleTest docs: https://google.github.io/googletest/primer.html
- Boost.Asio docs: https://www.boost.org/doc/html/boost_asio/overview.html
- Boost.Beast docs: https://www.boost.org/library/develop/beast/
- Boost.JSON docs: https://www.boost.org/libs/json/
- SQLite docs: https://www.sqlite.org/docs.html
- SQLite C/C++ intro: https://www.sqlite.org/cintro.html
- SQLite C API reference: https://www.sqlite.org/capi3ref.html
- Clang Static Analyzer docs: https://clang.llvm.org/docs/ClangStaticAnalyzer.html
- AddressSanitizer docs: https://clang.llvm.org/docs/AddressSanitizer.html
- UndefinedBehaviorSanitizer docs: https://clang.llvm.org/docs/UndefinedBehaviorSanitizer.html
- Go diagnostics docs: https://go.dev/doc/diagnostics.html
- Go command docs: https://go.dev/doc/cmd
- Go PGO docs: https://go.dev/doc/pgo
