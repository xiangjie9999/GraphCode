# Research unfamiliar work and write the plan before building

**For any unfamiliar or complex work, do extensive, task-specific research and save a detailed
implementation plan before implementation.** This applies across the whole game and its tools,
not only to particular asset types. Recognizing a feature's name or knowing its creation command
is not enough to understand how to build it well.

**Resolve each still-ambiguous step before spending production effort on it.** A broad plan is
not permission to improvise its unresolved details. Think through the construction, dependencies,
connections and proof of each step; record the decision and its evidence before carrying it out.

This is the standing game-making workflow requested on 2026-09-28. It complements the quality
bar in [ASSET-QUALITY.md](ASSET-QUALITY.md) and the verification in [VALIDATION.md](VALIDATION.md).
The guides teach the method; each project's plan records its own requirements and decisions.

## When this applies

Use this workflow when you cannot yet explain how the requested result will be produced,
controlled, integrated, inspected and maintained; when dependencies or interactions are complex;
or when an earlier attempt fails for reasons you do not understand. Apply it to each unfamiliar
part of a larger task, including outside software and services.

For familiar work with a current, proven plan, check that the assumptions still hold and reuse it.
Do not research the entire engine on every task. "Extensive" means resolving the questions that
determine a correct, high-quality result, not collecting the largest number of links. A new engine
version, different target, changed requirement or unexplained failure can reopen those questions.

## 1. Define the result before choosing a technique

Read the user's request, project decisions, existing implementation and relevant task route.
Write what the player or author must be able to see and do, the intended quality, and what would
count as failure. Include the real context: camera/view, input, timing, interactions, target
platform and scale where relevant. Separate required outcomes from optional additions.

List what you know, what you are assuming and what you need to learn. An assumption must not
quietly become a requirement or a verified fact. Use existing answers; ask only about a missing
decision that materially changes the result and cannot be resolved within existing authorization.

## 2. Research the actual problem thoroughly

Work from questions, not a broad search for the feature's name:

1. **Find the precedent.** Inspect a working counterpart in the project, its source assets and
   actual behaviour. Reuse established patterns when they fit. Explain any proposed departure.
2. **Learn the concepts and recommended workflow.** Read the relevant official documentation for
   the installed engine, framework or service version. Learn how the parts interact, not just how
   to create the first asset. Read the prerequisites, limits and lifecycle as well as the quickstart.
3. **Inspect a complete example.** Prefer an official sample, maintained reference implementation
   or an existing project result. Trace its inputs through to the visible or functional outcome.
   Check which parts are demonstration shortcuts and which fit production use.
4. **Investigate the difficult edges.** Read the relevant integration, performance, debugging,
   platform and failure guidance. Follow dependencies until you understand the ones this task uses.
   Compare plausible approaches when the choice affects quality, cost or maintainability.
5. **Check the authoring path separately.** Ask the installed tools for current help and grammar;
   read the narrow matching guide, then implementation source if a material question remains.
   Engine support, installed-plugin support, exposed authoring support and runtime support are
   separate facts. A supported asset kind does not establish support for every feature inside it.
6. **Cross-check the important claims.** Corroborate a consequential choice with relevant primary
   documentation, an inspectable example or installed source. A search snippet, remembered
   tutorial or repeated claim across several derivative pages is not sufficient evidence.

Search online when the topic is unfamiliar or current information matters. Prefer primary sources:
official docs, maintained source, release notes, official examples and original technical papers.
Videos, talks and community demonstrations can reveal useful techniques; verify the parts you
intend to rely on against the installed system. A still image of a demonstration does not reveal
its setup, runtime behaviour or cost. When a source cannot be accessed, say so and find another
usable source; do not pretend to have read it.

For outside tools, research their current integration, input/output formats, automation or API
support, and how the result returns to the project. Use existing authorized tools, credentials
and budgets. Do not assume a connector exists, copy an obsolete API request, or invent a command
because an online example used a similar product.

Record findings as decisions and evidence, not pasted articles:

| Question | Finding and consequence | Source/version/date | Confidence and next action |
|---|---|---|---|
| What must be resolved? | What was learned; how it changes the proposed work | Direct link or exact local file/example, with relevant version | Observed locally, documented, inferred or still unknown; how to resolve it |

