// Copyright OraStudio. All Rights Reserved.

// The placement resolver — `GC-SPATIAL-AUTHORING-DESIGN.md` §20, task P0.
//
// `@anchor` / `@surface` / `@facing` / `@scale` in, one transform out. Pure arithmetic over the
// structs below: no world, no level, no actor. Same contract as `GraphCodeSpatial.h` and for the same
// reason — a caller resolves names to `FGcPlacementTarget` values and asks, so every ruling in §20 is
// assertable with no map loaded.
//
// WHAT A TARGET IS. An oriented box and a frame, which is §19.5's ruling: a thing is declared by its
// box extent, and a target without one is a point you measure FROM rather than a thing you snap TO.

#pragma once

#include "AuthoringCoreTypes.h"
#include "CoreMinimal.h"
#include "GraphCodeSpatial.h"

/**
 * Refusal tokens that more than one spatial file raises — defined ONCE, here.
 *
 * `anchor_has_no_volume` is deliberately the same word in the query surface and in the placement
 * resolver: §20.19 ruled *"Z2's name for it, and Z2's name for it stays"*, because a caller meeting
 * the refusal should not have to know which half answered. Both files spelled it with their own
 * anonymous-namespace constant to achieve that — **two things that must agree, with nothing making
 * them agree**, so a rename in one is silent in the other ([[deleted-in-one-place-shipped-in-many]]).
 *
 * **AND IT BROKE THE BUILD TWICE.** An anonymous namespace stops external linkage, not a second
 * definition in the same translation unit, and a unity blob is what decides where "the same
 * translation unit" ends: `GraphCodeSpatialQuery.cpp` and `GraphCodeSpatialPlacement.cpp` land in one.
 * `1fd6692` was the first collision (`Refuse`), fixed by renaming; this is the second on the same
 * pair of files, so the second fix removes the duplicate instead of renaming past it.
 */
namespace GcSpatialRefusal
{
	inline const TCHAR* const AnchorHasNoVolume = TEXT("anchor_has_no_volume");
}

/**
 * Which of §20.6's three spellings the anchor's pair was written in.
 *
 * They state the same two numbers and differ only in what factors cheaply, so input takes any one and
 * a writer canonicalises to the declared one — a regenerator choosing from what the file said last
 * time would make the bytes depend on the file's history.
 */
enum class EGcPlacementSpelling : uint8
{
	/** `forward=` / `right=`, centimetres. Rows and grids. */
	Cartesian,

	/** `bearing=` degrees clockwise from the target's forward, `distance=` centimetres. Rings. */
	Polar,

	/** `forward=90%` — a fraction of the target's half-extent, so it needs a box. */
	Percent
};

/**
 * `align=(right: self=0%, up: self=0%)` — WHICH PART OF THE SUBJECT sits on the anchored point, per axis
 * of the anchor's frame. §22.18, and it is Shape's `align=` under the one reading a World position has.
 *
 * **SHAPE'S `self=`, IN SHAPE'S CURRENCY** (`GC-SHAPE-GRAMMAR.md` §4.1): a fraction of the subject's OWN
 * span along that axis, 0 its low end, 1 its high end, 0.5 the centre every unaligned axis already has.
 * Not the ±100% of a half-extent that `@anchor[forward=45%]` takes — that one is a fraction of the
 * TARGET, and two currencies meaning different things on one line is what §22.12a's *same value forms*
 * rules out.
 *
 * **ONLY THE SELF HALF, FOR THE REASON SHAPE GIVES UNDER `along=`**: the anchored point is a POINT — the
 * target's midpoint moved by `forward`/`right`/`up` — so a fraction of the target has nothing to be a
 * fraction of here. Where on the target to measure from is already `forward=45%`.
 */
struct GRAPHCODE_API FGcPlacementAlign
{
	/** Per frame axis: forward, right, up — the same three words the offsets use. */
	bool bStated[3] = { false, false, false };

	/** 0..1 along each stated axis. */
	double Self[3] = { 0.5, 0.5, 0.5 };

	bool IsStated() const { return bStated[0] || bStated[1] || bStated[2]; }
};

/** `@anchor[target=…]` — where it stands, from the target's MIDPOINT (§20.2). Required (§20.0). */
struct GRAPHCODE_API FGcPlacementAnchor
{
	/** An actor name, or `world` — §20.8's spelled-out absolute frame. Empty when `xyz=` was written. */
	FString Target;

	/**
	 * `in=Dungeon_01` — somewhere inside its volume, uniform. `walkable=` is the same with the domain
	 * narrowed to the navmesh. §20.19, task Z4.
	 *
	 * A KEY ON `@anchor` AND NOT A DIRECTIVE, which is §20.7's ruling one directive over: the word on
	 * the left says what the reference MEANS, so there is no mode field to lose in an edit. `@anchor`
	 * stays required and complete on its own, where a `@region` directive would have had to relax that,
	 * and two keys naming one reference cannot both be written — so the exclusion needs no refusal.
	 *
	 * **A PERCENTAGE STATES THE POINT; A REGION DELEGATES IT.** That is the whole of what Z4 adds:
	 * `@anchor[target=Dungeon_01, forward=-45%]` — *the far corner of the dungeon* — already ships.
	 */
	FString Region;
	bool bWalkable = false;

	/**
	 * `seed=` — a SALT and never a requirement, because a required one is a hazard rather than a help.
	 *
	 * §8 and C18's second-pass-zero rule need one document to produce one set of transforms, so a sample
	 * needs a seed that lives in the file. **The ENTRY'S NAME is already in the file, already unique, and
	 * is already what an author changes when they mean a different minion** — `Goblin_01`, `Goblin_02`,
	 * `Goblin_03` scatter with nothing written. Requiring a number would put the author back to choosing
	 * ones that differ, which is a coordinate wearing a hat: three goblins sharing `seed=7` land on the
	 * same point.
	 *
	 * The name is always mixed in, so two entries can never collide however the salt is written.
	 */
	int32 Seed = 0;
	bool bHasSeed = false;

