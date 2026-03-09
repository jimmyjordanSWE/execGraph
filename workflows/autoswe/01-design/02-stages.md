# Software Design Stage Workflow

## Purpose

Define the mandatory reasoning, minimal artifact, and gate for each design stage.

The model must perform the reasoning.
The model should emit only the minimum artifact needed downstream.

## 1. Problem Definition

Mandatory reasoning:

- users and actors
- goals
- constraints
- risks
- non-goals
- success and failure conditions

Minimal artifact:

- short problem statement
- constraints list
- non-goals
- success criteria

Gate:

- the problem is scoped tightly enough that solution design is not solving the wrong thing

## 2. Requirements Definition

Mandatory reasoning:

- functional requirements
- non-functional requirements
- scope edges
- priority and tradeoff pressure
- compliance and policy constraints

Minimal artifact:

- functional requirements
- non-functional requirements
- explicit scope edges
- priority notes

Gate:

- the system is constrained by explicit requirements rather than inferred intent

## 3. Solution Framing

Mandatory reasoning:

- why software is appropriate
- broad solution class
- major alternatives
- architecture direction
- key tradeoffs
- major assumptions and risks

Minimal artifact:

- chosen solution shape
- rejected alternatives summary
- major risks and assumptions

Gate:

- one clear high-level direction exists and there is no unresolved fundamental approach split

## 4. Domain And Data Modeling

Mandatory reasoning:

- core entities
- states
- events
- invariants
- terminology
- ownership of truth
- lifecycle rules
- important relationships

Minimal artifact:

- domain glossary
- entity and state list
- invariants
- ownership map

Gate:

- the system has a stable conceptual model of what exists, what changes, and who owns each important fact

## 5. Behavior And Workflow Design

Mandatory reasoning:

- key user and system flows
- control flow and state progression
- asynchronous versus synchronous behavior
- failure paths and recovery paths
- long-running workflow behavior

Minimal artifact:

- key workflow list
- flow notes
- critical path and failure path summary

Gate:

- important system behavior is explicit enough that decomposition and contracts are being designed around real flows rather than static nouns

## 6. System Decomposition

Mandatory reasoning:

- major subsystems
- responsibilities
- boundaries
- dependencies
- trust boundaries
- synchronous versus asynchronous interactions
- boundary ownership
- what should be isolated

Minimal artifact:

- subsystem list
- responsibility map
- boundary notes
- dependency map

Gate:

- each major responsibility has a home and the system can be divided into implementable parts without boundary confusion

## 7. Contract Design

Mandatory reasoning:

- APIs
- events and messages
- schemas
- data representations
- legal state transitions
- error behavior
- compatibility and versioning
- idempotency
- retries and timeouts where relevant
- cross-boundary invariants

Minimal artifact:

- contract specifications
- schemas
- error model
- versioning rules

Gate:

- cross-module interactions are precise enough that independent agents can build against them without guessing

## 8. Internal Subsystem Design

Mandatory reasoning:

- internal module structure
- state handling
- persistence
- workflows
- concurrency
- caching
- observability
- security
- local failure handling

Minimal artifact:

- per-subsystem internal design note covering structure, state, persistence, and critical local rules

Gate:

- each subsystem is internally clear enough for implementation without redesigning its public boundary

## 9. Security And Trust-Boundary Design

Mandatory reasoning:

- trust boundaries
- permissions and authority model
- secret handling
- security invariants
- isolation expectations
- abuse and misuse paths
- auditability requirements

Minimal artifact:

- security boundary map
- permission model
- security invariants
- audit and isolation notes

Gate:

- security-critical assumptions and trust boundaries are explicit before implementation begins

## 10. Operational Design

Mandatory reasoning:

- deployment shape
- migration strategy
- rollout strategy
- rollback and recovery strategy
- observability requirements
- capacity and failure-operating assumptions
- operational ownership boundaries

Minimal artifact:

- operational design note covering deployment shape, migration and rollback strategy, observability, and recovery expectations

Gate:

- the system is operationally designable without leaving migration, rollout, and recovery to ad hoc implementation guesswork

## 11. Verification Design

Mandatory reasoning:

- what must be tested or proven
- contract compliance
- integration risks
- invariants
- failure scenarios
- performance checks
- reliability checks
- security checks
- review gates

Minimal artifact:

- verification plan
- acceptance criteria
- required tests and checks

Gate:

- there is a clear method for determining whether implementation conforms to design

## 12. Implementation Handoff

Mandatory reasoning:

- task slicing
- dependency order
- parallelizable work
- required context per sub-agent
- blocked areas
- escalation rules for ambiguity
- what implementation discovery must decide
- what is fixed by design versus left to implementation discovery

Minimal artifact:

- implementation packets containing scope, dependencies, constraints, contracts, and acceptance criteria
- implementation-discovery inputs covering decision criteria, hard constraints, and unresolved technology choices that must not be guessed silently

Gate:

- the design phase is complete, implementation can start with bounded tasks, and the implementation side knows exactly what must be discovered, what is already fixed, and when to escalate back to design

Output convention:

- if design emits follow-on product documents, place them in a dedicated project docs folder
- maintain a numbered sequence for sibling follow-on documents
- keep the project README/index ordered to match the sequence