Keep facts, choices and evidence distinct. "The documentation supports this approach", "we chose
it", and "we proved it here" are three different statements. Never invent measurements, examples
inspected or research that was not performed.

**For an unexplained native-system failure, read the mechanism before tuning it.** Trace the
selected engine implementation and defaults, including its input assumptions, exclusions and
coordinate conventions. Audit the captured starting state with local and aggregate measurements.
Check that constraints and acceptance criteria can hold together; distinguish authored dimensions
from simulation samples, collision proxies and visible output. Use an inexpensive offline replay
when it can answer the question, with its limitations stated. Before another build/run, name the
causal hypothesis, predicted observation and smallest distinguishing control. A passing simple
example establishes only its own setup. Locate the first divergence rather than repeatedly adjusting
iterations, resolution or unrelated settings against the final failure.

## 3. Save an actionable plan before implementation

Use the existing owning design/implementation note where possible. For a substantial feature,
create one focused plan and link it from the project's agent guide or board. Put each complex
subsystem in its own section, with explicit dependencies between them. Split into linked files
only when that improves reading; do not create a second competing design or queue.

Lead the owning plan with current conclusions, confidence, unresolved causes and the next
discriminating experiment. Keep chronological run records in its linked ledger when they bury
that answer; follow `Docs/AgentGuide/DOC-STRUCTURE.md` for preservation and stable references.

The plan must explain **how** the result will be made. "Research, implement, test, polish" is
not a detailed plan. Identify the actual components, data, assets, connections, sequence of work,
choices and outcome checks. Follow this template, replacing every prompt with task-specific facts:

```markdown
# <Feature or deliverable> — research and implementation plan

## Outcome and scope
- Required player/author experience and quality references:
- Concrete success and failure conditions:
- Existing decisions, constraints and explicitly excluded work:
- Target environment, installed versions and relevant budgets:

## Research findings
- Questions investigated, direct sources/examples and their versions:
- What the sources teach that changes this implementation:
- Existing project precedent and what will be reused:
- Documented capability versus locally verified capability:
- Remaining assumptions/unknowns and how each will be resolved:

## Chosen approach
- Parts of the solution and what each owns:
- Data/control flow from input through behaviour to the final result:
- Sources, outputs, dependencies and integration boundaries:
- Parameters, units, coordinate/time conventions and derivations where relevant:
- Lifecycle: initialization, update, interruption, completion and cleanup:
- Other approaches considered and why this one fits the requirement:

## Prerequisites and authoring path
- Required source assets, tools, plugins, formats and their availability:
- Installed authoring/readback/render/runtime paths and known coverage gaps:
- External preparation or API steps and how their outputs enter the project:
- Dependencies or user decisions that actually block a step:
- Preservation of existing assets, source files and recoverable states:

## Ordered implementation
For each stage:
1. Purpose and dependency on earlier work.
2. Ambiguities, plausible options, evidence needed and the chosen resolution with its reason.
3. Exact assets/files/components to create or change and their responsibilities.
4. Configuration and connections, with the source of important choices.
5. The smallest representative result this stage produces.
6. How it will be inspected/tested and what must pass before expanding it.
7. Readiness: ready, needs a defined experiment, or blocked on a named dependency/decision.

## Integration and quality
- Connection to the existing game and other systems:
- Art, motion, sound, UI and interaction requirements that apply:
- For game content/presentation: theme references, planned craft beyond the minimum (or explicit
  simplicity), deliberate forms/timing, and seam/detail/phase/state/transition coverage through
  renders, playback, listening or interaction as appropriate:
- For realistic surfaces: causes/scales of imperfection, geometry versus material/decal bake,
  protected interfaces, supported bake inputs/outputs and source-to-baked comparison:
- Persistence, reuse, scaling or multiplayer obligations that apply:
- Performance expectations and the representative workload to measure:
- Failure, cancellation, retry and degraded-capability behaviour:

## Verification and acceptance
- Reproducible starting state, actions and expected visible/functional result:
- Normal, boundary, repeated and interrupted cases:
- Readback, playback, reload and package checks relevant to the claims:
- Measurement/capture method and where evidence will be kept:
- Quality review criteria and unresolved checks that prevent completion:

## Risks and recovery
- What is most likely to invalidate the approach:
- Smallest experiment that could resolve each critical uncertainty:
- Supported alternatives if a dependency or assumption fails:
- Safe recovery path that preserves the owner's work:

## Progress and evidence
- Stage status and observed results, clearly separate from planned results:
- Decisions changed after research or testing, and why:
- Link to the board for remaining work:
```

