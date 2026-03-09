# Minimal Specification Rule

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

## Operational Rule

The model must complete the design reasoning for each stage.

The stage artifact should contain only:

- the constraints
- the decisions
- the interfaces
- the proofs required downstream
