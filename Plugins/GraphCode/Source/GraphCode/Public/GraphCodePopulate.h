// Copyright GraphCode Sdn Bhd. All Rights Reserved.
//
// The `populate { }` block — WHAT FILLS A PLACE, `GC-TERRAIN-DESIGN.md` §12a.3.
//
// **A GENERATOR IS AN OPERATION WITH A DOMAIN, NOT A PROPERTY OF A REGION** (user-ruled 2026-08-19).
// It shipped first as a token on a region body, where the region WAS its domain and `except=(…)` cut
// holes in it — which says one region, or one region minus others, and cannot say a point with a
// radius or a course. §9.8 had already classified it correctly before it was built: `nav(…)` stays
// region-body because it is a property of a region (this region is walkable); terrain and `pcg` are
// operations with a domain. So the block is where it belongs, and the region-body form is retired by
// name (`gcw_old_format`) rather than left legal beside it — two spellings of one concept in shipped
// code is the bug this project polices.
//
// **THE PLACE VOCABULARY IS TERRAIN'S, RUN THROUGH TERRAIN'S OWN CODE.** `in=`, `at=`, `along=`, the
// offsets and the `[all(…)/any(…)/not(…)]` filter are `FGcTerrainDomain`/`FGcTerrainFilter` and are
// parsed by `GcwSpatialText`, so a `populate { }` line and a terrain line say a place with the same
// characters because they run the same function ([[one-concept-two-grammars]]).

#pragma once

#include "CoreMinimal.h"
#include "GraphCodeTerrain.h"

/**
 * One line of the block: `Meadow = pcg(graph=/Game/PCG/PCG_Meadow, in=$Valley, seed=11) [filter]`.
 *
 * **THE NAME IS THE VOLUME'S TAG**, which is why it is unique across the whole block rather than per
 * region: it names the actor the entry produces, and two of them would be one actor answering to
 * both. Under the region-body form the volume was `Region_Generator`, because a name was only unique
 * inside its room; a file-scope line has no room to be scoped by, and the author picks a name that
 * says what grows rather than one that repeats where.
 */
struct GRAPHCODE_API FGcPopulateEntry
{
	FString Name;

