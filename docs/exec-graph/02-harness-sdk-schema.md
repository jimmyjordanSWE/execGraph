# Harness SDK Definition Schema

## Purpose

This document expands the node-definition contract in [design.md](../design.md).

## Serialization

- UTF-8 JSON
- snake_case field names
- semantic version string in `version`

## Top-Level Shape

Required fields:

- `kind`
- `version`
- `display`
- `configuration_schema`
- `inputs`
- `outputs`
- `runtime`
- `capabilities`
- `sandbox`
- `observability`

Optional fields:

- `health_checks`
- `examples`
- `notes`

## Port Shape

Each port definition must declare:

- `name`
- `type`
- `transport`
- `cardinality`
- `required`
- `validation`
- `binding`

Recommended optional fields:

- `description`
- `default`
- `examples`

## Runtime Block

Required fields:

- `execution_mode`
- `lifecycle_mode`
- `role`
- `entry`

`entry` may reference:

- command and argv
- module and function
- executable path
- container image and command

## Capability Block

Capability declarations should be explicit and least-privilege. Typical fields:

- filesystem access
- network access
- environment access
- process spawning
- container support

## Sandbox Block

The sandbox declaration should constrain:

- working directory policy
- writable paths
- readable paths
- environment allowlist
- timeout defaults
- resource limits

## Observability Block

Observability policy should declare:

- log capture requirements
- stream capture requirements
- artifact publication behavior
- health probe exposure
- metrics and tracing hooks

## Versioning Rules

- New required fields require a version bump.
- Removing or changing public meaning is breaking.
- Runtime adapters and validators must key behavior by `kind` and `version`.