Use only applicable sections, but do not omit a relevant uncertainty because it is inconvenient.
Be specific enough that another AI can continue without reconstructing the design from chat.
Scale detail with complexity; a long plan filled with generic advice is still a weak plan.

## 4. Check each step for ambiguity before doing it

Before starting a stage, revisit its plan against the current evidence. Break vague actions such
as "make it fit", "add the effect" or "polish the UI" into decisions that can be implemented and
checked. Ask:

- Is the intended result clear, including the cases that could change the design?
- Are the inputs, outputs, ownership, interfaces and dependencies defined?
- Is the technique supported here, and are the consequential settings and tradeoffs understood?
- Is there enough reference information to construct the result without inventing hidden details?
- What proves success, and what will happen if the approach fails?

For every material unknown, write the question, alternatives, evidence needed, selected answer and
reason in the owning plan. Keep the decision record concise so another AI can see the conclusion
and its basis. Do the research, calculations, reference preparation
or comparison needed to resolve it. A guess with a precise number is still a guess.

**Do not start dependent production work while its construction or acceptance criteria remain
ambiguous.** This includes bulk asset generation, mesh construction, integration and repetition.
Mark the step as needing research, a defined experiment or a specific user decision. Continue
independent steps whose prerequisites are resolved. Ask the owner only when the missing decision
cannot be settled from their instructions, project precedent or authorized engineering judgment.

An unresolved later step can stay listed with its resolution plan; it must pass this check before
work reaches it. Resolve it earlier if its answer could invalidate the work about to begin. Repeat
this check whenever requirements, evidence or dependencies change. "Decide during implementation"
is not a resolution for a consequential choice.

## 5. Resolve uncertainty with a planned, small experiment

Some answers require running the actual system. Before that experiment, write its question,
minimum setup, precise scope, expected observation and safe recovery. Use a new disposable
example when learning a risky operation. The experiment is part of the plan, not permission to
modify the owner's valuable assets by trial and error.

Only the bounded experiment proceeds while that question is open. Do not build the whole feature
and call it research. Plan reference generation in the same way: state which missing view or design
decision the image must resolve before spending generation effort or credits.

Record what happened and revise the implementation plan before proceeding past that dependency.
Do not describe an unrun experiment as proof. If required tooling is unavailable, mark the claim
unverified, identify the missing dependency and continue useful independent work. Investigate a
supported alternative rather than guessing a tool schema or silently lowering the requirement.

## 6. Implement in stages and keep learning from the result

Start with one representative part or interaction that exposes the hardest integration and
quality requirement. Inspect it through the same route a player or author will use. A parser
pass, successful import or correct property value cannot establish the final result on its own.
Use [TOOLS.md](TOOLS.md) for safe authoring and [VALIDATION.md](VALIDATION.md) for evidence.

Compare the observation against the plan. Diagnose the cause of a mismatch, do additional
targeted research where understanding is missing, and update the plan before changing direction.
Do not stack speculative fixes until the result happens to look plausible. Expand only after the
representative result meets its applicable quality and behaviour gates.

Keep source artifacts, chosen settings, evidence and durable lessons in their existing canonical
homes. Link to shared guides instead of copying them into project notes. Update a guide when
research reveals a reusable missing lesson; keep project-specific choices in the project.

## Planning does not create an extra approval checkpoint

For an authorized implementation task, research and write the plan, make it reviewable, then
continue implementation within that authorization. Do not stop merely to ask whether you may
carry out work already requested. Ask only for a real missing decision or permission required
by the current scope. Respect the owner's agreed review cadence.

A request only for research or a plan remains that scope. Finish with the findings and actionable
plan; do not start implementation without authorization. Never use extensive research as an
excuse to leave an authorized deliverable at the planning stage.