	bool HasRegion() const { return !Region.IsEmpty(); }

	EGcPlacementSpelling Spelling = EGcPlacementSpelling::Cartesian;

	/** Cartesian and Percent. Percent is 0–100 with the sign carrying the direction. */
	double Forward = 0.0;
	double Right = 0.0;

	/**
	 * Polar. `bearing` is degrees clockwise from the target's forward; `offset` is how far out.
	 *
	 * **`offset` AND NOT `distance`, and the two words split by WHAT THEY MEASURE FROM** (user-directed
	 * §20.15a): everything on `@anchor` is measured from the anchor's midpoint and is an OFFSET;
	 * everything on `@surface` is measured from a surface and is a DISTANCE. `forward`/`right`/`up` are
	 * offsets too, so the polar pair is not a special case — it is the same word for the same thing.
	 */
	double Bearing = 0.0;
	double Offset = 0.0;

	/**
	 * `xyz=(1200,500,220)` — a MEASURED frame, and the other way to spell a target.
	 *
	 * A named target supplies a frame you measure `forward`/`right`/`up` in; so does this, spelled with
	 * numbers because it came off a photograph rather than out of the outliner. **It is `target=world`
	 * with the origin moved**: identity rotation, no volume, so the three offsets stay world axes and a
	 * percentage still refuses for want of a box.
	 *
	 * THE AXES STAY WORLD AXES DELIBERATELY, and the alternative was tried on paper first. Building a
	 * frame out of the normal would make `up` mean *off the surface*, so `up=10` would be ten out from a
	 * wall rather than ten higher up it. On a vertical wall — the case this exists for — world up already
	 * IS up the wall, and inventing an axis convention to serve a ramp nobody has asked about would cost
	 * every ordinary line its obvious reading.
	 */
	bool bHasPoint = false;
	FVector Point = FVector::ZeroVector;

	/**
	 * `normal=(0,-1,0)` — the surface's OUTWARD normal, exactly as `screen_to_world` hands it back.
	 *
	 * It answers `@surface`'s direction without the author restating it as a word. `toward=forward`
	 * reads the OBJECT's own frame (`Oriented.GetAxisX()`), so it means *into the wall* only once a
	 * `@facing` has pointed the object at the wall — one directive written purely to give another a
	 * reference. A measured normal says it once.
	 *
	 * **Outward, and the resolver negates it.** The sign that reaches the sweep is the direction of
	 * TRAVEL, which is into the surface; asking the author to flip a number the engine just gave them is
	 * a bug generator. An explicit `toward=` still wins, because a stated word beats a measured default.
	 */
	bool bHasNormal = false;
	FVector Normal = FVector::ZeroVector;

	/**
	 * The vertical. It is the SWEEP'S STARTING POINT and not a competing answer to `@surface`.
	 *
	 * **`up_with_surface` is deleted, and the argument that removed it is measurable** (user-directed
	 * 2026-08-11, §20.21b): `Query.From = Anchored`, so the height the anchor states is where the sweep
	 * begins, and one actor can present more than one surface along the sweep's axis. A bookshelf is
	 * five shelves; starting at 95 and sweeping down lands on a different one than starting at 30, and
	 * `up` is the only thing in the file that says which. The refusal was reading a starting position as
	 * a second answer and throwing away an input the resolver consumes.
	 *
	 * On a plain floor it is genuinely inert — `up=12` and `up=500` land in the same place. That is a
	 * legibility cost rather than a correctness one, and the evidence line reports what the anchor set
	 * and what the surface then did to it.
	 */
	double Up = 0.0;
	bool bHasUp = false;

	/**
	 * `up=50%` — halfway up the target.
	 *
	 * SEPARATE FROM `Spelling`, and this is a reading of §20.6 rather than a restatement of it. That
	 * section refuses more than one spelling *of the pair*, because two spellings of one position is
	 * two things deciding it. `up` is a third number on a different axis, so a percentage there
	 * clashes with nothing — `up=150` above an altar whose pair is written in percentages is an
	 * ordinary thing to want.
	 */
	bool bUpIsPercent = false;

	/**
	 * `align=(right: self=0%)` — which part of the subject sits on the point the fields above state.
	 * §22.18. Unstated, every axis is the centre, which is what midpoint-to-midpoint (§20.2) already is.
	 */
	FGcPlacementAlign Align;
};

/**
 * Which way to look for the surface — §20.15, and it exists because the walls are often ONE ACTOR.
 *
 * **NOT A FACE NAME, AND THAT IS THE RULING** (user-directed): *the AI should not know the name of the
 * side of a mesh.* A mesh's local `forward` is an import-time artifact of how somebody authored an
 * FBX; nothing in a level shows it, so a line naming it is a guess about an asset's internals — the
 * same defect as putting the PIVOT in the arithmetic (§20.2), and §20.2 had already deleted face names
 * once. An intermediate version of this enum brought them back as `side=` and was wrong.
 *
 * **What an author does know is a DIRECTION**: gravity, their own facing, or the way to something they
 * can name. Each of the values below is one of those. The direction picks the face by meeting it — so
 * the six names never have to exist, and a room shell and a floor slab stop needing to be told apart.
 *
 * `Nearest` is the default and covers everything you are OUTSIDE of: the area-weighted patch (§20.3a)
 * picks the surface, which is what *rest on the floor* means with no more said.
 */
