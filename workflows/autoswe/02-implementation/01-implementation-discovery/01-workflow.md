# Implementation Discovery Workflow

## Purpose

Define the mandatory reasoning, minimal artifacts, and gates for implementation discovery.

The model must perform the reasoning.
The model should emit only the minimum artifact needed to constrain downstream implementation design.

Implementation discovery is the first step of the implementation phase.
It maps approved design into a concrete implementation environment without silently redesigning the system.

## Global Rules

- Do not silently change approved scope, boundaries, contracts, invariants, security assumptions, operational constraints, or verification obligations.
- Do not select technology just because it is familiar, trendy, or fast to start.
- Do not treat buy-vs-build, toolchain, or library choice as “obvious” when the decision materially affects delivery risk or long-term maintainability.
- Do not hide redesign pressure inside implementation discovery. Escalate it.
- Prefer a small number of explicit, auditable decisions over a large number of implied defaults.
- Assume an LLM-driven engineering team by default unless the project explicitly states a different team model.

## Required Decision Dimensions

Every major implementation discovery decision must be evaluated against at least:

- design fit
- correctness risk
- performance feasibility
- operational fit
- security and supply-chain risk
- verification feasibility
- delivery speed
- team capability and maintainability
- economic and licensing impact
- release-engineering fit
- interoperability and compatibility fit
- migration cost
- long-term changeability

## Inputs

Implementation discovery consumes:

- approved implementation-handoff artifacts
- approved contracts and schemas
- approved invariants
- approved security and operational constraints
- approved verification obligations
- leaf constraint packages
- actual repository structure and existing code, if any
- actual team and environment constraints, if known

If no explicit team model is provided, assume:

- implementation is primarily LLM-driven
- humans provide approval, escalation answers, and high-leverage review
- technology choices should favor agent readability, agent debuggability, and agent-operable workflows in addition to ordinary human maintainability

## 1. Freeze The Design Boundary

Mandatory reasoning:

- what is already fixed by design
- what is intentionally left for implementation discovery
- what ambiguities are real gaps versus harmless local freedom
- what would require escalation if changed

Minimal artifact:

- non-negotiables list
- escalation list for any design ambiguity still hiding in the handoff

Gate:

- the team knows what implementation discovery is allowed to decide and what remains design-owned

## 2. Inspect The Target Reality

Mandatory reasoning:

- current repository structure
- existing code and languages
- deployment environment
- build and CI environment
- runtime platform constraints
- organizational constraints
- team skill reality
- third-party integration constraints

Minimal artifact:

- environment and repo inventory
- inherited constraints list

Gate:

- discovery is grounded in the real target environment rather than an abstract greenfield assumption

## 3. Define The Discovery Questions

Mandatory reasoning:

- what concrete decisions must be made before implementation design can begin
- which decisions are architectural enough to escalate
- which decisions can remain local
- which decisions depend on each other

Typical questions:

- what language should the control plane use
- what persistence technology should back metadata
- what transport should be used
- what testing stack should be used
- what frameworks and libraries should be adopted
- what should be built in-house

Minimal artifact:

- discovery question set
- dependency map between discovery questions

Gate:

- the discovery scope is explicit and not hidden in ad hoc local choices

Output convention:

- implementation-discovery follow-on docs should be emitted into an ordered project docs folder
- sibling docs should use explicit numeric prefixes when they form a sequence
- the project docs index should be kept ordered rather than becoming a flat unnumbered list

## 4. Generate Candidate Options

Mandatory reasoning:

- what the credible candidate set is for each discovery question
- which options are serious enough to compare
- which options are non-starters and why
- which options are only variants of the same underlying choice

For each option, reason about:

- what it is
- what it replaces or avoids
- maturity level
- ecosystem strengths
- ecosystem weaknesses
- compatibility with the approved design

Minimal artifact:

- candidate option set per discovery question
- eliminated-options note where obvious non-starters exist

Gate:

- there is a real comparison set instead of a default-first choice

## 5. Evaluate Design Fit

Mandatory reasoning:

- whether each option respects the approved boundary model
- whether it supports the contract shape cleanly
- whether it preserves invariants naturally
- whether it introduces accidental coupling
- whether it forces awkward representation conversions
- whether it creates pressure to redesign module seams

Minimal artifact:

- design-fit assessment per option

Gate:

- any option that fundamentally fights the approved design is eliminated or escalated

## 6. Evaluate Delivery And Team Fit

Mandatory reasoning:

- human team competence
- LLM fit for the candidate ecosystem
- onboarding cost
- debugging ergonomics
- implementation speed
- local development speed
- hiring and maintainability risk
- long-term bus factor

When the default LLM-driven model applies, also reason about:

- codebase readability for agents
- ease of automated editing and refactoring
- quality of compiler, linter, and test feedback for agent loops
- ecosystem stability for tool-assisted implementation
- availability of mature libraries that reduce bespoke code generation burden

Minimal artifact:

- delivery-fit assessment
- team-risk notes

Gate:

- the selected direction is realistic for the actual team and timeframe, not just theoretically elegant

## 7. Evaluate Operational And Security Fit

Mandatory reasoning:

- runtime isolation and permission control
- supply-chain and dependency risk
- operational observability
- deployment and rollback safety
- migration safety
- backup and recovery consequences
- failure-mode behavior

Minimal artifact:

- operational-risk assessment
- security-risk assessment

