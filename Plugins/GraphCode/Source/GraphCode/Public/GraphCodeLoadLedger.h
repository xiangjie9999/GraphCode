// Copyright GraphCode Sdn Bhd. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * WHAT THIS SESSION LOADED ON AN AGENT'S BEHALF, AND THE PASS THAT LETS IT GO. SM1.
 *
 * **THE EDITOR NEVER GIVES A LOADED ASSET BACK, AND THAT IS BY DESIGN.** `GarbageCollection.h:28`
 * makes `GARBAGE_COLLECTION_KEEPFLAGS` `(GIsEditor ? RF_Standalone : RF_NoFlags)`, and every asset
 * is born `RF_Public | RF_Standalone | RF_Transactional` (`AssetTools.cpp:1823`) — so an in-editor
 * collect steps over an unreferenced ASSET on purpose, and **a periodic `CollectGarbage` would free
 * nothing.** The only thing that frees an asset in the editor is the act the Content Browser's
 * *Unload* performs: clear `RF_Standalone` on the objects in the package, collect, and put the flag
 * back on whatever turned out to still be reachable. That act is `UPackageTools::UnloadPackages`,
 * and this class is the record that says WHICH packages are ours to offer it.
 *
 * **MEASURED 2026-09-11, in a session whose open level held two actors**: 846 StaticMesh (704 MB),
 * 883 Texture2D (629 MB), 111 World. `stat RayTracingGeometry` read 498.96 MB always-resident
 * against a 400 MB pool while only 9.05 MB was referenced, because a ray-tracing BLAS registers
 * when a mesh LOADS and its coarsest LOD is exempt from eviction — so a loaded-but-unplaced mesh
 * costs exactly what a visible one costs, for as long as the editor runs. Several agents cooking
 * `.gcshape` into one shared editor fill both render budgets in a sitting. Raising either budget
 * hides the warning and frees nothing.
 *
 * **THE RECORD IS SCOPED TO A CALL, WHICH IS THE WHOLE SAFETY ARGUMENT.** `FGcLoadScope` is alive
 * only while a tool is executing, so what lands here is what an agent's call pulled in — never the
 * asset a person opened in the Content Browser while an agent happened to be running elsewhere. A
 * sweep that unloaded whatever the process had loaded would be reaching into somebody's session.
 *
 * **AND IT LISTENS RATHER THAN INSTRUMENTING THE CALL SITES.** `FCoreUObjectDelegates::OnAssetLoaded`
 * fires from `EndLoad` for every asset an editor load produced, so it sees the dependency a
 * `CopyMeshToStaticMesh` pulled in as surely as the `LoadObject` somebody wrote — which is where
 * the megabytes actually are. Bookkeeping added to `GraphCodeGenerator.cpp` and
 * `GraphCodeShapeBuild.cpp` by hand would have recorded the two calls a reader can see and missed
 * the material, the texture and the nanite fallback behind them.
 */
class GRAPHCODE_API FGcLoadLedger
{
public:
	/**
	 * Start listening. Idempotent, called from module startup so it precedes every load — but
	 * nothing is RECORDED until a `FGcLoadScope` is alive, so an editor with no agent attached
	 * pays one empty delegate call per loaded asset and nothing else.
	 */
	static void Install();

	/** Stop listening and drop the record. Called from module shutdown. */
	static void Remove();

	static bool IsInstalled();

	/** Is a scope alive — is anything being charged to the record right now? */
	static bool IsRecording();

	/**
	 * How long since the last scope closed, or a very large number when none ever has.
	 *
	 * **THIS IS WHAT *BETWEEN CALLS* ACTUALLY MEANS, AND THE TASK QUEUE CANNOT ANSWER IT.** A
	 * synchronous tool call arrives over the transport and runs without ever being queued, so a
	 * queue with nothing pending and nothing running is not proof that nothing is executing. The
	 * scope is the one thing every route opens, so it is the one thing that knows.
	 */
	static double SecondsSinceLastCall();

	/** How many packages are on the record. */
	static int32 PendingCount();

	/** Drop the record without releasing anything. */
	static void Forget();

	/** The packages on the record, sorted. For the report and for specs. */
	static TArray<FString> Pending();

	/**
	 * Charge a package to the record by name, as though a load had produced it.
	 *
	 * **FOR THE LISTENER, FOR SPECS, AND FOR A CALLER THAT ALREADY KNOWS**, because the delegate
	 * cannot be provoked: a package loads once per session, so a spec that wanted to watch the
	 * listener would have to find an asset nothing in the suite had touched yet and would stop
	 * working the day something did. `FGcLoadErrors` documents the same trap for the same reason.
	 *
	 * **THIS DOES NOT ASK ABOUT THE SCOPE.** The scope is a fact about the LISTENER — whether an
	 * editor load may be charged to an agent — and the listener asks it. A release pass re-records
	 * what it decided to keep, and it runs BETWEEN calls, where no scope is alive; a `Record` that
	 * honoured the scope would silently drop every kept entry and the record would empty itself
	 * one pass after any package was ever skipped.
	 */
	static void Record(FName PackageName);
};

