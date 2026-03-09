# Implementation Execution Workflow

## Purpose

Define the mandatory reasoning, minimal artifacts, and gates for implementation execution.

The model must perform the reasoning.
The model should emit only the minimum additional state needed to control safe coding, proof, and iteration.

Implementation execution is where the approved implementation packets become code, tests, benchmarks, and runtime behavior.
It must not silently redesign the system while coding.

## Global Rules

- Do not treat coding as a freeform phase detached from design, discovery, and implementation design.
- Do not let local convenience violate approved boundaries, contracts, invariants, or ownership rules.
- Do not postpone all testing until the end of the project.
- Do not force blanket test-first behavior where interfaces are still moving quickly, but do require explicit proof strategy before coding each packet.
- For native or performance-sensitive systems, treat sanitizers, static analysis, memory tools, and benchmarks as part of the implementation loop rather than a separate afterthought.
- Require regression proofs for fixed bugs unless there is an explicit reason they are impossible or wasteful at the current stage.
- Assume an LLM-driven engineering team by default unless the project explicitly states a different team model.

## Required Decision Dimensions

Every major implementation-execution decision must be evaluated against at least:

- packet-scope fit
- boundary integrity
- correctness risk
- ownership and lifetime clarity
- concurrency and resource-safety risk
- verification value
- benchmark or performance sensitivity
- local iteration speed
- reversibility of the change
- evidence quality

## Inputs

Implementation execution consumes:

- approved design state
- approved implementation-discovery outputs
- approved implementation-design blueprint
- repo-local implementation packets and packet acceptance notes
- verification obligations
- actual repository state at execution time
- current failing tests, smoke paths, and benchmark baselines if they exist

If no explicit team model is provided, assume:

- implementation is primarily LLM-driven
- humans provide approval, escalation answers, and high-leverage review
- code structure, diagnostics, and commands should optimize for explicitness, fast proof loops, and easy debugging

## 1. Freeze The Packet Boundary

Mandatory reasoning:

- which implementation packet is currently in scope
- what files, modules, and test targets it is allowed to touch
- what boundaries it must not cross
- what design- or discovery-owned assumptions remain fixed
- what would force escalation if changed

Minimal artifact:

- active packet definition
- packet-scope do-not-cross list

Gate:

- the team knows what it is implementing now and what remains out of scope

## 2. Define Proof Obligations Before Coding

Mandatory reasoning:

- what this packet must prove when complete
- what invariants must remain true throughout execution
- what failures would invalidate the packet
- what tests, smoke paths, or manual probes are required now
- what tests are intentionally deferred and why
- whether this packet is performance-sensitive enough to require benchmark evidence now

Minimal artifact:

- packet proof obligations
- deferred-proof note

Gate:

- coding cannot begin until success and failure are explicit

## 3. Prepare The Local Execution Loop

Mandatory reasoning:

- what build command should be the main inner loop
- what test command should be the fast proof loop
- what diagnostics should be easy to run repeatedly
- what sanitizer, memory-check, or static-analysis loop should be attached for this packet
- what benchmark loop should exist if the packet is performance-sensitive

Minimal artifact:

- packet operating loop
- packet command set

Gate:

- the team has a short, repeatable loop for coding and proof instead of a vague “implement then figure it out”

## 4. Implement The Smallest Vertical Slice

Mandatory reasoning:

- what smallest end-to-end slice proves the packet direction
- what interfaces must exist first
- what code can stay skeletal versus what must be real immediately
- what temporary fakes are legitimate and what would be misleading

Minimal artifact:

- smallest vertical-slice plan

Gate:

- implementation starts by proving shape and wiring, not by creating uncontrolled surface area

## 5. Run The Fast Correctness Loop

Mandatory reasoning:

- what should be built and run after each meaningful change
- what failures indicate design misunderstanding versus local coding bugs
- whether the packet is converging or just accumulating code

Minimal artifact:

- loop evidence notes

Gate:

- implementation progress is measured by passing proof steps, not only by added code

## 6. Review Ownership, Lifetimes, And Resource Safety

Mandatory reasoning:

- where ownership lives
- how resources are acquired and released
- how memory, buffers, processes, file handles, sockets, or transactions are bounded
- how long-lived versus short-lived state is separated
- how concurrency or async behavior avoids races, leaks, or shutdown ambiguity

This must stay technology-agnostic at the workflow level.
The project-specific answer may use RAII, GC discipline, arenas, pools, borrow rules, or other ecosystem-local techniques selected in implementation discovery.

Minimal artifact:

- ownership and lifetime notes
- resource-safety note

Gate:

- the packet has an explicit local correctness model rather than implicit lifetime guesses

## 7. Add Regression And Performance Protection

Mandatory reasoning:

- what new regression tests are required because of the implemented behavior or a fixed bug
- what contract, smoke, or integration tests must be added or updated
- whether performance-sensitive behavior needs a benchmark baseline or comparison run
- which metrics matter for this packet and what counts as a regression

Minimal artifact:

- regression additions
- benchmark note or benchmark waiver

Gate:

- changed behavior is protected against silent breakage and silent slowdown where it matters

## 8. Integrate The Packet Cleanly

Mandatory reasoning:

- whether the packet integrates without blurring subsystem boundaries
- whether interfaces remain narrow and coherent
- whether temporary shims should now be removed
- whether docs, examples, fixtures, and commands need updating

Minimal artifact:

- integration notes
- cleanup list

Gate:

- the packet lands as part of a coherent system, not as a pile of local hacks

## 9. Run Packet Acceptance Checks

Mandatory reasoning:

- what exact checks prove packet completion
- what build, test, smoke, sanitizer, and benchmark checks are mandatory
- what must pass locally before the packet can be called done

Minimal artifact:

- packet acceptance results

Gate:

- the packet cannot be marked done without explicit evidence

## 10. Record Evidence, Risks, And Follow-On Work

Mandatory reasoning:

- what evidence now exists
- what residual risks remain
- what follow-on packet or remediation item is newly exposed
- whether anything must escalate back to design or implementation design

Minimal artifact:

- execution evidence record
- residual-risk note
- escalation note if needed

Gate:

- the implementation state is durable and auditable after the packet finishes

## 11. Recurse Or Escalate

Mandatory reasoning:

- whether the next best action is another execution pass, broader verification, hardening, or escalation
- whether the current branch of work is still the best leverage point

Minimal artifact:

- next execution decision

Gate:

- implementation moves intentionally to the next loop instead of drifting

## Required Outputs

Implementation execution must produce:

- active packet definition
- packet proof obligations
- deferred-proof note
- packet operating loop
- smallest vertical-slice plan
- loop evidence notes
- ownership and lifetime notes
- resource-safety note
- regression additions
- benchmark note or benchmark waiver
- integration notes
- cleanup list
- packet acceptance results
- execution evidence record
- residual-risk note
- escalation note if needed
- next execution decision

## Completion Standard

Implementation execution is complete for a packet only when:

- the scoped functionality is implemented
- the packet proof obligations are satisfied
- required tests and smoke checks for the packet pass
- required sanitizer, memory-tool, static-analysis, or benchmark checks have been run or explicitly waived
- ownership, lifetime, resource, and shutdown behavior are explicit enough for the chosen stack
- residual risks and follow-on work are recorded clearly
