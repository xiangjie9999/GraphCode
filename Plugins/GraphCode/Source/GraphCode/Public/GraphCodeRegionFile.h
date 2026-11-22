// Copyright OraStudio. All Rights Reserved.

// The `.gcw` region document — `GC-SPATIAL-AUTHORING-DESIGN.md` §21, tasks R1 and §21.1b.
//
// R0's kernel is pure arithmetic over an `FGcRegionSpace` and says outright that nothing in it reads
// a file. This is the file: a `regions { }` block whose entries are rooms, each declaring its OWN
// points and edges, parsed into that space and rendered back out of it. R2 turns the names below into
// placement targets; R5 writes.
//
// **A REGION OWNS ITS POINTS AND EDGES — §21.1b, user-ruled 2026-08-12.**
//
//     regions {
//       Hall = region(min=0, max=400) {
//         p1 = (0, 0)
//         p2 = (1200, 0)
//         p3 = (1200, 800)
//         p4 = (0, 800)
//         south = p1 -> p2
//         east  = p2 -> p3
//         north = p3 -> p4
//         west  = p4 -> p1
//       }
//     }
//
// **THE ROOMS WERE FILE-SCOPE `region Name  min:… { }` UNTIL M6.1, 2026-08-20**
// (`GC-VCS-MERGE-DESIGN.md` §5.10), and that spelling STILL READS — the reader takes both and the
// writer emits the nested one. It is not deprecation politeness: a file's rooms had to become
// ENTRIES of one block before a three-way merge could tell two rooms apart, because a `.gc` block
// takes its name from a single token and every room in a flat file was therefore a block called
// `region` — the second one a duplicate name, and so a corrupt document.
//
// Three things fall out of that shape, and each replaces a rule rather than adding one:
//
//   * **THE ORDER OF THE EDGES IS THE RING**, so `loop:` is gone. There is no second list to keep in
//     step with the declarations and therefore no way to write a region whose membership and whose
//     ring disagree.
//   * **NAMES ARE SCOPED TO THE REGION.** Every room has a `north` and a `p1`. The flat pool made an
//     alias unique per FILE, so the second room to name a wall was refused `gcw_name_taken` and every
//     room after the first needed hand-rolled prefixes — which is the job a namespace exists to do.
//   * **NOTHING IS SHARED.** *"a region is its own. sharing points makes references and easy to break
//     when i want to change one region."* Two rooms that meet declare their own coincident corners,
//     and `gcw_ask`'s crack list is what measures the gap between them. `split` and `round` still
//     never reach out of the room they were asked about.
//     **THE ONE EXCEPTION, USER-RULED 2026-08-21 (R16, §21.12q): A COURSE MAY NAME ANOTHER ROOM'S
//     POINT** — `road = course b -> $Crypt.gate`. *A `.gcw` contains no cross-references at all* was
//     true until then and is the reason the rest of this paragraph holds; what makes the exception
//     safe is that a course is NOT in `Region.Loop`, so a borrowed point can never deform the room
//     that borrowed it — the user's *easy to break when i want to change one region* is about SHAPE,
//     and a reaching course changes none. Deleting a region a course reaches into is refused at the
//     parser (`gcw_unknown_region`) rather than dangling, which a whole-payload write makes possible
//     to check in one place.
//
// The old file-scope `points { }` and `edges { }` blocks, and `loop:`, are REFUSED BY NAME rather
// than treated as unknown blocks — an unknown block is carried over verbatim, and carrying a whole
// pool of geometry over as opaque text would lose a file's regions in silence.
//
// THE `.gcw` IS NOT ONLY REGIONS. §18.3 made it the home of every `.gcs` subscription — `enemies.gcs
// { … }` — and W0 owns those. This parser must therefore be able to read a file it only half
// understands: it keeps every block it does not know VERBATIM and puts it back, because the
// alternative is that authoring a region silently deletes an author's subscriptions.
//
// THREE THINGS THIS FILE DECIDES THAT §21 DOES NOT STATE:
//
//   1. **A NAME IS A BARE IDENTIFIER.** No quoting, no spaces — letters, digits and underscore,
//      starting with a letter or an underscore. It is forced rather than chosen: these names are
//      embedded in `@anchor[target=$Crypt.north]`, whose grammar owns `[`, `]`, `,` and `=`, and
//      nothing in that grammar parses a quoted identifier. AuthoringCore's backtick spelling exists
//      for engine names an artist typed; a `.gcw` name is minted here and can simply be legal.
//
//   2. **THE MINT COUNTER IS RECORDED, because deriving it from the highest existing name reuses a
//      deleted one.** §21.3 makes the case against renumbering on INSERT; delete-then-mint is the
//      same defect entered from the other side, and it is worse because the reused name resolves.
//      Drop `e9`, mint an edge, and every `@anchor[target=$Crypt.e9]` in the project now points at a
//      different wall with nothing erroring anywhere. The high-water mark is written on the REGION's
//      own line as `minted:p6,e9` — and only when it exceeds what that region's names would derive,
//      so an ordinary room that has never lost an edge carries no bookkeeping at all. It moved off
//      the header with §21.1b: a per-file counter would make adding a second room renumber the
//      first one's corners.
//
//   3. **A POINT IS A COORDINATE — there is no `@` in a `.gcw`.** User-ruled 2026-08-11. A spot
//      partway along an edge is said where it is USED, `@anchor[target=e1@t=50%]`, and resolved through
//      `FGraphCodeRegionSolver::PointOnEdge`. Writing it here bought nothing and cost three things:
//      a second point origin, a second parse pass (a point could name an edge declared below it),
//      and a refusal guarding endpoints against a derivation cycle. All three are gone with it.
//
// RENDERING IS CANONICAL, and the round trip that matters is `render -> parse -> render`, byte for
// byte. It is not `parse -> render` against the author's own bytes: block order, spacing and the
// spelling of every number are this writer's, exactly as they are for a `.gc`. What the author keeps
// is their COMMENTS, because a committed hand-authored file whose comments a tool eats is
// [[ignored-content-is-invisible-loss]] with extra steps.

