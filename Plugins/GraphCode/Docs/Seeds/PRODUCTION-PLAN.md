# <Project> — production targets and evidence

*Use for a vertical slice or production game. Fill from existing decisions and observed results;
mark unknowns explicitly. Keep tasks on the board and link to the plugin's production and
validation guides. This file carries only this project's answers.*

## Target

- Player journey and completion condition: <link to the game direction, plus the slice boundary>.
- Build mode and current milestone: <mode; milestone; exit condition>.
- Art, motion and sound bar: <links to the owning design notes and accepted reference/capture>.
- Platform and input: <platforms, minimum/representative devices and input methods>.
- Display and performance: <resolution, quality settings, frame-time and hitch limits>.
- Resource budgets: <memory, loading and package limits; identify provisional assumptions>.
- Session model: <single-player or network model; travel, persistence and supported save versions>.
- Review: <who accepts art/feel and when; link to existing permissions and spending limits>.

## Dependencies that decide the slice

<Keep only dependencies that could change the approach or prevent the milestone. Link the
research/implementation plan and installed capability evidence; do not copy tool schemas here.>

| Required outcome | Owning source and runtime consumer | Authoring route and dependencies | Smallest proof, result and evidence |
|---|---|---|---|
| <player-visible requirement> | <file/asset/component; who consumes its result> | <verified route; required product/plugin/source> | <scenario; pass/fail/not run; evidence link> |

<For an unresolved row: exact question, bounded experiment or missing decision, affected work,
and next action. Keep planned capability separate from what this candidate has demonstrated.>

## Reproduce the slice

- Source revision and engine/plugin versions: <identity>.
- Start map, player and initial state: <paths and setup>.
- Player actions and expected outcome: <scenario or link>.
- Failure, interruption and retry: <scenario or link>.
- Save/load and supported travel: <scenario or not applicable with reason>.
- Build/run: <verified command or Launcher profile, configuration and prerequisites>.
- Safe test reset: <disposable profile/save and reset method; preserve real progress>.

## Evidence for this candidate

**Candidate:** <artifact path/build identity, source revision and date>.

| Claim | Target/configuration | Observed result and evidence | Status |
|---|---|---|---|
| Intended player journey | <map, input, initial state> | <capture/log and actual outcome> | <pass/fail/not run> |
| Visual, motion and audio bar | <camera/settings> | <viewed capture, runtime observation, review> | <status> |
| Save/reload and recovery | <profile/version> | <restart outcome> | <status or justified not applicable> |
| Performance and stability | <device/settings/scenario/duration> | <measured values and trace> | <status> |
| Packaged candidate | <platform/configuration> | <launch and critical journey result> | <status> |
| Meaningful variation and scale | <second content variant and intended simultaneous workload> | <shared behaviour, differences and measured cost> | <status> |
| Destination handoff, if promised | <editable project/asset and declared destination dependencies> | <fresh destination open and use> | <status or justified not applicable> |
| Additional project requirement | <requirement> | <evidence> | <status> |

Keep technical results and art/feel review separate. Mark an outstanding agreed review as
**review pending**. Name the candidate each observation covers and the changes that invalidate it.

## Continue from here

- Last completed outcome: <observed result and evidence, not just the last successful tool call>.
- Next executable work: <link to one board item and its acceptance condition>.
- Blocking dependency, if any: <exact missing capability, input or decision; independent work that can proceed>.
- Sources and recovery: <owning recipes/import sources, relevant history and safe reset locations>.
- Delivery: <exact artifact, launch instructions, declared runtime dependencies and known limits>.

## Known limits and decisions

<Unverified targets, accepted constraints, current evidence limits and links to open board items.
Do not copy the board here. Supersede old candidate results explicitly when a new build changes
the claim; preserve useful prior evidence in the project's completed-work record.>
