# Constraint Package Workflow

## Purpose

Define the leaf-module handoff packet that accompanies implementation-handoff artifacts.

The constraint package is for the implementation agent, not for humans writing long design prose.

## Required Fields

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

## Rule

Design the seam precisely.
Leave the interior loose unless the interior contains a high-risk constraint.

## Keep Explicit

- public interface shape
- shared schema shape
- ownership of state
- invariants
- non-obvious failure behavior
- required verification

## Leave Implicit

- helper functions
- internal naming below the public boundary
- ordinary framework boilerplate
- standard local refactors
- routine algorithmic choices unless correctness depends on them
