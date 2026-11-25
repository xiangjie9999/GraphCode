#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "GraphCodeSettings.generated.h"

/**
 * Editor-only GraphCode sidecar policy.
 *
 * **`Config=GraphCode`, SO THE FILE IS `Config/DefaultGraphCode.ini` AND NOTHING ELSE IS IN IT**
 * (S0.0, `GC-SETTINGS-DESIGN.md` §2.1). A customer's `DefaultEditor.ini` is a shared room: our
 * section sits among the engine's, a hand edit is a diff nobody can attribute, and *"delete
 * GraphCode's settings"* has no answer. A file named after the plugin can be deleted, diffed,
 * reviewed and `.gitignore`d as one thing — and every engine behaviour still applies, because this
 * is the engine's own mechanism and not a format of ours.
 */
UCLASS(Config=GraphCode, DefaultConfig, meta=(DisplayName="GraphCode"))
class GRAPHCODE_API UGraphCodeSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UGraphCodeSettings();
	virtual FName GetCategoryName() const override { return TEXT("Plugins"); }

#if WITH_EDITOR
	virtual FText GetSectionText() const override
	{
		return NSLOCTEXT("GraphCode", "SettingsSectionText", "GraphCode");
	}

	/**
	 * Persist to `Config/DefaultGraphCode.ini` on every edit, and act on what the edit implies. S0.1.
	 *
	 * **`UDeveloperSettings` DOES NOT SAVE ITSELF.** The Project Settings page calls
	 * `TryUpdateDefaultConfigFile()` on the way out; a details view over the same CDO — which is
	 * what the panel's Setup view will be (§2, door 2) — does not. Without this an exclusion added
	 * in the panel works until the editor restarts and then silently is not there, which is the
	 * worst of the three possible outcomes because the folder was quiet in between.
	 */
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

	/** Seed the before-picture the next edit is compared against. See `ReconcileSidecarFiles`. */
	virtual void PostInitProperties() override;
