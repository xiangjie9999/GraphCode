// Copyright OraStudio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GraphCodeRigProgram.h"
#include "GraphCodeCharacterControls.h"

/**
 * A PARSED `.gcrig`. `GC-RIG-GRAMMAR.md` is the grammar and it is canonical; `GC-RIG-EXAMPLE.md` is
 * the worked file this type was written against.
 *
 * ── REWRITTEN 2026-09-16 into FIVE BLOCKS, each holding ONE kind of line (ledger rows 39-49):
 *
 *      points  { }   where things are — a hand-written coordinate, and nothing else
 *      regions { }   where skin is and how far it reaches — `region($point, long=, wide=)`
 *      chains  { }   who hangs off whom — each chain's name is an IK chain's
 *      joints  { }   what each bone can do — `turns_in=`, `range=`, `side=`, `twists=`
 *      skin    { }   how each region's skin behaves — `crease=`, `folds=`, `bulge=`, …
 *
 *    plus `build { }` and `sockets { }`, unchanged. **A key written in another block's place is
 *    refused BY NAME**, which is what turns ledger row 28's line — a bone is a CAUSE, skin is a
 *    CARRIER — from a sentence in a design file into something the parser holds.
 *
 * **EVERY PER-ITEM PROPERTY IS A `[ ]` MODIFIER, NEVER A TRAILING `key:`** (ledger §0a). `key:value`
 * survives on the header line only, which is where the whole corpus puts it.
 *
 * **NOTHING HERE RESOLVES A NAME, AND AFTER THE REDESIGN THAT MATTERS MORE THAN BEFORE.** Every rule
 * that spans two lines is the LINKER's: whether a joint has a point, whether the region it turns in
 * exists, whether a twist-only joint's region was asked to crease, whether an inherited point still
 * sits inside a child's mesh. A line may be a partial override of its parent's or of its own mirror
 * twin's, so only the merged rig can say whether two lines agree — a refusal lands once, in the
 * place that can see the whole picture.
 */

/**
 * A length, an angle or a fraction.
 *
 * **THE BASE OF A PERCENTAGE IS THE FIELD'S, not one global rule** — `GC-GRAMMAR-REFERENCE.md` §3a.
 * It survives on exactly one key now: `wide=40%`, a fraction of the way round the limb's ring. A
 * position may never be one (ledger row 24), and the parser refuses it there by name.
 */
struct FGcRigScalar
{
	double Value = 0.0;
	bool bPercent = false;

	bool IsSet() const { return bSet; }
	bool bSet = false;
};

/** `(x, y, z)` in the mesh's own space, in centimetres. */
struct FGcRigPoint
{
	FGcRigScalar X;
	FGcRigScalar Y;
	FGcRigScalar Z;
};

/**
 * WHICH WAY A JOINT CLOSES, in words about the creature — read against the header's `faces:`.
 *
 * **A CROSS-CHECK SINCE 2026-09-16, NOT THE SOURCE (ledger row 47).** The region a joint turns in
 * says which side closes: the side the region sits on. `closes=` is kept so a file can state it too,
 * and the linker refuses the two when they disagree — the same *two answers that must agree* as the
 * shortest ring and the square one.
 */
enum class EGcRigCloses : uint8
{
	Unset,
	Back,
	Forward,
	In,
	Out,
	Up,
	Down,
};

/**
 * A LIMIT, in degrees — **always a pair, minimum to maximum, and zero is straight.**
 *
 * User-ruled 2026-09-16: *"zero is straight, no bending yet"* — and *"there's a minimum to maximum,
 * so usually is negative."* A rest pose is simply somewhere inside the range; a finger resting 20°
 * bent sits at +20. The one-number `range=145` of the old grammar measured from the rest pose, which
 * differs on every mesh, so it could not inherit — it is refused by name.
 */
struct FGcRigRange
{
	double Low = 0.0;
	double High = 0.0;
	bool bSet = false;
};

/**
 * The modifiers of ONE statement.
 *
 * **EACH BLOCK VALIDATES ITS OWN KEYS, AND AN UNKNOWN KEY IS REFUSED** — a change on 2026-09-16.
 * The old reader kept unknown keys as text for a consumer to judge, which let a misspelled key
 * through silently; once every block has one kind of line, each block knows its whole key list and a
 * typo is worth a refusal naming the keys it does take.
 */
struct FGcRigModifiers
{
	/** `key=value`, in written order, value unparsed. A key may appear once per statement. */
	TArray<TPair<FString, FString>> Keyed;

	/** A bare flag — `[keep]`. Corpus form: `[instance_editable]`, `[linear]`, `[auto]`. */
	TArray<FString> Flags;

	const FString* Find(const TCHAR* Key) const;
	bool HasFlag(const TCHAR* Flag) const;
};

