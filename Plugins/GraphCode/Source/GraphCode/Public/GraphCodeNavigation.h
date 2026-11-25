// Copyright GraphCode Sdn Bhd. All Rights Reserved.

// Navigation's WORLD half — `GC-NAVIGATION-DESIGN.md` §0 ruling 1, task N1.
//
// *Where can anything go*, as a property of the SPACE. The agent that decides where to go and drives
// itself there is Move's and is not here. §0's test: if moving the character out of the level leaves
// the statement true, it is this file's.
//
// N0 made a nav bounds volume have a SHAPE. Until then an agent could place one from text and it
// enclosed nothing, so every question below answered `no` for a reason that had nothing to do with
// the level. This file is the other end: something that builds the mesh, and something that can be
// asked what the mesh means.
//
// THREE QUESTIONS, AND THEY ARE THE THREE §7's N1 NAMES. Is this point navigable; is B reachable
// from A; is any of this area standable. Everything else navigation could be asked is either derived
// from these (§5's connectivity artifact is the pairwise reach over named regions) or belongs to the
// agent rather than the ground.
//
// AND EACH OF THE THREE TAKES A MESH — §7 N1b. Unreal generates a SEPARATE navmesh per agent size out
// of the level's own geometry, so *which body can get through here* is a fact about the ground and
// belongs in this file. §0's test settles it the usual way: take the mouse out of the level and the
// mouse-sized navmesh is still there. What stays Move's is which AREAS a given character may use —
// a `UNavigationQueryFilter`, a fact about who is asking. Omitting the mesh keeps the level's default,
// which is what every caller written before N1b meant.
//
// IT SAMPLES NOTHING, and that is inherited from `FGraphCodeLevelNavigation`'s own rule rather than
// invented here: the CALLER supplies the points, because the resolver that owns `walkable=` already
// picks them purely and seeded from an entry's name, and a second sampler with its own RNG inside a
// file whose answers have to reproduce across an engine upgrade is a second answer to one question.
// `Standable` therefore takes points, never a shape.
//
// THE BUILD IS THE ONLY WRITE IN GRAPHCODE'S READ PATH, so it is a separate call that a caller has
// to make on purpose. Navigation data is derived — a level `.gc` omits `built_data` by design — but
// deriving it dirties packages, and [[reading-must-not-write]] is why no query here builds first. A
// query against a stale mesh says so instead.

#pragma once

#include "CoreMinimal.h"

class ANavigationData;
class UNavigationSystemV1;
class UWorld;

/**
 * What one build did, or refused to do and why.
 *
 * THE REFUSAL IS THE POINT OF THIS STRUCT. `UNavigationSystemV1::Build()` returns `void` and opens
 * by returning silently when there is nowhere to build and when a build lock is held — so the call
 * an agent makes after authoring a nav volume, and the call it makes after authoring one WRONG, are
 * indistinguishable at the call site. Everything the engine decides quietly is decided here first
 * and named.
 */
struct GRAPHCODE_API FGcNavBuildResult
{
	/** True only when the engine was actually asked to generate. */
	bool bBuilt = false;

	/** Why nothing was built, when nothing was. Empty on success. */
	FString Refusal;

	/**
	 * Nav bounds volumes PLACED, and the ones the system has REGISTERED. They differ, and the
	 * difference is a defect this struct exists to show.
	 *
	 * `IsThereAnywhereToBuildNavigation()` answers true off a bare `TActorIterator<ANavMeshBounds
	 * Volume>` (`NavigationSystem.cpp:2540`) while the generator builds inside `RegisteredNavBounds`,
	 * which a freshly spawned volume does not reach until the system TICKS. So a build immediately
	 * after an apply passes every guard the engine offers and generates an empty mesh.
	 */
	int32 PlacedVolumes = 0;
	int32 RegisteredVolumes = 0;

	/** The registered bounds added up — what was actually searched. */
	FBox Bounds = FBox(ForceInit);

	/** `RecastNavMesh` and friends after the build — `Build()` spawns a missing one in editor mode. */
	int32 NavDataActors = 0;

	/** Wall time, which for a level of any size is the reason a caller wanted a progress report. */
	double Seconds = 0.0;

	/** True when the mesh answers nothing afterwards: built, and empty. */
	bool bEmpty = false;
};

/** One point, asked of the ground. */
struct GRAPHCODE_API FGcNavPoint
{
	bool bNavigable = false;

	/** Where the mesh actually is, which is rarely where the caller asked. */
	FVector Projected = FVector::ZeroVector;

	/** How far the projection moved it. A large drop is a floor below the one the caller meant. */
	double Moved = 0.0;
};

/**
 * Is B reachable from A.
 *
 * `bPartial` IS NOT A DETAIL, it is the whole reason this is a struct. Recast answers a query for an
 * unreachable goal with a path that goes as far as it can and `Success`, unless partial paths are
 * refused — `RecastNavMesh.cpp:3394` picks `Success` or `Fail` off exactly that flag. So *is there a
 * path* has a true answer and a plausible one, and the plausible one is the default.
 */
struct GRAPHCODE_API FGcNavReach
{
	/** The real answer: a complete path exists from A to B. */
	bool bReachable = false;