#pragma once

#include "CoreMinimal.h"
#include "GraphCodePopulate.h"
#include "GraphCodeTerrain.h"
#include "GraphCodeWater.h"
#include "GraphCodeRegion.h"
#include "GraphCodeSpatialPlacement.h"

/** Comment lines and blank lines an author put around a declaration. Typography, never meaning. */
struct GRAPHCODE_API FGcwTrivia
{
	/** Full lines immediately above the declaration — `#` comments and blanks, trimmed, in order. */
	TArray<FString> Leading;

	/** The `# …` after the declaration, hash and leading space removed. Empty when there is none. */
	FString Trailing;
};

/**
 * Which apply-time instruction a line is. There is ONE.
 *
 * **`split` AND `round` WERE RETIRED 2026-08-21, user-ruled** — *"no split or round operation"*.
 * Both rewrote geometry at apply time and left nothing in the file saying it had happened: a split
 * is two walls the author could have written as two walls, and a fillet was frozen the moment
 * either wall moved. A corner's roundness belongs to the corner, as a `[…]` modifier on the point
 * that declares it, where it survives an edit instead of having to be run again.
 */
enum class EGcwOperation : uint8
{
	/** `build nav` — rebuild the level's navigation after the document has been applied. */
	BuildNavigation,
};

/**
 * One APPLY-TIME instruction. `build nav` is the only one left.
 *
 * **IT DOES NOT PERSIST** — `gc_apply` carries it out and it is gone from the file that lands. The
 * renderer has no branch that writes one, so a committed `.gcw` cannot hold an instruction waiting
 * to happen.
 */
struct GRAPHCODE_API FGcwOperation
{
	EGcwOperation Verb = EGcwOperation::BuildNavigation;
};

/**
 * The filters that choose a set of actors — §18.3b, and a group's set is their CONJUNCTION.
 *
 * **AN UNSTATED FILTER FILTERS NOTHING**, which is the rule that makes *everything in the forest* and
 * *one named actor* the same feature rather than two: `class:` was never required, and a group of one
 * is a group. Every field here is optional, and since §18.3i flattened the grammar the one shape that
 * refuses is a line with no filter at all — §5 refuses an ABSENT selector and `in:$world` is a present
 * one. With no mouth left to inherit from, that refusal no longer has a *unless the document scoped
 * it* half.
 *
	 * `in:` IS THE SPATIAL FILTER (§18.3c): it names a region or `$world`, because a wall is a line
	 * and a line contains no actors. The same field is legal on tracking and `exclude` lines; a region
	 * does not get a separate exclusion mechanism.
 */
struct GRAPHCODE_API FGcwSelector
{
	/** `class:BP_Minion` — matched by the tracker, not by this parser. */
	FString Class;

	/** `name:BP_Boss_01` — `.gcp`'s selector, which matches a name OR a label. A group of one. */
	FString Name;

	/** `tag:objective` */
	FString Tag;

	/** `in:$BossRoom` or `in:$world`. A region or the level; never an edge or a corner. */
	FString In;

	/** `near:Door_01` — an anchor to measure from. */
	FString Near;

	/** `within:500`. REQUIRED whenever `Near` is set, for `FGcSpatialSearchRequest`'s own reason. */
	TOptional<double> Within;

	/** Nothing was stated at all — the shape §18.3b sends through the empty-selector refusal. */
	bool IsEmpty() const
	{
		return Class.IsEmpty() && Name.IsEmpty() && Tag.IsEmpty() && In.IsEmpty()
			&& Near.IsEmpty() && !Within.IsSet();
	}
};

/** §18.3's `as:` — §20.6's two spellings of one pair. It is a spelling of the POSITION, never facing. */
enum class EGcwAnchorAs : uint8
{
	Polar,
	Offset,
};

/**
 * `anchor:BP_Boss_01  as:polar` — where a group's members are measured FROM.
 *
 * **THE `from` VERB WENT WITH §18.3i**, which extends §18.3c.1's *one filter spelling, and it is
 * `key:value`* to the whole line. `anchor:` was exempt while a group's clauses sat on lines of their
 * own and the space was what told a clause from a filter by eye; on one flat line that distinction
 * has nothing left to mark, so the verb was a second spelling and nothing else.
 */
struct GRAPHCODE_API FGcwAnchor
{
	bool bStated = false;

	/** The reference: `$BossRoom`, `$BossRoom.door`, or an actor this file knows nothing about. */
	FString From;

	EGcwAnchorAs As = EGcwAnchorAs::Polar;

	/** Written back only when the author said it, so an unstated spelling cannot read as a choice. */
	bool bHasAs = false;
};

/**
 * §20.7's references, and since §22.0 there are THREE of them.
 *
 * `relative` measures off a frame; `target` is aimed AT it; `toward` is aimed at the NEAREST POINT OF
 * A RUN, which `target` cannot say because an unparameterised course answers at its MIDDLE — so a row
 * of twenty houses along one street all aim at the same spot and eighteen of them point wrong.
 *
 * **A THIRD ENUMERATOR AND NOT A SECOND FIELD, because this side already chose an enum.** The
 * placement struct says the same thing with flags (`FGcPlacementFacing::bToward`) since that is the
 * shape `normal=` already had there; what must line up between the two files is the grammar, not the
 * C++ spelling (§22.12a's *near-identity is of the SHAPE*).
 */
