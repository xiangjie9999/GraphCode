// Copyright OraStudio. All Rights Reserved.
//
// Realizing a `terrain { }` program onto a landscape — `GC-TERRAIN-DESIGN.md` §5, §12's L1 and L3.
//
// **THE PROGRAM IS CORE'S AND THE LANDSCAPE IS WORLD'S, and this file is the seam.**
// `FGraphCodeTerrain::Evaluate` is pure arithmetic over centimetres and knows nothing about
// landscapes; that is deliberate, and it is what makes a program a pure function of its text. Here is
// where the numbers meet an actor: the grid says how big the field is, the actor's scale says what a
// centimetre is worth in the engine's `uint16`, and the edit layer says the human's sculpt underneath
// is not being touched.
//
// **THE CONVERSION IS THE PART TO GET RIGHT AND THE ENGINE'S OWN HELPER GETS IT WRONG FOR US.**
// `LandscapeDataAccess::GetTexHeight` CLAMPS — a program asking for 6 km on a landscape that can hold
// 256 m comes back flattened at the ceiling, silently, and the file still says 6 km. So the quantiser
// here reports what does not fit instead, by name and with the number.

#pragma once

#include "CoreMinimal.h"
#include "GraphCodeTerrain.h"
#include "GraphCodeTerrainRealize.h"

class ALandscape;
class UWorld;
struct FGcwDocument;

/**
 * A landscape's CREATION-TIME grid, as plain numbers.
 *
 * §1 measured that `ComponentSizeQuads`, `SubsectionSizeQuads` and `NumSubsections` carry no
 * `EditAnywhere` and the engine's own comment on them is *"Data set at creation time"* — so this is
 * read off an actor and never written back, and changing it is a regenerate rather than a patch
 * (user-ruled).
 *
 * **A STRUCT OF NUMBERS AND NOT THE ACTOR, so every piece of arithmetic below is testable without an
 * editor, a landscape or a render device.** The half of realization that can be wrong quietly is the
 * conversion, and the conversion needs nothing but these.
 */
struct GRAPHCODEWORLD_API FGcTerrainGrid
{
	/** How many components across and up. */
	int32 ComponentsX = 1;
	int32 ComponentsY = 1;

	/** Quads per component edge — 63 for the engine's default. */
	int32 ComponentSizeQuads = 63;

	int32 SubsectionSizeQuads = 63;
	int32 NumSubsections = 1;

	/** The actor's own transform. Location gives Z zero; scale gives what one quad and one unit mean. */
	FTransform Actor = FTransform::Identity;

	/** Vertices, not quads — a 1x1 component grid of 63 quads is 64 vertices across. */
	int32 VertsX() const { return ComponentsX * ComponentSizeQuads + 1; }
	int32 VertsY() const { return ComponentsY * ComponentSizeQuads + 1; }

	/** What one step of the grid is worth in world centimetres. */
	double QuadSize() const { return Actor.GetScale3D().X; }

	/** The Z any centimetre is measured from, and what one `uint16` step is worth in centimetres. */
	double BaseZ() const { return Actor.GetLocation().Z; }
	double HeightStep() const;

	/** The lowest and highest world Z this landscape can represent at all. */
	double LowestZ() const;
	double HighestZ() const;

	bool IsValid() const;
};

/**
 * What `stream:tiles(components=N)` asks for, in pieces — §12's L6.
 *
 * A landscape is ONE ACTOR by default, so a program a kilometre across loads whole to draw one hill.
 * Cutting it into `ALandscapeStreamingProxy` actors changes nothing about the document: the
 * instructions, domains and filters are still written against the whole ground.
 *
 * **A STRUCT OF NUMBERS, for the same reason `FGcTerrainGrid` is** — the arithmetic of *how many
 * pieces* is testable with no editor, no landscape and no World Partition.
 */
struct GRAPHCODEWORLD_API FGcTerrainStreamPlan
{
	/** Components per tile edge, straight from the program. Zero means the header did not ask. */
	int32 Components = 0;

	/** How many tiles the landscape's own extent makes at that size. */
	int32 TilesX = 0;
	int32 TilesY = 0;

	int32 Tiles() const { return TilesX * TilesY; }

	/**
	 * The landscape is ALREADY cut this way, so this run has nothing to do.
	 *
	 * Its own field rather than an absent plan: *nothing changed* and *nothing was asked* are
	 * different answers, and a re-run of a settled document is the common one.
	 */
	bool bAlready = false;

	/** True when this run actually moved components into proxies. False on a dry run and on a no-op. */
	bool bChanged = false;
};

class GRAPHCODEWORLD_API FGraphCodeWorldTerrain
{
public:
	/**
	 * How many tiles a grid makes at a given component count. Pure, and the whole of L6's arithmetic.
	 *
	 * ROUNDED UP, because the last tile of a row holds the remainder rather than being dropped: a
	 * 8x8-component landscape cut at 3 is three tiles across, the last holding two components.
	 */
	static FGcTerrainStreamPlan PlanStream(const FGcTerrainGrid& Grid, int32 Components);