	/** A path exists but stops short of the goal — reported so a caller sees HOW it failed. */
	bool bPartial = false;

	/** Both ends, projected. A `false` on either is why an unreachable answer is usually unreachable. */
	FGcNavPoint From;
	FGcNavPoint To;

	/** Along the navmesh, and through the air. Their ratio is how far around the route goes. */
	double PathLength = 0.0;
	double StraightLine = 0.0;

	/**
	 * WHERE THE ROUTE ACTUALLY WENT — the string-pulled corners, start and goal included.
	 *
	 * A LENGTH SAYS *HOW FAR AROUND*; ONLY THE ROUTE SAYS *THROUGH WHAT*. The connectivity build asks
	 * whether a wall is the way through, and the honest form of that question is whether the route
	 * passes through that wall — not whether it is short enough to have probably done so, which is a
	 * constant somebody has to tune and which answers the same for a detour and for a long room.
	 *
	 * Empty when nothing was reachable, and empty is not the same as *it went nowhere*: a query that
	 * never ran leaves it empty too, so read `bReachable` first.
	 */
	TArray<FVector> Route;

	/** Sentence form, for a caller that is going to print one anyway. */
	FString Note;
};

/**
 * WHICH NAVMESH ANSWERED, and the two failures are DIFFERENT KINDS OF ANSWER — §7 N1b.
 *
 * A named mesh resolves to null two ways and a caller that cannot tell them apart reports the wrong
 * thing for one of them. *You named a body this world does not have* is the caller's mistake and a
 * refusal; *that body is declared and nothing has been built for it yet* is the level's state and a
 * measurement that could not run. It is the same resolve-versus-measure line `gcw_ask` draws for an
 * anchor: a subject that cannot be RESOLVED is an error, and a subject that resolves with a facet
 * that cannot be MEASURED is an answer with a hole in it.
 */
enum class EGcNavMesh : uint8
{
	/** Found, and `OutData` is the navmesh generated for that body. */
	Resolved,

	/** No such body among this world's Supported Agents. Refuse and name what it does declare. */
	Undeclared,

	/** Nothing to ask: no navigation system, or a declared body with no navmesh generated for it. */
	Unbuilt,
};

/** `Standable` over a set of points a caller chose. */
struct GRAPHCODE_API FGcNavArea
{
	int32 Asked = 0;
	int32 Navigable = 0;

	/** The navigable ones, in the order they were asked, so a caller can path from one. */
	TArray<FVector> Points;

	bool IsStandable() const { return Navigable > 0; }
};

/**
 * The world half of navigation. Static, because a nav system is a subsystem of a world that can be
 * created after any handle to it would have been — the same reason `FGraphCodeLevelNavigation` holds
 * a world and looks the system up per call rather than caching it.
 */
class GRAPHCODE_API FGraphCodeNavigation
{
public:
	/**
	 * Does this world have navigation data that can be queried at all?
	 *
	 * FALSE IS THREE DIFFERENT FACTS and callers that need to tell them apart should call `Build`
	 * with `bDryRun`: no navigation system, a system with no nav data actor, and a nav data actor
	 * whose mesh is empty all answer `false` here on purpose, because every query below is equally
	 * meaningless in all three.
	 *
	 * A named `MeshId` that this world does not declare answers `false` too — a caller that needs to
	 * tell *no such body* from *nothing built for it* calls `ResolveMesh`, which is the only place
	 * that distinction exists.
	 */
	static bool HasNavigation(const UWorld* World, FName MeshId = NAME_None);

	/**
	 * Which navmesh a `MeshId` names, and how it failed when it names none.
	 *
	 * `NAME_None` IS THE LEVEL'S DEFAULT and not a fourth state: every caller written before N1b asked
	 * the default instance, so omitting the mesh has to keep meaning exactly that.
	 *
	 * THE ID IS THE EDITOR'S SUPPORTED AGENT NAME, which is the same string a `.gcw` already writes on
	 * the left of a `nav()` line — `FNavDataConfig::Name`. There is no second vocabulary to learn and
	 * no mapping to maintain: `Mouse = nav(type=walkable, radius=8, height=12)` declares the body, and
	 * `mesh:'Mouse'` asks the navmesh that declaration generated.
	 *
	 * DECLARED IS READ OFF THE SUPPORTED AGENTS AND NOT OFF THE NAV DATA, deliberately. A body that is
	 * declared but whose mesh has never been generated is the ordinary state of a level nobody has
	 * built yet, and answering *no such body* there would send an author to fix a spelling that is
	 * already right. An agent the mask has invalidated is not declared: `UNavigationSystemV1` blanks a
	 * masked-out entry's class in place rather than removing it (`NavigationSystem.cpp:6227`), so the
	 * list holds entries that cannot generate anything, and `IsValid()` is what tells them apart.
	 */
	static EGcNavMesh ResolveMesh(const UWorld* World, FName MeshId, const ANavigationData*& OutData);

