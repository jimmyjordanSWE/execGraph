# Constraint Package

## Purpose

A constraint package is the final design artifact for a leaf module.

Its job is to let a strong implementation agent build the module correctly without redesigning the system and without needing a long implementation document.

It travels alongside the implementation-handoff artifacts and implementation-discovery inputs produced at the end of design.

## Required Properties

A good constraint package is:

- compact
- explicit
- bounded
- testable
- sufficient for one-shot or near-one-shot implementation

If a module still cannot be implemented safely from its package, either:

- the package is missing a critical constraint, or
- the module is too large or badly scoped

## Required Fields

Every leaf constraint package should define:

- module name
- purpose
- responsibilities
- non-responsibilities
- dependencies
- inputs
- outputs
- data definitions and schemas
- invariants
- error behavior
- performance, reliability, or security constraints when relevant
- required tests
- acceptance criteria
- escalation conditions

## Recommended Template

```md
# <module-name>

## Purpose
- What this module is for.

## Responsibilities
- What this module must do.

## Non-Responsibilities
- What this module must not own or decide.

## Dependencies
- Upstream contracts
- Downstream consumers
- External systems

## Inputs
- Public API inputs
- Event inputs
- Configuration inputs

## Outputs
- Return values
- Emitted events
- Stored data mutations

## Data Definitions
- Core schema fragments
- Field-level constraints where important
- Ownership notes

## Invariants
- Conditions that must always hold

## Error Behavior
- Expected error categories
- Retry or failure rules

## Special Constraints
- Performance
- Reliability
- Security
- Compliance

## Required Tests
- Contract tests
- Integration tests
- Edge cases

## Acceptance Criteria
- Observable conditions for completion

## Escalate If
- Conditions that require returning to design
```

## What To Keep Explicit

Always keep these explicit when they matter:

- public interface shape
- shared schema shape
- ownership of state
- invariants
- non-obvious failure behavior
- required verification

## What To Leave Implicit

Usually leave these to the implementation agent:

- helper functions
- internal naming below the public boundary
- ordinary framework boilerplate
- standard local refactors
- routine algorithmic choices unless correctness depends on them

## Design Rule

Design the seam precisely.
Leave the interior loose unless the interior contains a high-risk constraint.
