# Implementation Design Workflow

## Purpose

Define the mandatory reasoning, minimal artifacts, and gates for implementation design.

The model must perform the reasoning.
The model should emit only the minimum artifact needed to constrain implementation work safely and concretely.

Implementation design is the second step of the implementation phase.
It converts approved design plus approved implementation-discovery choices into a buildable repository plan without silently redesigning the system.

## Global Rules

- Do not silently change approved scope, boundaries, contracts, invariants, security assumptions, operational constraints, verification obligations, or implementation-discovery decisions.
- Do not hide repository or codebase mismatches inside “we will sort it out while coding.”
- Do not let framework defaults or local file-layout convenience redefine subsystem boundaries.
- Prefer explicit module seams, ownership, and build targets over implied conventions.
- Keep the implementation design concrete enough that coding can begin without ecosystem or structure guessing.
- Assume an LLM-driven engineering team by default unless the project explicitly states a different team model.

## Required Decision Dimensions

Every major implementation-design decision must be evaluated against at least:

- approved design fit
- implementation-discovery fit
- repository fit
- build and packaging fit
- integration clarity
- ownership clarity
- verification traceability
- migration safety
- operational traceability
- maintainability
- LLM operability
- reversibility of local choices

## Inputs

Implementation design consumes:

- approved design state
- approved implementation-discovery outputs
- approved mandatory implementation-context manifest from implementation discovery
- implementation-handoff artifacts
- approved contracts and schemas
- approved invariants
- approved security and operational constraints
- approved verification obligations
- leaf constraint packages
- actual repository structure and existing code, if any
- actual implementation profile selected in implementation discovery

If no explicit team model is provided, assume:

- implementation is primarily LLM-driven
- humans provide approval, escalation answers, and high-leverage review
- repository structure, file targets, and test targets should optimize for explicitness, editability, and debuggability

## 1. Freeze The Implementation Boundary

Mandatory reasoning:

- what is already fixed by design
- what is already fixed by implementation discovery
- what mandatory implementation context applies to implementation design and must not be re-decided locally
- what implementation design is allowed to choose
- what would require escalation if changed

Minimal artifact:

- implementation-boundary checklist
- escalation list for any remaining repo-level ambiguity

Gate:

- the team knows what implementation design owns, what mandatory implementation context it must inherit, and what remains outside its authority

## 2. Inspect Repository Reality

Mandatory reasoning:

- current repository layout
- existing packages, modules, libraries, and build files
- current naming conventions
- existing test layout
- fixture and schema locations
- implementation debt or accidental structure already present
- whether the current repo should host the implementation directly or whether a split is required

Minimal artifact:

- repository reality map
- repository mismatch list

Gate:

- implementation design is grounded in the actual repo rather than an abstract target shape

## 3. Define The Realization Map

Mandatory reasoning:

- how each approved subsystem maps into the chosen codebase
- what becomes a library, module, package, service, executable, or support area
- which shared support areas are legitimate versus accidental coupling
- which boundaries need explicit interface layers

Minimal artifact:

- subsystem-to-codebase realization map

Gate:

- every approved subsystem has a concrete home in the implementation

## 4. Define Source Tree And Build Target Layout

Mandatory reasoning:

- top-level source tree shape
- build target boundaries
- test target boundaries
- where schemas, fixtures, examples, and scripts live
- where binaries, libraries, and support tools live
- how the layout supports incremental builds and isolated work

Minimal artifact:

- source tree plan
- build target map

Gate:

- implementers know where code should go and what each buildable unit is

Output convention:

- implementation-design follow-on docs should be emitted into an ordered project docs folder
- sibling docs should use explicit numeric prefixes when they form a sequence
- the project docs index should be kept ordered rather than becoming a flat unnumbered list

## 5. Define Internal Interface Boundaries

Mandatory reasoning:

- what internal interfaces exist below approved public boundaries
- which interfaces must be narrow and stable even if not public
- what data should cross subsystem seams
- what data should remain internal
- what helper layers are acceptable versus boundary blur

Minimal artifact:

- internal interface map
- below-boundary seam notes

Gate:

- coding can begin without inventing hidden cross-module contracts

## 6. Define Persistence And State Realization

Mandatory reasoning:

- where durable state lives in code
- how repositories or storage adapters are split
- how migrations are organized
- how transaction boundaries map into implementation units
- what state is cached, pooled, or transient
- where persistence concerns must stop to avoid bleeding into domain ownership

Minimal artifact:

- persistence realization plan
- migration ownership plan

Gate:

- persistence work is explicit enough that implementation will not redefine truth ownership accidentally

## 7. Define Runtime, Concurrency, And Resource Realization

Mandatory reasoning:

- how scheduler and runtime concurrency map into actual implementation units
- how long-lived processes, services, event loops, workers, or supervisors are organized
- how resource ownership and shutdown semantics are represented
- how allocator, arena, pool, or buffer strategies are scoped where relevant
- what must be proven by spikes before broader implementation proceeds

Minimal artifact:

- runtime and concurrency realization plan
- resource ownership notes

Gate:

- the runtime shape is explicit enough that implementers are not inventing execution semantics ad hoc

## 8. Define Verification Mapping

Mandatory reasoning:

- how verification obligations map into actual test targets
- where unit, contract, integration, smoke, reliability, and security tests live
- what fixtures and golden files are required
- what local commands and CI commands should exist
- what evidence each subsystem must produce

Minimal artifact:

- verification mapping
- fixture and evidence plan

Gate:

- every approved verification obligation has a concrete implementation home

## 9. Define Integration And Dependency Plan

Mandatory reasoning:

- dependency order between modules
- what can be implemented in parallel
- which modules block others
- how integration seams will be proven
- what temporary adapters or fakes are legitimate during bring-up

Minimal artifact:

- dependency and integration plan
- parallelization notes

Gate:

- build sequencing is explicit and integration risk is visible

## 10. Define Migration Or Extraction Strategy

Mandatory reasoning:

- whether the current repo layout can absorb the implementation cleanly
- whether existing files should be preserved, moved, or isolated
- whether the product should be split from tooling
- how to introduce the implementation without destabilizing reference docs and workflow tooling
- what transitional structure is acceptable and for how long

Minimal artifact:

- repository migration or extraction plan

Gate:

- repo-local reality is handled intentionally rather than drifting into a mixed, confusing layout

## 11. Define Implementation Packets And Execution Order

Mandatory reasoning:

- how the implementation profile and repo layout turn into concrete coding packets
- what each packet should deliver
- what file, module, and test targets each packet touches
- what acceptance checks gate each packet
- what spikes must happen before or during packet execution

Minimal artifact:

- repo-local implementation packet plan
- packet sequencing and acceptance notes

Gate:

- implementation can start through bounded packets rather than an unstructured coding wave

## 12. Define Developer And Agent Operating Surface

Mandatory reasoning:

- what commands implementers should run
- how formatting, linting, build, and test loops should work
- what scripts or tasks should exist
- what logs, diagnostics, and failure evidence are easiest for humans and agents to use
- how local iteration should stay fast and deterministic

Minimal artifact:

- implementation operating surface
- standard command set

Gate:

- the implementation environment is concrete enough for sustained human and agent work

## 13. Record Mismatches, Risks, And Escalations

Mandatory reasoning:

- what conflicts still exist between design, discovery, and repo reality
- what risks remain local versus cross-boundary
- what must escalate back to design
- what decisions remain intentionally deferred

Minimal artifact:

- mismatch register
- residual implementation-risk register
- escalation list

Gate:

- unresolved issues are visible and not hidden in upcoming coding work

## 14. Synthesize The Implementation Blueprint

Mandatory reasoning:

- whether the full implementation plan is coherent end to end
- whether every approved subsystem, test obligation, and operational constraint has a concrete home
- whether coding can start without reopening ecosystem or structure questions

Minimal artifact:

- implementation blueprint

Gate:

- the implementation team can start coding from a concrete, repo-local plan without guessing structure

## Required Outputs

Implementation design must produce:

- implementation-boundary checklist
- repository reality map
- repository mismatch list
- subsystem-to-codebase realization map
- source tree plan
- build target map
- internal interface map
- persistence realization plan
- runtime and concurrency realization plan
- verification mapping
- fixture and evidence plan
- dependency and integration plan
- repository migration or extraction plan
- repo-local implementation packet plan
- implementation operating surface
- mismatch register
- residual implementation-risk register
- escalation list
- implementation blueprint

## Completion Standard

Implementation design is complete only when:

- every approved subsystem has a concrete home in the codebase
- build targets, test targets, and fixture locations are explicit
- below-boundary interfaces are explicit enough to prevent ad hoc coupling
- persistence, runtime, and integration concerns are mapped into real implementation units
- implementation packets are concrete enough to begin coding safely
- repository migration or extraction issues are handled intentionally
- important unresolved issues are either reduced by spikes or explicitly escalated
- no repo-local ambiguity is being hidden inside “we will figure it out while implementing”