	/**
	 * WHICH VERB THIS LINE IS — `pcg(…)`, `foliage(…)` (G8 rung 2) or `audio(…)` (§13).
	 *
	 * **THE BLOCK WAS ALWAYS PLURAL AND ONLY EVER HELD ONE.** Its own header says *what fills a
	 * place*, and the user's direction 2026-08-20 was that painted foliage joins it rather than
	 * growing a block of its own: the domain, the band, the filter, the seed and the priority are
	 * the same five questions, so a second block would be [[one-concept-two-grammars]]. A reverb
	 * zone asks the same questions and joined on the same ruling 2026-08-21.
	 *
	 * **WHAT THE VERB DECIDES IS THE OTHER HALF OF THE LINE, and the halves do not overlap at all.**
	 * `graph=`, `grid=`, `trigger=` and `params=` are things a PCG COMPONENT has; `types=`
	 * and `density=` are things a scatter has; `effect=`, `volume=`, `fade=`,
	 * `interior=` and `exterior=` are things an `AAudioVolume` has. A key belonging to another verb
	 * is refused BY NAME rather than ignored, because a `grid=` silently dropped from a foliage line
	 * reads exactly like a `grid=` that worked.
	 *
	 * **TWO KEYS HAVE MORE THAN ONE OWNER, WHICH IS WHY THE TABLE ANSWERS A MASK AND NOT A VERB.**
	 * `enabled=` belongs to `audio(…)`, `postprocess(…)` and `triggervolume(…)`; `tags=` belongs to
	 * `pcg(…)` and `triggervolume(…)`. Each is ONE fact reaching one engine field, and splitting
	 * either into a per-verb spelling would be two carriers of one thing
	 * ([[two-carriers-of-one-fact-is-a-race]]).
	 *
	 * **AND THE VERB ALSO DECIDES WHICH SHARED KEYS ARE REQUIRED**, which is the shape this block
	 * had from its second verb onward: `graph=` is required on one and refused on the next, `seed=`
	 * is required on the two that roll and refused on the one that does not, and `priority=` is
	 * optional on the two that can stack and required on the one whose ties the engine breaks
	 * arbitrarily.
	 */
	enum class EVerb : uint8
	{
		Pcg,
		Foliage,
		/**
		 * `audio(…)` — an `AAudioVolume`, `GC-WORLD-GENERATION-DESIGN.md` §13. User-ruled into this
		 * block 2026-08-21 rather than into a region body or a block of its own.
		 *
		 * **A REVERB ZONE FILLS A PLACE TOO**, and it asks the same three questions of one: which
		 * region, how tall, what does the filter cut out. §13.2 parked it as a region-body token only
		 * because a domain-carrying line would then have shipped with three of its four domains
		 * refused; §12a.6's rung 3 closed two of those three, so the condition it named is gone.
		 *
		 * **WHAT IT DOES NOT SHARE IS THE SEED.** Every other verb here owes the determinism
		 * contract because it ROLLS something; a zone is deterministic by construction, so `seed=`
		 * is refused on this verb rather than required — a key that reaches nothing is worse than
		 * a key that is missing ([[a-silent-no-op-is-worse-than-a-refusal]]).
		 */
		Audio,
		/**
		 * `postprocess(…)` — an `APostProcessVolume`, `GC-WORLD-GENERATION-DESIGN.md` §14. Row G5's
		 * successor, user-ruled into this block 2026-08-21.
		 *
		 * **THE ROW ARRIVED ASKING A DIFFERENT QUESTION AND THE RULING REFRAMED IT.** G5 was *a
		 * hand-authored volume takes a region's outline*, which read as a way to ADOPT an actor
		 * somebody else placed. It is not: the gap is that this block raises two volume types and
		 * stops. A post-process volume is the same act as an audio one — a region, a band, a
		 * filter and a class — so it is a verb here rather than a mechanism anywhere else.
		 *
		 * **IT ROLLS NOTHING, SO `seed=` IS REFUSED**, exactly as on `Audio` and for the same
		 * sentence. **`priority=` IS OPTIONAL, WHICH IS THE OPPOSITE OF `Audio`** and is measured
		 * rather than assumed: §14.3 trap 1 — `PostProcessVolumeLess` is a total order (priority,
		 * unbound, size, GUID), where `AAudioVolume`'s own comment says ties are undefined. One
		 * verb's requirement must not become another's
		 * ([[a-shared-pass-lends-one-consumer-s-constraint-to-another]]).
		 */
		PostProcess,
		/**
		 * `triggervolume(…)` — an `ATriggerVolume`, `GC-WORLD-GENERATION-DESIGN.md` §14.6. The
		 * fifth verb, user-ruled into this block 2026-08-21 in the same breath as §14.
		 *
		 * **THE ONLY VERB HERE WHOSE CLASS DECLARES NOTHING, AND THAT IS MEASURED.**
		 * `ATriggerVolume` is `AVolume` plus a constructor that sets the `Trigger` collision profile
		 * and a green brush colour — not one `UPROPERTY` of its own (§14.6a). So where §13 and §14
		 * read a header and let the grammar fall out of the class, this verb's key set had no
		 * header to read and was a CHOICE, which is the user's rather than an implementation's
		 * ([[an-analogy-is-not-a-licence-to-invent]]).
		 *
		 * **WHAT IT SAYS IS *A SOLID IN THE RIGHT PLACE CARRYING A HANDLE*.** A trigger does nothing
		 * by itself; something else has to find it. `Name` is already the volume's actor tag for
		 * every verb in this block, so `Airlock = triggervolume(in=$Corridor)` answers
		 * `GetActorsWithTag` the moment it exists — which is why the ruled key set adds NO new key
		 * and widens two that already exist: `tags=` from `Pcg` (the same `AActor::Tags` array) and
		 * `enabled=` from `Audio`/`PostProcess` (here `SetActorEnableCollision`).
		 *
		 * **IT ROLLS NOTHING, SO `seed=` IS REFUSED** — §13's sentence, third customer. **AND IT IS
		 * THE FIRST VERB TO REFUSE `priority=`**, which no key-ownership table can express: nothing
		 * arbitrates trigger volumes and no engine sort reads a trigger's rank, so the key would
		 * reach nothing ([[a-silent-no-op-is-worse-than-a-refusal]]). That is *this verb has no use
		 * for it*, not *this key belongs to another verb*, and the two refusals must not read alike.
		 *
		 * **AND IT HAS NO *SAYS WHERE, NEVER SAYS WHAT* REFUSAL — §14.6d.** The other three have one
		 * because a volume with no content fills its region with nothing; this one's content IS the
		 * region. Adding it for symmetry would make the most ordinary line anybody writes illegal.
		 */
		TriggerVolume,
		/**
		 * `physicsvolume(…)` — an `APhysicsVolume`, `GC-WORLD-GENERATION-DESIGN.md` §14.7. The
		 * sixth verb, user-directed 2026-08-22 in the same breath as the seventh.
		 *
		 * **A PLACE WHERE THE RULES OF MOVEMENT ARE DIFFERENT** — terminal velocity, fluid friction,
		 * whether it counts as water, and whether touching it is enough. Five `UPROPERTY`s, all
		 * `EditAnywhere`, all read at runtime by `UCharacterMovementComponent` or by the volume's
		 * own `IsOverlapInVolume` (MEASURED 2026-08-22; no key here reaches nothing).
		 *
		 * **`priority=` IS REQUIRED, AND IT IS THE ONLY VERB HERE WHOSE DEFAULT MAKES THE ACTOR
		 * INERT.** `USceneComponent::UpdatePhysicsVolume` starts from the world's default volume and
		 * takes another only on `V->Priority > NewVolume->Priority` — STRICTLY greater
		 * (`SceneComponent.cpp:3181`). Neither `APhysicsVolume`'s constructor nor
		 * `ADefaultPhysicsVolume`'s assigns `Priority`, so both are 0, and a volume left at the
		 * default never wins the comparison and never applies to anything. So this is not `audio`'s
		 * *ties are undefined* — it is *silence ships a volume that does nothing*, which is the one
		 * outcome this grammar refuses to let an author reach
		 * ([[a-silent-no-op-is-worse-than-a-refusal]]).
		 *
		 * **AND IT IS THE THIRD VERB WITH A *SAYS WHERE, NEVER SAYS WHAT* REFUSAL.** A line carrying
		 * only a place and a rank describes the default volume with a bigger number on it: the two
		 * scalars come from `UPhysicsSettings` for the default volume and for this one alike, so
		 * nothing about moving through it would change. At least one of `water=`, `friction=`,
		 * `terminal=` and `contact=` has to say what is different.
		 *
		 * **IT ROLLS NOTHING, SO `seed=` IS REFUSED** — §13's sentence, fourth customer.
		 */
		PhysicsVolume,
		/**
		 * `killzvolume(…)` — an `AKillZVolume`, `GC-WORLD-GENERATION-DESIGN.md` §14.7. The seventh
		 * verb, and the one that costs almost nothing because it is the sixth with a class swap.
		 *
		 * **`AKillZVolume` IS `APhysicsVolume` PLUS ONE OVERRIDE AND NOT ONE `UPROPERTY`** (MEASURED,
		 * `GameFramework/KillZVolume.h`): `ActorEnteredVolume` calls `FellOutOfWorld`. So its key set
		 * is not *like* the physics one, it IS the physics one — the same five fields on the same
		 * class — and giving it a table row rather than a `kills=` key on the verb above is what
		 * keeps one line from choosing an actor's CLASS ([[a-new-token-not-a-new-line]] cuts the
		 * other way here: the two words already exist, in the engine).
		 *
		 * **`priority=` IS REQUIRED FOR A SHARPER VERSION OF THE SAME REASON.** `ActorEnteredVolume`
		 * is called from `SetPhysicsVolume`, which only runs for the volume that WON the comparison
		 * above — so a kill volume at the default rank does not kill anything. A trigger that never
		 * fires is the worst thing this block could ship.
		 *
		 * **AND IT HAS NO *SAYS WHERE, NEVER SAYS WHAT* REFUSAL**, unlike the verb it borrows every
		 * key from — `triggervolume(…)`'s exemption, for `triggervolume(…)`'s reason. Its content is
		 * the class: a kill volume with all five fields unstated still kills, which is the whole
		 * thing anybody wants one for.
		 */
		KillZVolume,
		/**
		 * `Lava = /Game/Blueprints/BP_LavaVolume(in=$Pit, min=0.0, max=200.0, DamagePerSecond=25.0)`
		 * — the GENERIC line, user-directed 2026-08-22. `GC-WORLD-GENERATION-DESIGN.md` §14.8.
		 *
		 * **IT IS NOT AN EIGHTH VERB. IT IS THE ABSENCE OF ONE**, and that is what the class path in
		 * the verb's position says: any `AVolume` subclass at all, with its properties spelled as the
		 * class declares them, reached through the same codec `actors { }` already uses.
		 *
		 * **WHAT IT EXISTS FOR IS THE CLASS NOBODY HERE CAN WRITE A VERB FOR.** A project's own
		 * `BP_LavaVolume` is an `AVolume` with a `DamagePerSecond` float, and GraphCode ships on Fab —
		 * so the project that declares the class is the CUSTOMER'S ([[the-project-is-not-the-customer]]).
		 * Under a verb-only grammar that volume could never follow a region, ever, and no amount of
		 * verbs we add would change it.
		 *
		 * **AND IT IS A FALLBACK RATHER THAN A REPLACEMENT — A CLASS THAT HAS A VERB IS REFUSED BY
		 * NAME.** The typed verbs are not sugar over reflection; they are the classes where reflection
		 * is WRONG. Measured: `enabled=` is `SetActorEnableCollision` and there is no `bEnabled` on a
		 * trigger or a physics volume at all; `AAudioVolume`'s four authored members are private
		 * behind setters that also update the audio device's proxy, so writing the members leaves a
		 * level that sounds like the old document; `unbound=true` must also clear
		 * `bIsSpatiallyLoaded`, a side effect only the details panel's `PostEditChangeProperty`
		 * applies. A generic line reaching any of those would half-apply in silence. So one spelling
		 * per shape ([[one-concept-two-grammars]]) is enforced by a refusal that NAMES the verb.
		 *
		 * **THE HONEST LIMIT, STATED RATHER THAN DISCOVERED**: a generic line gets plain property
		 * writes — no setters, no side effects, and no required-key refusal, because nothing here
		 * knows what a stranger's class requires. That is not a new risk: it is exactly the contract
		 * `actors { }` already has, in a file that adds a place. **The generic line IS an `actors { }`
		 * entry with a named place instead of an absolute transform** — same class spelling, same
		 * property spelling, a different frame, which is the relationship a `.gcs` has to a `.gc`.
		 *
		 * **IT ROLLS NOTHING AND RANKS NOTHING.** `seed=` is refused for §13's reason, fifth
		 * customer. `priority=` is refused because a rank here would be OURS and the class's own is
		 * the class's — an `APhysicsVolume` subclass writes `Priority=1`, the property, and there is
		 * exactly one carrier ([[two-carriers-of-one-fact-is-a-race]]).
		 */
		VolumeClass,
		/**
		 * `Patrol = spline(along=$Town.high)` — a run PUT INTO THE WORLD, and the ninth verb.
		 * `GC-WORLD-GENERATION-DESIGN.md` §14.12.
		 *
		 * **IT EXISTS BECAUSE UNREAL SHIPS NO PLAIN SPLINE ACTOR, WHICH IS MEASURED AND NOT
		 * ASSUMED** (`GraphCodeWorldSplineTestTypes.h`, 2026-08-22): `ALandscapeSplineActor` carries
		 * a `ULandscapeSplinesComponent` — a different class entirely — and `AWaterBodyRiver` lives
		 * in an optional experimental plugin. So §14.9's class line, which runs a course onto a class
		 * carrying exactly one `USplineComponent`, can only reach a class THE CUSTOMER WROTE. A
		 * `.gcw` could declare a run and had no way to put one in the world on its own, and every
		 * consumer that reads a curve at runtime — an AI following a route, a spline mesh, PCG —
		 * starts at a curve that is actually there ([[the-project-is-not-the-customer]]).
		 *
		 * **SO THE ACTOR IS A BARE `AActor` CARRYING AN INSTANCE `USplineComponent`, AND BOTH ARE
		 * THE ENGINE'S.** That is what a person does by hand — an Empty Actor with a Spline
		 * Component added — and it is what keeps the LEVEL free of GraphCode: `GraphCodeWorld` is an
		 * **Editor** module (`GraphCodeWorld.uplugin`), so a class shipped from it would not exist
		 * in a cooked game and a level holding one could not load. A route has to survive cooking or
		 * it is not a route.
		 *
		 * **IT IS THE FIRST VERB HERE THAT RAISES NO SOLID**, which is where its refusals come from
		 * rather than from a choice: a curve has no top, no bottom and no interior, so the band
		 * (`min=`/`max=`/`follows=`) and `width=` are refused in the sentences §14.9 already wrote
		 * for a spline class line, and `enabled=` goes with them because nothing overlaps a line.
		 * **IT ROLLS NOTHING, SO `seed=` IS REFUSED** — §13's sentence, sixth customer — and it
		 * RANKS nothing, for `triggervolume(…)`'s reason: nothing arbitrates two curves.
		 *
		 * **WHAT IT MUST SAY IS WHERE IT RUNS.** Every other verb here may state a place and let its
		 * class be the content; this one's content IS the run, so a line with no course says
		 * nothing at all and is refused.
		 */
		Spline,
	};
	EVerb Verb = EVerb::Pcg;