Gate:

- the option set has been tested against real operational and security consequences, not just developer convenience

## 8. Evaluate Verification Fit

Mandatory reasoning:

- contract testing feasibility
- integration testing feasibility
- end-to-end smoke path feasibility
- reproducible local verification
- CI feasibility
- debugging failed-test evidence quality

Minimal artifact:

- verification-feasibility assessment

Gate:

- the implementation stack can actually prove the system, not merely build it

## 9. Evaluate Economic And Licensing Fit

Mandatory reasoning:

- infrastructure and hosting cost implications
- licensing constraints
- commercial dependency risk
- long-term total cost of ownership
- cost of switching away later

Minimal artifact:

- economic and licensing assessment

Gate:

- the candidate set is economically and legally viable, not just technically attractive

## 10. Evaluate Compatibility And Interoperability Fit

Mandatory reasoning:

- external system compatibility
- protocol and file-format compatibility
- browser, OS, runtime, or platform compatibility where relevant
- portability across target environments
- constraints created by existing repo or deployment infrastructure

Minimal artifact:

- compatibility and interoperability assessment

Gate:

- the candidate set can actually interoperate with the environments and systems it must live in

## 11. Evaluate Release-Engineering Fit

Mandatory reasoning:

- packaging and artifact publishing model
- versioning strategy
- CI/CD fit
- rollback mechanics
- reproducible build expectations
- release evidence generation

Minimal artifact:

- release-engineering assessment

Gate:

- the candidate implementation stack can be built, packaged, shipped, and rolled back safely

## 12. Evaluate Ownership And Maintainability Fit

Mandatory reasoning:

- module ownership expectations
- dependency ownership expectations
- operational ownership expectations
- support burden
- long-term maintainability and replacement difficulty

Minimal artifact:

- ownership and maintainability assessment

Gate:

- the chosen direction has a credible long-term ownership model rather than only a short-term implementation plan

## 13. Make Buy-Vs-Build Decisions

Mandatory reasoning:

- what should be built
- what should be adopted
- what should be wrapped
- what should be postponed
- what lock-in or ownership cost each choice creates

For each major capability, reason about:

- why it is being bought or built
- lock-in risk
- replacement difficulty
- integration surface
- long-term ownership burden

Minimal artifact:

- buy-vs-build register

Gate:

- major dependencies and ownership burdens are explicit

## 14. Define Evidence Policy And Required Spikes

Mandatory reasoning:

- what decisions can be made from paper analysis
- what decisions require empirical evidence
- which uncertainties are too expensive to leave unresolved
- which decisions cannot be settled by paper analysis alone
- which experiments are necessary
- what each spike must prove or falsify

Evidence policy should answer:

- when a spike is mandatory
- what counts as sufficient evidence
- what level of benchmark, prototype, or integration proof is required
- which choices are reversible enough to proceed without a spike

Minimal artifact:

- evidence policy
- spike plan
- spike success criteria

Gate:

- unresolved risk is handled intentionally rather than hand-waved

## 15. Score And Select Options

Mandatory reasoning:

- how the candidate options compare across the required decision dimensions
- what weighting or priority logic is being used
- which tradeoffs are dominating the final choice
- whether any “winner” still loses on a non-negotiable dimension

Minimal artifact:

- decision scoring summary
- selected option set

Gate:

- the final choice is explainable and not just the result of intuition or the loudest local preference

## 16. Synthesize The Implementation Profile

Mandatory reasoning:

- which option set forms the most coherent overall implementation direction
- which choices are primary versus secondary
- which decisions are deferred
- whether the combined profile still fits the approved design

The implementation profile should cover:

- primary language and runtime
- key frameworks and libraries
- metadata and persistence choices
- build, test, lint, and type-check toolchain
- packaging approach
- repo-local structure assumptions
- major external dependencies
- buy-vs-build posture
- deferred decisions

Minimal artifact:

- implementation profile

Gate:

- the implementation ecosystem is explicit enough to support detailed implementation design

## 17. Record Tradeoffs And Escalations

Mandatory reasoning:

- why the chosen options won
- what was rejected and why
- what downside is being accepted
- what risks remain
- what follow-up work is required
- what must escalate back to design

Minimal artifact:

- technology selection record
- residual-risk register
- escalation list

Gate:

- the chosen path is auditable and the unresolved risks are visible

## Required Outputs

Implementation discovery must produce:

- non-negotiables list
- environment and repo inventory
- inherited constraints list
- discovery question set
- candidate option comparison
- design-fit assessment
- delivery/team-fit assessment
- operational and security assessment
- verification-feasibility assessment
- economic and licensing assessment
- compatibility and interoperability assessment
- release-engineering assessment
- ownership and maintainability assessment
- buy-vs-build register
- evidence policy
- spike plan where needed
- decision scoring summary
- selected option set
- implementation profile
- technology selection record
- residual-risk register
- escalation list

## Completion Standard

Implementation discovery is complete only when:

- the target implementation environment is explicit
- the technology choices are justified against the design constraints
- the implementation team can begin detailed implementation design without guessing the ecosystem
- major buy-vs-build decisions are recorded
- licensing, cost, compatibility, and release implications are visible
- the evidence threshold for risky decisions is explicit
- important unresolved questions are either reduced by spikes or explicitly escalated
- no redesign pressure is being hidden inside “tool choice”