/**
 * `knee_B_L = (-30, 11, 44)` — **A POINT IS WRITTEN BY HAND, and nothing else is.**
 *
 * User-ruled 2026-09-16: *"gc render only answers. the points need to hand written, and yes,
 * possibly rechecked."* So there is one form — a coordinate — and no `between()`, `at()` or `mid()`:
 * each was a way for the tool to compute a position, and the tool only answers questions. What the
 * build does instead is RE-CHECK: a joint's point must have a closed loop round it, a region's centre
 * must sit on the skin, and an inherited point is checked against the child's own mesh (row 44).
 *
 * **A JOINT SITS AT THE POINT WITH ITS NAME.** A point nothing names is the centre of a region, which
 * names it as `$popliteal_L`; two regions can then share one centre, which is the reason named
 * points came back after row 30 deleted them.
 */
struct FGcRigNamedPoint
{
	FString Name;
	FGcRigPoint At;
	int32 Line = 0;
};

/**
 * `knee_back_L = region($popliteal_L, long=6, wide=40%) [along=($hip_B_L, $hock_B_L), tilt=10]`
 *
 * **WHERE SKIN IS AND HOW FAR IT REACHES — never how it behaves**, which is `skin { }`'s. A region
 * reaches two ways (ledger row 33): `long=` along the limb in centimetres, and `wide=` round it —
 * a fraction of the ring, or centimetres where no ring goes round (row 49: *the unit decides*).
 *
 * **AND A JOINT TURNS INSIDE ONE** (row 47): the region a joint names with `turns_in=` is where its
 * weight passes to the next bone, its fold axis is the direction the region spreads round the limb,
 * and the side it sits on is the side that closes.
 */
struct FGcRigRegion
{
	FString Name;
	int32 Line = 0;

	/** The point it is centred on, without the `$`. Every position in the file is in `points { }`. */
	FString Centre;

	/** Centimetres along the limb. Unset derives it from the size of the ring there. */
	FGcRigScalar Long;

	/** Round the limb: a percentage of the ring, or centimetres. Unset is 100% — a full band. */
	FGcRigScalar Wide;

	/**
	 * The two points whose line the ring is cut square to, without `$`. Empty means the line between
	 * the neighbouring joints of whatever turns in this region (ledger rows 34, 36). **Two points,
	 * never an axis letter** — row 28's rule, since a letter would name the bone's unseeable frame.
	 */
	TArray<FString> Along;

	/** How far the shortest ring may lean from square, in degrees — `tilt=10` keeps it out of a finger gap. */
	TOptional<double> Tilt;

	FGcRigModifiers Modifiers;
};

/**
 * `leg_B_L = pelvis -> hip_B_L -> knee_B_L -> hock_B_L -> paw_B_L`
 *
 * **A CHAIN IS PARENTAGE, AND ITS NAME IS THE IK CHAIN'S, NOT A BONE'S** — and since 2026-09-16 it
 * has its own block (ledger row 42). Inside `joints { }` it had the same `name = …` shape a position
 * has, so `leg_B_L` read as a joint. `kind:ik_rig`'s retarget chains name a start and an end bone,
 * which is exactly what this line is, so the IK rig falls out of it.
 */
struct FGcRigChain
{
	FString Name;
	TArray<FString> Joints;
	FGcRigModifiers Modifiers;
	int32 Line = 0;

	/** `dynamic=0.4` — the chain swings on its own. Writes the engine's setup, adds no solver. */
	TOptional<double> Dynamic;

	/** Where the IK goal sits, when it is not the chain's last joint. */
	FString Goal;
};

/**
 * `knee_B_L [turns_in=$knee_back_L, range=(-5, 145), side=(0, 0), closes=back]`
 *
 * **ONE JOINT IS ONE BONE, and a bone is a CAUSE: it says what it can DO and nothing about how the
 * skin looks when it does it** (ledger row 28). It has no position — it sits at the point with its
 * name — and no skin keys, which are `skin { }`'s under the region it turns in. A readout may instead
 * transcribe its existing rest frame with `orientation=` until an author replaces that with intent.
 *
 * **A JOINT TURNS ONLY INSIDE A REGION IT NAMES** (row 47). A joint with limits and no `turns_in=`
 * is asked *"where does it turn?"* — by the linker, because a child's line may carry only the one key
 * it overrides. A joint with neither is RIGID: a skull, a claw, a bolted-on lamp.
 *
 * **THREE LIMITS IN A FRAME THE JOINT ITSELF SUPPLIES** (rows 37, 45): fold about the region's axis,
 * sideways about the axis square to it and to the limb, twist about the limb's own line.
 */
struct FGcRigJoint
{
	FString Name;
	int32 Line = 0;
	FGcRigModifiers Modifiers;

	/** The region this joint turns in, without the `$`. */
	FString TurnsIn;

	/** `twists_in=`: the band its roll is spread along, without the `$`; empty where the roll is taken in `TurnsIn`. */
	FString TwistsIn;

	/**
	 * The imported bone's exact component-space rest orientation.
	 *
	 * `orientation=(pitch, yaw, roll)` is the transcribed alternative to `turns_in=`: a reader writes
	 * what an existing skeleton already has, while an authored joint names the region it turns in and
	 * lets the rig derive its frame. They never coexist, because that would give one frame two owners.
	 */
	TOptional<FQuat> Orientation;

