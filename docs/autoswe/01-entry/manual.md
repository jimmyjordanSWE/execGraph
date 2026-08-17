# Human + LLM Workflow Manual

## Purpose

This manual explains how a human and a single LLM should use autoSWE together.

It is an operator guide.
It does not redefine the workflow.

Canonical workflow files live under:

- [workflows/autoswe/README.md](../../../workflows/autoswe/README.md)
- [workflows/autoswe/01-design/README.md](../../../workflows/autoswe/01-design/README.md)
- [workflows/autoswe/02-implementation/README.md](../../../workflows/autoswe/02-implementation/README.md)
- [workflows/autoswe/01-design/05-skill-coverage.md](../../../workflows/autoswe/01-design/05-skill-coverage.md)

The source-of-truth tool is:

- [autoSWE CLI](../../../autoswe/cli.py)

## Operating Model

Use this system as:

- one human decision-maker
- one active design LLM
- one SQLite project database

The LLM starts as the orchestrator by default.

The LLM should move up and down the design tree as needed:

- move up when a lower-level task exposes unresolved higher-level ambiguity
- move down when a stage is stable enough to refine
- stop descending when a leaf module can be handed off through implementation-handoff artifacts plus a constraint package

## Division Of Labor

### Human

The human should:

- provide the initial problem description
- answer high-cost or ambiguous tradeoff questions
- approve major scope and architecture decisions
- reject over-engineering or under-specified handoffs
- decide when uncertainty is acceptable

### LLM

The LLM should:

- control the stage flow
- perform the mandatory reasoning for the current stage
- emit only the minimum artifact needed downstream
- store approved state through `designctl`
- resolve cross-boundary ambiguity explicitly instead of guessing
- produce implementation-handoff artifacts and constraint packages for leaf modules

## Core Rule

The LLM must do all important design reasoning.

The LLM must not dump all of that reasoning into documentation.

The LLM should write down only:

- decisions
- constraints
- interfaces
- invariants
- required tests
- handoff artifacts

## Project Start

User-facing entry point:

- use `python3 -m autoswe resume`
- do not make the human pick sub-workflows or low-level `designctl` commands
- if the client supports slash-command aliases, `/design` should call `python3 -m autoswe resume`
- in chat, treat `design` or `/design` as the instruction to resume workflow state from the database

At the beginning of a project:

1. Create the project database.
2. Record the top-level project goal.
3. Create the first design node for `problem_definition`.
4. Begin the stage loop.

Example:

```bash
python3 -m autoswe --db project.db project init \
  --name my-project \
  --goal "Build a backend-first workflow system for agent-led software design"

python3 -m autoswe --db project.db node create \
  --project my-project \
  --title "Problem definition" \
  --stage problem_definition
```

## Normal Working Loop

For each iteration:

1. Query the current tree or next work item.
2. Select the highest unresolved stage.
3. Perform the required reasoning for that stage.
4. Ask the human only if the ambiguity is expensive or cross-boundary.
5. Write the resulting state into the database.
6. Move to the next stage only after the gate passes.

Useful commands:

```bash
python3 -m autoswe resume
python3 -m autoswe --db project.db query tree --project my-project
python3 -m autoswe --db project.db query next-work --project my-project
python3 -m autoswe --db project.db query events --project my-project
```

## How To Work A Stage

For each stage, the LLM should follow the same pattern.

### 1. Read State

Read:

- current node
- parent context
- recorded decisions and resolved escalations
- related decisions
- code map if code already exists

### 2. Think

Perform the mandatory reasoning required by the current stage.

Do not skip the stage because the artifact will be short.

### 3. Emit Minimal Artifact

Create only the artifact needed to constrain downstream work.

Examples:

- problem statement
- subsystem map
- API contract
- verification plan

### 4. Persist It

Write the result through `designctl`.

Examples:

```bash
python3 -m autoswe --db project.db artifact add \
  --project my-project \
  --node-id node_123 \
  --type problem_statement \
  --title "Scoped problem definition" \
  --file problem.json

python3 -m autoswe --db project.db decision record \
  --project my-project \
  --node-id node_123 \
  --decision "Use a single SQLite file as the source of truth" \
  --rationale "Portable, durable, and easy to query"
```

### 5. Escalate Or Proceed

If the stage is blocked by unresolved ambiguity, stop, get the missing answer, and record the resolved escalation.

Do not leave durable open questions in the project database.

Example:

