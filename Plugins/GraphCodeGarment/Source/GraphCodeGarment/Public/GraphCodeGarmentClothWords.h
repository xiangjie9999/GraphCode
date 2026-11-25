// Copyright GraphCode Sdn Bhd. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * The runtime cloth of an `[as=cloth]` build said in the recipe (G1 slice 2, skirt S16; the lead's rulings of 17 Nov, "the runtime cloth as keys
 * on `hang()`"): `hang(from=$Hips, damping=0.05, local_damping=0.3, iterations=12, substeps=4, ccd, self=0.1cm, repair)`, and (19 Nov) the
 * simulation's spacing, `spacing=1.25cm`, a build-time word the asset's mesh is made at. `hang()` is the one word
 * only `[as=cloth]` reads, so the keys reach exactly the game cloth. Every field unset or false = NOT WRITTEN: the asset keeps what the build writes
 * without it (today's 8 / 16 iterations, 2 substeps, self-collision 0.2 x the spacing; the engine's damping 0.01, local damping 0, CCD and repair
 * off), so no file that does not write a key changes. Written into the asset by `GcGarmentClothAsset::ApplySolverWords`.
 *
 * Ranges are the ENGINE'S own clamps (`UPROPERTY` meta of `UChaosClothConfig` / `UChaosClothSharedSimConfig`, UE 5.7
 * `Plugins/ChaosCloth/Source/ChaosCloth/Public/ChaosCloth/ChaosClothConfig.h`), never picked (check G1-10): a value outside one is refused.
 */
struct FGcGarmentClothSolverWords
{
	/** `damping=` and `local_damping=`: 0 to 1 (`DampingCoefficient` line 287, `LocalDampingCoefficient` line 296: ClampMin 0, ClampMax 1). */
	static constexpr double LeastDamping = 0.0;
	static constexpr double MostDamping = 1.0;

	/** `iterations=` and `substeps=`: 1 to 100 (`IterationCount` 561, `MaxIterationCount` 568, `SubdivisionCount` 575: ClampMin 1, ClampMax 100). */
	static constexpr int32 LeastCount = 1;
	static constexpr int32 MostCount = 100;

	/** `self=<length>`: above 0 (0 is `self=off`), at most 1000 cm (`SelfCollisionThickness` 244: ClampMax 1000). */
	static constexpr double MostSelfCm = 1000.0;

	/**
	 * `spacing=` (19 Nov): 1 to 2.5 cm, both the garment's own, never picked (static_asserts in Build.cpp hold them to their sources).
	 * Least, 1 cm: the cloth's own points (`GcGarmentKit::Spacing`); the simulation mesh is made from the hung cloth at that spacing
	 * (`ClothMiddle`), so finer only splits the cloth's own triangles - points with no more of the garment in them. Most, 2.5 cm: the build's
	 * own (`SimulatedSpacingCm`, Build.cpp) and its hang's, which settles the garment and lays its gathers on that lattice (Drape.cpp
	 * `GcGarmentDrape::SimulatedSpacingCm`); coarser could not carry the folds the build's own hang made. The engine's own limit is a count,
	 * not a length: at most 65536 simulated points a drawn garment (`FMeshToMeshVertData`), refused by name at the build (`AddSimulation`).
	 */
	static constexpr double LeastSpacingCm = 1.0;
	static constexpr double MostSpacingCm = 2.5;

	/** `damping=`: the whole cloth's velocity damping (`DampingCoefficient`). Unset: the engine's 0.01. */
	TOptional<double> Damping;

	/** `local_damping=`: each point's velocity pulled toward its whole cloth's rigid motion (`LocalDampingCoefficient`). Unset: the engine's 0. */
	TOptional<double> LocalDamping;

	/**
	 * `iterations=`: solver iterations a 60 Hz step (`IterationCount`). Chaos scales them with the step and clamps them to the cap
	 * (`ChaosClothingSimulationSolver.cpp` 2194-2209), so the cap moves with them by the build's own rule, 8 at 60 Hz is 16 at 30 Hz
	 * (`AddSimulation`): twice the count, at most the engine's 100. Unset: the build's 8, cap 16.
	 */
	TOptional<int32> Iterations;

	/** `substeps=`: steps a frame, each detecting collisions again (`SubdivisionCount`). Unset: the build's 2. */
	TOptional<int32> Substeps;

	/** `ccd`: sweep each point against the body's colliders (`bUseCCD`; the body only, never cloth against cloth). Unwritten: off (engine). */
	bool bCcd = false;

	/** `self=off`: the cloth never meets itself (`bUseSelfCollisions` false). */
	bool bSelfOff = false;

	/** `self=0.25cm`: the self-collision distance around each point, cm (`SelfCollisionThickness`; a pair is kept the two added together apart).
	 *  Unset: the build's max(half the cloth's thickness, 0.2 x the spacing). */
	TOptional<double> SelfCm;

	/** `repair`: cloth that has passed through itself is found and put back (`bUseSelfIntersections`). Unwritten: off (engine). */
	bool bRepair = false;

	/**
	 * `spacing=1.25cm` (19 Nov; her wrap skirt's S6): about how far apart the game cloth's simulated points are, cm. The drawn garment keeps its
	 * own detail, each drawn point bound to one simulated triangle, so a drawn edge bends only where one triangle hands over to the next. Unset:
	 * the build's 2.5 cm (`MostSpacingCm`). The build's own self-collision, a fifth of the spacing, follows it (ClothAsset.cpp `AddSimulation`).
	 */
	TOptional<double> SpacingCm;

	/** No key written: the asset is the build's without words, byte for byte. */
	bool IsEmpty() const
	{
		return !Damping.IsSet() && !LocalDamping.IsSet() && !Iterations.IsSet() && !Substeps.IsSet() && !bCcd && !bSelfOff && !SelfCm.IsSet() && !bRepair
			&& !SpacingCm.IsSet();
	}

	/** The spacing the game cloth is simulated at: the written one, or the build's own. */
	double SimulatedAtCm() const
	{
		return SpacingCm.Get(MostSpacingCm);
	}

	/** The cap `iterations=N` writes: twice N (8 at 60 Hz is 16 at 30 Hz, the build's rule), at most the engine's 100. */
	static int32 CapFor(const int32 Count)
	{
		return FMath::Min(2 * Count, MostCount);
	}
};
