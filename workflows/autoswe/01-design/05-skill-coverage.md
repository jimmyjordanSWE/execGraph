# Software Design Skill Coverage

## Purpose

This document maps the original `docs/autoswe/99-reference/reference/software-design-skill.md` teaching flow into the operational autoSWE workflow.

The goal is to make sure the workflow does not silently skip important reasoning just because the emitted artifacts stay compact.

## Coverage Map

Original skill flow:

1. understand the problem
2. define requirements
3. model the domain
4. break the problem into parts
5. define boundaries
6. choose representations
7. design behavior and flows
8. pick technical structure
9. implement in layers
10. test the right things
11. observe and refine

Workflow coverage:

### 1. Problem Definition

Covers:

- understand the problem
- identify users, goals, risks, constraints

### 2. Requirements Definition

Covers:

- define requirements explicitly
- capture functional requirements
- capture non-functional requirements
- make scope edges and tradeoff pressure visible

### 3. Solution Framing

Covers:

- why software is the right answer
- broad architecture direction
- major tradeoffs and alternatives

### 4. Domain And Data Modeling

Covers:

- model the domain
- define entities, states, events, invariants, terminology

### 5. Behavior And Workflow Design

Covers:

- design behavior and flows explicitly
- define key workflow paths, failure paths, and recovery paths

### 6. System Decomposition

Covers:

- break the problem into parts
- define boundaries
- assign responsibility and ownership

### 7. Contract Design

Covers:

- choose representations
- design APIs, events, schemas, error models, and state transitions
- define behavior at cross-module seams

### 8. Internal Subsystem Design

Covers:

- implement in layers as a design concern
- internal structure, persistence, workflows, concurrency, observability, and local failure handling

### 9. Security And Trust-Boundary Design

Covers:

- make trust boundaries explicit
- define permissions, isolation, and security invariants before implementation

### 10. Operational Design

Covers:

- deployment, migration, rollout, rollback, recovery, and observability as design concerns
- operational structure that materially constrains implementation

Concrete language, framework, library, and tool selection is intentionally deferred to implementation discovery after design.

### 11. Verification Design

Covers:

- test the right things
- define contract, integration, reliability, performance, and security checks

### 12. Implementation Handoff

Covers:

- bounded implementation packets
- implementation-discovery inputs
- escalation rules for implementation agents

## Iteration Rule

The original skill's "observe and refine" step is not a single terminal stage.

It is the loop around the workflow:

- reassess when new evidence appears
- move back upward when implementation or verification exposes design ambiguity
- update approved design state instead of silently patching around it

## Rule

The workflow may compress artifacts.

It may not compress away required reasoning from the original skill.
