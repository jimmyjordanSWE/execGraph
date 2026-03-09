# Minimal Specification Rule

## Purpose

This document defines how to balance full design reasoning with compact design output.

The design system must force the model to think deeply.
The design system must not force the model to print every thought.

## Core Rule

Specify only what the downstream agent is likely to get wrong in a costly way.

Leave routine implementation detail implicit.

## Always Specify

- subsystem purpose
- responsibility boundaries
- API and event contracts
- shared data definitions
- ownership of state and truth
- invariants
- failure behavior when non-obvious
- required tests
- acceptance criteria
- escalation conditions

## Usually Leave Implicit

- helper function layout
- internal naming choices
- standard framework patterns
- ordinary validation boilerplate
- common library usage
- routine local algorithms
- simple internal refactors

## Decision Test

Keep a detail explicit if removing it would likely cause:

- cross-module inconsistency
- contract drift
- ownership confusion
- invalid data handling
- hidden architectural change
- verification gaps

Drop a detail if removing it would only affect:

- local style
- internal tidiness
- code aesthetics
- low-cost local implementation choices

## Operational Rule

The model must complete the design reasoning for each stage.

The stage artifact should contain only:

- the constraints
- the decisions
- the interfaces
- the proofs required downstream

This is how the system avoids both under-specification and documentation bloat.
