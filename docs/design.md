# Exec Graph Design Specification

## 1. Purpose

This project defines a headless platform for building, executing, and observing node graphs whose nodes wrap runnable programs, scripts, and services. The platform is backend-first and UI-agnostic. Display, interaction, and rendering are separate concerns.

The system is intended to support:
- one-shot process execution
- long-lived services
- typed graph validation over untyped process boundaries
- file, stream, and structured data exchange
- observability, reproducibility, and controlled sandboxing

This document defines contracts and public API surfaces for the exec-graph platform. It does not define implementation details.

The separate autoSWE documents under `docs/autoswe/README.md` are exploratory tooling and are not part of this specification.

## 2. Design Principles

- The graph engine owns graph semantics, validation, scheduling contracts, and run state.
- Wrapped tools are exposed through a formal harness contract.
- Canonical graph types are stable and implementation-independent.
- Runtime transport is untyped; port contracts define meaning and validation.
- Frontends interact through commands, queries, and event streams.
- Module boundaries are narrow and testable.
- Persistent state and ephemeral execution are modeled explicitly.

## 3. System Model

The platform consists of four conceptual layers:

1. Harness SDK
- Defines how a runnable tool declares inputs, outputs, artifacts, capabilities, and lifecycle behavior.

2. Graph Engine
- Defines nodes, ports, edges, graphs, validation, transactions, and run orchestration contracts.

3. Runtime Adapters
- Execute tools in supported environments such as local Linux processes or containers.

4. Clients
- UI frontends, automation systems, and other services that operate the graph engine through APIs.

## 4. Module Boundaries

The system is divided into the following modules.

### 4.1 canon-types

Responsibilities:
- define canonical type algebra
- define type compatibility
- define coercion categories
- define validation result format

Public concerns:
- type identifiers
- type expressions
- coercion policies
- validation diagnostics

### 4.2 graph-core

Responsibilities:
- define graph, node, edge, port, subgraph, and metadata models
- define immutable identities and revisioning
- define transaction boundaries

Public concerns:
- graph snapshot model
- command model
- patch model
- version model

### 4.3 node-defs

Responsibilities:
- define node kinds and harness contracts
- define input/output schemas
- define execution bindings and capability declarations
- define node definition versioning

Public concerns:
- node definition schema
- port declaration schema
- runtime binding schema
- lifecycle declaration schema

### 4.4 graph-validate

Responsibilities:
- structural graph validation
- type compatibility validation
- node definition validation
- runtime contract validation

Public concerns:
- validation report schema
- diagnostic levels
- validation entry points

### 4.5 runtime-api

Responsibilities:
- define the abstract execution contract for tasks, services, and sources
- define lifecycle operations
- define run record schema

Public concerns:
- execution request schema
- execution result schema
- lifecycle control operations
- event schema

### 4.6 runtime-linux

Responsibilities:
- implement runtime-api for Linux processes and containers

Public concerns:
- conformance to runtime-api

### 4.7 artifact-store

Responsibilities:
- define artifact identities and metadata
- define artifact materialization and lookup contracts
- define content addressing and retention contracts

Public concerns:
- artifact descriptor schema
- artifact reference schema
- integrity metadata

### 4.8 scheduler

Responsibilities:
- define run planning
- dependency ordering
- persistent node reuse policy
- cache and recomputation contracts

Public concerns:
- scheduling policy schema
- run plan schema
- dependency state model

### 4.9 api-server

Responsibilities:
- expose command, query, and event APIs
- manage sessions, runs, and subscriptions

Public concerns:
- transport-agnostic service contract
- client subscription model

## 5. Canonical Type Model

Canonical types describe graph-level meaning, not raw transport representation.

### 5.1 Primitive Types

- null
- bool
- int
- float
- string
- bytes

### 5.2 Structured Types

- list<T>
- map<K,V>
- record
- enum
- union
- optional<T>

### 5.3 Operational Types

- json
- path
- stream<T>
- artifact
- directory
- table

### 5.4 Type Modes

The platform supports two type modes:

- portable
  - values are interpreted only through canonical types

- native
  - node definitions may expose host-specific or runtime-specific representations in addition to canonical types

Portable mode is the default interoperability mode.

### 5.5 Coercion Policy

Connections may be:
- exact
- implicitly coercible
- explicitly adaptable
- invalid

The type system defines only the contract shape for coercion policy. It does not require a specific implementation strategy.

### 5.6 Canonical Type Grammar

Canonical types use a normalized UTF-8 string form.

Grammar:

- `type := primitive | list<type> | map<key_type,type> | record{field[,field...]} | enum{symbol[,symbol...]} | union<type[,type...]> | optional<type> | stream<type>`
- `primitive := null | bool | int | float | string | bytes | json | path | artifact | directory | table`
- `key_type := string | int | bool | path`
- `field := name:type`
- `name := [a-zA-Z_][a-zA-Z0-9_]*`
- `symbol := [A-Z][A-Z0-9_]*`