	/**
	 * **WHERE THIS BONE AIMS, when the chain's own answer is wrong** — `points_to=$Point`, without
	 * the `$`. USER-RULED 2026-09-16, and it is a named point rather than a bare direction so that
	 * every coordinate in the file stays in `points { }`.
	 *
	 * **IT SAYS ONE AXIS AND LEAVES THE REST TO `turns_in=`, WHICH IS WHY THE TWO COMPOSE.** A bone
	 * needs two facts and they are different: where it AIMS, and which way it BENDS. `orientation=`
	 * pins all three axes at once, so nothing is left for a region to contribute and the two are
	 * refused together — correctly, for that key. Aim plus fold leaves nothing over: the third axis
	 * is their cross product. That is what lets a thumb, a clavicle or any bone seated at an odd
	 * angle both aim where it really points AND declare where it folds, which `orientation=` alone
	 * could never do (ledger 7e: the trial's thumb was left rigid for exactly this reason).
	 */
	FString PointsTo;

	/** How far it folds. */
	FGcRigRange Range;

	/** How far it swings sideways. */
	FGcRigRange Side;

	/** How far it rolls about its own line. **The helper-bone COUNT is derived from this.** */
	FGcRigRange Twists;

	/**
	 * `twists_raised=(-90, 73.8)` (MOVE-CUT1, Saved/Research/MOVE-CUT1/SHOULDER-TWIST-ELEVATION-DESIGN.md section 3): the same roll's pair with
	 * the joint's bone LEVEL, 90 degrees from the trunk's down (`twists=` is its pair hanging). It must contain `twists=` and needs a ball joint
	 * (`range=` and `side=`); both are asked by the linker, because a child's line may carry only this key. It reaches no native asset: the
	 * IK rig keeps `twists=` and carries this pair as a metadata record a `.gcmove` reads. Never a helper count (`Twists` alone is).
	 */
	FGcRigRange TwistsRaised;

	/** A cross-check on the side its region sits on. */
	EGcRigCloses Closes = EGcRigCloses::Unset;

	/** Degrees about the bone's own length, orienting an intent-derived rest pose. */
	TOptional<double> Roll;
	/** Default animation angles: fold from straight, side from the native bind. */
	TOptional<double> RestFold;
	TOptional<double> RestSide;
	/** `rest=(twist=N)`: degrees about the bone's own length, from the native bind. `rest=(twist=forward)` (user 2026-10-18: "don't hardcode", the paw
	 * "in the rig"): the rig works it out, the turn that points this joint's limb end along `faces:`, shared with the other joints of its chain that say so. */
	TOptional<double> RestTwist;
	bool bRestTwistForward = false;

	/**
	 * **THIS CHARACTER'S NORMAL FULL EFFORT AT THIS JOINT** — `velocity=` in deg/s, `accel=` in deg/s^2, `ramp=` in seconds
	 * (the quickest it builds to its full acceleration). Each overrides the header's, key by key. A reference, not a
	 * maximum; unset is unknown, and nothing is defaulted onto the joint.
	 */
	TOptional<double> Velocity;
	TOptional<double> Accel;
	TOptional<double> Ramp;

	/**
	 * `dynamic=0.75` (MOVE-RELAX1, Docs/GC-MOVE-RELAX-PLAN.md 2.1): HOW FREELY THIS JOINT'S PHYSICS BODY MOVES when something
	 * simulates it, 0..1. 0 is a body only animation moves, 1 one that swings free, and between a spring holds it toward its
	 * animation pose, weaker as the number rises. Unset is 1, because a rig-built asset today has no drive on any joint, so
	 * "nothing declared" and "free" are the same joint. The `physics_asset()` writer turns it into an angular drive and body
	 * damping; the chain's `dynamic=` is a different, parked key.
	 */
	TOptional<double> Dynamic;

	// There is no `owns_skin` here. Rev 2 had a word for "the skin past this band is this joint's"; Rev 3 (user-ruled
	// 2026-10-06, GC-RIG-BIND-OWNERSHIP-PLAN.md) says the band already says it, so the linker infers it from `turns_in=`
	// (`FGcRigLinkedJoint::bOwnsSkin`) and the parser refuses the old word.

	bool HasLimits() const { return Range.bSet || Side.bSet || Twists.bSet; }
};

/**
 * `belly [crease=0.6, folds=3, bulge=0.4]` — **HOW ONE REGION'S SKIN BEHAVES**, under that region's
 * name. Skin is a CARRIER (ledger row 28): nothing here moves; everything here describes what the
 * surface does when a joint turning in it moves.
 *
 * **EVERY BEHAVIOUR IS AN AMOUNT, AND ZERO OR ABSENT MEANS OFF** (ledger row 40). The types the design
 * first named — a tuck, a roll, a slide, a wedge — were numbers in disguise, and a number inherits
 * and overrides one key at a time where a word could only be swapped for another word.
 */
struct FGcRigSkin
{
	FString Name;
	int32 Line = 0;
	FGcRigModifiers Modifiers;