	/**
	 * Every body this world declares, in the editor's own order — for a refusal that names the way out.
	 *
	 * Empty means two things and the caller already knows which: a world with no navigation system at
	 * all, and one whose agents are all masked off. Neither can answer a `mesh:`, so neither needs its
	 * own sentence here.
	 */
	static TArray<FString> MeshNames(const UWorld* World);

	/**
	 * Generate the navmesh.
	 *
	 * NO WAIT LOOP, AND THAT IS MEASURED RATHER THAN ASSUMED: `UNavigationSystemV1::Build()` ends
	 * with `EnsureBuildCompletion()` on every nav data (`NavigationSystem.cpp:4529`, under its own
	 * comment *block until build is finished*), and Recast's implementation of that pumps tile tasks
	 * until none remain. A timeout and a remaining-task count would be reporting on machinery that
	 * has already finished by the time the call returns.
	 *
	 * WHAT THIS ADDS to that call is everything the engine decides in silence: `Build()` returns
	 * `void` and opens with two early returns, it reads registered bounds a just-spawned volume has
	 * not reached, and it can complete over an empty bounds set and leave a mesh that answers no.
	 *
	 * `bDryRun` reports what a build WOULD do and touches nothing — not even the missing navigation
	 * system, which a real build creates. It is how a caller asks why `HasNavigation` is false.
	 */
	static bool Build(UWorld* World, bool bDryRun, FGcNavBuildResult& OutResult, FString& OutError);

	/**
	 * Make navigation forget actors that have been destroyed. Call after deleting, before building.
	 *
	 * A FIFTH ENGINE CALL THAT ANSWERS THE WRONG QUESTION, and this one was measured rather than
	 * read: **`UWorld::DestroyActor` takes an actor out of the level and leaves its element in the
	 * navigation octree**, so the next build rasterises geometry that no longer exists. A `.gcw` that
	 * drops a `type=blocked` region destroys the volume it owns — the apply reports `removed 1` —
	 * and the rebuilt mesh still refuses to cross where the blocker used to be, with receipts that
	 * read perfectly clean because every input they hash really did change back.
	 *
	 * `GraphCode.World.Navigation.CrossingsFollowTheDoor` is the spec that found it, and the octree
	 * query in that world is the evidence: after the delete and the rebuild, an element with one area
	 * modifier is still sitting in the doorway with its source object already collected.
	 *
	 * **TWO NARROWER FIXES WERE TRIED AND MEASURED FAILING, which is why this one is so blunt.**
	 * (1) `FNavigationSystem::UnregisterComponent` and `OnActorUnregistered` on the actor and every
	 * component, while all of them were still alive and fully registered — the door the engine's own
	 * teardown uses — left the element exactly where it was. (2) Removing it by handle through
	 * `UNavigationSystemV1::RemoveNavigationElement` ensures instead: the OCTREE answers
	 * `GetDataForElement` for that handle while the object repository has no record of it, so the two
	 * registries disagree and the public removal door only reaches the one that is already empty.
	 *
	 * So the octree is rebuilt instead of edited. Everything that still exists is re-gathered from
	 * the levels and the repository, and everything that does not is dropped by construction.
	 *
	 * IT COSTS A LEVEL RE-GATHER, so it is priced like a build and not like a query: call it when an
	 * apply actually destroyed something, never on the way into an ordinary read.
	 */
	static void ForgetDestroyed(UWorld* World);

	/** The nearest navigable point within `Extent`, and how far that moved it. */
	static bool Project(const UWorld* World, const FVector& Point, const FVector& Extent,
		FGcNavPoint& OutPoint, FName MeshId = NAME_None);

	/**
	 * A COMPLETE path from A to B, over one of the level's navmeshes.
	 *
	 * PER BODY, NOT PER CHARACTER, and that is where §0's seam actually falls. This shipped hardcoded
	 * to the default instance under the reason *which areas a particular character may use is a nav
	 * FILTER, a fact about who is asking, and Move's* — true of filters, and one word too wide as a
	 * reason to hardcode the mesh. A mouse and a human are not one mesh read through two filters:
	 * Unreal generates a separate one per agent size out of the level's own geometry, and taking the
	 * mouse out of the level leaves the mouse-sized navmesh exactly where it was. That makes it a fact
	 * about the ground, so it is answerable here (§7 N1b).
	 *
	 * A body this world does not declare is a REFUSAL through `OutError`, not an unreachable answer —
	 * see `ResolveMesh`. Omitted, this is the level's default agent and the question it always asked.
	 */
	static bool Reach(const UWorld* World, const FVector& From, const FVector& To,
		FGcNavReach& OutReach, FString& OutError, FName MeshId = NAME_None);

	/** How many of these points are on the mesh. The caller picked them; see the file header. */
	static bool Standable(const UWorld* World, TArrayView<const FVector> Points,
		const FVector& Extent, FGcNavArea& OutArea, FName MeshId = NAME_None);

	/** The extent a projection uses when a caller states none — half a metre out, two up. */
	static FVector DefaultExtent();

private:
	/** The world's nav system, or null. Never creates one: a query may not change the world. */
	static UNavigationSystemV1* SystemOf(const UWorld* World);

	/** The default nav data, or null. */
	static const ANavigationData* DataOf(const UNavigationSystemV1* System);
};