Normalization rules:

- whitespace is not significant
- `optional<T>` is the canonical nullable form and is equivalent to `union<null,T>`
- nested unions are flattened and duplicate members are removed
- record compatibility is based on field names and field types, not declaration order
- enum values are case-sensitive and preserve declaration order

Examples:

- `string`
- `list<record{name:string,size:int}>`
- `map<string,optional<json>>`
- `stream<bytes>`

### 5.7 Exact Coercion Matrix

The platform uses four coercion classes.

Exact:

- same normalized type expression
- `optional<T>` to `optional<T>`
- records with the same required field set and exact field types
- enums with the same ordered symbol set

Implicitly coercible:

- `int -> float`
- `T -> optional<T>`
- `null -> optional<T>`

Explicitly adaptable:

- `float -> int`
- `string -> bool | int | float | json | path`
- `bool | int | float | path -> string`
- `string <-> bytes` with declared text encoding
- `path <-> artifact`
- `table <-> json`

Invalid:

- scalar or structured values to `stream<T>` or from `stream<T>` unless the source and target are both `stream` with exact inner type
- `list<T> <-> map<K,V>`
- record to enum, enum to record, or any unrelated structured-type conversion
- `optional<T> -> T` without an explicit presence check
- any conversion that changes cardinality or drops required record fields

## 6. Transport and Binding Model

Node ports declare semantic meaning. Runtime bindings declare where that meaning is obtained from or written to.

Supported binding targets:
- command arguments
- standard input
- standard output
- standard error
- environment variables
- declared file paths
- declared directories
- runtime request channels
- runtime response channels

Supported transport classes:
- value
- stream
- artifact
- control

Each port declaration must define:
- canonical type
- transport class
- cardinality
- required or optional status
- validation policy
- binding target

## 7. Node Definition Contract

A node definition is a versioned harness contract for a reusable tool kind.

Each node definition must declare:
- stable kind identifier
- version
- display metadata
- input ports
- output ports
- configuration schema
- execution mode
- lifecycle mode
- capability requirements
- sandbox requirements
- observability policy

Optional declarations may include:
- health checks
- reset behavior
- migration rules
- caching hints
- adapter hints

### 7.1 Node Definition Serialization Format

Node definitions are serialized as UTF-8 JSON documents.

Required top-level fields:

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

Optional top-level fields:

- `health_checks`
- `reset`
- `migration`
- `caching`
- `adapter_hints`

Port entries must contain:

- `name`
- `type`
- `transport`
- `cardinality`
- `required`
- `validation`
- `binding`

`runtime` must contain:

- `execution_mode`
- `lifecycle_mode`
- `role`
- `entry`

Versioning rules:

- `kind` is stable across compatible revisions
- `version` is semantic versioning
- major version changes may break port, configuration, or lifecycle compatibility
- minor version changes may add optional fields and capabilities
- patch version changes may clarify metadata without changing contract shape

## 8. Execution and Lifecycle Model

Execution trigger behavior and lifecycle behavior are independent.

### 8.1 Trigger Modes

- demand
- event
- continuous
- manual
- scheduled

### 8.2 Lifecycle Modes

- ephemeral
- session
- persistent

### 8.3 Runtime Roles

- task
  - one-shot execution with defined completion

- service
  - long-lived process with callable or stateful behavior

- source
  - long-lived producer of events or data

- sink
  - receiver of externalized outputs

### 8.4 Run Policy

Run requests may specify policy such as:
- reuse warm services
- restart persistent services
- reset stateful nodes
- start missing dependencies
- require clean execution

These policies are runtime controls, not graph structure.

### 8.5 Service Health Contract

Services expose both lifecycle state and health state.

Lifecycle states:

- `starting`
- `running`
- `stopping`
- `stopped`
- `failed`

Health states:

- `healthy`
- `degraded`
- `unhealthy`
- `unknown`

Every service status record must include:

- `service_id`
- `run_id`
- `lifecycle_state`
- `health_state`
- `started_at`
- `last_health_at`
- `restart_count`
- `last_exit`
- `details`

Runtime control requirements:

- `stop` attempts graceful shutdown within a caller-supplied timeout
- `kill` force-terminates the owned process or process group if graceful shutdown does not complete
- `restart` is equivalent to `stop` followed by `start` with a new `run_id`
- `probe` returns the current service status record without changing lifecycle state

### 8.6 Cache Key Derivation Rules

Scheduler cache keys are hashes over normalized execution inputs.

Cache key inputs:

- node definition `kind` and `version`
- normalized node configuration
- canonicalized bound input values
- referenced artifact content hashes
- execution-affecting runtime policy
- declared capability set
- adapter identity

Cache key exclusions:

- timestamps
- session identifiers
- log verbosity
- retry counters
- UI-only metadata

Persistent or session lifecycle services are not cache hits. Only completed deterministic task executions may populate or satisfy cache entries.

## 9. Artifact Contract

Artifacts are first-class outputs and inputs.