enum class EGcSnapDirection : uint8
{
	/**
	 * No direction stated: the nearest surface, by footprint — UNLESS the anchor carries a measured
	 * `normal=`, which answers the same question with a number instead of a word and takes precedence
	 * over the patch. A stated `toward=` still beats both.
	 */
	Nearest,

	/** Gravity, world frame — §20.5's `z`, and the only direction that needs no reference at all. */
	Down,
	Up,

	/** The OBJECT's own frame, which `@facing` has just decided — so *behind me* is a stated fact. */
	Forward,
	Back,
	Right,
	Left,

	/** Toward a named actor: *the wall on the door's side*, said without knowing which wall that is. */
	Target
};

/** `@surface[target=…]` — what it rests against. Snaps; `offset` clears; `snapToNormal` tilts. */
struct GRAPHCODE_API FGcPlacementSurface
{
	FString Target;

	/**
	 * Clear of the contact, along the patch normal. A tall lantern and a short one clear by the same.
	 *
	 * A DISTANCE because it is measured from the surface — the other half of `@anchor`'s `offset`.
	 */
	double Distance = 0.0;

	bool bSnapToNormal = false;

	/** `toward=down`, `toward=back`, `toward=BP_Door_01`. Absent means the nearest surface. */
	EGcSnapDirection Toward = EGcSnapDirection::Nearest;

	/** The actor `toward=` named, when it named one. */
	FString TowardTarget;
};

/**
 * One angle of `@facing` — degrees off the baseline, or the word `keep`.
 *
 * **THE BASELINE IS *AIMED AT IT*, ON ALL THREE AXES** (user-directed 2026-08-11, §20.7c): *look at the
 * target, look left, look at the target, look up, look at the target, rotate yourself.* One reference,
 * three departures from it, and zero on every axis is pointed straight at the thing and level.
 */
struct GRAPHCODE_API FGcFacingAxis
{
	/** Degrees off the baseline. Ignored when `bKeep`. */
	double Degrees = 0.0;

	/**
	 * The author named this axis, whatever they said about it.
	 *
	 * It is what makes a named axis beat `snapToNormal` (§20.7d), which otherwise overwrites the whole
	 * rotation and leaves a picture's roll owned by the wall it hangs on.
	 */
	bool bStated = false;

	/**
	 * `pitch=keep` — the subject's own angle on this axis, untouched.
	 *
	 * A WORLD ANGLE WHERE AN OFFSET IS AN OPERATION, and the asymmetry is the meaning rather than an
	 * inconsistency: *the streetlamp's head still points down* is a statement about the world, not about
	 * the object's own frame. So kept axes are re-imposed on the composed result as rotator components.
	 *
	 * **A WORD RATHER THAN A SCOPING RULE, and the alternative had a cliff at zero.** Letting a named
	 * axis narrow what the directive touches would mean nought keys claims three axes and one key claims
	 * one — so adding an explicit `yaw=0`, which reads as stating the default, would silently stop pitch
	 * and roll being aimed. Here a number always means a number however many keys sit beside it, and the
	 * verbosity lands on the rare intent instead of the common one.
	 */
	bool bKeep = false;
};

/** `@facing` — a rotation stated against a reference. The KEY carries it, so there is no mode field. */
struct GRAPHCODE_API FGcPlacementFacing
{
	/**
	 * `target=` or `relative=`. **EMPTY IS LEGAL**, and that is what makes §20.7d's portrait picture
	 * writeable: with no reference the baseline is the subject's own rotation, so `@facing[roll=90]` is
	 * *however it ends up pointing, rolled ninety* — an axis claimed off a surface with nothing else said.
	 */
	FString Reference;

	/** False: `target=`, aimed AT it. True: `relative=`, its own rotation as the baseline (§20.7). */
	bool bRelative = false;

	/**
	 * `toward=$Street.walk` — aimed at THE NEAREST POINT OF A RUN, which `target=` cannot say.
	 *
	 * **THE DEFECT IT EXISTS FOR WAS MEASURED, NOT IMAGINED (§22.0).** An unparameterised course
	 * answers at its MIDDLE, so `target=$Street.walk` aims every house at one spot: on a row of
	 * twenty, eighteen point wrong, and the two nearest the middle look right — which is exactly how
	 * a placement bug survives a screenshot.
	 *
	 * **A THIRD FLAG BESIDE `bHasNormal` RATHER THAN A MODE ENUM**, because `normal=` already IS this
	 * shape: a whole baseline that is neither of the first two, told apart by its KEY (§20.7). Adding
	 * a mode field would re-key what every existing reader of `bRelative` answers, for a case neither
	 * of them has.
	 *
	 * **AND IT IS NOT A SYNONYM FOR `target=`.** A reference with no run refuses rather than falling
	 * back, because the two words would then mean the same thing on every actor in the level and
	 * differ only on courses — which is a distinction discovered rather than read.
	 *
	 * Shape spells the same SHAPE `orient=tangent`, and §22.12a rules that near-identity is of the
	 * shape and not the name: one slot, taking a baseline, defaulting to the run.
	 */
	bool bToward = false;

	/**
	 * `normal=(0,-1,0)` — the baseline is LYING AGAINST a surface facing this way, and it is the third
	 * kind of reference: the same gap `xyz=` filled one directive over (§20.7e).
	 *
	 * `relative=Wall_01` already means *face the way that thing faces* — but only from an ACTOR. A
	 * caller who has MEASURED a normal off a `screen_to_world` hit had no way to hand it over, and had
	 * to reach for `snapToNormal`, which runs AFTER the snap and overwrites whatever facing decided.
	 *
	 * **IT DISSOLVES THE ORDERING PROBLEM RATHER THAN PATCHING IT.** State the orientation here and the
	 * surface only MOVES the object — so there is nothing to overwrite, no precedence rule to remember,
	 * and `toward=forward` becomes meaningful again because the object frame it reads is decided by the
	 * time a surface reads it. Outward, exactly as the hit reports it, and it builds the same basis
	 * `snapToNormal` does: one meaning, two ways in.
	 */
	bool bHasNormal = false;
	FVector Normal = FVector::ZeroVector;