	/** How much the skin gathers rather than stretching, 0..1. */
	TOptional<double> Crease;

	/** **How it creases, as a COUNT of folds** — `1` at a knuckle, `3` for a heavy wrist. */
	TOptional<int32> Folds;

	/** How much the flesh rolls OUTWARD as it closes, 0..1. Above 0 adds the helper that carries it. */
	TOptional<double> Bulge;

	/** How much the skin stays put while the bone turns under it, 0..1 — a kneecap, an elbow point. */
	TOptional<double> Slide;

	/**
	 * **HOW MUCH THIS FLESH KEEPS MOVING AFTER THE BONE HAS STOPPED, 0..1 — a belly, a jowl, an
	 * elephant's flank.**
	 *
	 * **IT IS THE ONLY KEY HERE THAT IS ABOUT TIME.** Every other one is a fact about a pose: state
	 * the joint angle and the surface follows from it. Loose skin is not — where it sits depends on
	 * where it was a moment ago and on which way the body just moved, so no weighting and no
	 * per-vertex rule can express it. What can is a bone with a spring on it, which is how every
	 * shipped character does a jowl or a belly, and the solver is the engine's own (`AnimDynamics`).
	 *
	 * **AND IT ASKS FOR NO JOINT, WHICH IS WHAT SEPARATES IT FROM `bulge=`.** Flesh rolls outward
	 * because a fold closes, so `bulge=` refuses a region nothing turns in and says so. A belly
	 * wobbles because the body moved, and a belly bends at nothing — so this is the first skin key
	 * that means something on a region no joint touches.
	 */
	TOptional<double> Jiggle;

	/**
	 * **HOW MUCH OF ITS THICKNESS THE LIMB HOLDS AS THE JOINT CLOSES, 0..1 — and the WHOLE range is
	 * the feature.** User-ruled 2026-09-17: *"some might want some smoother bend, some might want a
	 * volume loss bend. Knee is in the middle"*, which is why this is a dial an author turns and not
	 * a constant somebody measures once and bakes in.
	 *
	 * **IT IS A SKINNING RULE, NOT GEOMETRY, AND THAT IS THE POINT.** Linear blend skinning averages
	 * two rigid transforms, so the section flattens at every fold — ledger §0b.4 measured 69.3% of
	 * the volume gone at 145 degrees and, decisively, measured that THREE ring placements and eight
	 * times the rings do not move it. `keep_volume=` mixes the fold's skin toward the section-preserving
	 * rule instead: **0 lets it pinch flat, which is the old behaviour and a legitimate look; 1
	 * keeps the full section and never creases, which is a bent hose; a knee is between them.**
	 *
	 * **THE BAND IS DERIVED — only the amount is typed.** `long=` and `wide=` already say where the
	 * region is, so the gather runs across it and fades over its outer quarter. That is a stated
	 * shape rather than a tuned constant, and it is what stops the crease being a hairline on the
	 * region's exact centre line (ledger §13.12, measured).
	 *
	 * Unset, nothing is written and the mesh skins the way it always did.
	 *
	 * **THE NAME WAS CHOSEN TO BE READ COLD BY A MACHINE** — user-ruled 2026-09-17,
	 * *"choose one that any ai can understand"*. `holds=` was the placeholder and it failed
	 * that test: it does not say WHAT is held. `keep_volume=` carries the quantity and the
	 * direction in the word, and *volume preservation at a joint* is the phrase every model
	 * has already read a thousand times.
	 */
	TOptional<double> KeepVolume;

	/**
	 * **How far the bulge bone travels at full fold, in centimetres** — the override for the
	 * derived `radius x bulge` (ruling 69).
	 *
	 * Unset, the distance comes from the region's own offset from the bone's line, which is what
	 * that region states about the thickness of the flesh there. `travel=` is for where that
	 * derivation is wrong: a jowl whose centre sits close to the jaw states a thin limb and is not
	 * one. **It has no meaning without `bulge=`** and is refused by name there.
	 */
	TOptional<double> Travel;

	/**
	 * **How far, in cm, a held thing may press into this region's skin**, where it gives more than the
	 * `physics_asset() [gives=]` the rest of the skin does: the web between thumb and forefinger round a
	 * pistol's backstrap. Faded to that figure across the region's edge, as every amount here is.
	 */
	TOptional<double> Gives;

	/**
	 * **Degrees of fold per ring in this area**, overriding the header's `max_step:` (ruling 71).
	 *
	 * Past this step a bend creases visibly, so it is the floor `loops=` is derived from. Measured
	 * rather than chosen: shipped rigs in this tree spend 9 degrees a ring on a hand and 1-3 on a
	 * film body, and bending a leg puts the curve's knee at 21-24 (ledger 0b.1, 0b.3).
	 */
	TOptional<double> MaxStep;

	/** Extra half-rings on the closing side only — denser where it gathers, none wasted where it stretches. */
	TOptional<int32> Wedge;

	/** Where along the region a twist is taken up, 0 at the parent's end to 1 at the child's. Unset spreads it. */
	TOptional<double> WringAt;

