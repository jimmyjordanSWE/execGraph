# Implementation Boundary

## Purpose

This document defines the boundary between the design phase and the implementation phase.

The design phase ends at `Implementation handoff`.
The implementation phase begins at `Implementation discovery`.

## Design Owns

Design must determine:

- problem and requirements
- domain model
- key behaviors and workflows
- subsystem boundaries
- contracts and representations
- internal subsystem constraints
- security and trust boundaries
- operational design constraints
- verification requirements
- bounded implementation handoff packets

Design should not silently defer these to implementation.

## Implementation Discovery Owns

Implementation discovery determines:

- which language or languages to use
- which frameworks, libraries, SDKs, and tools to use
- what should be built versus reused
- how the approved design maps onto the actual target repository and runtime environment
- what implementation-local constraints exist in the chosen ecosystem

Implementation discovery may refine execution choices.
It must not redesign approved boundaries, contracts, invariants, security assumptions, or operational requirements without escalation back to design.

## Rule

If a technology choice changes:

- boundaries
- contracts
- invariants
- security posture
- operational behavior
- verification obligations

then the choice is not local implementation discovery.
It must escalate back into design.