	/**
	 * Derived rather than stored, and that is deliberate: a separate `bHasReference` beside a `Reference`
	 * is two fields that can disagree, and every caller that builds this struct by hand — every spec, and
	 * the level writer — would have to remember to set both. Emptiness already carries the fact.
	 */
	bool HasReference() const { return !Reference.IsEmpty() || bHasNormal; }

	/** `yaw=`, the old `angle=`. Named on its own it is a stated yaw and the other two still aim. */
	FGcFacingAxis Yaw;
	FGcFacingAxis Pitch;
	FGcFacingAxis Roll;

	bool StatesAnyAxis() const { return Yaw.bStated || Pitch.bStated || Roll.bStated; }
};

/** Every §20 directive on one entry. Absent members are absent claims, never defaults. */
struct GRAPHCODE_API FGcPlacementDirectives
{
	bool bHasAnchor = false;
	FGcPlacementAnchor Anchor;

	/** Repeatable when the snaps are INDEPENDENT — §20.3b. Two claiming one direction refuse. */
	TArray<FGcPlacementSurface> Surfaces;

	bool bHasFacing = false;
	FGcPlacementFacing Facing;

	/** ABSOLUTE, in the object's own frame, because that is where the engine applies it (§20.7a). */
	bool bHasScale = false;
	FVector Scale = FVector::OneVector;

	bool IsEmpty() const
	{
		return !bHasAnchor && !bHasFacing && !bHasScale && Surfaces.Num() == 0;
	}
};

/**
 * An optional authored sampling shape behind a placement target.
 *
 * Ordinary actor targets have none and keep using `FGraphCodeSpatialSolver::SamplePoint` over their
 * box. A `$Region` supplies one backed by R0's polygon sampler, so the placement resolver chooses a
 * point through the same target map without learning `.gcw`, loops, edges, or files.
 */
class GRAPHCODE_API IGcPlacementSamplingDomain
{
public:
	virtual ~IGcPlacementSamplingDomain() = default;

	/** Pure and deterministic in `(Seed, Index, authored geometry)`. */
	virtual bool SamplePoint(int32 Seed, int32 Index, FVector& OutPoint,
		FString& OutRefusal) const = 0;
};

/**
 * A thing a directive can name.
 *
 * `Frame` is the target's rotation and origin and carries NO SCALE — Z0's fold, restated here because
 * a caller filling this from a component meets exactly the same trap. `Volume` is already scaled.
 *
 * THE MIDPOINT IS THE VOLUME'S CENTRE WHEN THERE IS ONE, and the frame's location otherwise. §20.2
 * measures everything from a midpoint and notes that one exists on everything and needs no box; what
 * a box adds is a SIZE, which is what percentages and snapping need.
 */
struct GRAPHCODE_API FGcPlacementTarget
{
	FString Name;

	/** Rotation and translation only. A scaled frame here is a bug, not a variation. */
	FTransform Frame = FTransform::Identity;

	bool bHasVolume = false;
	FGcZoneVolume Volume;

	/**
	 * The target's own RUN, when it is a line — world points in its declared direction.
	 *
	 * **THE THIRD THING A PLACE CAN BE, and until now this record could not say it.** A frame is a
	 * point with a direction and a volume is a box; §21.7's table already calls an edge *a line* and
	 * writes *none* in its volume column, so a wall resolved through here arrived as its MIDPOINT and
	 * its shape was gone. Anything wanting to run along it — a ridge, a path, a fence — had to reach
	 * past the resolver into the region solver and re-derive geometry the helper had just discarded,
	 * which is the drift [[a-read-answers-through-the-write-path]] is about.
	 *
	 * So one helper answers with all three parts and the CALLER takes the one it needs: a placement
	 * reads `Frame`, a percentage reads `Volume`, a course reads this. Nothing here is in plan or in
	 * any other space — it is world XYZ, converted once, where the frame is.
	 *
	 * Empty on everything that is not a line, which is most targets. Its direction is the DECLARED
	 * one — `$Crypt.north` runs the way the wall was written, so reversing the declaration reverses
	 * anything drawn along it, and that is the author's control rather than an accident of winding.
	 */
	TArray<FVector> Course;

	/**
	 * How the run BENDS at each of those points — §21.19f, and it is EMPTY or exactly `Course.Num()`.
	 *
	 * **EMPTY IS THE WHOLE CORPUS AND IS THE POINT.** No committed `.gcw` states a bend, so this comes
	 * back empty for every one of them and every consumer keeps doing exactly what it did — a spline
	 * actor lays `Curve` points, a river lays `Curve` points. A course that states one arrives with
	 * one entry per vertex, the invented ones plain ([[optionality-can-be-owed-to-the-corpus]]).
	 *
	 * **PARALLEL RATHER THAN FOLDED INTO `Course`, because almost nobody wants it.** Terrain reads
	 * this array as a plan band, `gcw_ask` reads it as a run, the offset walk translates it — none of
	 * them has any use for an interpolation word, and widening the element type would have made every
	 * one of them say `.Position` to ask the question they actually have. `CurveAt` is what makes the
	 * pairing safe: a caller can never index past the end, and a target that carries no bends answers
	 * the plain one rather than nothing at all.
	 *
	 * **A RIGID OFFSET LEAVES IT ALONE, deliberately.** `FGcTerrainDomain::OffsetCourse` translates
	 * every vertex by one vector; a tangent is a DIRECTION, so a translation is exactly the operation
	 * that does not touch it. A rotation would, and there is none here.
	 */
	TArray<FGcCourseCurve> CourseCurve;