enum class EGcwFacingMode : uint8
{
	Relative,
	Target,
	Toward,
};

/**
 * `facing:(relative=$BossRoom.door)` — and this is the whole reason §18.3b's funnel existed.
 *
 * ONE `against:` COULD NOT SERVE BOTH: *where does it stand* and *which way does it look* are measured
 * from different things all the time. `@anchor` and `@facing` were already two structs with two
 * targets, so this carries its own reference for the same reason they do — and that half of §18.3b
 * survived the flattening untouched, because it was never about structure.
 *
 * **THE PARENTHESES ARE MEASURED, NOT CHOSEN — `GC-GRAMMAR-REFERENCE.md` §3a.** A parenthesised value
 * after a colon exists in the corpus (`anchor:(600, 400)`) and keyed members inside parens exist
 * everywhere; `key:subkey=value` appears nowhere in it at all. So the reference keeps the word that
 * says what the angle is measured against (§20.7) without inventing a construction this language has
 * never used.
 */
struct GRAPHCODE_API FGcwFacing
{
	bool bStated = false;
	EGcwFacingMode Mode = EGcwFacingMode::Relative;
	FString Reference;
};

/**
 * One group — ONE LINE, and §18.3i is why it has no name.
 *
 * **THE NAME WENT BECAUSE NOTHING READ IT.** User-ruled 2026-08-13: *"if minions is a declared name
 * and unused by anywhere else, it's not even needed."* §18.3b's groups carried names and nothing
 * anywhere referenced one — not the output, not an anchor, not a filter. **A GROUP'S FILTER LINE IS
 * ITS HEADER**, and it is what identifies the group here, in a refusal, and in the trivia map.
 *
 * **AND THE SCOPE WENT WITH IT**: *"the track in boss room itself is a filter, class of bp minion is
 * also a filter … so all the filters in one line along with anchor and direction."* A group and its
 * document's `track:` were two scopes expressing with STRUCTURE what one line says with content.
 *
 * A GROUP IS STILL THE UNIT §18.4's FACTORING IS COMPUTED OVER, which survived the flattening intact:
 * eight minions sharing `offset:300` factor to one number, where factoring across minions and heroes
 * together would average two populations that have nothing to do with each other.
 */
struct GRAPHCODE_API FGcwGroup
{
	/** Stated whole — there is no baseline left to narrow (§18.3i). */
	FGcwSelector Selector;

	FGcwAnchor Anchor;
	FGcwFacing Facing;

	/**
	 * `align:(right: self=0%)` — which part of each member sits on the point its entry states. §22.18.
	 *
	 * **ON THE GROUP LINE AND NEVER IN AN ENTRY**, for §22.9c's reason: a `.gcs` is regenerated from
	 * measurements and a measurement never answers a fraction. So the rule belongs to the question, and
	 * an entry's `right=` comes back as the plain distance of the face it lines up.
	 */
	FGcPlacementAlign Align;

	/**
	 * `fields: Health, bIsAggro, Team` — §19.13, and ABSENT MEANS PLACEMENT ONLY, i.e. today's file.
	 *
	 * TOP-LEVEL PROPERTY NAMES ONLY. `Inventory[0].Count` refuses by name (§19.13d): the policy this
	 * lowers to resolves only the first path segment, and faking the rest would silently track a whole
	 * array or silently zero a sibling. W1 owns the lowering — the complement of this list becomes
	 * `IsExcludedField`, so an untracked property is never written and never RESET.
	 */
	TArray<FString> Fields;

	/**
	 * Does this list cover a property PATH? §19.13d's reach limit, asked in ONE place.
	 *
	 * **THE GENERATOR AND THE APPLY BOTH ASK IT, WHICH IS WHY IT LIVES HERE.** One decides which
	 * property to WRITE into a `.gcs` and the other which to ACCEPT back out of one, so two copies of
	 * this rule is a field the generator writes and the apply rejects, with nothing erroring. They
	 * were two copies, and they had already drifted: one compared case-sensitively and the other
	 * through `TArray::Contains`, whose `operator==` folds case ([[ue-string-equality-folds-case]]).
	 *
	 * **CASE-SENSITIVE, AND STATED RATHER THAN INHERITED.** A property name is the reflection system's
	 * own spelling; matching `health` to `Health` would track a property the author did not name.
	 * The looser half was the accident — nobody wrote *fold case here*, they wrote `Contains`.
	 */
	bool TracksField(const FString& PropertyPath) const;
};

/**
 * `enemies.gcs  listen:true { … }` — one output file, declared in the level's `.gcw`. §18.3, §18.3i.
 *
 * **A FILE STILL HOLDS SEVERAL GROUPS, and that is what stopped the flattening going one step too
 * far.** User-ruled 2026-08-13: *"gcs file has multiple filters. so enemies in boss room, i can have 2
 * groups. in boss room + BP Minion filter, with anchor and direction to boss, and boss actor inside
 * boss room anchor to the throne."* The mouth went; the groups stayed, each carrying its whole filter
 * set.
 */
struct GRAPHCODE_API FGcwDeclaration
{
	/** `enemies.gcs` — the output NAMED rather than implied, which is what makes the pair navigable. */
	FString Output;

