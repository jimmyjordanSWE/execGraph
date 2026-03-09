# Implementation Verification Execution Workflow

## Purpose

Define the mandatory reasoning, minimal artifacts, and gates for implementation verification execution.

This is the phase that runs the broader verification matrix against implemented packets and determines whether the current build is stable enough to continue, release, or harden.

## Global Rules

- Do not treat “tests passed once” as sufficient verification evidence for a meaningful implementation step.
- Do not reduce verification to unit tests alone.
- Do not run benchmarks only when something feels slow; performance-sensitive subsystems require explicit evidence over time.
- Do not separate sanitizer, static-analysis, memory-check, and benchmark evidence from the implementation story for native or low-level systems.
- Prefer a layered verification matrix with clear failure ownership over one giant opaque check command.

## Inputs

Implementation verification execution consumes:

- approved verification design
- implemented packets and packet acceptance results
- current build/test/benchmark/sanitizer command set
- current fixtures, smoke paths, and benchmark baselines
- current residual-risk register

## 1. Freeze Verification Scope

Mandatory reasoning:

- what implementation packets and subsystems are in scope
- what verification obligations must be exercised now
- what is intentionally out of scope for this run

Minimal artifact:

- verification scope

Gate:

- the team knows what is being verified and why

## 2. Assemble The Verification Matrix

Mandatory reasoning:

- what unit, contract, integration, smoke, reliability, security, sanitizer, memory-tool, static-analysis, and benchmark checks apply
- what order they should run in
- which checks are fast feedback versus slower confidence gates

Minimal artifact:

- verification matrix

Gate:

- the verification run is structured rather than ad hoc

## 3. Execute Functional And Behavioral Checks

Mandatory reasoning:

- whether the functional behavior matches the approved contracts and flows
- whether smoke paths prove real end-to-end behavior
- whether failures are local, integration, or boundary failures

Minimal artifact:

- functional verification results

Gate:

- implemented behavior is proven against more than local happy paths

## 4. Execute Safety And Correctness Checks

Mandatory reasoning:

- what static analysis should run
- what sanitizer or memory tools should run
- what concurrency or shutdown-sensitive checks should run
- what constitutes a blocker

Minimal artifact:

- safety verification results

Gate:

- low-level correctness risks have explicit evidence, not just optimism

## 5. Execute Performance Verification

Mandatory reasoning:

- which benchmarks matter now
- what baseline or comparison should be used
- what counts as regression, improvement, or noise
- whether benchmark instability invalidates conclusions

Minimal artifact:

- benchmark results
- regression comparison

Gate:

- performance-sensitive work has measurable evidence rather than subjective claims

## 6. Triage Failures And Regressions

Mandatory reasoning:

- which failures are genuine defects
- which are flaky or invalid tests
- which regressions are acceptable for now
- which must block forward progress
- whether the right action is fix, defer, or escalate

Minimal artifact:

- failure triage
- regression triage

Gate:

- failed verification produces an explicit decision, not confusion

## 7. Reconcile Against Requirements And Release Gates

Mandatory reasoning:

- whether current evidence satisfies the approved verification obligations
- whether current residual risks are acceptable
- whether the build is ready for the next milestone, release preparation, or more hardening

Minimal artifact:

- verification verdict
- release-gate status

Gate:

- the team has a clear yes or no on readiness

## 8. Record Evidence And Feed Back Into The Loop

Mandatory reasoning:

- what evidence should be stored durably
- what follow-on packet or hardening work is now highest priority
- whether the workflow should return to implementation execution, move to release preparation, or escalate

Minimal artifact:

- verification evidence record
- next-step decision

Gate:

- verification changes the project state durably and drives the next action

## Required Outputs

Implementation verification execution must produce:

- verification scope
- verification matrix
- functional verification results
- safety verification results
- benchmark results
- regression comparison
- failure triage
- regression triage
- verification verdict
- release-gate status
- verification evidence record
- next-step decision

## Completion Standard

Implementation verification execution is complete only when:

- the in-scope verification matrix has been run or explicitly waived
- functional, safety, and performance evidence are all accounted for where relevant
- regressions and failures have explicit disposition
- the current build has a clear readiness verdict
- the next workflow action is explicit