	/**
	 * The class a `VolumeClass` line names, verbatim — `/Game/Blueprints/BP_LavaVolume`.
	 *
	 * **RESOLVED AT REALISE AND NOT AT PARSE**, which is `graph=`'s arrangement and for its reason:
	 * a `.gcw` must parse in a spec with no editor and no class loaded, and a refusal about a class
	 * that will not load is a claim about the PROJECT rather than about the document's grammar.
	 * Empty on every other verb.
	 */
	FString ClassPath;

	/**
	 * `DamagePerSecond=25.0, bPainCausing=true` — the properties a `VolumeClass` line states, in the
	 * order the author wrote them.
	 *
	 * **A KEY IS A PROPERTY UNLESS IT IS A SHARED KEY, and the shared set is exactly the one every
	 * verb already has**: the place words, the band words, `layers=`, `tags=` and `enabled=`. A key
	 * belonging to another VERB is still refused by name through `KeyOwners` — `blend=` on a class
	 * line sends the author to `postprocess(…)` rather than becoming a property nothing declares.
	 *
	 * **AUTHORED ORDER, NEVER SORTED**, the rule `types=` and `materials=` already keep: the codec
	 * applies these in order, and a renderer that sorted them would make the document's bytes depend
	 * on something the author cannot see.
	 *
	 * **PATCH, NOT SNAPSHOT — the one place this differs from a `values { }` block.** These are
	 * written and nothing is reset, because the line's OTHER half is a shape the realiser just built:
	 * a snapshot import would reset `BrushBuilder` and `BrushType` to the class default and undo the
	 * volume it is standing on ([[omitting-a-field-is-not-excluding-it]] read the other way — here
	 * omission must NOT mean the archetype, because the archetype is not what made this actor).
	 *
	 * **AND THEY ARE AN APPLY-TIME INPUT: A PARSED LINE MAY CARRY THEM AND A RENDERED ONE NEVER
	 * DOES — user-ruled 2026-08-22, §14.8g.** `build nav`'s contract word for word: carried out,
	 * REPORTED, then stripped from what lands on disk. `RenderEntry` has no branch that writes one,
	 * the reflect pass has no branch that reads one back, and a committed `.gcw` therefore cannot
	 * hold a property waiting to be reasserted.
	 *
	 * **THE REASON IS TWO CARRIERS, NOT TIDINESS.** An actor's properties are already recorded —
	 * completely, and by the file whose job that is — in the level `.gc`'s `actors { }` block. A
	 * `.gcw` that also stated a subset would go stale the moment somebody tuned the volume in the
	 * Details panel, and the next apply would silently reassert the old numbers over their work
	 * ([[two-carriers-of-one-fact-is-a-race]]). So the `.gcw` says WHERE and WHICH CLASS — intent —
	 * and the `.gc` says what the actor IS ([[the-gc-on-disk-is-the-second-carrier]] is the same
	 * seam seen from the other side).
	 *
	 * **WHAT THE KEYS BUY IS THE FIRST APPLY.** Declaring the volume here and then tuning it in the
	 * `.gc` is two steps and always was; stating a value on the line does both at once, once, and
	 * hands the fact over to the file that keeps it.
	 */
	TArray<TPair<FString, FString>> Properties;

	/** `graph=` — a `UPCGGraphInterface` asset path. Required on `pcg(…)`; refused on the others. */
	FString Graph;

	/**
	 * `types=(/Game/Foliage/FT_Pine, /Game/Foliage/FT_Oak)` — the palette this line scatters.
	 * Required on `foliage(…)`, refused on the others.
	 *
	 * **A LIST BECAUSE A PATCH IS A PALETTE**, which is how the engine's own painter works: you pick
	 * several types and paint them together. One type per line would make a mixed wood N lines that
	 * must all carry the same domain, band, filter and seed — four chances for them to drift apart
	 * and no way for a reader to see they were meant as one stand.
	 *
	 * **AND THE ORDER IS PART OF THE CONTRACT** — see `InstanceKey` below. Authored order, never
	 * sorted, for the reason every ordered list in this format keeps: a renderer that sorted them
	 * would make the document's bytes depend on something the author cannot see.
	 */
	TArray<FString> Types;