	/**
	 * `listen:true` — §18.3's ruling that CONFIGURATION lives here and not in the file it governs.
	 *
	 * Whether a file is maintained is a decision somebody made; only measurements belong in the output.
	 *
	 * **ON THE DECLARATION'S OWN HEAD LINE SINCE §18.3i, AND THE FLATTENING IS WHAT MOVED IT.** A
	 * declaration's body is now nothing but group lines, so a `listen:true` sitting among them would be
	 * a line whose only key is not a filter — the very shape a reader has no way to tell from a group
	 * that misspelt one. `GC-GRAMMAR-REFERENCE.md` §3a measured the home it went to: a block header is
	 * a name, then `key:value` pairs, then `{`, which is what `region Crypt  min:0  max:400 {` already
	 * is. So this needed no new form, only the one the file next door was using.
	 */
	bool bListen = false;
	bool bListenStated = false;

	TArray<FGcwGroup> Groups;

	/**
	 * `exclude  class:BP_Debug  tag:temporary` — one complete exclusion selector per line.
	 *
	 * Fields on ONE line are a conjunction, exactly like a tracking group; several lines are a union.
	 * These are declaration-wide: a match is absent from every group and from `untracked`. A region
	 * uses the same `in:$House` field here that it uses on a tracking line.
	 */
	TArray<FGcwSelector> Exclusions;
};

/**
 * One `.gcw`: the header, the geometry, the author's typography, and the blocks W0 owns.
 *
 * TRIVIA IS KEYED BY NAME AND NOT BY INDEX — `points/p1`, `edges/e1`, `region/Crypt`. A parallel
 * array is one insert away from attaching a comment to the wrong wall, and a rename is a re-key
 * rather than a resync.
 */
struct GRAPHCODE_API FGcwDocument
{
	/** `gcw 1`. Only 1 exists; a later one refuses by name rather than being read optimistically. */
	int32 Version = 1;

	/** `level:/Game/Maps/Dungeon`. Optional — the file sits beside the level and already says so. */
	FString LevelPath;

	/**
	 * `gen:2` on the header line — which version of THIS PROJECT'S generators the file was authored
	 * against. Unstated is 1, and 1 is never written.
	 *
	 * **ON THE HEADER BECAUSE A VERSION BELONGS AT THE TOP**, user-directed 2026-08-21: *"if needed
	 * header, use the header just like other gc files, version at the top. not middle of the
	 * file"*. It used to ride each `terrain Name  gen:1 {` head, which put a second version line
	 * in the middle of a file whose first line is already `gcw 1` — and made a file with two
	 * programs able to claim two generations of one build.
	 *
	 * **IT IS NOT `Version`.** `gcw 1` is the GRAMMAR's version, which decides whether this reader
	 * can read the file at all; this one pins the generator implementations a program's shape came
	 * out of, and a build that improves a noise function bumps it without changing a character of
	 * the grammar.
	 *
	 * **AND `gen:1` IS NORMALISED AWAY**, because absence and 1 are the same claim and a document
	 * that could say it two ways renders differently depending on which one its author typed.
	 */
	int32 Generation = 1;

	FGcRegionSpace Space;

	/** Lines above the header, and lines after everything. Verbatim, trimmed. */
	TArray<FString> Leading;
	TArray<FString> Trailing;

	/**
	 * Keyed `region/<Region>`, `point/<Region>.<name>`, `edge/<Region>.<name>`, `end/<Region>`.
	 *
	 * QUALIFIED SINCE §21.1b, because `point/p1` now names one corner per room and a single key would
	 * hand the Hall's note to the Cellar. The qualification is this map's alone — nothing in the file
	 * is written that way.
	 *
	 * **A GROUP IS KEYED BY ITS OWN CANONICAL LINE**, `group/<output>.<the line>`, because §18.3i left
	 * it no other identity. That is this map's rule and not an exception to it: the alternative is an
	 * ordinal, and an ordinal is what *keyed by name and not by index* exists to refuse — insert a
	 * group above and every note below it slides onto somebody else's line. The line is unique because
	 * two identical ones refuse (`gcw_name_taken`), and the cost is stated rather than hidden: EDIT a
	 * group's anchor and its comment does not follow, because as far as this map is concerned that is
	 * a different group.
	 */
	TMap<FString, FGcwTrivia> Trivia;

	/**
	 * Pending instructions — `build nav`, and since 2026-08-21 nothing else.
	 *
	 * A PARSED DOCUMENT MAY CARRY ONE AND A RENDERED ONE NEVER DOES: `Render` has no branch that
	 * writes it, so a committed `.gcw` cannot hold an instruction waiting to happen.
	 */
	TArray<FGcwOperation> Operations;

	/**
	 * The `.gcs` subscriptions this file declares, in the order they were written — W0, §18.3.
	 *
	 * OWNED SINCE 2026-08-13, where R1 carried them as opaque text. §18.3c rules what that changes for
	 * the writer: a declaration is part of the complete payload like a region, so an omitted one is
	 * REMOVED and `dry_run` prices it by name.
	 */
	TArray<FGcwDeclaration> Declarations;

	/**
	 * The `terrain Name { }` programs this file declares — `GC-TERRAIN-DESIGN.md` §3.
	 *
	 * A BLOCK RATHER THAN A REGION BODY TOKEN, unlike `nav(…)` and `pcg(…)`: terrain spans scales, so
	 * its coarse-to-fine stream is ONE ordered sequence rather than scattered across region bodies
	 * where its order — which is its semantics — would be lost (§9.8). A region is a PLACE an
	 * instruction references, not the owner of the instruction.
	 */
	TArray<FGcTerrainProgram> Terrain;

