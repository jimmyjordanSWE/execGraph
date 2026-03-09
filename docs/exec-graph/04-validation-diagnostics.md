# Validation Rules And Diagnostics

## Purpose

This document expands the validation subsystem behavior for exec-graph graphs.

## Validation Passes

Validation should run in this order:

1. structural graph checks
2. node-definition resolution checks
3. port and type compatibility checks
4. runtime capability and lifecycle checks
5. graph-level policy checks

## Structural Rules

Required checks:

- all referenced nodes and edges exist
- ports exist on both sides of every edge
- no duplicate stable ids
- no illegal self-links unless explicitly supported
- acyclic task-only segments unless a cycle-capable construct is defined

## Type Rules

Required checks:

- source and target types normalize successfully
- compatibility class is not `invalid`
- required target fields are not dropped
- stream/non-stream mismatches are rejected

## Runtime Rules

Required checks:

- node definition exists for every runnable node
- requested capabilities are declared
- lifecycle mode is compatible with scheduler usage
- required sandbox policy can be satisfied by selected runtime

## Diagnostic Shape

Each diagnostic should include:

- `code`
- `severity`
- `message`
- `owner`
- `node_id`
- `edge_id` when applicable
- `path` or field reference when applicable

## Minimum Diagnostic Catalog

Structural:

- `graph.missing_node`
- `graph.missing_port`
- `graph.duplicate_id`
- `graph.invalid_cycle`

Type:

- `type.parse_error`
- `type.incompatible`
- `type.required_field_loss`
- `type.invalid_stream_conversion`

Runtime:

- `runtime.definition_missing`
- `runtime.capability_mismatch`
- `runtime.sandbox_unsatisfied`
- `runtime.lifecycle_conflict`

## Rules

- Diagnostics must be deterministic for the same input graph and definition set.
- Validation must not execute user code.
- Validation reports should be complete when possible, not fail-fast on the first user error.