	/**
	 * `source=file'/Game/_GraphCode/Foliage/<level>/Painted_01.gcfoliage'` — a PAINTED forest, captured
	 * into the file this names. G8 rung 4, user-ruled 2026-09-11.
	 *
	 * **THE OTHER HALF OF `types=`, AND NEVER BESIDE IT.** A scatter says which types and where, and a
	 * seed rolls the rest; a painted forest has nothing to roll, so its file holds every instance's type
	 * and transform and the line states no place, band, density, seed or filter. Empty on every line
	 * that is not one.
	 */
	FString Source;

	/**
	 * `density=0.4` — a MULTIPLIER on each type's own density, not a replacement for it.
	 *
	 * **NOT A SECOND CARRIER OF THE TYPE'S DENSITY** ([[two-carriers-of-one-fact-is-a-race]]), which
	 * is the objection this key has to answer: `UFoliageType::Density` says how thickly this species
	 * grows and ships in `kind:foliage_type` (G4); this says how much of that this PATCH wants. The
	 * alternative is duplicating a type asset per density, which is exactly the bookkeeping this
	 * format exists to delete.
	 *
	 * Unstated is 1.0 and is not written, one spelling per shape (§20.6).
	 */
	bool bHasDensity = false;
	double Density = 1.0;

	/**
	 * THE ORDERING CONTRACT rung 3's delta keys on — decided here because §5.2a says it must be, and
	 * because a delta keyed on something a scatter never promised is the silently-wrong version of
	 * the whole feature.
	 *
	 * **AN INSTANCE IS `<type path>#<ordinal within that type>`, NEVER A GLOBAL INDEX.** Each type
	 * draws from a stream seeded by (`Seed`, the type's PATH) — **its identity, never its slot in
	 * `Types`**, which is the correction building the sampler forced: seeded by slot, inserting a
	 * species in the middle re-rolls every later type's whole forest, so the key would still resolve
	 * and would name a tree that had moved for reasons nobody wrote down. Given that, so:
	 *
	 * - adding a type — at the end OR in the middle — renumbers nothing, because an ordinal is
	 *   scoped to its own type rather than to the line
	 * - a hand-ADDED instance belongs to no type's sequence, so it cannot collide with an ordinal;
	 *   rung 3 records it as an addition, which is a different statement from a move
	 * - changing `seed=`, the domain, the band or the filter re-rolls the scatter and VOIDS the
	 *   delta — the same rule a volume's `moved(…)` already lives by when its builder changes
	 *
	 * A global index fails the first two ([[an-identity-key-orders-the-whole-corpus]]): inserting one
	 * instance at the front would move every tree after it onto a neighbour's record, and nothing
	 * about the file would look wrong.
	 */
	static FString InstanceKey(const FString& TypePath, int32 Ordinal)
	{
		return FString::Printf(TEXT("%s#%d"), *TypePath, Ordinal);
	}

	/**
	 * `seed=` — **required and never defaulted, on every verb that rolls something.**
	 *
	 * `GC-SPATIAL-AUTHORING-DESIGN.md` §8's determinism contract, applied rather than re-argued: a
	 * request without a seed is refused. A generator whose seed came from the engine produces a
	 * different forest on a machine that loaded the packages in a different order.
	 *
	 * **`audio(…)` IS EXEMPT AND REFUSES THE KEY**, which is the contract honoured rather than
	 * dropped: a zone rolls nothing, so a seed on that line would index a sequence nobody draws
	 * from. Required would make every author type a number that changes nothing, and accepted-then-
	 * ignored is the silent version of the same thing.
	 */
	int32 Seed = 0;

	enum class ETrigger : uint8
	{
		Load,
		Demand,
		Runtime,
	};

	/**
	 * `trigger=load|demand|runtime`, lowercase per `GC-STATE-MACHINE-DESIGN.md` R8.
	 *
	 * **THE DEFAULT IS NOT WRITTEN** — one spelling per shape (§20.6): `trigger=load` in a payload is
	 * accepted and canonicalised away, so two files that mean the same thing cannot differ in bytes.
	 */
	ETrigger Trigger = ETrigger::Load;

	/**
	 * `params=(Density=0.8, Species="Pine")` — overrides into the component's own graph instance.
	 *
	 * **THE BUNDLE IS THE COMPLETE OVERRIDE SET.** An override IS its membership in
	 * `PropertiesIDsOverridden`, so a parameter this stops naming stops being overridden and returns
	 * to the graph's own value — `kind:pcg_graph_instance`'s shipped ruling (G3b), kept rather than
	 * given a second answer.
	 */
	TArray<TPair<FString, FString>> Params;

	/**
	 * `priority=` — **WHICH GENERATOR WINS where two of them claim the same ground.**
	 *
	 * `GC-SPATIAL-AUTHORING-DESIGN.md` §21.9 ruled that regions MAY overlap and that containment
	 * answers both, deliberately unarbitrated — right for a QUESTION. Generation is not a question:
	 * two lines whose domains overlap both realise, both graphs run over the shared ground, and the
	 * result is double density with nothing anywhere saying so.
	 *
	 * **HIGHER WINS, AND A PRIORITY IS COMPILED INTO THE EXCLUSION THAT ALREADY SHIPS** — the loser
	 * gains the winner's outline as a HOLE before realisation, so this costs no geometry code at all.
	 *
	 * **UNSTATED IS ADDITIVE AND IS NOT THE SAME AS `priority=0`**, which is why the flag exists
	 * beside the number. Overlapping generators are often deliberate — grass and boulders on one
	 * hillside — so silence must not mean *arbitrate*; it means *both*, and the plan REPORTS the
	 * overlap so a double density nobody asked for is visible without being refused.
	 *
	 * **ONE MEANING, TWO MECHANISMS, AND `audio(…)` IS THE SECOND.** *Higher wins the overlap* is
	 * what this key says for every verb; what differs is who enforces it. A generator's loser is cut
	 * — the winner's outline becomes a hole before anything is spawned — while an audio volume's
	 * loser stays exactly where it is and the ENGINE picks per listener, sorting its proxies by
	 * `Priority` and taking the first that contains them (`AudioVolume.cpp:120,190`). So an audio
	 * line never enters `Arbitrate` and never cuts a generator, which is the only way one key could
	 * have become [[one-concept-two-grammars]].
	 *
	 * **AND ON `audio(…)` IT IS REQUIRED RATHER THAN OPTIONAL** — §13.3 trap 2. UE says so in the
	 * property's own comment: *the order is undefined if two or more overlapping volumes have the
	 * same priority*, and the field defaults to 0, so a file leaving it silent would let an
	 * overlapping room and corridor pick a winner by `TMap` order and the determinism contract would
	 * be quietly false ([[equal-ranks-sort-arbitrarily]]). A verb changing whether a shared key is
	 * required is this block's own established shape — `graph=` is required on one verb and refused
	 * on the next.
	 *
	 * **A WHOLE NUMBER EVEN THOUGH THE ENGINE'S FIELD IS A FLOAT.** A rank is a count, and one
	 * spelling of a rank across three verbs is worth more than reaching the fractions between two
	 * of them — the comparison is `>` at both ends.
	 */
	int32 Priority = 0;
	bool bHasPriority = false;