	/**
	 * `water { }` — every body and island in the level, in authored order.
	 *
	 * ONE BLOCK AND NOT AN ARRAY OF THEM, unlike `Terrain`: a terrain program names the landscape it
	 * shapes and a level may hold several, while water names nothing and the zone is an actor with
	 * its own entry (`GC-TERRAIN-DESIGN.md` §12i.8/7).
	 *
	 * **THE ORDER IS LAYOUT AND NOT SEMANTICS** (§12i.9). The engine sorts bodies itself before
	 * carving — Ocean, Lake, River, Custom, ties on the actor's full name — so what this array's
	 * order buys is a file that does not churn, not a rule about who wins.
	 */
	FGcWaterBlock Water;

	/**
	 * The `populate { }` block — every generator this file declares, in authored order.
	 *
	 * **TERRAIN MAKES THE GROUND AND PCG POPULATES IT**, which is the seam between the two design
	 * files and the reason these sit beside each other rather than inside one another. A generator is
	 * an OPERATION WITH A DOMAIN (§12a.3, user-ruled 2026-08-19), so it left the region body it
	 * shipped in: as a token there the region WAS its domain, which cannot say a point with a radius,
	 * a course, or the whole world. The retired form refuses by name (`gcw_old_format`).
	 */
	FGcPopulateBlock Populate;

	/**
	 * The `instances { }` block — what a hand did to instances a `foliage(…)` line planted. G8 rung 3.
	 *
	 * **AFTER `populate { }` IN THE FILE AND IN THIS STRUCT, because it is a record ABOUT those
	 * lines** — the same reason `volumes { }` sits after `actors { }` in a level document. A reader
	 * meets the instruction before the exceptions to it.
	 *
	 * **ITS PRESENCE IS THE SWITCH** (user-ruled 2026-08-23): it protects exactly the instances it
	 * names, everything else keeps being replanted from the instruction, and deleting the block is
	 * the instruction to rebuild from intent.
	 */
	FGcwInstancesBlock Instances;


	/**
	 * Anything else this parser does not own, kept whole and put back.
	 *
	 * One entry per block, raw lines joined by newlines, in the order they were read. The `.gcs`
	 * blocks left this list when W0 took them; what stays is somebody's future feature, and the
	 * guarantee it keeps is the one that mattered — authoring a region cannot silently delete a block
	 * this build has never heard of.
	 */
	TArray<FString> ForeignBlocks;

	/** The trivia for a declaration, or nullptr. Key it with `TriviaKey`. */
	static FString TriviaKey(const TCHAR* Kind, const FString& Name);

	/** `Hall.p1` — how a declaration inside a region is keyed above. Never a spelling in the file. */
	static FString ScopedKey(const FString& RegionName, const FString& Name);
};

/** What a `$`-marked name turned out to be. */
enum class EGcwRefKind : uint8
{
	/**
	 * Not a declared name at all — no `$`, so it is somebody else's to resolve.
	 *
	 * A RESULT AND NOT A REFUSAL, deliberately. R2 asks this document first and falls through to the
	 * level's actors, so *this is not mine* has to be distinguishable from *this is mine and it is
	 * wrong*, or an actor named `Crypt` becomes unreachable the day a region takes the name.
	 */
	NotDeclared,

	/** `$world` — §21.4's reserved mark, which is why no region may be called `world`. */
	World,

	/** `$Crypt` — the prism. */
	Region,

	/** `$Crypt.north` — a wall, by minted name or by `as:` alias. */
	Edge,

	/**
	 * `$Crypt.spine` — a run the region declares that IS NOT A WALL OF IT (§21.12n, R14).
	 *
	 * **THE LINE HALF OF `Landmark`, and its own kind for the same reason.** A wall bounds the room:
	 * move it and the shape changes, and `connects:`/`sealed:` are facts about getting through it. A
	 * course is drawn inside: move it and nothing about the room changes, and there is nothing to get
	 * through. Collapsing the two would make *a course is not part of the boundary* an accident of
	 * which code path ran rather than something the format states.
	 *
	 * Its GEOMETRY is a wall's, which is the one thing they share — points, `bulge:`, the same
	 * tessellation, `@t` by arc length — so `FGraphCodeRegionTargets::ResolveOne` answers both in one
	 * branch, exactly as `Corner` and `Landmark` share theirs.
	 */
	Course,

	/** `$Crypt.p2` — a corner, which is a point an edge of that loop ends at. */
	Corner,

	/**
	 * `$Crypt.altar` — a point the region declares that NO wall of it runs through (§21.12j, R13).
	 *
	 * A SEPARATE KIND AND NOT A SECOND SPELLING OF `Corner`, because the two answer different
	 * questions about the same coordinate. A corner is a place ON the boundary: it moves when the
	 * wall through it moves, and the reason to name it is that the ring passes there. A landmark is
	 * a place IN the room — the altar, the spawn, where the chest goes — and nothing about the ring
	 * has any claim on it. Collapsing them would make *moving a wall does not move the altar* an
	 * accident of which code path ran rather than a fact the format states.
	 *
	 * Its FRAME is the region's, which is the one thing it shares with a corner: neither has an
	 * outward normal of its own, so both take the rotation of the room that declared them (§21.7).
	 * Its Z is the region's `anchorZ:`, for the reason §21.18c gives — a landmark sits at the height
	 * the region is at, and `min:` may be unbounded.
	 *
	 * A landmark OUTSIDE its own region is refused, at the same place an `anchor:` outside one is —
	 * `GraphCodeRegionWrite.cpp`, under `GcwToken::NotInRegion`. The header on `AnchorOutside`
	 * already called the two one rule; this is the half it was named for.
	 */
	Landmark,

};