	/**
	 * Cut a landscape into streamed pieces — or refuse, by name, on a level that cannot hold them.
	 *
	 * **THE ENGINE'S OWN CALL RETURNS SILENTLY ON A NON-PARTITIONED LEVEL.**
	 * `ULandscapeSubsystem::ChangeGridSize` opens with `if (!IsGridBased()) { return; }`
	 * (`LandscapeSubsystem.cpp:1354`), and `IsGridBased()` is `UWorld::IsPartitionedWorld`. That is
	 * the same defect shape `populate { }`'s `grid=` hit, where `SetIsPartitioned` drops the flag
	 * with no subsystem to register with — so this refuses up front rather than reporting a change
	 * that never landed ([[a-silent-no-op-is-worse-than-a-refusal]]).
	 *
	 * **IT DELETES AND RECREATES ACTORS**, which is why it takes `bDryRun` like every other write
	 * here: a plan says how many pieces it would make and touches nothing.
	 */
	static bool Stream(UWorld* World, ALandscape& Landscape, int32 Components, bool bDryRun,
		FGcTerrainStreamPlan& OutPlan, FString& OutError);

	/**
	 * A field sized and placed to match a landscape's own vertices — one sample per grid vertex.
	 *
	 * NOT A RESOLUTION WE CHOOSE. Sampling at anything but the landscape's own spacing would make the
	 * program's output depend on a number nobody wrote down, and every filter that reads slope would
	 * read a different face than the one the engine finally builds.
	 */
	static FGcTerrainField PlanField(const FGcTerrainGrid& Grid);

	/**
	 * World centimetres to the engine's `uint16`, REPORTING what does not fit.
	 *
	 * The engine's own `GetTexHeight` clamps, which turns *this landscape cannot hold your mountain*
	 * into a plateau at the ceiling that looks authored. `OutError` names the sample and the height,
	 * and nothing is written.
	 */
	static bool Quantise(const FGcTerrainField& Field, const FGcTerrainGrid& Grid,
		TArray<uint16>& OutHeights, FString& OutError);

	/** The inverse — what a stored value means in world centimetres. Exact, and its own spec. */
	static double HeightOf(uint16 Stored, const FGcTerrainGrid& Grid);

	/** Read the grid off an actor. */
	static FGcTerrainGrid GridOf(const ALandscape& Landscape);

	/**
	 * The whole run: find the landscape the program names, evaluate over its grid, and write the
	 * result to an EDIT LAYER above whatever is already there.
	 *
	 * **NEVER TO THE BASE HEIGHTMAP** (§5). A human's sculpt is the ground the program starts from and
	 * its bytes are not ours to touch; writing above it is also what makes re-running the program
	 * non-destructive rather than a one-way import.
	 */
	static bool Realize(UWorld* World, const FGcwDocument& Document, const FGcTerrainProgram& Program,
		bool bCreate, bool bDryRun, FGcTerrainRealizeResult& OutResult, FString& OutError);

	/**
	 * L3's capture half — freeze the ground a human built into a delta file the program can read.
	 *
	 * **IT MOVES THE GROUND, IT DOES NOT COPY IT** (user-ruled 2026-08-19). The bytes go into a
	 * committed file beside the `.gcw` and the landscape's own heights are flattened, so the terrain
	 * is stated exactly once — in the document ([[two-carriers-of-one-fact-is-a-race]]). Nothing is
	 * lost: the blob is the engine's own `uint16` verbatim, so `sculpt(…)` on the program's first
	 * line puts it back on the identical values. What it buys is a terrain that survives a fresh
	 * clone of a repository whose `Content/` is ignored, and a program whose `height=`/`slope=`
	 * filters can finally see the ground a person made.
	 *
	 * **THE PROGRAM'S OWN LAYER IS EXCLUDED FROM WHAT IT TAKES.** `gc_<Landscape>` is derived — it is
	 * what the last realize wrote — so capturing it would fold the program's output into the file
	 * that feeds the program, and every run would compound. What is captured is the ground that is
	 * NOT ours.
	 *
	 * **THERE IS NO COPY MODE, AND THAT IS DELIBERATE.** A flag for *take the file and leave the
	 * ground where it is* would be a second, quieter way to end up with the terrain stated twice —
	 * which is the exact state this act exists to leave. `bDryRun` reports every number and the line
	 * and writes nothing at all: not the blob, not the flatten.
	 *
	 * **AND THERE IS NO TOOL VERB EITHER** (user-ruled 2026-08-20). The act happens when a person
	 * changes the ground and saves; a `gc_apply` parameter beside that was a second door onto one
	 * thing. `FGraphCodeTerrainSave` is the caller.
	 */
	static bool RecordSculpt(UWorld* World, const FGcwDocument& Document,
		const FGcTerrainProgram& Program, const FString& DeltaName, bool bDryRun,
		FGcTerrainSculptResult& OutResult, FString& OutError);

	/**
	 * L3's creation door — make a landscape that does not exist yet.
	 *
	 * Separate and public because it is a different KIND of act: realizing edits, and this brings an
	 * actor into the world. A caller has to ask for it.
	 */
	static ALandscape* Create(UWorld* World, const FString& Label, const FGcTerrainGrid& Grid,
		FString& OutError);

	/**
	 * The shape a landscape gets when nobody has said one — the engine's own default new landscape.
	 *
	 * **A DEFAULT RATHER THAN A GRAMMAR** ([[a-new-token-not-a-new-line]]). §7's `@grid[…]` is the
	 * record's spelling and it is READ-ONLY by ruling, so a size stated in a `.gcw` would be a second
	 * place the grid lives; nothing in the corpus has asked for a specific one yet. When something
	 * does, it joins `@grid[…]` on `kind:landscape`, not the terrain header.
	 */
	static FGcTerrainGrid DefaultGrid();
};
