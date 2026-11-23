// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

namespace UE::Geometry { class FDynamicMesh3; }

/**
 * **WHICH FACES A `[ ]` MODIFIER MEANS, WRITTEN WHEN THE FACE WAS MADE** — `GC-SHAPE-GRAMMAR.md` §5.
 *
 * **THIS IS PROVENANCE, NOT MEASUREMENT.** We know which faces are the cap a depth instruction
 * closed because we are the ones who closed it; nothing here looks at a normal, a height or a
 * winding to decide. That is the whole reason a tag can survive a `bend` that turns a top cap
 * sideways, and it is why §5 can say *a tag never moves*.
 *
 * **THE FOUR VALUES ARE POLYGROUP IDS 0..3 AND THAT IS LOAD-BEARING.** `FDynamicMeshEditor::
 * AppendMesh` — which is what every boolean merges through — offsets the appended mesh's groups
 * past the subject's `MaxGroupID()`, so after a boolean *any group above 3 came from the tool*.
 * `TagBooleanResult` is that one sentence and nothing else, which is why it needs to know neither
 * the offset nor the operation.
 */
enum class EGcShapeFace : uint8
{
	/** The cap a depth instruction started FROM. */
	Bottom = 0,
	/** The cap a depth instruction CLOSED. */
	Top = 1,
	/** Everything a depth instruction swept between the two. */
	Side = 2,
	/** What a `subtract()` or `intersect()` made — and `cut(fill=true)`, the same kind of face. */
	Cut = 3,
};

namespace GraphCodeShapeFace
{
	/** Every group id at or below this came from us; anything above it was appended. */
	constexpr int32 Highest = static_cast<int32>(EGcShapeFace::Cut);

	/** The word §5 spells this face with, for a refusal that has to name one. */
	GRAPHCODESHAPE_API const TCHAR* Word(EGcShapeFace Face);

	/** `top` `bottom` `side` `cut` — or false, which is how the modifier parser tells a word apart. */
	GRAPHCODESHAPE_API bool FromWord(const FString& Word, EGcShapeFace& OutFace);

	/**
	 * Whether a polygroup id is one of §5's four — the invariant every operation that ADDS geometry
	 * has to be swept for, because `FMeshBevel` and `FOffsetMeshRegion` both allocate fresh ids and
	 * neither reports all of them.
	 */
	inline bool IsFaceGroup(const int32 Group)
	{
		return Group >= 0 && Group <= Highest;
	}

	/** Every triangle in the mesh, tagged the same. Enables groups if they are not on yet. */
	GRAPHCODESHAPE_API void TagAll(UE::Geometry::FDynamicMesh3& Mesh, EGcShapeFace Face);

	/** Just these triangles — what the instruction that made them knows it made. */
	GRAPHCODESHAPE_API void TagThese(UE::Geometry::FDynamicMesh3& Mesh,
		const TArray<int32>& Triangles, EGcShapeFace Face);

	/**
	 * **A SWEPT TUBE, TAGGED BY THE GENERATOR'S OWN VERTEX LAYOUT** — for everything built on
	 * `FGeneralizedCylinderGenerator`, which is `extrude` and `sweep`.
	 *
	 * `Vertices[Index + Ring * CrossSectionVertices]`, MEASURED at `SweepGenerator.cpp:698` and
	 * already relied on by `extrude`'s own ring placement. **A `FlatTriangulation` cap adds NO
	 * vertices** — it triangulates over the end ring (`SweepGenerator.cpp:130`, `:176`) — so a
	 * triangle whose three corners all sit in the FIRST ring is the bottom cap, one whose corners
	 * all sit in the LAST ring is the top cap, and a side triangle can never be either because it
	 * spans two rings. `FGeneralizedCylinderGenerator::CapType` is public and defaults to
	 * `FlatTriangulation`; the caller asserts it rather than assuming it, so an engine that changes
	 * the cap fails loudly instead of tagging a lid as a wall.
	 *
	 * Safe on an UNCAPPED tube: no triangle has three corners in one ring, so everything reads
	 * `side` without the caller having to know whether caps were asked for.
	 */
	GRAPHCODESHAPE_API void TagSweptTube(UE::Geometry::FDynamicMesh3& Mesh,
		int32 CrossSectionVertices, int32 Rings);

