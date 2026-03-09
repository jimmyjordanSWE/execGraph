# Runtime Linux Conformance

## Purpose

This document defines what a Linux runtime adapter must do to conform to `runtime-api`.

## Required Capabilities

- spawn one-shot processes
- supervise long-lived services
- capture stdout and stderr
- materialize declared inputs and outputs
- emit normalized lifecycle events
- enforce graceful stop then force kill

## Request Handling

For each execution request the adapter must:

1. validate declared bindings and sandbox policy
2. materialize required input artifacts
3. prepare working directory and environment
4. start the process or service
5. capture outputs and lifecycle transitions
6. publish terminal result or service status

## Service Controls

Supported controls:

- `start`
- `stop`
- `kill`
- `restart`
- `probe`

Conformance rules:

- `stop` must attempt graceful shutdown within timeout
- `kill` must force terminate the owned process or process group
- `restart` must emit a fresh lifecycle transition sequence
- `probe` must update health state without mutating graph state

## Output Handling

- stdout and stderr may be streamed as events
- bounded capture must still preserve terminal diagnostics
- declared file and directory outputs must publish through `artifact-store`
- partial outputs may remain available on failure with explicit failure state

## Sandbox Expectations

At minimum the adapter must enforce:

- working directory confinement
- normalized file paths
- environment variable allowlists
- process-group ownership
- timeout and resource limits

Optional OCI container support may extend isolation but must not change public contract semantics.

## Failure Semantics

The adapter must surface explicit terminal causes for:

- spawn failure
- timeout
- non-zero exit
- health-check failure
- forced kill
- output publication failure

## Evidence

A conforming adapter is not done until it passes:

- one-shot task smoke path
- long-lived service smoke path
- forced kill path
- artifact publication path
- sandbox/path confinement checks