	/**
	 * `grid=25600` — the streaming grid this generator's COMPONENT sits on, in centimetres. **0 is
	 * unstated**, which is exactly today's behaviour: unpartitioned, on the unbounded grid.
	 *
	 * ONE TOKEN FOR TWO ENGINE FIELDS (`bIsComponentPartitioned` and `GenerationGridSize`) because
	 * they are one decision. The legal values are the engine's and nothing else — `GcPopulate::
	 * LegalGridSizes()`, which a GraphCodeWorld spec cross-checks against `PCGHiGenGrid::
	 * IsValidGridSize` so this list cannot drift from the enum it mirrors.
	 *
	 * **THE GRAPH HALF IS THE GRAPH'S.** `bUseHierarchicalGeneration` and `HiGenGridSize` live on
	 * `UPCGGraph` and belong to `kind:pcg_graph`'s property projection. This key says where the
	 * component sits, which is a fact about this level.
	 */
	int32 Grid = 0;

	/**
	 * `tags=(Biome="Forest", BiomePrio=2, Riverbank)` — what the volume TELLS the graph that reads it.
	 *
	 * **AND SINCE §14.6, WHAT IT TELLS ANYTHING ELSE THAT GOES LOOKING.** `triggervolume(…)` widened
	 * this key rather than growing a word of its own, because the field is the same field: these
	 * land in `AActor::Tags`, which is where a PCG graph reads `Biome:Forest` from and equally where
	 * an overlap handler reads `Airlock` from. One array, one key ([[one-concept-two-grammars]]).
	 *
	 * A generated volume already carries `GraphCode.Region:<region>`, and that is the whole
	 * vocabulary: a graph can find our volume by the region's name and nothing else. Biome graphs
	 * are driven by attributes they read off what they are given (`BIOME`, `BIOMEPRIO`, `PRIO`,
	 * `RecursionLevel` in `BiomeCore_DifferenceByPriority`), so a volume that can carry only its own
	 * name can be found but cannot be TOLD anything.
	 *
	 * **THE DIVIDER IN THE FILE IS `=` AND THE DIVIDER ON THE ACTOR IS `:`, AND THAT IS MEASURED
	 * RATHER THAN CHOSEN.** `PCG::Private::ParseTag` (`PCGTagHelpers.cpp`) splits an actor tag on a
	 * COLON — `Biome:Forest` is attribute `Biome` value `Forest`, a numeric right side becomes a
	 * double and `True`/`False` becomes a bool. So the document keeps this grammar's own `key=value`
	 * spelling, which `params=` already uses, and the realiser emits the engine's spelling. A `.gcw`
	 * that said `Biome:Forest` would be [[one-concept-two-grammars]] against the line above it.
	 *
	 * A VALUELESS ENTRY IS A PLAIN HANDLE — `Riverbank` becomes the bare tag `Riverbank`, which is
	 * what a graph filtering on tag presence reads. Stored with an unset value rather than an empty
	 * one, because `Riverbank` and `Riverbank:` are different tags to `ParseTag`.
	 *
	 * **THE VALUE IS STORED EXACTLY AS THE AUTHOR WROTE IT**, quotes and all, and is unquoted once
	 * at realisation. Held unquoted here, a round trip would re-quote it and the file would grow a
	 * layer per pass ([[a-reader-that-spells-owes-the-writer-an-inverse]]).
	 */
	struct FTag
	{
		FString Name;
		FString Value;
		bool bHasValue = false;
	};
	TArray<FTag> Tags;

	/**
	 * `layers=(/Game/DataLayers/DL_Interior)` — the DATA LAYERS this line's actor belongs to.
	 * World Partition row 7, `GC-WORLD-PARTITION-DESIGN.md` §6.1.
	 *
	 * **THE ONLY KEY EVERY VERB IN THIS BLOCK TAKES, and it is the one thing every verb here does:
	 * it puts something in the world.** `graph=` is a PCG component's, `types=` a scatter's,
	 * `effect=` a zone's — this is the world's, so no verb is exempt and `KeyOwners` answers 0.
	 *
	 * **PLURAL, BECAUSE THE ENGINE IS — user-ruled 2026-08-22.** `AActor::DataLayerAssets` is a
	 * `TArray<TSoftObjectPtr<UDataLayerAsset>>` and the Data Layers outliner lets a person tick
	 * several, so a singular `layer=` could not state what §6.1's reflect would read back off an
	 * actor somebody put in two ([[inverting-a-lossy-projection-can-be-impossible]]).
	 *
	 * **NAMED BY THE LAYER ASSET'S PACKAGE, which is row 6's spelling and not a new one.**
	 * `data_layers { }` names its entries the same way, for the same measured reason: a layer
	 * INSTANCE's name is `DataLayer_<a fresh FGuid>` ([[gc-simple-name-ruling]]). One vocabulary for
	 * a data layer across the `.gcw`, the level `.gc` and the actor's own document.
	 *
	 * **ABSENCE IS THE DEFAULT AND IS LEGAL — user-ruled 2026-08-21.** A line with no `layers=`
	 * places its actor in no data layer, streamed by the spatial grid alone: every `.gcw` written
	 * before this key keeps meaning exactly what it meant, so nothing had to be regenerated. What
	 * absence costs is that the reflect must be able to REMOVE the key and not only write it — a
	 * person dragging a volume out of a layer is a 1→0 transition
	 * ([[a-gate-on-the-desired-state-cannot-see-a-removal]]).
	 *
	 * **AND ON `foliage(…)` IT REACHES A DIFFERENT DOOR, WHICH IS MEASURED RATHER THAN ASSUMED.** A
	 * scatter raises no actor of its own — it plants into an `AInstancedFoliageActor`, which is a
	 * PARTITION actor whose identity is `(Class, GridGuid, ContextHash)` and whose `ContextHash`
	 * folds in `UDataLayerManager::GetDataLayerEditorContextHash()`. So the engine already keeps a
	 * separate IFA per layer set, and this key selects WHICH ONE a line plants into rather than
	 * moving an actor afterwards. See `GraphCodeWorldFoliagePlant.cpp` for why the afterwards form
	 * is the trap.
	 */
	TArray<FString> Layers;

	/**
	 * One end of an `AAudioVolume`'s ambient zone — `interior=(volume=0.6, lpf=2000.0)`, §13.1.
	 *
	 * **THE FOUR NAMES ARE THE ENGINE'S OWN AND SO IS WHICH SIDE IS WHICH**, which is worth writing
	 * down because both readings are plausible and only one is UE's. `FInteriorSettings::
	 * InteriorVolume` is *the volume of sounds INSIDE the zone heard while the listener is OUTSIDE
	 * it*, and `ExteriorVolume` is *the volume of sounds outside heard from within* — so
	 * `interior=` is about the sounds this zone contains and `exterior=` about the world beyond it.
	 * Read off `AudioVolume.h`'s own comments in 5.7, never inferred from the names.
	 *
	 * **A PARENTHESISED GROUP RATHER THAN EIGHT FLAT KEYS.** `params=(Density=0.8)` is already this
	 * shape and `GC-GRAMMAR-REFERENCE.md` §3a says keyed members inside parens exist everywhere in
	 * the corpus while `key:subkey=value` exists nowhere in it. Flat, `interiorLpfTime=` and
	 * `exteriorLpfTime=` are a grouping the reader has to know rather than see.
	 *
	 * **EACH MEMBER CARRIES ITS OWN FLAG AND AN UNSTATED ONE IS THE ENGINE'S DEFAULT**, which the
	 * realiser supplies by starting from a default-constructed `FInteriorSettings` — so this struct
	 * never has to hold a copy of numbers the engine owns ([[config-on-disk-not-the-header-default]]).
	 */
	struct FAmbientZone
	{
		bool bHasVolume = false;
		double Volume = 0.0;
		bool bHasTime = false;
		double Time = 0.0;
		bool bHasLpf = false;
		double Lpf = 0.0;
		bool bHasLpfTime = false;
		double LpfTime = 0.0;