	/**
	 * **A SURFACE OF REVOLUTION, TAGGED BY WHICH GROUPS THE ENGINE ALLOCATED LAST.**
	 *
	 * MEASURED, `RevolveGenerator.cpp:278`/`:470`/`:660`: all three revolve generators sweep the
	 * body first and then fill the two ends in a `CapIndex` loop, each end taking a FRESH
	 * `AllocateTriangleGroup()`. So the two highest group ids in the finished mesh are the caps,
	 * the lower of the two is `CapIndex == 0` — the end the sweep started from, reversed to face
	 * outward, which is `bottom` — and the higher is `top`.
	 *
	 * `bHasTwoCaps` is the engine's own condition read at our call site rather than guessed:
	 * `bProfileCurveIsClosed && !bSweepCurveIsClosed && CapFillMode != None` (`:231`). A closed
	 * outline carried less than the whole way round has two; a full turn closes on itself and has
	 * none. **The revolved OPEN path is the one case still unmeasured** — pass false and every
	 * face reads `side`, which is honest about what is known rather than confident about what is
	 * not.
	 */
	GRAPHCODESHAPE_API void TagRevolved(UE::Geometry::FDynamicMesh3& Mesh, bool bHasTwoCaps);

	/**
	 * **AFTER A BOOLEAN, EVERYTHING THE TOOL CONTRIBUTED IS `cut`.**
	 *
	 * The subject's own faces keep the tags they were made with, which is what makes *the block's
	 * faces are `top`/`bottom`/`side` while the tunnel wall is `cut`* true without anybody
	 * measuring where the tunnel is. Reads only *is this group above ours*, so it does not care
	 * what offset the append chose or which of the three operations ran.
	 */
	GRAPHCODESHAPE_API void TagBooleanResult(UE::Geometry::FDynamicMesh3& Result);

	/**
	 * **AFTER A UNION, THE OTHER ENTRY'S FACES KEEP THEIR OWN TAGS** — §5: *`add($Other)` brings the
	 * other entry's faces in WEARING THEIR OWN TAGS*, so after `add($Plinth)` a `[top]` means both
	 * caps. That is the opposite of `TagBooleanResult`, and the difference is the whole reason
	 * `add` is not spelled the same way as `subtract`.
	 *
	 * `OwnFaceForGroup` maps a group id IN THE RESULT to the face the tool built it as — the
	 * engine's own `FMeshBoolean::SecondMeshGroupMap` read backwards. A result group the map does
	 * not name is the subject's own and is left exactly alone.
	 *
	 * **IT TAKES A MAP BECAUSE ARITHMETIC CANNOT DO THIS, AND THAT IS MEASURED — MB30, 2026-09-12.**
	 * ~~*`AppendedBase` is the subject's `MaxGroupID()` read before the boolean, the offset
	 * `AppendMesh` shifts the tool's groups by, so subtracting it recovers the tag.*~~ There is no
	 * offset: `FDynamicMeshEditor::AppendMesh` allocates a FRESH group id per incoming group **on
	 * first encounter, in triangle order**, so the tool's ids arrive PERMUTED rather than shifted.
	 * On an ordinary box encountered side-first the permutation is a rotation, and subtracting the
	 * base then renames every face to its neighbour: `side` -> `bottom`, `bottom` -> `top`,
	 * `top` -> `side`. Two boxes added apart measured `top 4 / bottom 18 / side 18` where 4/4/32
	 * is correct, and that arithmetic accounts for all forty triangles exactly.
	 *
	 * **AND THE SPEC THIS COMMENT USED TO NAME AS ITS GUARD IS WHY IT SURVIVED SO LONG**: *a union
	 * whose plinth keeps its `top`* stays green under a rotation, because the plinth's BOTTOM
	 * arrives wearing `top` and `[top]` duly selects something. A predicate that asks whether a
	 * tag exists cannot see the tags being permuted underneath it
	 * ([[a-gate-that-checks-identity-cannot-see-an-arrangement]]); the assertion is a face COUNT
	 * now.
	 */
	GRAPHCODESHAPE_API void TagUnionResult(UE::Geometry::FDynamicMesh3& Result,
		const TMap<int32, int32>& OwnFaceForGroup);

	/** How many triangles carry this tag — what a spec asserts, and what `[rim]` cannot have. */
	GRAPHCODESHAPE_API int32 CountOf(const UE::Geometry::FDynamicMesh3& Mesh, EGcShapeFace Face);
}