	/** The bend at one course vertex — the plain one where nothing was stated. Never out of range. */
	FGcCourseCurve CurveAt(int32 Index) const
	{
		return CourseCurve.IsValidIndex(Index) ? CourseCurve[Index] : FGcCourseCurve();
	}

	/** R3: present for an authored non-box region; absent means sample `Volume` as a box. */
	TSharedPtr<const IGcPlacementSamplingDomain> SamplingDomain;

	FVector Midpoint() const
	{
		return bHasVolume ? Volume.Frame.GetLocation() : Frame.GetLocation();
	}
};

/**
 * The object being placed.
 *
 * `LocalHalfExtent` is UNSCALED — `@scale` multiplies it here, so a caller hands over the archetype's
 * own size and never pre-applies.
 *
 * `Rotation` IS THE WHOLE ROTATION AND NOT A YAW, and that is the fix rather than a generalisation.
 * The footprint needs an orientation before `@facing` has resolved, and when no `@facing` is written
 * it is also the answer — but this field held a bare `double Yaw` until 2026-08-11, and a resolver
 * that can only receive a yaw can only answer with one. `@facing` then rebuilt the rotation from that
 * one number, so a line stating a yaw ZEROED a pitch and a roll nobody had mentioned, against §20's
 * own rule that absent members are absent claims. Worse, the midpoint-to-origin conversion at the end
 * used that flattened rotation even when nothing stated one, so a bare `@anchor` on a pitched actor
 * with an off-centre pivot landed it wrong. **Since §20.7c pitch and roll are SAYABLE too**, and this
 * field is what they are stated against: `pitch=keep` answers with the angle stored here.
 *
 * `LocalCentre` IS WHERE THE BOUNDS SIT RELATIVE TO THE PIVOT, and it is the field that keeps §20.2's
 * own worked example true. That section measures midpoint to midpoint on BOTH ends — *a 20-thick wall
 * and a 60-wide crate leave a 10 gap* is only 50 if the crate contributes its own half — while an
 * actor's stored location is its ORIGIN, which for most placed meshes sits at the base. Resolving to
 * the origin instead would make every line mean something different depending on where an artist put
 * a pivot inside an asset nobody can see from the level. Zero for an actor with no bounds, where the
 * midpoint and the origin are the same point and nothing changes.
 */
struct GRAPHCODE_API FGcPlacementSubject
{
	/**
	 * The entry's own name, and it is here because it is the SEED (§20.19).
	 *
	 * Carried on the subject rather than passed beside it so that `ResolveAll` — which already holds a
	 * name per request — cannot hand the two halves of one entry to the resolver out of step. Only
	 * `in=`/`walkable=` read it; every other directive resolves the same with it empty.
	 */
	FString Name;

	FVector LocalHalfExtent = FVector::ZeroVector;

	FVector LocalCentre = FVector::ZeroVector;

	FQuat Rotation = FQuat::Identity;
};

/**
 * What a snap was told to look for — §20.16, task P2, and it is one question asked of a world.
 *
 * THE BOX DECIDES WHICH WAY TO LOOK AND COLLISION DECIDES WHAT IS THERE, which is the whole of the
 * split. *Which surface did you mean* is a question about intent and the declared box answers it
 * perfectly — `toward=` states a direction outright, and with nothing stated §20.3a's area-weighted
 * patch picks one. *What stops the object* is a question about geometry, and there a box is an
 * approximation of the level rather than a description of it.
 */
struct GRAPHCODE_API FGcPlacementContactQuery
{
	/** The actor `@surface[target=…]` named. */
	FString Target;

	/** The anchored MIDPOINT — where the object stands before the snap moves it. */
	FVector From = FVector::ZeroVector;

	/** Unit, and the direction to look ALONG: down for a floor, not up. */
	FVector Look = FVector::DownVector;

	/** The subject's resolved rotation, so the swept shape is the shape that will stand there. */
	FQuat Rotation = FQuat::Identity;

	/** Already scaled, matching everything else the resolver measures with. */
	FVector HalfExtent = FVector::ZeroVector;
};

/** Which half answered, and it is reported rather than inferred — §20.16's second ruling. */
enum class EGcContactAnswer : uint8
{
	/** Swept against the target's own collision. */
	Swept,

	/**
	 * There is nothing in a world to sweep — no source was offered, or this target is an actor the
	 * payload is CREATING and has not spawned. The declared box answers, and the evidence says so.
	 *
	 * NOT THE FALLBACK §20.16 FORBIDS, and the difference is the whole reason this is a third value
	 * rather than a refusal. That ruling is about a target whose COLLISION is missing, where the two
	 * paths disagree precisely because the geometry is interesting; this is a target that is not in
	 * the world at all, where there is no collision to be missing and no disagreement to hide.
	 */
	NotInWorld,

	/** `collision_missing`, or a start already inside the geometry. Carried in `Refusal`. */
	Refused
};

/** What the sweep found. */
struct GRAPHCODE_API FGcPlacementContact
{
	EGcContactAnswer Answer = EGcContactAnswer::NotInWorld;

	/** `token: message`, set only on `Refused`. */
	FString Refusal;

	/** Where the subject's MIDPOINT comes to rest, before `distance` clears it. */
	FVector Midpoint = FVector::ZeroVector;

	/** The surface's own normal at the contact, pointing back at the subject. Tilts `snapToNormal`. */
	FVector Normal = FVector::UpVector;

	/** `collision, contact (…)` — the middle column of §20.12's evidence line. */
	FString Found;
};

