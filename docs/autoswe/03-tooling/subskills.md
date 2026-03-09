# Design Subskills

## Purpose

This document defines the recommended subskill layout under the design orchestrator.

The orchestrator owns control flow.
Subskills own expert reasoning standards.

## Initial Subskill Set

Start with these subskills.

### 1. Problem Framing

Own:

- problem extraction
- success criteria
- non-goals
- constraints
- risk framing

Use when:

- the problem statement is solution-shaped
- users or goals are ambiguous
- scope control is weak

### 2. System Architecture

Own:

- solution framing
- decomposition
- subsystem boundaries
- trust boundaries
- major tradeoffs

Use when:

- major structure is still undecided
- responsibilities are colliding
- boundaries are unclear

### 3. Contracts And Interfaces

Own:

- APIs
- events
- schemas
- invariants across boundaries
- error models
- versioning

Use when:

- multiple modules or teams must build independently
- schemas are unstable
- interface drift is a risk

### 4. Internal Subsystem Design

Own:

- internal structure
- state handling
- persistence
- concurrency
- observability
- local security constraints

Use when:

- a subsystem is approved externally but underspecified internally
- local realization decisions still affect correctness

### 5. Behavior And Workflow Design

Own:

- key flows
- control flow
- long-running workflows
- failure and recovery paths

Use when:

- the system has stable nouns but unclear behavior
- important flows are being hand-waved
- failure paths would otherwise be invented during implementation

### 6. Security And Trust Boundaries

Own:

- permissions and authority boundaries
- trust boundaries
- isolation assumptions
- security invariants
- auditability expectations

Use when:

- security posture changes boundary design
- authority or permission rules are unclear
- implementation might otherwise guess trust boundaries

### 7. Operational Design

Own:

- deployment shape
- migration and rollback strategy
- rollout safety
- recoverability
- operational ownership assumptions

Use when:

- deployment or migration choices materially constrain the design
- rollback, recovery, or observability cannot be left implicit
- implementation might otherwise invent unsafe rollout behavior

### 8. Verification And Handoff

Own:

- acceptance criteria
- contract tests
- integration checks
- leaf constraint packages
- escalation rules for implementation agents

Use when:

- design must be converted into implementation-ready packets
- verification is weak or missing

## Later Optional Subskills

Add separate subskills only when the domain is large enough to justify them.

Examples:

- migration and rollout execution
- performance and reliability
- data modeling
- distributed systems

### 9. Implementation Discovery

Own:

- ecosystem option generation
- language and framework evaluation
- buy-vs-build decisions
- toolchain decisions
- licensing and dependency governance
- compatibility and release-engineering evaluation
- evidence and spike policy
- implementation risk synthesis

Use when:

- design is complete and implementation is about to begin
- the implementation ecosystem is still undecided
- there are major technology tradeoffs with architectural consequences if chosen badly

Default assumption:

- team-fit analysis assumes an LLM-driven engineering team unless a different team model is specified

It should not silently redesign the approved system.

## Subskill Discipline

Each subskill should define:

- what must be reasoned through
- what must be output
- what common traps to catch
- when to escalate back to the orchestrator

Subskills should not:

- redefine the global process
- own project state
- assume a specific storage or graph system
- produce implementation instructions beyond approved design constraints
