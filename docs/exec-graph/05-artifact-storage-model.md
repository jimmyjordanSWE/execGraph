# Artifact And Storage Reference Model

## Purpose

This document expands the artifact-store responsibilities described in [design.md](../design.md).

## Storage Split

V1 storage is split into:

- SQLite metadata store for artifact descriptors and retention metadata
- local content-addressed filesystem for bytes and directories

## Descriptor Model

Each published artifact descriptor should contain:

- stable artifact id
- content hash
- artifact kind
- retention class
- size metadata
- creation timestamp
- producer run id
- materialization hints

## Retention Classes

- `ephemeral`
- `run`
- `pinned`

Rules:

- `run` is the default class
- only pinned artifacts are retained indefinitely
- cleanup must never delete content still referenced by a pinned descriptor

## Write Path

1. accept produced bytes or directory tree
2. normalize paths and metadata
3. compute content hash
4. write content to managed storage
5. publish descriptor transactionally

If any step fails, no descriptor is published.

## Materialization

Consumers should resolve artifacts by descriptor, not by raw path.

Materialization must:

- stay inside managed directories
- verify referenced content exists
- preserve read-only semantics unless explicitly copied out

## Security

- never trust producer-provided filenames directly
- normalize and confine all paths
- treat artifact bytes as untrusted payload
- treat descriptor metadata as trusted system state only after successful publication

## Reference Implementation Notes

- enable content dedupe by hash
- record retention operations in metadata
- keep descriptor APIs separate from graph-core and runtime-linux internals
