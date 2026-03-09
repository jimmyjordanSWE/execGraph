# autoSWE

## Purpose

This document defines a design-first system for building large software with LLM agents.

The system separates:

- design
- implementation
- verification

autoSWE exists to produce the minimum sufficient constraints needed for strong implementation agents to build leaf modules correctly without silently redesigning the system.

It is not a coding workflow. It is a design workflow.

## Core Assumptions

- Design is a separate discipline from implementation.
- autoSWE must force complete design reasoning without forcing verbose design output.
- The design output must be compact, explicit, and shaped for downstream agents.
- The system should specify boundaries, contracts, invariants, and ownership precisely.
- The system should avoid describing routine implementation detail that strong models can infer safely.
- Custom project tooling may manage graph state, progress, dependencies, and storage, but the skill system should encode expertise and process, not project database structure.

## Primary Goal

Produce implementation-handoff artifacts plus constraint packages for leaf modules.

The handoff artifacts should be sufficient to start implementation discovery and bounded implementation work without reopening approved design.
A constraint package should be sufficient for a strong implementation agent to one-shot or near-one-shot a bounded module while staying inside approved design.

## What The Design System Produces

autoSWE should produce:

- scoped problem definitions
- solution framing decisions
- domain and data definitions
- subsystem boundaries
- API and contract definitions
- internal subsystem constraints where needed
- operational design constraints
- verification requirements
- implementation-handoff artifacts and implementation-discovery inputs
- leaf-level constraint packages

autoSWE should not produce:

- detailed implementation documentation
- class-by-class coding plans
- boilerplate framework instructions
- unnecessary prose about standard engineering techniques

## Process Shape

Use one design orchestrator plus design subskills.

The orchestrator owns:

- stage selection
- gating
- escalation
- artifact requirements
- tool suggestion and tool request behavior

The subskills own:

- deep reasoning standards for each design domain
- checklists
- failure modes
- review criteria
- compact artifact patterns

The project toolchain owns:

- storage
- dependency tracking
- progress visualization
- indexing
- traceability
- optional graph editing

## Design Principle

The model must do all important design thinking.

The model must emit only the minimum artifact needed to constrain downstream work.

This is the governing balance of the system:

- mandatory reasoning depth
- minimal artifact verbosity

## High-Level Stage Model

The design workflow is split into three conceptual layers.

### 1. Problem Design

- Problem definition
- Requirements definition
- Solution framing

### 2. System Design

- Domain and data modeling
- Behavior and workflow design
- System decomposition
- Contract design
- Security and trust-boundary design

### 3. Delivery Design

- Internal subsystem design
- Operational design
- Verification design
- Implementation handoff

The design phase ends at `Implementation handoff`.
The next phase is implementation, whose first step is `Implementation discovery`.

Implementation discovery chooses languages, frameworks, libraries, implementation-local tools, buy-vs-build posture, and the practical delivery ecosystem around the approved design.
Design should constrain those choices when necessary, but should not guess them by default.
Implementation discovery should also emit a mandatory implementation-context artifact so downstream implementation steps inherit selected dependencies, required tools, forbidden substitutions, and escalation triggers without rediscovering them locally.

The next step after that is `Implementation design`.

Implementation design maps the approved architecture into the actual repository, source tree, build targets, internal seams, verification targets, migration strategy, and repo-local implementation packets.

Implementation discovery is therefore the point where a professional team would do its major ecosystem and tooling tradeoff analysis.
It should include explicit evaluation of design fit, delivery fit, operational risk, security risk, verification fit, economics and licensing, compatibility, release engineering, ownership, buy-vs-build decisions, and evidence thresholds.
By default, that delivery-fit analysis assumes an LLM-driven engineering team unless the project explicitly says otherwise.
Implementation design, implementation execution, and implementation verification should then consume that mandatory implementation context rather than reinterpreting discovery decisions from scratch.

These stages are expanded in the canonical design workflow folder [workflows/autoswe/01-design/README.md](../../../workflows/autoswe/01-design/README.md).

## Main Artifacts

The primary downstream artifacts are:

- implementation-handoff artifacts and implementation-discovery inputs
- the constraint package described in [workflows/autoswe/01-design/03-constraint-package.md](../../../workflows/autoswe/01-design/03-constraint-package.md)
- the design-to-implementation boundary defined in [workflows/autoswe/02-implementation/00-implementation-boundary.md](../../../workflows/autoswe/02-implementation/00-implementation-boundary.md)
- the implementation-phase workflows in [workflows/autoswe/02-implementation/01-implementation-discovery/01-workflow.md](../../../workflows/autoswe/02-implementation/01-implementation-discovery/01-workflow.md), [workflows/autoswe/02-implementation/02-implementation-design/01-workflow.md](../../../workflows/autoswe/02-implementation/02-implementation-design/01-workflow.md), [workflows/autoswe/02-implementation/03-implementation-execution/01-workflow.md](../../../workflows/autoswe/02-implementation/03-implementation-execution/01-workflow.md), and [workflows/autoswe/02-implementation/04-implementation-verification-execution/01-workflow.md](../../../workflows/autoswe/02-implementation/04-implementation-verification-execution/01-workflow.md)

## Status

This document is now supporting rationale.

The canonical operational definition of autoSWE lives under [workflows/autoswe/README.md](../../../workflows/autoswe/README.md).

The workflow's reasoning coverage against the original [reference/software-design-skill.md](../99-reference/reference/software-design-skill.md) is tracked in [workflows/autoswe/01-design/05-skill-coverage.md](../../../workflows/autoswe/01-design/05-skill-coverage.md).

## Decision Rule

If an implementation agent could make an expensive wrong guess, specify the constraint.

If the choice is local, routine, and cheap to correct, let the implementation agent infer it.
