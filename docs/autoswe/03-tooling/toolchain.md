# Toolchain Boundary

## Purpose

This document defines what belongs in the external toolchain and what belongs in the skill system.

The boundary matters. The skill should encode expertise. The toolchain should encode state and enforcement.

## Skill System Responsibilities

The skill system should own:

- design methodology
- expert heuristics
- stage sequencing rules
- artifact requirements
- review criteria
- escalation rules
- criteria for when to suggest or request tools

## External Toolchain Responsibilities

The external toolchain should own:

- project state storage
- dependency tracking
- progress tracking
- traceability
- search and indexing
- graph or tree visualization if used
- schema storage
- history
- project-specific queries

## Tool Suggestion Policy

The orchestrator may suggest tools for:

- code indexing
- schema diffing
- contract validation
- dependency analysis
- architectural visualization
- progress tracking

The orchestrator should request tool support when:

- scale makes manual inspection weak
- correctness depends on structural analysis
- drift detection is required
- dependency management is too complex for prose tracking

## Suggested Minimum Tool Stack

If a custom toolchain is built, the first useful capabilities are:

1. Code structure index
2. Contract registry
3. Traceability store
4. Progress and dependency view
5. Verification hooks

These capabilities are enough to support disciplined large-system design without turning the skill itself into a project database.

## Fallback Rule

If a desired tool is unavailable:

- continue with lightweight artifacts
- state the limitation
- identify the risk introduced by the missing tool
- keep the design outputs tool-agnostic where possible