/** A resolved `$` name. Indices are into the document's `Space`. */
struct GRAPHCODE_API FGcwReference
{
	EGcwRefKind Kind = EGcwRefKind::NotDeclared;

	int32 Region = INDEX_NONE;
	int32 Edge = INDEX_NONE;
	int32 Point = INDEX_NONE;
};

/** Refusal tokens, so a caller can branch on the reason rather than on the sentence. */
namespace GcwToken
{
	inline constexpr const TCHAR* BadHeader = TEXT("gcw_bad_header");
	inline constexpr const TCHAR* BadVersion = TEXT("gcw_bad_version");
	inline constexpr const TCHAR* BadBlock = TEXT("gcw_bad_block");
	inline constexpr const TCHAR* BadPoint = TEXT("gcw_bad_point");
	inline constexpr const TCHAR* BadEdge = TEXT("gcw_bad_edge");
	inline constexpr const TCHAR* BadRegion = TEXT("gcw_bad_region");
	inline constexpr const TCHAR* BadName = TEXT("gcw_bad_name");
	inline constexpr const TCHAR* BadNumber = TEXT("gcw_bad_number");
	inline constexpr const TCHAR* NameTaken = TEXT("gcw_name_taken");
	inline constexpr const TCHAR* ReservedName = TEXT("gcw_reserved_name");
	inline constexpr const TCHAR* UnknownPoint = TEXT("gcw_unknown_point");
	inline constexpr const TCHAR* UnknownEdge = TEXT("gcw_unknown_edge");
	inline constexpr const TCHAR* UnknownRegion = TEXT("gcw_unknown_region");
	inline constexpr const TCHAR* UnknownPart = TEXT("gcw_unknown_part");
	inline constexpr const TCHAR* NotInRegion = TEXT("gcw_not_in_region");
	inline constexpr const TCHAR* UnclosedBlock = TEXT("gcw_unclosed_block");
	inline constexpr const TCHAR* BadSplit = TEXT("gcw_bad_split");
	inline constexpr const TCHAR* BadRound = TEXT("gcw_bad_round");
	inline constexpr const TCHAR* BadBuild = TEXT("gcw_bad_build");

	/** A `<MeshId> = nav(type=walkable, …)` line — its ID, type, or one of its props (§4.3). */
	inline constexpr const TCHAR* BadNav = TEXT("gcw_bad_nav");

	/**
	 * A `<Name> = pcg(graph=…, seed=…)` line — its name, its keys, or one of their values
	 * (`GC-WORLD-GENERATION-DESIGN.md` §3).
	 *
	 * **A MISSING `seed=` LANDS HERE RATHER THAN BEING DEFAULTED.** A generator with no seed is the
	 * one refusal this surface owes that nothing else can supply: the engine would happily pick one,
	 * and then the same document makes a different forest on a different machine.
	 */
	inline constexpr const TCHAR* BadGeneration = TEXT("gcw_bad_generation");

	/**
	 * An `instances { }` line — its identity, its verbs, or one of their numbers
	 * (`GC-WORLD-GENERATION-DESIGN.md` §5.2c, row G8 rung 3).
	 *
	 * **ITS OWN TOKEN AND NOT `BadGeneration`, because a caller branching on that one is asking *is
	 * this about a `populate { }` line?*** — and this block is about an INSTANCE a line planted. The
	 * two refusals it exists for are a line naming an instance and recording nothing about it, which
	 * states what an absent line already states, and a verb spelled as an absolute: every number here
	 * is a DIFFERENCE from what the instruction computes, and one read the other way silently teleports
	 * a tree to a coordinate near the world origin.
	 */
	inline constexpr const TCHAR* BadInstance = TEXT("gcw_bad_instance");

	/**
	 * A `water { }` block — its head, one of its entries, a control point, or a point's modifiers
	 * (`GC-TERRAIN-DESIGN.md` §12i).
	 *
	 * **THE TWO REFUSALS THIS BLOCK EXISTS FOR** are a `pool` asked to carve, which the engine's own
	 * custom body never can and would silently not do, and a property written as a `[modifier]`,
	 * which is the mistake this grammar's own design made once before the user's
	 * children/properties/name rule caught it.
	 */
	inline constexpr const TCHAR* BadWater = TEXT("gcw_bad_water");

	/**
	 * A `terrain Name { }` block — its header, one of its instructions, a filter, or a paint rule
	 * (`GC-TERRAIN-DESIGN.md` §3–§6).
	 *
	 * **TWO REFUSALS UNDER IT ARE THE ONES THE FORMAT EXISTS FOR:** a generator with no `seed=`, which
	 * would make the same document produce a different mountain on a different machine, and a paint
	 * rule that could never fire, which reads exactly like a rule that works.
	 */
	inline constexpr const TCHAR* BadTerrain = TEXT("gcw_bad_terrain");