```bash
python3 -m autoswe --db project.db question record \
  --project my-project \
  --node-id node_123 \
  --question "Should external integrations be in scope for V1?" \
  --resolution "No. External integrations are out of scope for V1." \
  --blocking
```

If the gate passes, create the next node or move to the next unresolved node.

## When The Human Must Be In The Loop

The human should be asked when:

- scope changes materially
- architecture direction is unclear
- a tradeoff changes cost, time, or risk significantly
- ownership of a major subsystem is unclear
- a public contract has multiple viable shapes with different consequences
- the LLM would otherwise guess across a boundary

The human does not need to be asked for:

- routine implementation detail
- internal helper structure
- ordinary naming choices
- standard framework mechanics

## What The LLM Should Never Do

The LLM should not:

- start implementation while design is still unstable
- change cross-module contracts silently
- treat prose docs as the canonical state instead of the database
- write random project files as a substitute for updating the DB
- over-document routine implementation detail

## Code Awareness

`designctl` refreshes the code map automatically on each run.

Use the code map when:

- existing code must be understood before planning
- a handoff artifact needs to align with current code structure
- a module boundary must map to real files or symbols

Example:

```bash
python3 -m autoswe --db project.db query code-map
```

## Producing Implementation Handoff

Implementation handoff has two parts:

1. implementation-handoff artifacts plus implementation-discovery inputs
2. a leaf constraint package

Examples of handoff artifacts:

- headers or interface files
- schema files
- API specifications
- protocol definitions
- typed contract modules

The LLM should choose the correct handoff artifact type for the target environment.
Concrete language, framework, library, and tool selection then happens in implementation discovery unless the design has already fixed a hard constraint.

The constraint package should then define:

- what the leaf module must do
- what it must not do
- its dependencies
- its invariants
- required tests
- acceptance criteria
- escalation conditions

## First Implementation Steps

After design reaches `Implementation handoff`, the implementation phase begins with:

1. `Implementation discovery`
2. `Implementation design`

Implementation discovery chooses the concrete ecosystem and maps the approved design into the target repository.

Implementation design then decides how the approved modules and constraints will live in the actual codebase without silently changing the approved design.

Implementation discovery should be treated as a serious engineering decision workflow, not a quick setup step.
It is where languages, frameworks, libraries, toolchains, buy-vs-build choices, delivery risks, licensing, compatibility, release engineering, ownership, and evidence thresholds are evaluated explicitly.
It should also emit a mandatory implementation-context artifact that downstream implementation steps must consume so agents do not silently re-decide approved dependencies, tools, or forbidden substitutions.
Unless a project says otherwise, team-fit reasoning during implementation discovery should assume an LLM-driven engineering team with humans acting primarily as approvers, reviewers, and escalation points.

Implementation design should be treated with the same seriousness.
It is where the chosen implementation profile is turned into the actual source-tree layout, build graph, internal interface map, persistence plan, runtime plan, verification mapping, repo migration strategy, and concrete implementation packets.
Implementation design, implementation execution, and implementation verification should all inherit the mandatory implementation-context artifact rather than relying on agents to rediscover prior technology selections from scattered docs or decisions.
When a project emits follow-on product docs from design or implementation design, those outputs should be placed in an ordered project-doc folder with numbered sibling files and a matching ordered README index.

Example:

```bash
python3 -m autoswe --db project.db constraint-package create \
  --project my-project \
  --node-id node_leaf \
  --module-name auth-session-store \
  --file auth-session-store-package.json
```

## Definition Of Progress

Progress means:

- the database contains better design state
- ambiguity has been reduced
- boundaries are clearer
- contracts are tighter
- leaf modules are closer to safe implementation

Progress does not mean:

- more prose
- more brainstorm notes
- more files without approved purpose

## Recommended Human Review Points

The human should review at least:

- after problem definition
- after solution framing
- after system decomposition
- after major contract design
- before implementation handoff begins broadly

## Practical Session Pattern

For a normal session:

1. Human states the current goal.
2. LLM queries the project DB.
3. LLM identifies the current unresolved stage.
4. LLM performs the stage reasoning.
5. LLM asks the human only if a real decision is blocked.
6. LLM persists artifacts, decisions, and questions.
7. LLM either moves to the next stage or prepares a handoff package.

## End State

The design workflow is done for a leaf module when:

- its parent context is stable
- its contracts are approved
- its handoff artifacts exist
- its constraint package exists
- its required tests are defined
- an implementation agent can build it without redesigning the system