/**
 * While this is alive, every asset the editor loads is charged to the ledger.
 *
 * Nests: an inner scope does not stop the outer one, and the record survives both — releasing is a
 * separate decision made between calls, never on the way out of one. A tool that unloaded its own
 * dependencies as it returned would be unloading them while its own result still pointed at them.
 */
struct GRAPHCODE_API FGcLoadScope
{
	FGcLoadScope();
	~FGcLoadScope();

	FGcLoadScope(const FGcLoadScope&) = delete;
	FGcLoadScope& operator=(const FGcLoadScope&) = delete;
};

/** What a caller may ask of a release pass. */
struct GRAPHCODE_API FGcReleaseRequest
{
	/** Decide everything and unload nothing. The report is the same shape either way. */
	bool bDryRun = false;

	/**
	 * Cap on packages offered to the engine in one pass. `UnloadPackages` flushes rendering, walks
	 * every object twice and collects twice, so an unbounded pass is a stall proportional to
	 * whatever the last hour loaded. What does not fit stays on the record for the next pass.
	 */
	int32 MaxPackages = 512;
};

/** What a release pass did, in the terms the row asked about. */
struct GRAPHCODE_API FGcReleaseReport
{
	/** False when a precondition refused; `Refusal` says which, and nothing was touched. */
	bool bRan = false;

	/** Why the pass did not run. Empty when it did. */
	FString Refusal;

	/** The pass decided everything and unloaded nothing, and the line it produces says so. */
	bool bDryRun = false;

	/** Packages on the record when the pass started. */
	int32 Recorded = 0;

	/** On the record and no longer loaded — somebody else's unload, or an earlier pass. */
	int32 AlreadyGone = 0;

	/** Still loaded, and no exclusion applied. These are what the engine was offered. */
	int32 Candidates = 0;

	/** Candidates that are gone now — or, on a dry run, the candidates a real pass would offer. */
	int32 Released = 0;

	/** Candidates the engine kept, because something reachable still references them. */
	int32 StillHeld = 0;

	/**
	 * What the released packages' objects reported holding, measured BEFORE the unload.
	 *
	 * An estimate, and named as one: `GetResourceSizeBytes(EstimatedTotal)` is the figure the
	 * engine's own memory report uses, and it is the closest honest answer to *what did this get
	 * back*. Measured after would be measuring nothing, since the objects are gone.
	 */
	int64 EstimatedBytesReleased = 0;

	/** Why candidates were skipped: reason -> count. */
	TMap<FString, int32> Skipped;

	/** The first few released package names, for the log line. */
	TArray<FString> ReleasedNames;

	double Seconds = 0.0;

	/** One line, for the log and for the console command's answer. */
	FString ToLine() const;
};

/**
 * The pass itself. Separate from the record because WHEN is not this module's question.
 *
 * `GraphCodeMCP` owns *between tasks* — it is the half that knows a tool has returned and no other
 * is running — and calls in. A person asks for one with `gc.ReleaseLoadedAssets` in the console.
 */
class GRAPHCODE_API FGcRelease
{
public:
	/**
	 * Why a pass must not run right now, or empty when it may.
	 *
	 * **PUBLIC AND SEPARATE FROM `Run`, BECAUSE THE CALLER ASKS IT ON A TICK.** The automatic sweep
	 * decides *is now a moment* far more often than it decides *what to release*, and the expensive
	 * half of that answer is the package walk. This is the cheap half.
	 */
	static FString WhyNotNow();

	/**
	 * Offer the recorded packages back to the engine, minus the exclusions.
	 *
	 * **THE EXCLUSIONS ARE THE DESIGN.** A package is skipped when it is DIRTY (the modal that
	 * blocked the bridge for 300 seconds is `UnloadPackages` reporting unsaved work, and this
	 * refuses to be the thing that raises it), when it is `PKG_NewlyCreated` (never saved, so
	 * unloading it destroys the only copy), when it holds a WORLD (`UnloadPackages` answers the
	 * open world by calling `CreateNewMapForEditing`, which would take a person's level away
	 * mid-session), when it is OPEN IN AN ASSET EDITOR (`UnloadPackages` closes the editors for
	 * every asset it takes, and a tab closing under somebody is not a memory optimisation), and
	 * when it belongs to the engine or to script.
	 *
	 * The dirty rule is `FMCPTool_OpenLevel`'s guard applied one door along rather than a second
	 * opinion about it: same fact, same consequence. The params overload of `UnloadPackages` is
	 * used so that anything this filter missed comes back as `OutErrorMessage` TEXT rather than as
	 * `FMessageDialog::Open` — the single-argument overload is the one that opens the modal.
	 */
	static FGcReleaseReport Run(const FGcReleaseRequest& Request = FGcReleaseRequest());

	/** What the last pass did this session. `bRan` false with no refusal means none has run. */
	static const FGcReleaseReport& LastReport();
};