	/** How hard the rings bunch toward the crease, 0 even to 1 packed. */
	TOptional<double> Crowd;

	/** How many edge rings. Unset derives `max(angle floor, 2*folds+1)`. */
	TOptional<int32> Loops;

	/**
	 * This area's edge length, overriding the header's `density:` — **and only a `[relay]` region
	 * may state one** (ledger row 78). A ring insert splits the edges that are already there and
	 * cannot choose a length, so anywhere else this key looked set and did nothing.
	 */
	TOptional<FGcRigScalar> Density;

	/** `$Name` of a `profile()` shaping the weight transition across the region, or empty. */
	FString Falloff;

	/** `[keep]` — leave this area's triangles alone. */
	bool bKeep = false;

	/**
	 * `[relay]` — throw this area's surface away and rebuild it.
	 *
	 * **THE THIRD VALUE OF ONE AXIS, not a second switch.** `keep` touches nothing, the default tops
	 * the rings up to `loops=`, and this rebuilds — increasing order of how much of the author's mesh
	 * the build is allowed to spend. **It is the only operation in this format that DELETES**, which
	 * is why it is named on the line rather than inferred: writing it is the author's consent to lose
	 * this patch's texture coordinates, so nothing has to guess from the mesh's provenance.
	 *
	 * **§11.9 REJECTED THIS FLAG AND ITS STATED REASON WAS FALSE** — *"a fourth kind of thing on a
	 * line that is otherwise all amounts"*, written as though the line carried only amounts when
	 * `keep` was already a bare flag on it and already shipped. User-found 2026-09-16.
	 */
	bool bRelay = false;
};

/** `muzzle = head [at=(6, 0, 1)]` — a name, the bone it rides, and an offset nobody should type. */
struct FGcRigSocket
{
	FString Name;
	FString Bone;
	FGcRigModifiers Modifiers;
	int32 Line = 0;
};

/**
 * One instruction of a morph's body: `displace(by=1.2) [in=$throat]` (Docs/GC-MORPH-DESIGN.md §3.2).
 *
 * **SHAPE'S WORD WITH SHAPE'S MEANING, READ ON THE RIG'S FINAL SKIN IN ITS BIND POSE.** `t` is the region's own fade
 * (`FGraphCodeRigRegion::Falloff`, 1 in the middle and 0 at the edge) and a vertex moves `by × slope(t)` along the skin's
 * normal, or along one of the rig's six words where `along=` says one. `displace` only so far; every other Shape verb is
 * refused by name in the parser.
 */
struct FGcRigMorphLine
{
	/** `displace`. */
	FString Verb;

	/** `by=`: how far the middle of the region moves, in centimetres. Negative is a dent. */
	double By = 0.0;

	/** `slope=`: empty or `mound` (smoothstep, the default), `linear` (the cone), or `$Profile`, without the `$`. */
	FString Slope;

	/**
	 * `along=`: one of the rig's six direction words, read against `faces:` exactly as `closes=` reads them
	 * (`FGraphCodeRigRegion::WordDirection`), every vertex of the line moving that one way. `Unset` (not written) is the
	 * skin's own normal at the shape's scale.
	 */
	EGcRigCloses Along = EGcRigCloses::Unset;

	/** `[in=$Region]`: the rig region whose skin moves, without the `$`. */
	FString Region;

	/** The instruction as written, for a note or a refusal. */
	FString Text;
	int32 Line = 0;
};

/**
 * `ThroatPuff = morph() { displace(by=1.2) [in=$throat] }` — a morph target of the skeletal mesh this rig builds.
 *
 * **A MORPH IS ITS DECLARATION** (GC-MORPH-DESIGN.md §3.1): every line of `morphs { }` is built into the skeletal mesh,
 * and nothing in `build { }` names one. Its lines add per vertex. A whole value under `parent:`, as a chain is: a child's
 * morph of the same name replaces its parent's.
 */
struct FGcRigMorph
{
	FString Name;
	TArray<FGcRigMorphLine> Lines;
	int32 Line = 0;

	/**
	 * `follows=elbow_L.fold`: the joint whose fold sets this morph's weight AT RUNTIME, a corrective (GC-MORPH-DESIGN.md §3.5,
	 * ruling R3). The generated post-process Control Rig reads the fold and sets the curve, so blends, runtime IK and ragdoll
	 * bring the shape in too. Its owner is the joint: it gets no control, and a control or a clip naming its curve is refused.
	 * Empty for a morph a clip plays.
	 */
	FString Follows;

	/** `range=(30, 140)`: weight 0 at the first fold (degrees from straight), 1 at the second, linear between and clamped to 0..1. */
	FGcRigRange FollowRange;

	/** `mirror=false` on an `_L` morph: no `_R` twin is derived from it (§3.3). File-wide `mirror:false` stops every twin. */
	bool bMirror = true;

	/**
	 * A DERIVED TWIN (§3.3, ruling R4): the `_L` morph whose BUILT difference this one reflects across the mirror plane. Set by
	 * the linker, never written; a written `_R` replaces it. A twin has no lines of its own: each of its vertices samples the
	 * left difference at its own mirror point on the skin.
	 */
	FString TwinOf;
};

