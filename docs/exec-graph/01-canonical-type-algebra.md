# Canonical Type Algebra

## Purpose

This document expands the canonical type model in [design.md](../design.md).

## Core Forms

Primitive forms:

- `null`
- `bool`
- `int`
- `float`
- `string`
- `bytes`
- `json`
- `path`
- `artifact`
- `directory`
- `table`

Constructed forms:

- `list<T>`
- `map<K,V>`
- `record{field:type,...}`
- `enum{SYMBOL,...}`
- `union<T,...>`
- `optional<T>`
- `stream<T>`

## Normalization Rules

- Ignore insignificant whitespace.
- Rewrite `union<null,T>` to `optional<T>`.
- Flatten nested unions and remove duplicate members.
- Preserve enum symbol order.
- Compare records by field set and field type, not source order.
- Normalize equivalent types to one canonical string form before compatibility checks.

## Internal Model

A reference implementation should use a typed AST with:

- `kind`
- `children`
- `fields`
- `symbols`
- `source_text`

Compatibility logic should operate on normalized AST values, not raw input strings.

## Compatibility Classes

- `exact`
- `implicit`
- `explicit`
- `invalid`

The compatibility engine must return:

- source normalized type
- target normalized type
- compatibility class
- diagnostics when non-exact

## Required Diagnostics

At minimum:

- parse error
- unsupported key type
- duplicate record field
- duplicate union member
- invalid stream conversion
- required field loss
- unsupported coercion

## Implementation Rules

- Parsing must be deterministic and side-effect free.
- Invalid input must never produce partial normalized output.
- Cache parsed normalized forms by raw input string.
- Keep the algebra implementation independent from runtime and storage code.