/**
 * Where a contact comes from when a world is live — the impure half of `@surface`, task P2.
 *
 * An interface and not a `UWorld*` for the reason every other seam in this file is one: §19.8's payoff
 * is that a project stays askable WITHOUT OPENING IT, and a sweep needs a loaded world with initialised
 * physics. The resolver never learns which it is talking to, so P0's specs keep proving §20 with no map
 * loaded and the box path stays pure — it is not legacy, it is the no-editor case.
 */
class GRAPHCODE_API IGcPlacementContacts
{
public:
	virtual ~IGcPlacementContacts() = default;

	virtual FGcPlacementContact Sweep(const FGcPlacementContactQuery& Query) const = 0;
};

/**
 * Where a walkable point comes from when a world is live — the impure half of `walkable=`, task Z4.
 *
 * AN INTERFACE FOR THE REASON `IGcPlacementContacts` IS ONE: §19.8's payoff is that a project stays
 * askable WITHOUT OPENING IT, and a navmesh needs a loaded, built level. The resolver never learns which
 * it is talking to, so `in=` stays pure and provable with no map.
 *
 * IT PROJECTS AND DOES NOT SAMPLE, which keeps the determinism where it is already gated. The engine's
 * own random-point-in-radius helpers carry their own RNG, and §8's second-pass-zero rule needs one
 * document to produce one set of transforms across an engine upgrade. So the resolver samples the BOX
 * with `FGraphCodeSpatial::SamplePoint` — pure, deterministic, already asserted — and asks this only
 * *is there navmesh at this point*.
 */
class GRAPHCODE_API IGcPlacementNavigation
{
public:
	virtual ~IGcPlacementNavigation() = default;

	/** False when the level has no built navigation data at all — `navmesh_missing`, never a fallback. */
	virtual bool HasNavigation() const = 0;

	/** The nearest navigable point within `Extent` of this one, or false. */
	virtual bool ProjectToNavigable(const FVector& Point, const FVector& Extent,
		FVector& OutPoint) const = 0;
};

/** One directive echoed with what it RESOLVED to — §20.12's middle column, the uninferable part. */
struct GRAPHCODE_API FGcPlacementStep
{
	/** `@anchor`, `@surface`, `@facing`, `@scale`. */
	FString Directive;

	/** `contact 12.5 up, patch normal (0,0,1)` — what a caller reads when something lands oddly. */
	FString Resolved;
};

/**
 * What the directives amount to.
 *
 * `bStatesLocation` and `bStatesRotation` are not conveniences: a `@scale` on its own states neither,
 * and a directive set with no `@facing` and no `snapToNormal` states no rotation AT ALL. Answering
 * with identity there would silently spin an existing actor to yaw zero because somebody moved it.
 */
struct GRAPHCODE_API FGcPlacementResult
{
	bool bResolved = false;

	/** `token: message`, in the refusal spelling the query surface already uses. */
	FString Refusal;

	/** The actor's ORIGIN, ready to hand to `SetActorLocation` — see `Midpoint` below. */
	bool bStatesLocation = false;
	FVector Location = FVector::ZeroVector;

	/**
	 * Where the object's BOUNDS ended up, which is the point every directive actually resolved.
	 *
	 * Reported beside `Location` rather than instead of it because the two answer different
	 * questions: the engine stores an origin, and §20 measures a midpoint. Reading a placement back
	 * out (§20.11) starts here, so a caller that had only the origin would have to redo the
	 * conversion — and doing it twice is how the read and the write drift apart.
	 */
	FVector Midpoint = FVector::ZeroVector;

	bool bStatesRotation = false;
	FQuat Rotation = FQuat::Identity;

	bool bStatesScale = false;
	FVector Scale = FVector::OneVector;

	TArray<FGcPlacementStep> Evidence;
};

/**
 * One entry of a payload resolved together — P1, and the unit §18.7's topological order sorts.
 *
 * `Name` is the actor's internal name, which is what a directive on a SIBLING entry will spell.
 */
struct GRAPHCODE_API FGcPlacementRequest
{
	FString Name;

	FGcPlacementDirectives Directives;

	FGcPlacementSubject Subject;
};

/** One request and what it resolved to, in the order the resolve actually ran. */
struct GRAPHCODE_API FGcPlacementOutcome
{
	FString Name;

	FGcPlacementResult Result;
};

/**
 * What to measure a placed thing AGAINST — W0's read half, `GC-SPATIAL-AUTHORING-DESIGN.md` §18.
 *
 * A `.gcs` states each actor as a departure from its group's anchor, so generating one is `Resolve`
 * run backwards: a transform in, a directive set out. This is the question that inverse asks.
 */
struct GRAPHCODE_API FGcMeasureRequest
{
	/** Where it is measured FROM — the group's `anchor:`, already resolved to a target. */
	FGcPlacementTarget Anchor;

	/** Which of §20.6's spellings to answer in. `Percent` is refused: see `FGcMeasuredPlacement`. */
	EGcPlacementSpelling Spelling = EGcPlacementSpelling::Polar;

	/**
	 * The group's `align:`, and carrying it here is what keeps the round trip shut — §22.18.
	 *
	 * The apply shifts the subject off the anchored point by a reach of its own box, so an inverse that
	 * did not subtract that shift would record every house a half-width further out than it was written,
	 * and the next apply would push it out again. A regenerated file that drifts on every save is the
	 * one failure §18.5's *no second arithmetic* rule exists to prevent.
	 */
	FGcPlacementAlign Align;

	/** The group's `facing:`, when it states one. */
	bool bHasFacing = false;

	/** §20.7's two references: `relative` measures off the frame, `target` off *aimed at it*. */
	bool bFacingRelative = true;

	/** §22.0's third: `toward` measures off *aimed at the nearest point of that run*. */
	bool bFacingToward = false;