	/**
	 * A REGION-SCOPED `populate { }` / `terrain { }` block, or the `$Region.Block` that names one —
	 * §14.11.
	 *
	 * **ITS OWN TOKEN AND NOT `gcw_bad_block`**, because every refusal under it is about the SEAM
	 * between two blocks rather than about either one: a reference naming nothing, a reference into
	 * the wrong stream, one block claimed twice. A caller branching on the reason wants to tell
	 * *this block is malformed* from *these two blocks disagree*.
	 *
	 * **AND NOT `gcw_bad_scope`, WHICH WAS THE FIRST NAME AND IS ALREADY TAKEN.** §18.3c spends that
	 * word on a `.gcs` declaration's `in:` — a different subject entirely — and the compiler caught
	 * the collision, which is luckier than it sounds: two refusals under one token read as one
	 * failure mode to every caller that branches on the reason ([[one-concept-two-grammars]]).
	 *
	 * **THERE IS NO REFUSAL HERE FOR A BLOCK NOTHING REFERENCES.** User-ruled 2026-08-24 — *"you
	 * never reject a valid grammar. you only warn that isnt referenced"* — so that one is a warning
	 * on the parse and the block's lines are kept verbatim.
	 */
	inline constexpr const TCHAR* BadRegionBlock = TEXT("gcw_bad_region_block");

	/**
	 * A file in a shape a ruling REPLACED — and every one of them names where the shape went.
	 *
	 * Two so far: §21.1b's file-scope `points { }`/`edges { }` and `loop:`, and §18.3i's `track:` mouth
	 * and named group blocks. ITS OWN TOKEN AND NOT `gcw_bad_block`, because these are the refusals
	 * that are a MIGRATION and the sentence has somewhere to send the author. It also has to come
	 * before the unknown-block path: an unknown block is carried over verbatim, and carrying a pool of
	 * geometry — or a subscription — over as opaque text would turn it into a comment nobody notices.
	 */
	inline constexpr const TCHAR* OldFormat = TEXT("gcw_old_format");

	/**
	 * `base:` or `height:` on a region — the two fields §21.18c replaced with `min:`/`max:`/`anchorZ:`.
	 *
	 * SEPARATE FROM `OldFormat`, which is a whole file in the shape §21.1b replaced. This one is a
	 * field inside a file that is otherwise current, and the sentence carries the author's OWN numbers
	 * converted — `max:` is an absolute world Z where `height:` was a span, so the conversion is the
	 * part that is got wrong by eye.
	 */
	inline constexpr const TCHAR* OldField = TEXT("gcw_old_field");

	/** An authored `anchor:` outside the region that declared it — §21.18c, and it is one rule with
	 *  `NotInRegion`: a region may not be spoken of by a point it does not contain. */
	inline constexpr const TCHAR* AnchorOutside = TEXT("gcw_anchor_outside");

	// `NotInRegion` (above) is the OTHER half of that rule, and since R13 it is the half about a
	// LANDMARK: a point a region declares that no wall runs through resolves as a place in the room,
	// so it must be a point the room contains. Both are checked in `GraphCodeRegionWrite.cpp` rather
	// than the parser, for the reason stated there — a file already on disk stays readable.

	/** `bulge:` on a wall with a bend in it — §21.1a's *a chain is straight-segmented*. */
	inline constexpr const TCHAR* BulgedChain = TEXT("gcw_bulged_chain");

	/** A declaration outside any region, which §21.1b leaves nowhere to put. */
	inline constexpr const TCHAR* OutsideRegion = TEXT("gcw_outside_region");

	/**
	 * A `conflict { }` block — an unresolved three-way merge, §4.12 of `GC-VCS-MERGE-DESIGN.md`.
	 *
	 * **REFUSED AT THE READER, WHICH IS NOT WHERE A `.gc` DOES IT, AND THE DIFFERENCE IS `sig:`.** A
	 * conflicted `.gc` states `sig:conflicted` in its header, so every digest check downstream gets
	 * a true answer and the write doors refuse on that. A `.gcw` HAS NO `sig:` (M6.2, deferred by
	 * the user 2026-08-20), so the block in the body is the only thing that says the file is a
	 * proposal — and this reader's rule for a block it does not know is to CARRY IT OVER VERBATIM,
	 * which would quietly preserve somebody's unresolved decision through every subsequent write.
	 *
	 * One refusal at the reader rather than one at each door, because there are four of them and a
	 * fifth is a merge away. `gc_versions mode:"resolve"` is unaffected: it scans the text itself.
	 */
	inline constexpr const TCHAR* Conflicted = TEXT("gcw_conflicted");

	// ---- W0's `.gcs` declarations — §18.3, §18.3b, §18.3c --------------------------------------

	/** `minions.gcs` where the stem is not a legal name, or a block head that is neither. */
	inline constexpr const TCHAR* BadDeclaration = TEXT("gcw_bad_declaration");

	/**
	 * A filter this grammar does not have, or one written `in $Room` instead of `in:$Room`.
	 *
	 * §18.3c's ruling, and a REFUSAL rather than a tolerated second spelling: a filter that is
	 * silently not applied returns a wider set that reads exactly like a match.
	 */
	inline constexpr const TCHAR* BadFilter = TEXT("gcw_bad_filter");

	/** `in:` naming a wall, a corner, or a name with no `$` — §18.3c, a scope is a place. */
	inline constexpr const TCHAR* BadScope = TEXT("gcw_bad_scope");

	/** A group line with no filter at all — §18.3b's refusal, and §18.3i left it no exception. */
	inline constexpr const TCHAR* EmptySelector = TEXT("gcw_empty_selector");

	/** `anchor:` / `facing:` / `as:` / `listen:` with a value that is not one of the words it takes. */
	inline constexpr const TCHAR* BadDirective = TEXT("gcw_bad_directive");

	/** `fields: Inventory[0].Count` — §19.13d, the policy reaches only the top-level segment. */
	inline constexpr const TCHAR* BadField = TEXT("gcw_bad_field");
}