#endif

	UPROPERTY(Config, EditAnywhere, Category="Sidecars")
	bool bEnableSidecars = true;

	/**
	 * Content folders that get NO `.gc`. **Ships EMPTY, and that is the ruling** — user-directed
	 * 2026-08-24: *"by default everything will have a sidecar, UNLESS TOLD TO NOT, by folder."*
	 * Opt out, never opt in.
	 *
	 * **A FOLDER, NOT A STRING PREFIX**, which is what the matcher has always done and what the old
	 * name `ExcludedPackagePrefixes` lied about: it matches a package that IS the entry or sits
	 * under `entry + "/"`, so `/Game/Textures` does not catch `/Game/TexturesOld`. Renamed for that
	 * reason in S1.2, before Fab rather than after — once the old spelling is in a customer's `.ini`
	 * a rename silently drops their whole list. `CoreRedirects` in `Config/DefaultEngine.ini`
	 * carries the transition.
	 *
	 * **AN EXCLUDED FOLDER IS STILL READABLE — user-ruled 2026-08-24, D2**: *"excluded folders wont
	 * have sidecars but readable."* This list says *do not keep text beside these*; it does not say
	 * *pretend these assets do not exist*. Until that ruling it sat inside
	 * `FGraphCodePolicy::IsPackageEligible`, which gates `gc_read`, `gc_search` and `gc_apply` as
	 * well as the file, so naming a vendored pack made every asset in it unanswerable — the setting
	 * did more than its name claimed and the extra was never asked for. It is
	 * `FGraphCodePolicy::IsUserExcludedFolder` now, asked by the two places that decide whether a
	 * FILE is kept and by nothing that reads.
	 *
	 * So an excluded folder behaves exactly as the whole project does with `bEnableSidecars` off,
	 * at one folder's scope: `SyncPackage` projects in memory, returns the text, writes nothing and
	 * reports `Suppressed`. Adding a folder here also REMOVES the `.gc` already under it (§3.3, via
	 * `FGcSidecarRemoval`) — leaving them would leave files nothing can regenerate; `.gcw` is never
	 * touched.
	 *
	 * Roots a TOOL owns are NOT here and cannot be put here — see `FGraphCodePolicy::IsToolOwnedRoot`.
	 */
	UPROPERTY(Config, EditAnywhere, Category="Sidecars", meta=(DisplayName="Excluded folders"))
	TArray<FString> ExcludedFolders;

	/**
	 * Record a person's terrain changes into the level's `terrain { }` program when the level is
	 * saved — `GC-TERRAIN-DESIGN.md` §6, user-directed: *"the delta is hooked to level save."*
	 *
	 * **IT ONLY EVER ACTS ON A LEVEL WHOSE `.gcw` DECLARES A TERRAIN PROGRAM**, which is what makes
	 * it safe on by default: authoring that block is the opt-in, and every other level in the
	 * project saves exactly as it did before. On such a level a save writes
	 * `/Game/_GraphCode/Terrain/<level>/<name>.r16`, adds or merges one `sculpt(…)` line, flattens
	 * the ground it took, and re-runs the program — so the viewport is unchanged and the terrain is
	 * stated once, in the document. **The blob goes in a root this plugin owns and never in the
	 * author's own folders** (user-ruled); a `.gcw` that already names one beside itself keeps
	 * resolving, because what moved is where a save MINTS one.
	 *
	 * Turning it off does not lose anything already recorded; it means a sculpt stays binary in the
	 * `.umap`, and NOTHING ELSE WILL RECORD IT — there is no verb for this act, because saving is the
	 * act. On a project that gitignores `Content/`, binary in the `.umap` is not anywhere
	 * ([[ignored-content-is-invisible-loss]]).
	 */
	UPROPERTY(Config, EditAnywhere, Category="Terrain",
		meta=(DisplayName="Record terrain sculpts on level save"))
	bool bRecordTerrainSculptsOnSave = true;

	/**
	 * Capture a hand-edited water body's spline into the level's `water { }` block on save —
	 * `GC-TERRAIN-DESIGN.md` §12i.4.
	 *
	 * **THE SAME BARGAIN AS THE SETTING ABOVE, ON A SUBJECT WHERE IT COSTS NOTHING.** It only ever
	 * acts on a level whose `.gcw` already declares water, and only on a body whose shape no longer
	 * matches the course the document says it was drawn from — so an authored river that nobody has
	 * touched stays authored, and the file keeps its `along=` forever.
	 *
	 * Where a sculpt writes a blob beside the document, this writes CONTROL POINTS — a few dozen
	 * numbers, in the file, readable in a diff. That is the whole reason water is where GraphCode
	 * mirrors rather than where it approximates (§12i.0).
	 *
	 * Turning it off means a person's reshaped river stays in the `.umap` and the document goes on
	 * claiming the course drew it — so the next `build:"water"` puts it back the way it was.
	 */
	UPROPERTY(Config, EditAnywhere, Category="Terrain",
		meta=(DisplayName="Capture hand-edited water splines on level save"))
	bool bRecordWaterSplinesOnSave = true;

	/** Per-frame game-thread budget for save-triggered projection work. */
	UPROPERTY(Config, EditAnywhere, Category="Performance", meta=(ClampMin="1.0", ClampMax="100.0"))
	double TickBudgetMilliseconds = 20.0;

	/**
	 * Between an agent's calls, let go of the assets those calls LOADED. SM1, `GraphCodeLoadLedger.h`.
	 *
	 * **THE EDITOR NEVER GIVES A LOADED ASSET BACK BY ITSELF** — `GARBAGE_COLLECTION_KEEPFLAGS` is
	 * `RF_Standalone` in-editor, so an unreferenced asset is kept on purpose and a periodic collect
	 * frees nothing. Measured 2026-09-11 in a session whose open level held two actors: 846 static
	 * meshes and 883 textures, 498.96 MB of always-resident ray-tracing geometry against a 400 MB
	 * pool. A ray-tracing BLAS registers when the mesh LOADS, so a mesh nothing placed costs what a
	 * visible one costs until the editor restarts.
	 *
	 * **ONLY WHAT AN AGENT'S CALL LOADED, AND ONLY WHAT IS SAFE TO TAKE**: clean, saved, not a
	 * world, not open in an asset editor. An asset a person opened themselves is never on the
	 * record, and nothing dirty is ever offered.
	 *
	 * Turning it off costs nothing but memory, and is the right answer for a solo project small
	 * enough never to reach either budget. `gc.ReleaseLoadedAssets` in the console runs one pass by
	 * hand either way, and `gc.ReleaseLoadedAssets dryrun` says what one would take.
	 */
	UPROPERTY(Config, EditAnywhere, Category="Performance",
		meta=(DisplayName="Release assets an agent's calls loaded"))
	bool bReleaseLoadedAssetsBetweenCalls = true;

	/**
	 * How many recorded packages it takes before an automatic pass is worth its cost.
	 *
	 * **A THRESHOLD RATHER THAN EVERY CALL, BECAUSE THE PASS IS NOT FREE**: `UnloadPackages`
	 * flushes rendering, closes loaders and collects twice. Paying that after every `gc_read` would
	 * be a tax on the calls that loaded nothing much, and it would unload the very packages the
	 * next call in a sequence is about to ask for again.
	 */
	UPROPERTY(Config, EditAnywhere, Category="Performance",
		meta=(ClampMin="1", ClampMax="10000", DisplayName="Release after this many loaded packages"))
	int32 ReleaseAfterPackages = 200;

	/**
	 * How long the queue has to stay empty before a pass runs, in seconds.
	 *
	 * The gap between two calls of one agent's sequence is under a second; the gap that means
	 * *they have stopped for now* is longer. Waiting is what keeps the pass out of the middle of a
	 * sequence, where it would unload what the next call reloads.
	 */
	UPROPERTY(Config, EditAnywhere, Category="Performance",
		meta=(ClampMin="0.5", ClampMax="600.0", DisplayName="Release settle window (seconds)"))
	double ReleaseSettleSeconds = 8.0;

	/**
	 * Build a `.gcshape` mesh with Nanite the FIRST time it is created. User-reported 2026-09-11:
	 * *"why the meshes isnt nanite by default when my whole project uses nanite by default"*.
	 *
	 * **NOTHING WAS DECIDING THIS, WHICH IS THE ACTUAL DEFECT.** The create door hands the geometry
	 * script a `FGeometryScriptCreateNewStaticMeshAssetOptions` and set only the two recompute
	 * flags, so `bEnableNanite` stayed at its struct default of false — and one door down
	 * `UE::AssetUtils::CreateStaticMeshAsset` writes `GetNaniteSettings().bEnabled = false`
	 * outright unless it is told otherwise (`CreateStaticMeshUtil.cpp:219`). So every mesh a
	 * program built was born non-Nanite because a parameter went unnamed, not because anybody chose
	 * it. **MEASURED in this checkout: of 564 meshes built from a `.gcshape`, 156 carry Nanite and
	 * 408 do not** — and all 156 got it from a human writing `NaniteSettings.bEnabled = true` into
	 * that mesh's own `.gc` afterwards.
	 *
	 * **AND THERE IS NOTHING TO INHERIT IT FROM.** A project-wide *new meshes are Nanite* is not a
	 * setting the engine has — `Config/DefaultEngine.ini` here carries no Nanite section at all —
	 * so "the whole project uses Nanite" is a convention held in the sidecars, and a convention has
	 * to be stated somewhere to be honoured. This is where.
	 *
	 * **FIRST BUILD ONLY, AND THE `.gc` OWNS IT FOREVER AFTER** — the same seam as a material slot's
	 * default path (`GC-PRODUCT-SPLIT.md` §5.2). A rebuild goes through `CopyMeshToStaticMesh`,
	 * which is never told about Nanite and therefore leaves whatever the asset wears: turning this
	 * on does not reach back and Nanite-ify the 408, and turning it off does not strip the 156.
	 * Changing one mesh is still `settings { NaniteSettings.bEnabled = … }` in its own `.gc`.
	 *
	 * Off is the right answer for a project targeting hardware without Nanite; a project that
	 * states it per asset today loses nothing by leaving it on.
	 */
	UPROPERTY(Config, EditAnywhere, Category="Shape",
		meta=(DisplayName="Build new .gcshape meshes with Nanite"))
	bool bBuildShapeMeshesWithNanite = true;

	/**
	 * How many edges a `.gcrig` `diagnostic()` will draw a tube for before it refuses.
	 *
	 * **IT WAS A HARDCODED 60,000 AND THAT NUMBER WAS SIZED FOR A CYLINDER.** Measured 2026-09-18 on
	 * a marketplace elephant: the SOURCE mesh is 59,141 edges — 859 below the old ceiling — so the
	 * subject could never have a wireframe even before the rig inserted a ring, and after the insert
	 * it is 307,745. The two diagnostics that show what a retopology did were therefore the two you
	 * could not have on the first real animal this format met, which is precisely when they are
	 * wanted.
	 *
	 * **AND THE REFUSAL WAS ADVISING TWO THINGS THAT DID NOT EXIST**: *draw a region rather than the
	 * whole subject* (`diagnostic()` takes no arguments, so there is no way to say which region) and
	 * *raise the ceiling deliberately* (it was a literal in the source). A refusal that names a
	 * remedy the tool does not have costs more than a bare no, because it sends the reader looking.
	 * This is that remedy, and the refusal now names it.
	 *
	 * **WHAT IT COSTS, so the number is chosen rather than guessed.** A tube is six triangles, so
	 * the ceiling is a triangle budget divided by six: 60,000 edges is ~360k triangles, and this
	 * elephant's 307,745 is ~1.85M. That is a heavy asset to keep in `Content/` and a fine one to
	 * look at once. Raise it when you need the picture; it refuses rather than silently drawing a
	 * tenth of the edges, because a sparse-looking wireframe of a dense mesh is a lie.
	 */
	UPROPERTY(Config, EditAnywhere, Category="Rig",
		meta=(ClampMin="1000", ClampMax="2000000",
			DisplayName="Wireframe diagnostic edge ceiling"))
	int32 RigWireframeEdgeCeiling = 60000;

	/**
	 * **WHETHER A BIND MEASURES THROUGH THE MESH OR STRAIGHT THROUGH THE AIR.** On, a bone reaches a
	 * vertex only along a path that stays inside the surface, which is what stops a tail from
	 * driving the thigh it hangs beside and a trunk from driving the chest it curls against.
	 *
	 * **THIS IS NOT AN AESTHETIC CHOICE, WHICH IS WHY IT IS NOT A `.gcrig` WORD.** Straight-line
	 * binding measured the back of the elephant's knee as **62.7% tail** — the tail was 78.6 units
	 * away and the stifle 80.3, and five tail bones each cast a full vote against the stifle's one
	 * (ledger §19.12). Euclidean distance cannot see the air between two limbs; this can.
	 *
	 * Off only for a mesh the voxel pass cannot classify — open shells, badly non-manifold soup —
	 * or when a bind is too slow to iterate on and the subject has no limb resting against another.
	 */
	UPROPERTY(Config, EditAnywhere, Category="Rig",
		meta=(DisplayName="Bind through the surface (geodesic)"))
	bool bRigBindThroughSurface = true;

	/**
	 * **THE GRID THE THROUGH-SURFACE BIND WALKS**, along the mesh's longest axis. This is the
	 * engine's own default for `FGeometryScriptSmoothBoneWeightsOptions::VoxelResolution`, and it
	 * is that on purpose — see below.
	 *
	 * Higher sees narrower gaps and costs cubically: the pass floods a grid **per bone**, and the
	 * engine's own comment on it reads "this is grossly inefficient".
	 *
	 * **IT SHIPPED AT 128 FOR ONE AFTERNOON AND THAT WAS A MISTAKE WORTH RECORDING.** The reasoning
	 * was that the gap to resolve is tail-to-thigh on the elephant, ~80 units on a 1191-unit axis,
	 * which 128 cells clear eightfold at 9.3 units each — so half the resolution would buy 8x the
	 * speed for nothing. **Measured, it did not**: an isolated probe that swings only the tail and
	 * diffs the rear view went **84,060 changed pixels at 128 to 48,597 at 256**, and the picture
	 * changed character — at 128 both whole thigh masses lit up, at 256 only the tail's own travel
	 * arcs do. The arithmetic about the gap was right about the gap and wrong about the mesh: a
	 * coarse occupancy grid also fattens the tail and the thigh toward each other, so what has to
	 * fit between them is not one cell but several.
	 *
	 * **The lesson is the general one**: an engine default that looks expensive is still a default
	 * somebody chose, and halving it is a change that owes a measurement exactly as much as
	 * doubling it would. Raise it beyond 256 for a subject with genuinely thin separations —
	 * fingers held together, a mane against a neck.
	 */
	UPROPERTY(Config, EditAnywhere, Category="Rig",
		meta=(ClampMin="16", ClampMax="512", EditCondition="bRigBindThroughSurface",
			DisplayName="Bind voxel resolution"))
	int32 RigBindVoxelResolution = 256;

	/** A failed write is retried this many times before waiting for the next real asset save. */
	UPROPERTY(Config, EditAnywhere, Category="Reliability", meta=(ClampMin="0", ClampMax="20"))
	int32 MaxWriteRetries = 3;

	/**
	 * How long a burst of changes has to go quiet before the version-control panel re-reads. E11.10.
	 *
	 * **THE DIAL IS THE WINDOW, AND THERE IS DELIBERATELY NO OFF SWITCH.** A refresh costs 350 ms of
	 * git and a parse of every `.gc`, so the cost of being live is paid per settled burst rather
	 * than per change — deleting forty assets is one refresh, not forty. Somebody who finds even
	 * that too eager raises this; somebody who wants the panel to keep up with an agent writing
	 * every second lowers it. What nobody gets is a panel that is quietly wrong, which is what an
	 * enable/disable flag would be offering (`GraphCodeChangeNotice.h`).
	 */
	UPROPERTY(Config, EditAnywhere, Category="Performance",
		meta=(ClampMin="0.05", ClampMax="60.0", DisplayName="Auto-refresh settle window (seconds)"))
	double AutoRefreshSettleSeconds = 0.35;

	/**
	 * Keep the previous canonical `.gc` body when a save replaces it. E11, `GC-LOCAL-HISTORY-DESIGN.md`.
	 *
	 * Local recovery for what GraphCode covers, and nothing else: a `.gcc` is ordinary canonical
	 * GraphCode text, so it carries exactly the facets the format carries and inherits that text's
	 * `omits:`. Git covers what you committed; this covers the last few saves.
	 */
	UPROPERTY(Config, EditAnywhere, Category="Local history")
	bool bEnableLocalHistory = true;

	/**
	 * How much disk the WHOLE PROJECT's local history may occupy, in megabytes. **A BUDGET, NOT A
	 * COUNT, AND NOT PER ASSET.**
	 *
	 * User-directed 2026-08-05, replacing `LocalHistoryLimitPerAsset`: *"have max size, not max file
	 * numbers. so more than a max size will reduce the cutting off date nearer to current"*, then
	 * *"max size for whole project"*.
	 *
	 * Both halves of that are corrections and both matter. **A count is a different promise for
	 * every asset** — ten versions of a 300-byte struct is 3 KB and ten of a rebuilt Widget
	 * Blueprint is several megabytes — so the number that was supposed to bound the store bounded
	 * nothing. And **a per-asset budget bounds nothing either**: it is a per-asset promise
	 * multiplied by however many assets somebody happens to edit, which is the number nobody knows
	 * in advance. One project-wide figure is the only one a person can reason about, because it is
	 * the only one that answers "how much of my disk is this".
	 *
	 * **EXCEEDING IT WARNS. IT DOES NOT DELETE ANYTHING** — user-directed 2026-08-28: *"don't even
	 * do auto pruning, since people lose their work without notice ... only warn them, if the size
	 * exceed their set warning limit, on startup"*. The check runs once at startup and says what the
	 * store holds; what to drop, and back to which date, is the owner's call. It used to trim the
	 * oldest entries across the whole store on every save, silently, which is how an asset worked on
	 * for weeks ends up with a history that begins this morning.
	 *
	 * `FGraphCodeHistory::OldestRetained` is how far back the store currently reaches.
	 *
	 * `0` STOPS NEW AUTOMATIC ENTRIES AND DELETES NOTHING (§3). Reading it as "keep nothing" would
	 * turn a setting somebody reached for to stop the store growing into one that empties it.
	 */
	UPROPERTY(Config, EditAnywhere, Category="Local history",
		meta=(ClampMin="0", ClampMax="65536", DisplayName="Local history budget, whole project (MB)"))
	int32 LocalHistoryBudgetMegabytes = 2048; // 2 GB (owner, 2026-11-24); was 256

	/**
	 * Say on screen when a save regenerated a `.gc` that no longer matched its own `sig:`. E6.6.
	 *
	 * **OFF SILENCES THE TOAST AND NOTHING ELSE.** The copy is still taken, it still lands under
	 * `Saved/GraphCode/Proposals`, and `LogGraphCodeProposal` still carries the warning with the
	 * destination path in it. This setting decides whether somebody is INTERRUPTED, not whether
	 * their text is kept — nothing in `PreserveProposalIfEdited` reads it, which is what stops a
	 * person who turned it off from losing anything by having turned it off.
	 *
	 * **IT CANNOT SILENCE THE FAILURE.** When the copy could not be written the text IS gone, and
	 * that toast is raised whatever this says. A switch that hid a loss would be the one setting
	 * nobody should be able to reach for, and E6.3 already rules that branch the one that has to
	 * look worse.
	 *
	 * **SHIPS ON, AND OFF IS A PROJECT'S CHOICE RATHER THAN THE PLUGIN'S.** For somebody editing
	 * `.gc` by hand this is the notice that catches a lost patch, and defaulting it off would
	 * disarm the feature for everyone to suit one workflow. What made it worth a switch is the
	 * other workflow: an agent that writes a `.gc` and then applies it leaves the file briefly out
	 * of step with its own `sig:`, so in a project authored that way the notice fires all day about
	 * text nothing lost — which is how a warning stops being read. User-directed 2026-09-02.
	 * Our development project turns it off in `Config/DefaultGraphCode.ini`, where the rest of its preferences are.
	 */
	UPROPERTY(Config, EditAnywhere, Category="Notifications",
		meta=(DisplayName="Warn when a save regenerates over an edited .gc"))
	bool bReportPreservedProposals = true;

	/**
	 * Report a REGISTERED GraphCode product as not installed: `move`, `play`, `world`.
	 * `GC-PRODUCT-PACKAGING-DESIGN.md` §3. `-GcWithoutProduct=move,world` does the same per run.
	 *
	 * **THIS EXISTS BECAUSE EVERY PRODUCT IS INSTALLED IN A DEVELOPMENT CHECKOUT**, so the gate that
	 * decides what a Core-only customer sees would otherwise never execute anywhere it is written.
	 * It is a simulation of an absence, not a licence: nothing is disabled, unloaded or hidden from
	 * the engine, and the code the switch pretends is missing is still linked in.
	 *
	 * It logs a warning on every query while active. A silent simulated absence is a debugging
	 * session lost to a switch nobody set.
	 */
	UPROPERTY(Config, EditAnywhere, Category="Products",
		meta=(DisplayName="Simulate missing products (development only)"))
	TArray<FString> SimulateMissingProducts;

	// `MergePanelWidget` WAS HERE AND IS GONE (E9.5). It named the Editor Utility Widget the
	// notifications offered to open, and it existed because a plugin could not carry its own panel:
	// the asset lived in `/Game`, so the path had to be per-project and the button had to be able
	// to vanish. The panel is a nomad tab this plugin registers now, so there is nothing to
	// configure and nothing to fail to resolve. A stale entry in an existing `.ini` is ignored.

#if WITH_EDITOR
private:
	/**
	 * S1.5 — act on what an edit CHANGED, which is not what an edit tells you.
	 *
	 * `PostEditChangeProperty` hands over the property that moved and its new value; the two
	 * settings here both owe an action that depends on the OLD one. Adding a folder removes the
	 * `.gc` under it and removing that same folder syncs them back (§3.3) — opposite acts, one
	 * property, and the array arrives looking identical either way. So the before-picture is kept
	 * and diffed.
	 *
	 * **BOTH DIRECTIONS, BECAUSE A MARK THAT PROTECTS MUST BE REMOVABLE**
	 * ([[the-override-is-a-switch-not-just-a-record]]) — an exclusion whose removal did not restore
	 * what it took would be a one-way door wearing a checkbox.
	 */
	void ReconcileSidecarFiles();

	/** What the last edit left behind. Seeded at load, refreshed after every reconcile. */
	TArray<FString> LastKnownExcludedFolders;
	bool bLastKnownEnableSidecars = true;
#endif
};