/** `Tendon = profile(0.0 = 0, 0.65 = 0.92, 1.0 = 1)` — Shape's own object, at file scope. */
struct FGcRigProfile
{
	FString Name;
	TArray<TPair<double, double>> Stops;
	int32 Line = 0;
};

/**
 * `SK_Wolf = skeletal_mesh(skeleton=/Game/Rigs/SK_Quadruped) [triangles=12000]`
 *
 * **THE ASSET KIND STANDS IN WHERE A SOURCE ENTRY WOULD BE, and that is not ceremony.** The rule is
 * *write the tokens that exist*: a data table row has no class, so `row(…)` stands in; a build line
 * here has no source entry, because the file declares ONE rig, so its asset kind does.
 */
struct FGcRigBuildLine
{
	FString AssetName;
	FString Kind;
	TArray<TPair<FString, FString>> Props;

	/**
	 * `map=(ours=theirs, ...)` on an `ik_retargeter()` line. The left name is always a chain from
	 * this file and the right name is always a chain on `onto=`, independent of `[out]`.
	 */
	TArray<TPair<FString, FString>> RetargetChainMap;

	FGcRigModifiers Modifiers;
	int32 Line = 0;
};

/**
 * WHICH WAY THE MESH'S GEOMETRY POINTS.
 *
 * **ONE TOKEN PER FILE, AND IT IS WHAT MAKES EVERY OTHER DIRECTION SAYABLE (ruling 28).** Every
 * `closes=` word is read against it, so no direction anywhere is expressed in a frame that does not
 * exist yet. It is checked by looking at a picture, which is the cheapest verification in the format.
 *
 * **AND IT DERIVES THE MIRROR PLANE**, which used to be its own `mirror:` token: the symmetry plane
 * is the one containing forward and up.
 */
struct FGcRigFacing
{
	/** `X`, `Y` or `Z`. */
	FString Axis = TEXT("X");
	double Sign = 1.0;

	/** `Z` unless the file says otherwise — UE's own character convention. */
	FString UpAxis = TEXT("Z");
	double UpSign = 1.0;
};

/** Every `key:value` on line one and its continuations. */
/**
 * **How tightly skin holds to its nearest bone** — the value of the header's `binds:`.
 *
 * Four words rather than a number, for the reason `stance:` is two: a rigger has an opinion about
 * whether a thing is flesh or armour plate, and none at all about whether 0.63 beats 0.58. The
 * stiffness each one maps to is `FGraphCodeRigBindStiffness()`.
 */
enum class EGcRigBind : uint8
{
	/** Loose flesh, a jowl, a hanging gut — and the engine's own default, kept reachable. */
	Soft,
	/** **The default.** An animal with bones in it. */
	Flesh,
	/** Muscle over bone with little give — a horse's cannon, a lean athlete. */
	Firm,
	/** Armour plate, carapace, anything mechanical: very nearly one bone a vertex. */
	Tight,
};

/** The stiffness `binds:` means, for `FGeometryScriptSmoothBoneWeightsOptions::Stiffness`. */
inline float FGraphCodeRigBindStiffness(const EGcRigBind Binds)
{
	switch (Binds)
	{
	case EGcRigBind::Soft:  return 0.20f;
	case EGcRigBind::Firm:  return 0.75f;
	case EGcRigBind::Tight: return 0.95f;
	case EGcRigBind::Flesh:
	default:                return 0.50f;
	}
}

/** The word, for a refusal or a build note to quote back. */
inline const TCHAR* FGraphCodeRigBindWord(const EGcRigBind Binds)
{
	switch (Binds)
	{
	case EGcRigBind::Soft:  return TEXT("soft");
	case EGcRigBind::Firm:  return TEXT("firm");
	case EGcRigBind::Tight: return TEXT("tight");
	case EGcRigBind::Flesh:
	default:                return TEXT("flesh");
	}
}

/**
 * **WHAT `binds:` RESOLVED TO — one of the four words, or a number between them.**
 *
 * Enum-only was the first shape and it was too strict, user-questioned 2026-09-18. This format is
 * already full of raw 0..1 amounts — `crease=`, `slide=`, `bulge=`, `keep_volume=`, `jiggle=`,
 * `crowd=` — and the grammar records the ruling that decided one of them: *"some might want some
 * smoother bend, some might want a volume loss bend. Knee is in the middle"*, i.e. **the whole
 * range is the feature**. A token that reads either way is precedent here too: a region's `wide=`
 * takes `35%` or `150` and means a different thing by each.
 *
 * So the words are the VOCABULARY — what an author reaches for, and what a build note quotes —
 * and the number is the escape hatch for somebody tuning one creature between two of them.
 */
struct FGcRigBind
{
	/** The named point, and what an unstated `binds:` means. */
	EGcRigBind Named = EGcRigBind::Flesh;

	/** Set only when the file wrote a bare number, which then wins over the word. */
	TOptional<double> Exact;