		bool IsEmpty() const { return !bHasVolume && !bHasTime && !bHasLpf && !bHasLpfTime; }
	};

	/** `interior=(…)` — `AmbientZoneSettings.Interior*`. Audio's; refused on the other two verbs. */
	FAmbientZone Interior;

	/** `exterior=(…)` — `AmbientZoneSettings.Exterior*`. */
	FAmbientZone Exterior;

	/**
	 * `effect=/Game/Audio/Rev_Cathedral` — a `UReverbEffect` asset path.
	 *
	 * **ITS PRESENCE IS `Settings.bApplyReverb`, and that is one fact with one carrier.** Stating
	 * both would let a file say *apply this reverb, and do not* ([[two-carriers-of-one-fact-is-a-race]]).
	 * The engine's own default is `bApplyReverb = true` with a null effect; a line that names no
	 * effect turns it OFF, because a zone that states only an ambient band is not asking for reverb.
	 */
	FString Effect;

	/**
	 * `volume=` — `Settings.Volume`, the reverb's own level. **Refused without `effect=`**: with
	 * `bApplyReverb` off it configures a reverb nothing applies, which reads exactly like a value
	 * that worked.
	 */
	bool bHasReverbVolume = false;
	double ReverbVolume = 0.0;

	/** `fade=` — `Settings.FadeTime` in seconds. Refused without `effect=`, for `volume=`'s reason. */
	bool bHasFade = false;
	double Fade = 0.0;

	/**
	 * `enabled=false` — `AAudioVolume::bEnabled`, and `APostProcessVolume::bEnabled`.
	 *
	 * **UNSTATED IS TRUE AND IS NOT WRITTEN** (§20.6), which is both this grammar's rule and the
	 * engine's own constructor value, so the two cannot disagree about what silence means.
	 *
	 * **SHARED BY EVERY VERB THAT RAISES A VOLUME, rather than one member each.** Two classes spell
	 * it `bEnabled`, default it true in their constructor and mean *this volume does nothing while
	 * it is false*; the other three have no such field and reach the same state through
	 * `SetActorEnableCollision`, which is the door a trigger, a physics volume and a kill volume all
	 * stop working at — every one of them is found by a collision query, so an actor with collision
	 * off is not returned to anybody. **FIVE DOORS, ONE QUESTION**, and a second member would be two
	 * carriers of one key ([[two-carriers-of-one-fact-is-a-race]]); `KeyOwner` is what stops a
	 * `pcg(…)` line reaching it.
	 */
	bool bEnabled = true;

	/**
	 * `materials=(/Game/PP/M_Desaturate, …)` — `Settings.WeightedBlendables.Array`, §14.2.
	 *
	 * **THE ONE FIELD OF `FPostProcessSettings` THAT NAMES AN ASSET**, and the direct analogue of
	 * `audio(…)`'s `effect=`: it is what this volume DOES, where the other ~300 fields are how it
	 * is tuned. Those stay in the `.gc`, which already records them accurately — putting them here
	 * would claim values a realise then reverts, turning an artist's exposure tweak into something
	 * the document undoes.
	 *
	 * **PLURAL, TAKING `types=(…)`'s SPELLING RATHER THAN `effect=`'s**, because the underlying
	 * field is an array: a singular key would have to grow a second spelling the first time
	 * somebody stacks two blendables ([[a-new-token-not-a-new-line]]). Per-entry `Weight` is not in
	 * v1 and every entry realises at 1.0 — named as deferred rather than discovered later.
	 */
	TArray<FString> Materials;

	/**
	 * `blend=` — `APostProcessVolume::BlendRadius`, world-space centimetres.
	 *
	 * **IGNORED BY THE ENGINE WHEN `unbound=true`**, and NOT refused with it. The two are
	 * independent properties whose combination is merely inert, unlike `volume=` without `effect=`
	 * one verb over, where the reverb block is switched OFF and the value would configure something
	 * nothing applies. A radius on an unbound volume is a value that stops mattering the day the
	 * volume becomes bounded again, which is a reason to keep it rather than to refuse it.
	 */
	bool bHasBlendRadius = false;
	double BlendRadius = 0.0;

	/** `weight=` — `BlendWeight`, 0 = no effect, 1 = full. The VOLUME's weight, not a blendable's. */
	bool bHasBlendWeight = false;
	double BlendWeight = 0.0;

	/**
	 * `unbound=true` — `APostProcessVolume::bUnbound`, *Infinite Extent* in the Details panel.
	 *
	 * **WRITING IT HAS A SIDE EFFECT THE ENGINE ONLY APPLIES FOR THE PANEL** — §14.3 trap 2.
	 * `PostEditChangeProperty` clears `bIsSpatiallyLoaded` when this becomes true, and a
	 * programmatic write never takes that path, so a world-covering volume would keep unloading
	 * with its cell ([[a-hand-set-field-must-outlive-the-import]]). The realiser sets both.
	 *
	 * **NO `bHasUnbound` BESIDE IT, WHICH IS `bEnabled`'S SHAPE AND FOR ONE REASON.** This defaults
	 * false, so `unbound=false` and silence are the same claim and the writer drops both; a flag
	 * recording that the author typed it would be state nothing reads. It is also what keeps the
	 * round trip honest — a stated false that COUNTED as *saying something about what this volume
	 * does* would parse, canonicalise away, and refuse on the next read
	 * ([[a-reader-that-spells-owes-the-writer-an-inverse]]).
	 */
	bool bUnbound = false;

	/**
	 * `terminal=` — `APhysicsVolume::TerminalVelocity`, the speed a falling character is clamped to.
	 *
	 * **UNSTATED IS THE PROJECT'S SETTING, NOT A NUMBER IN THIS FILE.** Both physics-volume
	 * constructors read `UPhysicsSettings::Get()->DefaultTerminalVelocity`, so silence here means
	 * *whatever `DefaultEngine.ini` says* — which is why there is no default written down beside
	 * this member and why the reflect pass compares against the CDO's value rather than a literal
	 * ([[config-on-disk-not-the-header-default]]).
	 */
	bool bHasTerminalVelocity = false;
	double TerminalVelocity = 0.0;

	/**
	 * `friction=` — `APhysicsVolume::FluidFriction`, how hard it is to move through.
	 *
	 * Same story as `terminal=` above: the constructor takes `DefaultFluidFriction` from the project
	 * settings, so unstated is the project's number and not one this grammar owns.
	 */
	bool bHasFluidFriction = false;
	double FluidFriction = 0.0;