	FGcPlacementTarget FacingReference;
};

/**
 * One actor measured — the values a `.gcs` entry carries, before §18.4 factors them.
 *
 * IT IS THE RESOLVER'S OWN INVERSE AND NOT A SECOND ARITHMETIC. §18.5's *no second resolver* rule
 * reaches the read half too, so this lives beside `Resolve` rather than in the generator: the pair of
 * functions has to stay on one screen, because a convention changed in one and not the other is a
 * document that reads back as a different world with nothing erroring.
 *
 * **`Percent` IS NOT ANSWERED, by ruling.** A percentage of the anchor's half-extent is a legal way to
 * WRITE a position and a terrible way to record one: the same actor standing still would be measured
 * differently the day somebody resizes the box it is measured against, so a regenerated file would
 * change with nothing having moved. §18.3's `as:` offers `polar` and `offset` for that reason.
 */
struct GRAPHCODE_API FGcMeasuredPlacement
{
	/** `Polar`: degrees clockwise from the anchor's forward, and how far out. */
	double Bearing = 0.0;
	double Offset = 0.0;

	/** `Cartesian`: centimetres in the anchor's own frame. */
	double Forward = 0.0;
	double Right = 0.0;

	/** Both spellings. The anchor's own up, which is why it is not part of the polar pair. */
	double Up = 0.0;

	/** The departure from the facing baseline, absent when the group states no `facing:`. */
	bool bHasFacing = false;
	double Yaw = 0.0;
	double Pitch = 0.0;
	double Roll = 0.0;

	/**
	 * Set when the subject stands exactly where its facing reference does.
	 *
	 * THE SAME CASE `facing_undefined` REFUSES ON THE WRITE SIDE, and it has to be reported rather
	 * than answered: there is no direction from a point to itself, so any number here would be
	 * invented. A generator states the entry's position and omits its facing.
	 */
	bool bFacingUndefined = false;
};

/**
 * The placement half of the spatial surface. Pure, static, and asserted with no editor.
 *
 * ORDER IS scale → anchor → facing → surface, and it is §20.7a's with one step made explicit.
 * The snap reads the object's footprint, which needs a rotation, so facing resolves before it — from
 * the ANCHORED position, which is the same rule §20.3a already states for the faces: nothing may
 * resolve against the snap's own result, or the snap depends on where it put things.
 *
 * `snapToNormal` is the one thing that runs AFTER and rewrites what facing decided, and since §20.7d
 * it rewrites only the axes the author left unnamed.
 *
 * AND WHEN IT DOES, THE SNAP IS MEASURED AGAIN — §20.7e.1. The face is still chosen by the pre-tilt
 * footprint, because choosing it from the tilt the face itself produces is the circularity §20.3a
 * refuses; what the second pass re-asks is only how far the object travels, so the pose that ends up
 * against the surface is the pose the contact was computed for.
 */
class GRAPHCODE_API FGraphCodePlacementSolver
{
public:
	/** §20.8's spelled-out world anchor. Reserved: an actor of this name cannot be a target. */
	static const TCHAR* WorldTargetName();

	/** `target=…, forward=…` → the directive, or a named refusal. Unknown fields refuse. */
	static bool ParseAnchor(const TArray<FAuthoringProperty>& Fields, FGcPlacementAnchor& OutAnchor,
		FString& OutRefusal);

	static bool ParseSurface(const TArray<FAuthoringProperty>& Fields, FGcPlacementSurface& OutSurface,
		FString& OutRefusal);

	static bool ParseFacing(const TArray<FAuthoringProperty>& Fields, FGcPlacementFacing& OutFacing,
		FString& OutRefusal);

	/**
	 * `align=(right: self=0%, up: self=0%)` — §22.18, and it takes the RAW VALUE rather than a field
	 * list, because the `.gcw` group line spells it `align:(…)` and has no property list to hand over.
	 * One door, so the two files cannot come to disagree about what a share of the subject means.
	 */
	static bool ParseAlign(const FString& Value, FGcPlacementAlign& OutAlign, FString& OutRefusal);

	/** The parse run backwards — what the `.gcs` head line and the lowered `@anchor` both write. */
	static FString RenderAlign(const FGcPlacementAlign& Align);

	/**
	 * HOW FAR THE SUBJECT'S MIDPOINT STANDS FROM THE ANCHORED POINT so the named part sits on it.
	 *
	 * The span along a frame axis runs midpoint ± reach and `self=` names one point on it, so the
	 * midpoint goes (1 − 2·self)·reach away — beyond the point for the low end, short of it for the
	 * high end, and exactly on it for the centre every unaligned axis already has.
	 *
	 * `FrameRotation` is the ANCHOR's, because `forward`/`right`/`up` here are the frame the offsets
	 * beside them are measured in; `SubjectRotation` is the subject's FINAL pose, because a house
	 * turned into a bend presents a different reach to the street than one square to it.
	 */
	static FVector AlignShift(const FGcPlacementAlign& Align, const FQuat& FrameRotation,
		const FQuat& SubjectRotation, const FVector& HalfExtent);

	/**
	 * HOW DEEPLY TWO PLACED BOXES INTERPENETRATE, in centimetres, or zero when they do not — the
	 * separating-axis test over fifteen candidate axes, which is exact for two oriented boxes.
	 *
	 * **A WARNING'S EVIDENCE AND NEVER A REFUSAL** (§22.9e, user-ruled): two houses overlapping by a
	 * centimetre is a typo and two overlapping by four metres is a mistake, and the only honest way to
	 * tell them apart in a sentence is to say the number. A depth also lets a caller sort, which is
	 * what makes a warning on a town of six hundred readable at all.
	 */
	static double OverlapDepth(const FGcZoneVolume& A, const FGcZoneVolume& B);