Each artifact descriptor must support:
- stable identifier
- storage locator
- content hash
- size metadata
- media or format metadata
- producing node reference
- producing run reference
- creation timestamp

The artifact contract does not mandate a storage backend.

### 9.1 Artifact Retention Policy

Every artifact declares a retention class:

- `ephemeral`
  - may be garbage-collected after the producing run is complete and no live consumer depends on it
- `run`
  - retained with the run record until that run is pruned by policy
- `pinned`
  - retained until explicitly unpinned or deleted by an authorized command

Required retention metadata:

- `retention_class`
- `created_at`
- `expires_at` or `null`
- `pinned_by` or `null`

Default behavior:

- produced artifacts default to `run`
- node definitions may explicitly mark outputs as `ephemeral`
- user or system commands may promote an artifact from `run` to `pinned`

## 10. Observability Contract

Every execution unit should produce structured run records.

Run records may include:
- execution identity
- node identity
- graph identity
- command metadata
- timestamps
- status transitions
- exit status
- captured logs
- produced artifacts
- validation results
- resource usage metrics
- cache hit or miss metadata

Observability must be available independently of any specific frontend.

## 11. Public API Model

The platform exposes three API classes:

### 11.1 Commands

State-changing operations such as:
- create graph
- add node
- connect ports
- set configuration
- start node
- stop node
- run graph
- cancel run

### 11.2 Queries

Read operations such as:
- get graph snapshot
- get node definition
- get validation report
- get run record
- get artifact metadata
- get service status

### 11.3 Events

Subscription-driven updates such as:
- graph changed
- validation updated
- node started
- node stopped
- run started
- run completed
- artifact created
- diagnostic emitted

The API contract is transport-agnostic. HTTP, WebSocket, gRPC, or in-process bindings may be provided independently.

### 11.4 Command And Event Schema Encoding

The canonical wire encoding is UTF-8 JSON.

Commands use this envelope:

- `protocol_version`
- `command_id`
- `session_id`
- `graph_id`
- `target`
- `name`
- `expected_revision`
- `idempotency_key`
- `payload`
- `issued_at`

Events use this envelope:

- `protocol_version`
- `event_id`
- `session_id`
- `graph_id`
- `run_id`
- `node_id`
- `name`
- `sequence`
- `at`
- `payload`

Encoding rules:

- timestamps use RFC 3339 UTC strings
- binary payloads are referenced as artifacts rather than inlined
- envelope keys use `snake_case`
- unknown fields must be ignored by readers and preserved by pass-through relays where practical

### 11.5 Session Model And Concurrency Control

Sessions are explicit backend resources.

A session record contains:

- `session_id`
- `client_id`
- `created_at`
- `last_seen_at`
- `expires_at`
- `subscriptions`
- `capabilities`

Concurrency rules:

- graph-mutating commands must include `expected_revision`
- if `expected_revision` does not match current graph revision, the command fails with `revision_conflict`
- run-control commands are idempotent per `idempotency_key`
- events are ordered per graph and run by monotonically increasing `sequence`
- expired sessions may no longer issue commands or hold subscriptions

## 12. Security and Capability Model

Capabilities are explicit.

Examples include:
- filesystem read
- filesystem write
- network access
- environment access
- subprocess execution
- container execution

The contract must support:
- capability declaration by node definition
- capability filtering by runtime policy
- auditable execution metadata

## 13. Extensibility

The system must support:
- new canonical types
- new runtime adapters
- new node definitions
- new observability backends
- new frontends

Extensibility must preserve stable contracts at module boundaries.

## 14. Non-Goals for This Specification

This specification does not define:
- frontend rendering behavior
- layout algorithms
- persistence backend internals
- scheduler implementation details
- process sandbox implementation details
- container orchestration internals

## 15. Resolved Specification Decisions

The previously unresolved contract-level topics are now defined in this document:

- canonical type grammar in `5.6`
- coercion matrix in `5.7`
- node definition serialization in `7.1`
- service health contract and runtime kill semantics in `8.5`
- cache key derivation in `8.6`
- artifact retention policy in `9.1`
- command and event encoding in `11.4`
- session model and concurrency control in `11.5`

## 16. Next Documents

Supporting follow-on specifications now live under [exec-graph/README.md](exec-graph/README.md):
- [exec-graph/01-canonical-type-algebra.md](exec-graph/01-canonical-type-algebra.md)
- [exec-graph/02-harness-sdk-schema.md](exec-graph/02-harness-sdk-schema.md)
- [exec-graph/03-runtime-linux-conformance.md](exec-graph/03-runtime-linux-conformance.md)
- [exec-graph/04-validation-diagnostics.md](exec-graph/04-validation-diagnostics.md)
- [exec-graph/05-artifact-storage-model.md](exec-graph/05-artifact-storage-model.md)
- [exec-graph/06-implementation-discovery.md](exec-graph/06-implementation-discovery.md)
- [exec-graph/07-implementation-design.md](exec-graph/07-implementation-design.md)