	/**
	 * `water=true` — `APhysicsVolume::bWaterVolume`, *this volume contains a fluid like water*.
	 *
	 * **NOT THE WATER PLUGIN, AND THE NAME IS THE ENGINE'S** — this is the flag
	 * `UCharacterMovementComponent` reads to decide whether a character is SWIMMING
	 * (`CharacterMovementComponent.cpp:3734` and three more sites). A `AWaterBody` is a different
	 * subject with its own carriers in the level document, and nothing here writes or reads one; the
	 * two never meet, which is why one short word can be spared for the older of them.
	 *
	 * Defaults false, so `water=false` and silence are one claim and the writer drops both —
	 * `unbound=`'s shape above, for `unbound=`'s reason.
	 */
	bool bWaterVolume = false;

	/**
	 * `contact=true` — `APhysicsVolume::bPhysicsOnContact`, TOUCHING is enough.
	 *
	 * By default an actor's ORIGIN has to be inside the volume for it to apply; with this set,
	 * overlapping the edge does it. Read by `APhysicsVolume::IsOverlapInVolume`, which both
	 * `USceneComponent::UpdatePhysicsVolume` and `UPrimitiveComponent` consult (MEASURED
	 * 2026-08-22), so the key reaches the same decision the priority comparison does.
	 *
	 * Defaults false and is written the same way as `water=` beside it.
	 */
	bool bPhysicsOnContact = false;

	/**
	 * WHERE it fills — `in=$Valley`, `at=$Peak_High radius=1800`, `along=$Valley.east width=600`.
	 *
	 * Stating nothing is the whole world, which is `FGcTerrainDomain`'s own default and correct for
	 * a scatter that covers everything. What each form COSTS to realise is the realiser's business
	 * and is not all built: see `GraphCodeWorldRegionGeneration.cpp`, which fills a region today and
	 * refuses the other three by name rather than filling the wrong volume quietly.
	 */
	FGcTerrainDomain Domain;

	/**
	 * HOW TALL it fills — `min=`, `max=` and `follows=`, §12a.6.
	 *
	 * **THE HALF THE PLACE WORDS NEVER CARRIED.** A domain says where in XY; terrain's consumer needs
	 * one Z per XY and this block's consumer builds a solid, which needs two. An entry that states
	 * none of the three keys is every committed `.gcw` today and keeps its shipped meaning: `in=` takes
	 * the region's own `min:`/`max:`, and the other three domains are still refused at realisation.
	 */
	FGcVolumeBand Band;

	/**
	 * `[not(any(inside=$Stream, inside=$Path))]` — what to leave alone, and the replacement for
	 * `except=(Stream, Path)`.
	 *
	 * `inside=` and `near=` name places; `height=` and `slope=` are values at each sample.
	 * Foliage evaluates the latter on its traced ground hit. PCG and volume verbs still refuse
	 * them: those verbs have no per-sample mask path in this build.
	 */
	FGcTerrainFilter Filter;

	/**
	 * The author's own grouping — full lines immediately above this one, `#` comments and BLANKS.
	 * `Trailing` is the `# …` after it, hash and leading space removed.
	 *
	 * **ON THE ENTRY AND NOT IN THE DOCUMENT'S TRIVIA MAP**, for the reason a terrain instruction
	 * carries its own: the map is keyed by name, and a block the AI rewrites moves lines and renames
	 * them. Keyed by name, a rename silently orphans the heading that explains the line; held here,
	 * the comment travels with the line through both.
	 */
	TArray<FString> Leading;
	FString Trailing;

	/**
	 * WHICH REGION'S BODY DECLARED THIS LINE, or empty for one written in `populate { }` itself.
	 *
	 * **THIS IS NOT §12a.3 COMING BACK.** That ruling retired `Trees = pcg(…)` as a BARE TOKEN on a
	 * region body, where the region WAS the domain and there was no way to say when it ran; the token
	 * form is still refused by name (`gcw_old_format`) and this does not reach it. A scoped block is
	 * NAMED, its lines keep the whole domain vocabulary, and the main block's reference decides the
	 * order — which is precisely the objection §12a.3 was made of, answered rather than reopened.
	 */
	FGcwScopeMark Scope;

	/**
	 * `[override=$PCGVolume_Valley_Forest]` — the `.gc` entry that owns this volume's SHAPE, after a
	 * hand dragged it away from what the domain describes. Held without the `$`.
	 *
	 * **IT SHARES THE FILTER'S BRACKETS, and that is the grammar's own rule rather than a squeeze.**
	 * `GC-GRAMMAR-REFERENCE.md` §133: `[ ]` is *modifiers on the thing to its left*. A filter is one
	 * such modifier and was simply the only one this line had; `override=` joins it by NAME, so the
	 * reader never has to guess which member is which ([[a-new-token-not-a-new-line]]).
	 *
	 * `GC-WORLD-GENERATION-DESIGN.md` §13.4a is the rule, and it is one rule for nav, PCG and water.
	 */
	FString Override;

	/**
	 * Every region this entry names BY A BARE NAME — the domain's, and the filter's.
	 *
	 * The anchor (`at=$Valley.p3`) and the course (`along=$Valley.east`) are deliberately NOT here:
	 * they are reference EXPRESSIONS answered by `FGraphCodeRegionTargets::ResolveOne`, and a second
	 * checker that agreed with it most of the time is the defect [[a-read-answers-through-the-write-path]]
	 * names. The document's check pass asks the resolver for those two and asks this for the rest.
	 */
	void CollectRegions(TArray<FString>& Out) const;
};

/**
 * `populate { }` — every generator in the file, in the order they were written.
 *
 * ONE BLOCK PER FILE, unlike `terrain Name { }`, which is named because a level may hold two
 * landscapes. A generator is placed by its own domain and there is nothing for a second block to be
 * about, so a second one is refused rather than merged.
 */
struct GRAPHCODE_API FGcPopulateBlock
{
	TArray<FGcPopulateEntry> Entries;

	/**
	 * Comments left at the BOTTOM of the block, under every line and inside the closing brace.
	 *
	 * Its own field because it belongs to no entry: attached to the last one it would move when that
	 * line moved, and a note about the block as a whole would follow a forest up the file.
	 */
	TArray<FString> Trailing;

	bool IsEmpty() const { return Entries.IsEmpty(); }
};

/**
 * ONE ORDINAL AND WHAT A HAND DID TO IT, in one facet — `moved(13 -510 710 260)`'s single term.
 *
 * **THE ORDINAL IS THE FIRST NUMBER INSIDE THE VERB, and that is `volumes { }`'s own spelling**
 * (`moved(i x y z, …)`, §12.9j) rather than a new one. It also had to be: `#` is this grammar's
 * COMMENT character, so `<type path>#<ordinal>` — the contract's notation in the design doc — cuts
 * every line in half in the file. Found by a spec, not by reasoning ([[one-concept-two-grammars]]).
 */
struct GRAPHCODE_API FGcwInstanceFacet
{
	/**
	 * Its place in its own type's sequence.
	 *
	 * **STABLE BY CONTRACT AND THAT IS WHY THIS BLOCK CAN EXIST.** Each type draws from a stream
	 * seeded by (`seed`, the type's PATH), so reordering `types=` or adding a species renumbers
	 * nothing, and raising the density is a strict PREFIX — every ordinal keeps its identity and new
	 * trees append ([[an-identity-key-orders-the-whole-corpus]]).
	 */
	int32 Ordinal = 0;