	/**
	 * `@scale[1.5]` or `@scale[forward=1, right=1, up=1.4]`, so this one takes the raw inside text:
	 * the uniform form is a bare value and a property list has nowhere to put it.
	 */
	static bool ParseScale(const FString& Inside, FVector& OutScale, FString& OutRefusal);

	/** Everything checkable without resolving a name — §20.0's refusal and §20.6's spelling clash. */
	static bool Validate(const FGcPlacementDirectives& Directives, FString& OutRefusal);

	/**
	 * The whole resolve. `Targets` is keyed by name; `world` is synthesised and need not appear.
	 *
	 * `Contacts` is §20.16's collision path and null is the ordinary pure call. Supplying one changes
	 * where a `@surface`'s CONTACT comes from and nothing else — not the grammar, not the order, not
	 * which surface is meant, and not one refusal above it.
	 */
	static FGcPlacementResult Resolve(const FGcPlacementDirectives& Directives,
		const FGcPlacementSubject& Subject, const TMap<FString, FGcPlacementTarget>& Targets,
		const IGcPlacementContacts* Contacts = nullptr,
		const IGcPlacementNavigation* Navigation = nullptr);

	/** Every name a directive set resolves against, deduplicated, in the order they are written. */
	static TArray<FString> Dependencies(const FGcPlacementDirectives& Directives);

	/**
	 * `Resolve` RUN BACKWARDS — a placed thing measured into the values a `.gcs` entry states. W0.
	 *
	 * `Subject` is the actor as it stands, and it is a TARGET rather than an `FGcPlacementSubject`
	 * because what gets measured is its MIDPOINT: §20.2's midpoint-to-midpoint rule is the one thing
	 * this inverse cannot get wrong quietly, since measuring a pivot instead is correct for every
	 * actor whose bounds happen to be centred on it and wrong for every mesh an artist based at its
	 * feet.
	 *
	 * Total, and the only value it cannot answer is a facing whose reference stands at the subject's
	 * own position — reported as `bFacingUndefined` rather than invented.
	 */
	static FGcMeasuredPlacement Measure(const FGcMeasureRequest& Request,
		const FGcPlacementTarget& Subject);

	/**
	 * The measured values back into a directive set — the other half of the round trip, and the ONLY
	 * place a `.gcs` entry is turned into something `Resolve` accepts.
	 *
	 * IT EXISTS SO THE GATE CAN BE WRITTEN AT ALL (§18.9). *Measure, reconstitute, resolve, and the
	 * transform is unchanged* is a claim about a loop, and a loop assembled by hand in a spec would be
	 * asserting the spec's own assembly. This is the assembly, so the generator and the gate share it.
	 */
	static FGcPlacementDirectives Reconstitute(const FGcMeasureRequest& Request,
		const FGcMeasuredPlacement& Measured, const FString& AnchorName, const FString& FacingName);

	/**
	 * What a placed object BECOMES as a target — the input to whatever anchors to it next.
	 *
	 * `Before` supplies the facets the result does not state, which is not a convenience: a `@scale`
	 * on its own states no location, and rebuilding a target from a partial result would teleport
	 * whatever names it to the origin.
	 */
	static FGcPlacementTarget Placed(const FGcPlacementTarget& Before,
		const FGcPlacementSubject& Subject, const FGcPlacementResult& Result);

	/**
	 * A whole payload, IN DEPENDENCY ORDER RATHER THAN DOCUMENT ORDER — §18.7, ruled for the `.gcw`
	 * apply and the same question here.
	 *
	 * A document may anchor one entry to another entry it also places. Resolving in the order the
	 * lines happen to appear would make the answer depend on where in the file somebody typed them,
	 * and would break §8's second-pass-zero rule outright: pass one measures against the target's OLD
	 * pose and pass two against its new one, so the same document produces two different levels.
	 * Each resolved entry replaces its own target before anything that names it runs.
	 *
	 * CYCLES REFUSE. Two entries anchored to each other have no fixed point to resolve to, and an
	 * arbitrary starting entry is exactly the silent precedence §17.2 removed.
	 *
	 * `Targets` seeds the map and must already carry an entry for every request — its pose as the
	 * level holds it now, or an identity frame for an actor this payload creates.
	 */
	static bool ResolveAll(const TArray<FGcPlacementRequest>& Requests,
		const TMap<FString, FGcPlacementTarget>& Targets, TArray<FGcPlacementOutcome>& OutOutcomes,
		FString& OutRefusal, const IGcPlacementContacts* Contacts = nullptr,
		const IGcPlacementNavigation* Navigation = nullptr);

	/**
	 * The seed a region sample uses: the entry's NAME, hashed from its TEXT, salted by `seed=`.
	 *
	 * HASH THE TEXT AND NEVER THE `FName` — the engine calls its own value unstable. `GetTypeHash(FName)`
	 * resolves to the entry id's hash, and that id is a position in the running process's name table
	 * (`FNameEntryId::ToUnstableInt`, *"Serialize as process specific unstable int"*). Seeding from it
	 * would arrange one way in the editor and another in a headless run of the same document.
	 */
	static int32 RegionSeed(const FString& EntryName, int32 Salt);

	/**
	 * The support extent of a box along a world direction — Σ |dot(Dir, axis_k)| · HalfExtent_k.
	 *
	 * The one measurement the snap and the percentage both need, and the one place negative scale can
	 * bite: a flipped half-extent compares empty rather than mirrored, which `GraphCodeZoneIndex.cpp`
	 * already takes `GetAbs()` for. Exposed because a caller checking a refusal wants the same number.
	 */
	static double SupportExtent(const FQuat& Rotation, const FVector& HalfExtent, const FVector& Dir);
};