	/** What reaches `FGeometryScriptSmoothBoneWeightsOptions::Stiffness`. */
	float Stiffness() const
	{
		return Exact.IsSet()
			? (float)FMath::Clamp(Exact.GetValue(), 0.0, 1.0)
			: FGraphCodeRigBindStiffness(Named);
	}

	/** How a build note or a refusal says it back. */
	FString Spelling() const
	{
		return Exact.IsSet()
			? FString::Printf(TEXT("%.2f"), Exact.GetValue())
			: FString(FGraphCodeRigBindWord(Named));
	}
};

struct FGcRigHeader
{
	int32 Version = 1;
	FString Into;

	/**
	 * The mesh being rigged.
	 *
	 * **EMPTY IS NOT AN ERROR STATE (ruling 25).** It means the file builds nothing — behaviour with
	 * no surface to lay, which is a perfectly good thing for another `.gcrig` to inherit: a preset.
	 * `Mesh` and `Parent` are independent, and a file may carry either, both or neither.
	 */
	FString Mesh;

	/**
	 * Another `.gcrig` this one inherits from, one key at a time.
	 *
	 * **EVERYTHING CROSSES THIS EDGE NOW, POSITIONS INCLUDED** (ledger row 44, reversing rows 24 and
	 * 27), so a child can reuse a whole rig and override nothing. The guard is the linker's: every
	 * inherited point is re-checked against this file's own mesh and refused by name if it no longer
	 * sits where it should.
	 */
	FString Parent;

	/** Which way the geometry points, and the frame every direction in the file is read in. */
	FGcRigFacing Faces;

	/** False turns mirroring off on something that is not symmetric. */
	bool bMirror = true;
	/** Replace this source's generated Control Rig functions unless override:false is authored. */
	bool bOverride = true;

	/** An explicit mirror plane normal, for a thing that faces nowhere. Empty derives it. */
	FString MirrorAxis;

	/** The most bones one vertex may be weighted to. A platform budget somebody decides. */
	int32 Influences = 4;

	/**
	 * **`binds:` — how tightly skin holds to its nearest bone**, and the only word the format has
	 * over the bind itself. `influences:` says how MANY bones may share a vertex; this says how
	 * sharply the share falls away with distance, which is the question that decides whether a
	 * limb reads as flesh over bone or as jelly (ledger §19.12).
	 *
	 * It reaches `FGeometryScriptSmoothBoneWeightsOptions::Stiffness`, whose engine default is
	 * `0.2` — the soft end of its own scale, and the value every rig this product built before
	 * 2026-09-18 silently inherited. The weight of a bone at normalised distance `d` is
	 * `(1/S(d))²` where `S(d) = (1-stiffness)·d + stiffness·d²`, so the word is really choosing an
	 * exponent: `soft` is very nearly inverse-square, `tight` very nearly inverse-fourth.
	 */
	FGcRigBind Binds;

	/** File-wide default edge length for the re-lay. Unset means nothing is re-laid — setting A. */
	TOptional<FGcRigScalar> Density;

	/**
	 * **`max_step:` — degrees of fold one ring may carry before the bend creases visibly**, the
	 * divisor `loops=`'s floor is derived from. A region's own `max_step=` overrides it, exactly as
	 * `density=` overrides `density:`.
	 *
	 * Absent, `FGraphCodeRigCut::DefaultMaxStep()` — 22 degrees, the middle of what two independent
	 * measurements found (ledger §0b.1, §0b.3). User-ruled declarable 2026-09-17 (row 71).
	 */
	TOptional<double> MaxStep;

	/**
	 * **`velocity:`, `accel:`, `ramp:` — THIS CHARACTER'S NORMAL FULL EFFORT, the file-wide default**, in deg/s, deg/s^2 and
	 * seconds (the quickest a joint builds to its full acceleration). A joint's own `[velocity=, accel=, ramp=]` overrides
	 * each, key by key, and `parent:` inheritance carries them as it carries `max_step:`.
	 *
	 * **A REFERENCE, NOT A MAXIMUM, AND NEVER AUTO-FILLED.** The speed report shares a measured peak against it, and a joint
	 * past 100% is a note rather than a refusal. Absent, the value is unknown, and a `.gcmove` asking a share of it is
	 * refused naming the joint (GC-MOVE-KINEMATIC-DIAGNOSTICS-PLAN.md, "Rig values are a REFERENCE").
	 */
	TOptional<double> Velocity;
	TOptional<double> Accel;
	TOptional<double> Ramp;

	/**
	 * **`body_accel:`, `body_speed:` — THE WHOLE BODY'S NORMAL FULL EFFORT**, in cm/s^2 (how hard the feet can speed the body
	 * up or brake it) and cm/s (its normal full-effort running speed). Read and inherited exactly as `velocity:` is, but
	 * header only: the body is not a joint, so no joint line takes them (GC-MOVE-ROOT-STATE-PLAN.md, MOVE-ROOT1 R4).
	 * References, not maximums; absent is unknown, and the body's state is then unbounded and says so.
	 */
	TOptional<double> BodyAccel;
	TOptional<double> BodySpeed;