/** The `.gcw`'s text surface. Read and write text; no world, no asset, no editor. */
class GRAPHCODE_API FGraphCodeRegionFile
{
public:
	/** `$`, and it is free in both grammars — §21.4 measured it before choosing it. */
	static constexpr TCHAR DeclaredMark = TEXT('$');

	/**
	 * Text to a document. Every refusal names a token from `GcwToken` and the line it is about.
	 *
	 * ONE PASS, since 2026-08-11. It was two — declarations, then a resolve — only because a point
	 * could be written `door = e2 @ 0.5` and name an edge the block below it declares. Points carry
	 * their own coordinates now, so nothing in this file refers forward and the second pass and its
	 * public `ResolveDerived` both went with it.
	 */
	/**
	 * `OutWarnings` — WHAT PARSED AND IS INERT. Optional, and every entry is one whole sentence.
	 *
	 * **IT EXISTS BECAUSE A RULING TURNED A REFUSAL INTO ONE** (user, 2026-08-24: *"you never reject
	 * a valid grammar. you only warn that isnt referenced"*). A §14.11 block nothing references is
	 * legal, keeps its text and does nothing — which is exactly the state a reader cannot tell from
	 * working, so the sentence is the only instrument there is.
	 *
	 * OPTIONAL BY DEFAULT so that no existing caller had to change to keep compiling — and every
	 * caller that reports to a person passes one.
	 */
	static bool Parse(const FString& Text, FGcwDocument& OutDocument, FString& OutRefusal,
		TArray<FString>* OutWarnings = nullptr);

	/** A document back to canonical text. Total: a document that parsed always renders. */
	static FString Render(const FGcwDocument& Document);

	/**
	 * One flat `.gcs` group line in its canonical spelling, without indentation.
	 *
	 * PUBLIC FOR THE GENERATED RECEIPT. Section 18.3i makes the declaration line both the group's
	 * identity in the `.gcw` and the receipt that gives a `.gcs` entry meaning. Rendering those two
	 * through separate functions would let the generated answer disagree with the authored question
	 * while both remained individually canonical.
	 */
	static FString RenderGroupLine(const FGcwGroup& Group);

	/**
	 * The one spelling of a facing mode's key — `relative`, `target` or `toward`.
	 *
	 * **ONE DOOR BECAUSE THE WORD IS WRITTEN IN TWO PLACES**: the `.gcw` group line renders it and the
	 * `.gcs` apply lowers it onto a `@facing` member. Those were two ternaries agreeing by hand, which
	 * is how a third value comes to be spelled in one of them only.
	 */
	static const TCHAR* FacingModeKey(EGcwFacingMode Mode);

	/** Parse one flat structural group line through the `.gcw` grammar. */
	static bool ParseGroupLine(const FString& Text, FGcwGroup& OutGroup, FString& OutRefusal);

	/** One declaration-wide rich exclusion in canonical per-line form, including `exclude`. */
	static FString RenderExclusionLine(const FGcwSelector& Selector);

	/** Parse one declaration-wide exclusion through the same grammar as its `.gcw`. */
	static bool ParseExclusionLine(const FString& Text, FGcwSelector& OutSelector, FString& OutRefusal);

	/** `p6` / `e9` — the next name THIS REGION will hand out, and it is never one it has used. */
	static FString NextPointName(const FGcwDocument& Document, int32 RegionIndex);
	static FString NextEdgeName(const FGcwDocument& Document, int32 RegionIndex);

	/**
	 * Add a point or an edge to one region under a freshly minted name, and return its index.
	 *
	 * MINTING LIVES HERE RATHER THAN IN THE CALLER so the high-water mark cannot be forgotten by one
	 * of them. §21.3's property — inserting renumbers nothing — is only true if every insert goes
	 * through one door. Since §21.1b that door also stamps the owner, so there is no way to add
	 * geometry to the file without saying which room it is in.
	 */
	static int32 MintPoint(FGcwDocument& Document, int32 RegionIndex, const FVector2D& Position);

	/**
	 * `Bulge` is the MODEL's sign — left of `StartPoint -> EndPoint`, not the file's outward (§21.6b).
	 * Callers here are producers working in plan geometry, and the writer carries it out to the file.
	 *
	 * **THE CHAIN FORM WENT PRIVATE 2026-08-21.** `MintEdgeThrough` was public for the retired `split`
	 * operation, which minted a wall through a point it had just inserted; nothing else ever called
	 * it, and `GraphCodeRegionOps.cpp` went with `split` and `round`. It survives as a file-local
	 * helper in the `.cpp` because this two-point form is still its only caller.
	 */
	static int32 MintEdge(FGcwDocument& Document, int32 RegionIndex, int32 StartPoint, int32 EndPoint,
		double Bulge = 0.0);

	/**
	 * Resolve `$Crypt`, `$Crypt.north`, `$Crypt.p2`, `$Crypt.door`, `$world`.
	 *
	 * A NAME WITHOUT THE MARK RETURNS `NotDeclared` AND SUCCEEDS — see `EGcwRefKind::NotDeclared`.
	 * A marked name that does not resolve REFUSES, and it looks ONLY inside the region named: since
	 * §21.1b the lookup cannot reach another room's wall even by accident. When the part does exist
	 * somewhere else the refusal says so by name, because *there is a `north`, in the Cellar* is the
	 * one sentence that turns a scoping miss into a one-word fix.
	 */
	static bool ResolveName(const FGcwDocument& Document, const FString& Name, FGcwReference& OutRef,
		FString& OutRefusal);

	/** Letters, digits and underscore, not starting with a digit. See this file's opening note 1. */
	static bool IsLegalName(const FString& Name);
};