	/** The three numbers after it — a displacement, an angle triple, or a scale delta. */
	FVector Delta = FVector::ZeroVector;

	bool operator==(const FGcwInstanceFacet& Other) const
	{
		return Ordinal == Other.Ordinal && Delta == Other.Delta;
	}
};

/**
 * WHAT A HAND DID TO ONE `foliage(…)` LINE'S INSTANCES OF ONE TYPE — `instances { }`, G8 rung 3.
 *
 * **EVERY FACET IS AN ADDITIVE OFFSET FROM WHAT THE INSTRUCTION COMPUTES, and that is MEASURED
 * rather than chosen** (`GC-WORLD-GENERATION-DESIGN.md` §5.2c). In the corpus every hand edit is an
 * exactly round delta at the editor's own snap — locations in multiples of 10 uu, a rescale in
 * multiples of 0.25 — while nothing about the absolute result is round, because an absolute is that
 * delta plus a base the person never saw. And the edit that does NOT void a delta is sculpting the
 * ground: a scatter's plan XY is pure and its Z is a downward trace, so a terrain edit moves every
 * computed point's height and an offset rides it while an absolute is left behind.
 *
 * **ONE ENTRY PER LINE-AND-TYPE, WHICH IS §12.6a's USER RULING FOLLOWED RATHER THAN ARGUED WITH.**
 * The case for a line per INSTANCE is real here in a way it was not for volumes — an ordinal is a
 * stable identity, so a per-instance line would let one more nudge add one line — and it is given up
 * because the punctuation it needs does not exist: see `FGcwInstanceFacet`.
 *
 * **THE ABSENT LIST IS THE RECORD.** A hand that only turned a tree moved it nothing, and a
 * `moved(9 0 0 0)` beside that would state a fact the line already states by leaving it out.
 */
struct GRAPHCODE_API FGcwInstanceEntry
{
	/** The `populate { }` line that planted them — `Firs`. */
	FString Line;

	/** The foliage type, as the line's own `types=` spells it. */
	FString TypePath;

	/** `moved(i dx dy dz, …)` — how far from the computed point, in uu. */
	TArray<FGcwInstanceFacet> Moved;

	/** `turned(i dpitch dyaw droll, …)` — how far from the dressing's own rotation, in degrees. */
	TArray<FGcwInstanceFacet> Turned;

	/** `scaled(i dx dy dz, …)` — added to the type's own scale, which is what the gizmo does. */
	TArray<FGcwInstanceFacet> Scaled;

	/**
	 * `removed(i, …)` — the instances a hand or a document took away. The line still computes them
	 * and the apply plants none of them. 2026-09-11.
	 *
	 * **ORDINALS ALONE**, because a tree that is gone has no position left to be a distance from.
	 * **A REMOVAL IS A GAP, NEVER A RENUMBERING**: every ordinal past it keeps its identity, so the
	 * deltas on the trees beside it stay on the same trees. Deleting a term brings that tree back,
	 * exactly as deleting a `moved(…)` term puts one back where the line says — the block is the
	 * switch in both directions.
	 */
	TArray<int32> Removed;

	/** Comments above the line, and the one at its end. */
	TArray<FString> Leading;
	FString Trailing;

	/** `Firs./Game/Foliage/FT_Pine` — the entry's identity, as the file spells it. */
	FString Key() const { return Line + TEXT(".") + TypePath; }

	/** A line stating no facet says nothing, and is refused rather than written. */
	bool IsEmpty() const
	{
		return Moved.IsEmpty() && Turned.IsEmpty() && Scaled.IsEmpty() && Removed.IsEmpty();
	}

	/** The delta for one ordinal in one list, or null. */
	static const FGcwInstanceFacet* Find(const TArray<FGcwInstanceFacet>& In, const int32 Ordinal)
	{
		return In.FindByPredicate(
			[Ordinal](const FGcwInstanceFacet& Each) { return Each.Ordinal == Ordinal; });
	}

	/**
	 * Do these two record the same thing?
	 *
	 * **THE COMMENTS ARE NOT PART OF THE ANSWER, and that is the point of asking here rather than
	 * comparing rendered text.** A capture carries an author's notes across onto the entry it
	 * rewrites, so a note is never a reason to say the record changed — while a re-ordered list, or
	 * a delta that moved in the last decimal, always is.
	 *
	 * **`Removed` IS PART OF THE ANSWER, and leaving it out was a SILENT LOSS rather than a wrong
	 * report.** The save hook writes the `.gcw` only when a capture names an entry as changed
	 * (`GraphCodeWorldVolumeSave.cpp`), so a comparison blind to the removal list would let a hand
	 * that only ERASED trees from a type already carrying a `moved(…)` entry save cleanly and lose
	 * the erasure — and the next apply would plant those trees again.
	 */
	bool operator==(const FGcwInstanceEntry& Other) const
	{
		return Line == Other.Line && TypePath == Other.TypePath && Moved == Other.Moved
			&& Turned == Other.Turned && Scaled == Other.Scaled && Removed == Other.Removed;
	}
	bool operator!=(const FGcwInstanceEntry& Other) const { return !(*this == Other); }
};

/**
 * `instances { }` — what a hand did that no `foliage(…)` line asks for.
 *
 * **THE BLOCK'S PRESENCE IS ITSELF THE SWITCH — USER-RULED 2026-08-23**: *"to override user work,
 * the apply needs to actively delete that [] that tells about user changed data."* So it protects
 * exactly the ordinals it lists and every other instance keeps being replanted from the instruction,
 * which is what lets rung 2 keep its value for the trees nobody touched. **Deleting the block is the
 * instruction to rebuild from intent** ([[the-override-is-a-switch-not-just-a-record]]).
 */
struct GRAPHCODE_API FGcwInstancesBlock
{
	TArray<FGcwInstanceEntry> Entries;

	/** Comments at the bottom of the block, belonging to no line. */
	TArray<FString> Trailing;

	bool IsEmpty() const { return Entries.IsEmpty(); }
};

namespace GcPopulate
{
	/**
	 * The grid sizes `grid=` accepts, in centimetres, ascending.
	 *
	 * **A MIRROR OF `EPCGHiGenGrid`, AND IT LIVES HERE BECAUSE CORE CANNOT ASK PCG.** The dependency
	 * runs one way: Core knows the NAME of every kind in all five products so it can refuse one by
	 * name, and must never need a product's code to do it — so the parser cannot call
	 * `PCGHiGenGrid::IsValidGridSize` even though that is the authority.
	 *
	 * **WHAT KEEPS IT FROM DRIFTING IS A SPEC, NOT CARE.** `GraphCode.World.Generation` walks this
	 * list against the engine's own validator in both directions, so a grid the enum gains or loses
	 * turns this into a red test rather than a silent divergence
	 * ([[gc-capability-registry]] — a hand-written list cannot notice a gap on its own).
	 *
	 * These are the ten the engine shows an author (`UMETA(DisplayName)` on `Grid4`..`Grid2048`).
	 * The enum also declares hidden powers above them; they are deliberately not offered, because a
	 * grid nobody can pick in the editor is not a size an author meant to type.
	 */
	GRAPHCODE_API const TArray<int32>& LegalGridSizes();
}