	/**
	 * **`stance:` — the pose this rig is AUTHORED in**, `a_pose` or `t_pose`, user-ruled 2026-09-17
	 * (row 75). It reaches the retargeter this build writes: two rigs authored in different stances
	 * retarget wrong, and with neither of them saying which is which the engine falls back to each
	 * skeleton's rest pose and hopes.
	 *
	 * **UNSET IS NOT A_POSE.** A stance guessed wrong is silent — the animation looks *nearly* right
	 * — so a file that does not say stands where the engine's default leaves it and the build says
	 * out loud that nothing was corrected.
	 */
	TOptional<EGcRigStance> Stance;

	/** Pins OUR half — grammar, joint-to-call mapping, solver defaults. `1` is never written. */
	int32 Gen = 1;

	/**
	 * WHICH TOKENS THE FILE ACTUALLY WROTE, by key.
	 *
	 * **A DEFAULT AND A STATED VALUE THAT HAPPEN TO MATCH ARE DIFFERENT SENTENCES, and only the
	 * parser can tell them apart.** `influences:4` is this format's default, so a child that leaves it
	 * out and a child that writes `4` look identical in the fields above — and under a `parent:` they
	 * mean opposite things: inherit the parent's 8, or override it with 4. The merge reads this set
	 * rather than comparing against a default it would have to hard-code a second time.
	 */
	TSet<FString> Stated;
};

struct GRAPHCODERIG_API FGcRigDocument
{
	FGcRigHeader Header;

	/** Empty is legal and common: `build { }` is OPTIONAL in this format, unlike Shape's and Move's. */
	TArray<FGcRigBuildLine> Build;

	TArray<FGcRigProfile> Profiles;
	TArray<FGcRigNamedPoint> Points;
	TArray<FGcRigRegion> Regions;
	TArray<FGcRigChain> Chains;
	TArray<FGcRigJoint> Joints;
	TArray<FGcRigSkin> Skin;
	TArray<FGcRigSocket> Sockets;
	TArray<FGcCharacterControl> Controls;
	/** Source generation policy, separate from the calibrated runtime control value. */
	TMap<FString, bool> ControlOverrides;
	TArray<FGcCharacterContact> Contacts;
	TArray<FGcCharacterGrip> Grips;
	TArray<FGcRigMorph> Morphs;

	/** The raw text, kept so a built asset's `rig:` provenance line can carry a digest of it. */
	FString SourceText;

	/**
	 * Set only on the read-out `PrependImport` puts in front of a file over an already-rigged mesh — never parsed, never
	 * written (Research/imported-bone-direction-and-correctives.md, GC-MOVE-METAHUMAN-BODY-ADAPTER-PLAN.md). Its chains
	 * are hierarchy coverage, one root-to-leaf path per leaf, not an author saying which way a limb runs: a MetaHuman's
	 * corrective leaves would otherwise vote on where its upper arm points.
	 */
	bool bImported = false;

	const FGcRigNamedPoint* FindPoint(const FString& Name) const;
	const FGcRigRegion* FindRegion(const FString& Name) const;
	const FGcRigChain* FindChain(const FString& Name) const;
	const FGcRigJoint* FindJoint(const FString& Name) const;
	const FGcRigSkin* FindSkin(const FString& Name) const;
	const FGcRigProfile* FindProfile(const FString& Name) const;
	const FGcRigMorph* FindMorph(const FString& Name) const;
};

class GRAPHCODERIG_API FGraphCodeRigParser
{
public:
	/**
	 * Parse a whole file. False leaves `OutRefusal` saying WHAT was found, with the line number.
	 *
	 * **WHAT IS REFUSED HERE IS A PROPERTY OF ONE LINE.** A name that resolves nowhere, a joint with
	 * limits whose region was never placed, a twist-only joint whose region was asked to crease —
	 * those are properties of the merged rig, and they belong to the linker. Splitting them the other
	 * way would put half a rule in each of two places.
	 */
	static bool Parse(const FString& Text, FGcRigDocument& OutDocument, FString& OutRefusal);

	/** `back` -> `EGcRigCloses::Back`, and `Unset` for anything else. Shared with the linker. */
	static EGcRigCloses ParseCloses(const FString& Word);

	/** A direction in a bone's own frame: `X`, `-Z`, `+Y` or `(x, y, z)`. False for anything else. */
	static bool ParseAxis(const FString& Word, FVector& Out);

	/**
	 * A direction stated by two of the rig's points, `($heel_r, $web_r)`: the way from the first to the second
	 * (2026-10-04, RifleMan.gcrig: a handle runs across the palm from its heel to the web, at a slant no axis
	 * of the hand bone names). The names come back without their `$`. False for anything else.
	 */
	static bool ParsePointLine(const FString& Word, FString& OutFrom, FString& OutTo);

	/** Bone names: one bare name or `(a, b, ...)`. False where there is none or one is empty. */
	static bool ParseNames(const FString& Word, TArray<FString>& Out);

	/** The word for a value, for a refusal or a read-back. */
	static const TCHAR* ClosesName(EGcRigCloses Value);
};
