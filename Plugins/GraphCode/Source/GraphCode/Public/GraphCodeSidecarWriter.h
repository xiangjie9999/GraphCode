#pragma once

#include "CoreMinimal.h"

#include "GraphCodeErrors.h"

/** The supported v1 asset kinds. The stable token is written on line one of every .gc file. */
enum class EGraphCodeAssetKind : uint8
{
	Blueprint,
	WidgetBlueprint,

	/**
	 * An ANIMATION Blueprint, and it is a kind rather than a flag on `Blueprint` for one reason:
	 * the capability registry is keyed by kind, and `animgraph { }` is a block only these have.
	 *
	 * The alternative was considered and rejected on 2026-08-07. Declaring `animgraph` on
	 * `Blueprint` and projecting it empty everywhere - the way `class_defaults` is carried on
	 * non-Actor Blueprints - would have satisfied the registry by telling every plain Blueprint it
	 * has an animation section, and moved the `sig:` of ~172 committed sidecars to say so. A kind
	 * costs more code and no lies.
	 *
	 * Its block set is `Blueprint`'s plus `animgraph`. Everything else about it is a Blueprint,
	 * because `UAnimBlueprint` is one: the variables, components, event graph and ordinary
	 * functions all project and write exactly as before. Only the anim graphs moved, and only out
	 * of `functions { }`, where array membership had put them. Truth-audit §6 Q1.
	 */
	AnimationBlueprint,

	/**
	 * THE THREE BLUEPRINT SUB-TYPES, and they are kinds for the reason the user gave when this was
	 * ruled: *"for interface compared to function macro etc, it's the same case with animbp and bp,
	 * just make it needing a specific type."*
	 *
	 * All three are a plain `UBlueprint` wearing a different `BlueprintType` — `BPTYPE_Interface`,
	 * `BPTYPE_FunctionLibrary`, `BPTYPE_MacroLibrary`. So this is NOT the AnimationBlueprint case
	 * of a distinct class with a block of its own: their Core half is identical to `Blueprint`'s and
	 * their capability rows say so by sharing its block list.
	 *
	 * **WHAT MAKES THEM KINDS ANYWAY IS CREATION.** `GraphCodeGreenfield` hardcoded
	 * `Factory->BlueprintType = BPTYPE_Normal`, so none of the three could be built through
	 * GraphCode at all, and `asset create` had to stay resident to make them (§11.4b). The
	 * alternative considered and withdrawn was a `BlueprintType` field in `settings { }`; the header
	 * already states the type, which means the factory switches on `kind` with no new field, no
	 * policy opt-in, and no *floor exceeds the header* case to model — an empty document still
	 * applies. It also kills a real ambiguity: a Macro Library's `parent:` is `/Script/Engine.Actor`,
	 * byte-identical to an ordinary Actor Blueprint's, so nothing but a stated kind can tell them
	 * apart.
	 *
	 * The corpus cost is SIX files, measured 2026-08-09 by reading `BlueprintType` off the assets
	 * rather than counting from memory — §11.4b said four. Two of each, and they are the only six.
	 */
	BlueprintInterface,
	BlueprintFunctionLibrary,
	BlueprintMacroLibrary,

	UserStruct,
	UserEnum,
	MaterialInstance,
	Material,
	MaterialFunction,

	/**
	 * A UDataAsset INSTANCE - the first kind here that is an object rather than a definition.
	 *
	 * Every kind above projects something that declares: a Blueprint's variables and graphs, a
	 * struct's members, a material's expressions. This one declares nothing. Its whole content is
	 * the property values that differ from its class CDO, which is why its single block is
	 * `values { }` and why it needs no IR of its own beyond the property codec.
	 */
	DataAsset,

	/**
	 * A LEVEL - `.umap`, not `.uasset`, and the first kind here that projects a collection.
	 *
	 * Its single `actors { }` block is written by the same code that writes a `.gcp` capture, under
	 * the opposite property rule: a capture prints every visible property because two captures get
	 * diffed and both sides need the line, and a projection prints only deviation from the archetype
	 * because adding one actor must not rewrite its neighbours. That is why this kind - alone so far
	 * - states `baseline:` in its envelope. WORLD-GC-DESIGN.md §3.
	 */
	Level,

	/**
	 * A DATA TABLE - `settings { }` plus one `rows { }` line per row.
	 *
	 * The first kind whose body is not reflected off a UObject at all. A row is `uint8*` memory in
	 * `UDataTable::RowMap` and the `RowStruct` is the only thing that says what is in it, so the
	 * codec is entered through its struct form rather than its object form. GC-IMPORT-DESIGN.md §2.
	 *
	 * EXACT CLASS, never a subclass: a `UCompositeDataTable` fills its RowMap from its parent tables
	 * at load, so projecting one would write rows it does not own and a write would then claim them.
	 */
	DataTable,

	/**
	 * A STRING TABLE — `settings { Namespace = … }` plus one `strings { }` line per entry.
	 * `GC-STRING-AND-CURVE-TABLE-DESIGN.md` §2.
	 *
	 * **The kind exists because the class has NO REFLECTED SURFACE AT ALL.** `UStringTable` declares
	 * zero `UPROPERTY`s (MEASURED, `StringTable.h:27-64`); its whole content is a hand-serialized
	 * `FStringTable` holding a namespace, key→source-string entries and per-entry metadata. That is
	 * the `UEnvQuery`/`UStateTree` shape — a class the generic value route would render as
	 * `values { }` with `omits: none`, a sidecar claiming nothing is missing while holding nothing.
	 * It escapes that today only by deriving from `UObject` rather than `UDataAsset`, so it is
	 * UNCOVERED rather than MIScovered, and this kind is what makes that deliberate.
	 *
	 * **The namespace is READ, never derived.** `UStringTable`'s constructor seeds it from
	 * `GetName()` (`StringTable.cpp:398`) and `Serialize` then overwrites it with the stored value.
	 * Deriving it from the asset's name at projection time would rewrite the body on a rename, with
	 * no authored change behind it — the defect `curve_float`'s sole channel already paid for.
	 *
	 * No `import { }`: there is no `UAssetImportData` on the class. The CSV door is
	 * `FStringTable::ImportStrings`, which produces entries rather than being a facet of them, so
	 * `omits: none` is honest rather than optimistic.
	 */
	StringTable,

	/**
	 * ENHANCED INPUT: one verb, and one set of key bindings for it. GC-INPUT-DESIGN.md.
	 *
	 * Two kinds rather than one because the BLOCK SET differs - an action has `triggers { }` and
	 * `modifiers { }`, a context has `mappings { }` - which is the same reason there are three curve
	 * kinds. Both are UDataAsset subclasses, and both were REFUSED by name inside `kind:data_asset`
	 * until this pair existed, because the codec exports an array whole and their entire authored
	 * content is arrays of `Instanced` sub-objects.
	 *
	 * The first kinds whose elements have NO IDENTITY. A trigger, a modifier and a mapping have no
	 * name and no GUID, and order is meaning (modifiers apply in array order), so the list IS the
	 * value: the payload carries all of it and a write replaces it wholesale. No `delete` line and
	 * no rename, unlike `rows { }`; no closed set, unlike `curves { }`.
	 */
	InputAction,
	InputMappingContext,

	/**
	 * A STANDALONE CURVE ASSET, one kind per class because the CHANNEL SET is what differs:
	 * `Value` for a float, `X`/`Y`/`Z` for a vector, `R`/`G`/`B`/`A` for a linear colour.
	 *
	 * The names are the engine's own (`FCurveOwnerInterface::GetCurves`), not a table here. Exact
	 * class in every case: `UCurveLinearColorAtlas` derives from `UCurveLinearColor` and carries a
	 * baked texture this does not read.
	 *
	 * Its keys are spelled EXACTLY as a Timeline track's are, through `BlueprintIR::ExportCurveKeys`
	 * - which is not a nicety, because a Timeline track can point at one of these assets, so the
	 * same curve is reachable through both surfaces. GC-IMPORT-DESIGN.md §2's neighbour, §8.3 of
	 * the extended-syntax proposal.
	 */
	CurveFloat,
	CurveVector,
	CurveLinearColor,

	/**
	 * A CURVE TABLE — `import { }`, `settings { }` and one `curves { }` line per ROW.
	 * `GC-STRING-AND-CURVE-TABLE-DESIGN.md` §3.
	 *
	 * **A row IS a named curve channel, so it is spelled as one** — `RenderChannel` and
	 * `BlueprintIR::ExportCurveKeys` are shared with the three curve kinds above rather than
	 * reimplemented, which is the same argument that put a Timeline track and a curve asset on one
	 * spelling: the alternative is one curve reading two ways depending on which file you opened.
	 *
	 * **`CurveTableMode` is carried by the ROW TOKENS.** `RichCurve` and `SimpleCurve` cannot
	 * coexist in one `RowMap` — `AddRichCurve`/`AddSimpleCurve` each `check()` the other mode out
	 * (`CurveTable.cpp:571`, `:590`) — so with even one row present the mode is implied and stating
	 * it in `settings { }` as well would be two carriers of one fact. The one state the tokens
	 * cannot carry is an EMPTY table that still remembers a mode, which `RemoveRow` makes reachable
	 * because it does not reset one (`:560`; only `EmptyTable` does). So `Mode` is written if and
	 * only if there are no rows.
	 *
	 * EXACT CLASS, on `DataTable`'s rule directly above and for the same reason.
	 */
	CurveTable,

	/**
	 * THE TWO MESH KINDS - and the first kinds whose asset is mostly BINARY that this deliberately
	 * does not carry. Queue row 11j.
	 *
	 * Geometry is not in the document and never will be: vertices, triangles, skin weights, LOD
	 * binaries and collision bodies are what the importer produced from a source file, and a text
	 * projection of them would be neither reviewable nor writable. They are on `omits:`, which is
	 * the sanctioned spelling for a comprehensiveness gap, and `import { }` states the source file
	 * that DID produce them. That is what keeps this from being the partial writer
	 * GC-PRODUCT-SPLIT.md 4.1's second ruling forbids: the ruling is against a writer that silently
	 * drops what it does not understand, and nothing here is silent.
	 *
	 * TWO KINDS RATHER THAN ONE, on the same reasoning as the three curves and the two input kinds:
	 * the BLOCK SET is the same but the material slot type is not - `FSkeletalMaterial` against
	 * `FStaticMaterial` - and a skeletal mesh additionally has a skeleton and a physics asset that
	 * `settings { }` states. One kind would have to say "the slot type depends on the class", which
	 * is a sentence the capability registry cannot key on.
	 *
	 * WHY THE KIND EXISTS AT ALL: `unreal_ue domain:"material" set_skeletal_mesh_material` was the
	 * last inherited operation the router still had a live route for, and it had no successor
	 * anywhere. `materials { }` is that successor.
	 */
	SkeletalMesh,
	StaticMesh,

	/**
	 * ONE TEXTURE — the third kind whose asset is mostly binary this deliberately does not carry, and
	 * the first whose settings ARE the import. C7.
	 *
	 * `texture_2d` RATHER THAN `texture`, user-directed 2026-08-13: *"the name should be specific,
	 * like animbp, not just normal blueprint"*. So a cube map or a volume texture becomes a kind of
	 * its own beside this one rather than arriving as a second meaning for one word — which is also
	 * what stops `omits:` and the block table from having to say "depends which class".
	 *
	 * EXACT CLASS `UTexture2D`, and this is the strongest exact-class case in Core rather than the
	 * usual caution: eight Runtime subclasses and not one is an ordinary image. Lightmaps, shadow
	 * maps, virtual-texture proxies and IES light profiles are all built rather than authored — and
	 * `UCurveLinearColorAtlas` is a CURVE asset that already has a kind, so an `IsChildOf` rule here
	 * would give one asset two projections. `data_table` and the three curves match exactly for the
	 * same class of reason.
	 *
	 * WHY THE SETTINGS AND THE IMPORT ARE ONE VOCABULARY: `UInterchangeTextureFactoryNode` carries 41
	 * of the texture's own properties (`SetCustomCompressionSettings`, `SetCustomSRGB`,
	 * `SetCustomMipGenSettings`, `SetCustomLODGroup`, `AddressX`/`AddressY`), each named for the
	 * asset's UPROPERTY with a `Custom` prefix. So the asset's name is the shared word and Interchange
	 * wraps it; `kind:import` gains `settings { }` on that reading, and plans one as of 2026-08-13.
	 *
	 * HOW ALIKE THE TWO DOCUMENTS ARE IS MEASURED, in `GC-IMPORT-DESIGN.md` §5: `settings { }` is one
	 * grammar completely, and `import { }` and `pipelines { }` are not. Do not restate "they differ
	 * only in the header" — that sentence was written from the plan and the artifacts disagree.
	 */
	Texture2D,

	/**
	 * ONE IMPORTED SOUND — `kind:texture_2d`'s shape a second time, on the same three blocks, and
	 * the resemblance is the ruling rather than a convenience (user-directed 2026-08-19: *"ensure
	 * consistency with the grammar with all other gc files"*). C13.
	 *
	 * An imported asset whose binary half stays behind has ONE shape in this language: where the
	 * file came from, how the importer was configured, and what the asset became. So `import { }`,
	 * `pipelines { }` and `settings { }` mean here exactly what they mean on a texture, and
	 * `omits: source_audio` is `source_pixels`' word class.
	 *
	 * `sound_wave` RATHER THAN `sound`, on `texture_2d`'s naming rule: a Sound Cue is not a variety
	 * of this and gets a token of its own if it is ever built, so one word covering both would make
	 * `omits:` answer "depends which class".
	 *
	 * EXACT CLASS `USoundWave`, and this is the strongest exact-class case in Core — stronger than
	 * the texture's, which is saying something. `UMetaSoundSource` derives from
	 * `USoundWaveProcedural` derives from `USoundWave`, so an `IsChildOf` rule would not merely give
	 * one asset two kinds the way `UCurveLinearColorAtlas` would have: it would let CORE silently
	 * claim an asset belonging to CORE EXTENSION, and project a runtime graph as if it were a file's
	 * encoding settings. Refused by name, naming that product.
	 *
	 * WHAT `pipelines { }` SAYS HERE, because it is the one block whose usual answer differs from
	 * the texture's: normally `# UNSUPPORTED:`. No Interchange translator claims audio in a stock
	 * build, so a wave arrives through `USoundFactory` and stores no pipeline stack. The block is
	 * written anyway — an absent block and an empty one are different claims — and it costs nothing
	 * to be right the day a project turns the `Interchange.FeatureFlags.Import.Audio.*` CVars on.
	 *
	 * The `factory { }` block belongs to `kind:import` and NEVER here: a `UFactory` is transient and
	 * its settings are never serialized onto the asset, so there is no stored factory state for a
	 * sidecar to project.
	 */
	SoundWave,

	/**
	 * ONE SOUND CUE — `settings { }` and one `soundcue { }` graph block. C13, un-deferred
	 * 2026-08-21 (user-directed), reversing the 2026-08-19 ruling that this was the superseded way
	 * to author audio behaviour.
	 *
	 * **IT IS NOT A VARIETY OF `sound_wave` AND SHARES NO BLOCK WITH IT EXCEPT `settings { }`.** A
	 * cue holds no audio at all — no samples, no source file, nothing to import — so `import { }`
	 * and `pipelines { }` are absent rather than empty, and `omits:` carries neither
	 * `source_audio` nor `reimport_history`. What it does share is real and was built ahead of it:
	 * a `USoundCue` IS a `USoundBase`, so the ~25 playback fields the wave's `SettingsPolicy()`
	 * exports mean the same thing here and are exported by that same policy.
	 *
	 * EXACT CLASS `USoundCue`, and the descendant that settles it is measured rather than
	 * hypothetical: `USoundCueTemplate` ships in the optional `SoundCueTemplates` plugin and
	 * BUILDS ITS GRAPH PROCEDURALLY from its own settings, rebuilding on every property change. An
	 * ancestry rule would write a generated graph into a file as though a person had authored it.
	 * Refused by name, saying that.
	 *
	 * The graph is read from `FirstNode`/`ChildNodes` — runtime data that survives a cook — and NOT
	 * from `SoundCueGraph`, which is `WITH_EDITORONLY_DATA`. `kind:behavior_tree` reads its editor
	 * graph because a BT decorator belongs to the link at runtime and to the node in the editor; a
	 * sound node has no such split, so the runtime side is both simpler and available headless.
	 *
	 * `GC-SOUND-CUE-DESIGN.md` carries the four measured findings that shape the grammar, of which
	 * two invert `kind:behavior_tree`'s own rulings: a cue graph is a DAG rather than a tree, so a
	 * shared node needs a name; and child order is pin order rather than `NodePosX`, so layout is
	 * cosmetic here and goes on `omits: node_layout`.
	 */
	SoundCue,

	/**
	 * ONE EXTERNAL ACTOR, in its own package under `__ExternalActors__`. Queue row 11m.
	 *
	 * The subject is One File Per Actor, NOT World Partition. OFPA is a property of the level
	 * (`ULevel::bUseExternalActors`) that World Partition forces on but does not own, so keying on
	 * the actor's own package is what makes this right for a partitioned world AND for a plain
	 * level somebody ticked the box on.
	 *
	 * IT SHARES `kind:level`'s BODY AND NOT ITS SCOPE. The block is `actors { }` with exactly one
	 * entry, written by the same `FGraphCodeActorEntryWriter` under the same `baseline: archetype` -
	 * a second grammar for a single-actor document would be a dialect nobody asked for. What does
	 * NOT carry over is the omission list: a level's names facets of the LEVEL (`level_blueprint`,
	 * `sublevels`, `bsp_geometry`, `built_data`) beside facets of its actors, and only the second
	 * group can be true of one actor package.
	 *
	 * This is the kind that makes an OFPA map reviewable. Its `.uasset` is 2-5 KB of binary at a
	 * content-addressed path, which git tracks and shows changing while saying nothing about what
	 * changed.
	 */
	ExternalActor,

	/**
	 * A `USkeleton` — **THE FIRST KIND IN THIS ENUM THAT CORE DOES NOT OWN.** GraphCode Move's,
	 * by `GC-PRODUCT-SPLIT.md` §4; what it carries is `GC-MOVE-ANIMATION-DESIGN.md` §2.1.
	 *
	 * So it is the first row where `FGraphCodeProducts::OwnerOf` answers something other than
	 * `Core`, and the first caller of `FGraphCodeCodecs::RegisterKindCodec` — which had been built,
	 * declared and never exercised, its own header naming World's Niagara as the expected first
	 * caller. **The name is here and the code is not**, which is this file's standing arrangement
	 * and the reason `TryParseKind` can answer *"that kind is projected by GraphCode Move, which is
	 * not installed"* instead of *"unsupported asset kind"* (`GraphCodeProducts.h`).
	 *
	 * WHY IT IS THE KIND MOVE STARTS AT: every other animation kind names one. A montage's slot
	 * track names a slot that lives HERE (`FAnimSlotGroup`), a retarget maps between two rigs built
	 * on skeletons, and an anim sequence's `RetargetSource` is a name off this asset. It is also the
	 * only one of them with no binary payload at all — sockets, virtual bones, slot groups and
	 * per-bone retargeting modes are typed, authored data and nothing else.
	 *
	 * **THE REFERENCE SKELETON IS NOT IN THE DOCUMENT** and never will be: `FReferenceSkeleton` is
	 * the bone hierarchy and reference pose derived from the skeletal mesh this was built from, and
	 * the editor offers no way to type a bone in. It is `omits: reference_skeleton`, which is the
	 * sanctioned spelling for a comprehensiveness gap — the same ruling the mesh kinds' `geometry`
	 * got, and for the same reason: an importer authors it, a document does not.
	 */
	Skeleton,

	/** A skeletal mesh's authored collision bodies and joint constraints; codec supplied by Move. */
	PhysicsAsset,

	/**
	 * A `UIKRigDefinition` — GraphCode Move's, `GC-MOVE-ANIMATION-DESIGN.md` §4.0b / §6.
	 *
	 * **IT IS ON THE CRITICAL PATH FOR IMPORTED ANIMATION, which is why it is built before the
	 * three animation kinds it comes after in that document's own ordering.** The pipeline the user
	 * is building generates motion on the GENERATOR's rig, so nothing it produces reaches the
	 * project's character without a retarget, and a retarget is a mapping between two of these. An
	 * imported animation that cannot be retargeted is an asset nobody can use.
	 *
	 * WHAT MAKES IT A KIND RATHER THAN A BLOCK is the `skeleton` argument exactly: there is no Core
	 * half of an IK rig to keep. Bone chains, goals, the retarget definition and the solver stack
	 * are all animation, so a Core-only install reading one has nothing to put in the file and takes
	 * `EGcProductScope::Kind`'s `omits: all@move` stub.
	 *
	 * **THE CLASS TEST HERE COMPARES A LITERAL CLASS PATH rather than calling `StaticClass()`**, and
	 * that is not a shortcut — it is what keeps Core free of a dependency on the IKRig plugin while
	 * still owning the vocabulary. `FAssetData::AssetClassPath` comparison is what every other test
	 * in `IsSupportedAssetData` does; only the way the path is spelled differs.
	 */
	IkRig,

	/**
	 * A `UIKRetargeter` — the mapping between two `IkRig`s, and the asset that actually does the
	 * retargeting an imported animation needs.
	 *
	 * The part a person edits is the RETARGET POSES: per-bone rotation offsets plus a root
	 * translation delta, which is how a generator's T-pose is aligned to the project character's.
	 * They are keyed by name, so an omitted one is a question rather than a truncation — the
	 * identity rule that decides this kind's deletion policy.
	 */
	IkRetargeter,

	/**
	 * A `UAnimSequence` — GraphCode Move's, `GC-MOVE-ANIMATION-DESIGN.md` §4.1 step 2.
	 *
	 * **THE KIND THE WHOLE IMPORTED-ANIMATION PATH WAS BLOCKED ON.** §4.0's reimport pipeline had no
	 * destination kind for the animations inside an `.fbx`, so a generated animation set could be
	 * imported and had nowhere to be described; §8.5 and §9.5 both close by naming this as the
	 * blocker they did not clear. Every other Move kind so far describes something an animation is
	 * played ON — a skeleton, a rig, a retarget — and this is the first that describes the motion.
	 *
	 * WHAT IT CARRIES IS THE PART A PERSON TYPED, and for this kind that is a genuinely different
	 * split from the two before it. The keyframes are megabytes of importer output living in an
	 * `IAnimationDataModel` (`omits: bone_animation`); the notifies, the sync markers, the float
	 * curves and the compression/additive/root-motion settings are authored on top of them and are
	 * exactly what a regeneration from the generator must not silently lose.
	 *
	 * **AND IT IS THE FIRST MOVE KIND WHERE THE REFLECTIVE PREMISE HOLDS.** A `USkeleton` and both IK
	 * assets keep their authored state in bare `UPROPERTY()` members, so a `settings { }` built by
	 * reflection saw nothing that mattered. `UAnimSequence` marks the whole Compression, AdditiveSettings
	 * and RootMotion groups `EditAnywhere`, so here the reflective block is the large one. The answer
	 * is per-member rather than per-product, which is why each kind measures it again.
	 */
	AnimSequence,

	/**
	 * A MONTAGE — `UAnimMontage`, exact class, and the last capability-shaped hole on the path to a
	 * playable game (`GRAPHCODE-ROADMAP.md` §8a row 1).
	 *
	 * A `UAnimMontage` is a `UAnimSequenceBase` and NOT a `UAnimSequence`, so the exact test above
	 * never reached it and this is a kind rather than a case of that one. What it holds is what a
	 * person authors on top of animations that already exist: slot tracks holding ordered segments,
	 * a graph of named sections, and the notifies that tell game logic when the swing lands.
	 *
	 * **AND IT IS THE KIND WHERE THE REFLECTIVE PREMISE FAILS HARDEST.** `CompositeSections` and
	 * `SlotAnimTracks` — the two things a person actually authors — carry no edit flag at all, so a
	 * `CPF_Edit`-gated policy returns a montage with its blend settings and none of its content.
	 * `kind:blend_space` directly above answers the opposite way on the same asset family
	 * (`GC-MOVE-ANIMATION-DESIGN.md` §5.1c).
	 */
	AnimMontage,

	/**
	 * A BLEND SPACE, and it is ONE kind over FOUR classes — `UBlendSpace`, `UBlendSpace1D`,
	 * `UAimOffsetBlendSpace` and `UAimOffsetBlendSpace1D`.
	 *
	 * The three Blueprint sub-kinds above exist because `parent:` cannot tell them apart. Here it
	 * can — a `UBlendSpace1D` states `/Script/Engine.BlendSpace1D` — and all four carry the same
	 * members, project the same three blocks and mean the same thing: a space with axes and samples
	 * in it. A different class is not a different document.
	 */
	BlendSpace,

	/**
	 * A `ULevelSequence` — the cinematic, and GraphCode Move's under the time-based ownership
	 * ruling of 2026-08-13 (`GC-PRODUCT-SPLIT.md` §4).
	 *
	 * **THE SECOND HOST OF A LAYER THAT WAS FACTORED FOR IT.** A `UWidgetAnimation` is a
	 * `UMovieSceneSequence` and so is this, so the tracks, sections, channels and keys are the same
	 * machinery and are already shipped; what a Level Sequence adds is that a binding is an ACTOR
	 * and the asset holds exactly one movie scene. Move owns the codec; Core owns this token and the
	 * class path, so an install without Move still classifies the asset and writes an honest
	 * `omits: all@move` stub rather than failing to name it.
	 */
	LevelSequence,

	/**
	 * A `UBehaviorTree` — GraphCode Move's gameplay-AI graph.
	 *
	 * The runtime tree is compiled state. The authored source is the editor-only `BTGraph`, whose
	 * child order is semantic and whose decorator/service nodes live in `UAIGraphNode::SubNodes`.
	 * Move owns the codec; Core owns this token and class path so an install without Move can still
	 * classify the asset and write an honest `omits: all@move` stub.
	 */
	BehaviorTree,

	/**
	 * A `UStateTree` — GraphCode Move's hierarchical state-machine AI asset (`GC-STATE-TREE-DESIGN.md`).
	 *
	 * `UStateTree : UDataAsset`, so — like `env_query` and the two input kinds — it must be classified
	 * BEFORE the data-asset test and refused inside `IsProjectableInstanceClass`, or it projects an
	 * empty `values { }` claiming coverage it does not have. The authored source is the editor-only
	 * `EditorData` (a `UStateTreeEditorData`): its `SubTrees` are the top-level states, each state a
	 * recursive tree of `Children`. The runtime `Frames`/`States`/`Nodes` and the instance data are
	 * baked, compiled through `StateTreeCompiler`, and declared `omits: compiled`.
	 *
	 * Move owns the codec; Core owns this token and the class path (a literal below) so an install
	 * without Move still classifies the asset and writes an honest `omits: all@move` stub.
	 */
	StateTree,

	/**
	 * A `UNiagaraSystem` — GraphCode Move's particle-simulation kind (`GC-MOVE-NIAGARA-DESIGN.md`).
	 *
	 * **Moved off World 2026-08-18**: every earlier comment named World as the owner; the user put
	 * it in Move, where the whole product-kind seam is already proven. What it carries is the editor
	 * STACK — each script section (system spawn/update, per-emitter spawn/update, particle
	 * spawn/update) as an ordered list of module calls — NOT the `UNiagaraGraph` node soup underneath,
	 * whose `->` pin edges become plain top-to-bottom order. Scratch Pad / inline dynamic-input
	 * graphs are `omits: scratch_pad_internals`.
	 *
	 * Move owns the codec; Core owns this token and the class path (spelled as a literal below) so an
	 * install without Move still classifies the asset and writes an honest `omits: all@move` stub.
	 */
	NiagaraSystem,

	/**
	 * A `UEnvQuery` — GraphCode Core Extension's Environment Query System asset (`GC-AI-AUTHORING-
	 * DESIGN.md` §3). A query scatters candidate points and scores/filters them; the asset is a
	 * recipe for "where should the AI go?".
	 *
	 * `UEnvQuery` IS a `UDataAsset`, so — like `input_action` — it must be classified BEFORE the
	 * data-asset test and refused inside `IsProjectableInstanceClass`, or it projects an empty
	 * `values { }` that claims coverage it does not have. The authored source is the editor-only
	 * `EdGraph` (options X-sorted, each option's tests in `UAIGraphNode::SubNodes` order); the
	 * runtime `Options` array is derived.
	 *
	 * Core Extension owns the codec because EQS's editor half is the optional
	 * `EnvironmentQueryEditor` plugin — the same gating MVVM, MetaSound and GAS take. Core owns this
	 * token and the class path (a literal below) so an install without Core Extension still
	 * classifies the asset and writes an honest `omits: all@core_extension` stub.
	 */
	EnvQuery,

	/**
	 * A `UPCGGraph` — GraphCode World's procedural-generation graph, and the FIRST kind World owns
	 * (`GC-WORLD-GENERATION-DESIGN.md` §4).
	 *
	 * A pure-dataflow node graph: no exec pins, every edge carries data, and the whole asset is a
	 * recipe rather than a result. That is what makes it the level-making mechanism the design picks
	 * over scatter — a ten-thousand-tree forest is one volume, one graph path and one seed, and the
	 * document does not grow with the forest.
	 *
	 * **ITS PINS ARE NAMED, WHICH IS WHERE IT PARTS FROM `kind:material`.** A `UMaterialExpression`
	 * is index-addressed and derives a pin's name from its index; PCG inverts that —
	 * `UPCGNode::GetInputPin(const FName&)` is the lookup, `UpdatePins` reconciles on a name basis
	 * and `RemoveDuplicates` enforces label uniqueness per node per direction. So an edge is
	 * `Label=node.Label` here where a material's is `in0=n0.out0`, and the difference is measured
	 * rather than stylistic (§4.0).
	 *
	 * World owns the codec; Core owns this token and the class path (a literal below) so an install
	 * without World still classifies the asset and writes an honest `omits: all@world` stub.
	 */
	PcgGraph,

	/**
	 * A `UPCGGraphInstance` — a PCG graph plus what one call site changed about it.
	 * `GC-WORLD-GENERATION-DESIGN.md` §4.3.
	 *
	 * **`kind:material_instance`'s SHAPE, ONE ASSET OVER, AND THE `parent:` LINE IS WHERE THAT SHOWS.**
	 * Every other World kind's parent is a `/Script` class the kind token already fixes; this one
	 * names a `UPCGGraph` — content, not code — because which graph an instance instances is the
	 * choice the document exists to record.
	 *
	 * **IT IS A SIBLING OF `PcgGraph`, NOT A NARROWING OF IT.** `UPCGGraphInstance` and `UPCGGraph`
	 * are both direct children of `UPCGGraphInterface`, which is why `GcPcgGraphClassPath` is tested
	 * for the EXACT class and why this kind exists at all: an ancestor test would swallow an instance
	 * into the graph kind and project its empty node list as though the graph it points at had none.
	 */
	PcgGraphInstance,

	/**
	 * A `UFont` — a NAMED SET of faces that UI text looks up by name. C7's last box.
	 *
	 * Its document is `settings { }` and `typeface { }`, and the second one is the asset: a font's
	 * whole job is to say that `Bold` means THIS face and `Italic` means that one, which is the
	 * relationship a binary hides and a line of text states.
	 *
	 * `FontCacheType` SPLITS THE CLASS IN TWO AND ONE KIND COVERS BOTH. A Runtime font is a composite
	 * font with no glyph data; an Offline font is 256 characters and an atlas texture with no faces.
	 * Measured on the engine's six fonts: not one holds both halves. They are one kind because the
	 * DOCUMENT is the same shape either way — settings that were authored, generated data declared on
	 * `omits: glyph_atlas` — where a `UFontFace` is a different asset doing a different job and is a
	 * kind of its own below.
	 */
	Font,

	/**
	 * A `UFontFace` — ONE imported `.ttf`/`.otf` and the decisions about rasterising it.
	 *
	 * `texture_2d`'s shape with ONE MEASURED DIFFERENCE: neither font class has a `UAssetImportData`,
	 * so there is no `import { }` and no `pipelines { }`. A face's provenance is the `SourceFilename`
	 * string on the asset, which is what the reimport factory reads, so it stays in `settings { }` as
	 * the authored field it is. The glyph outlines are `omits: source_binary`.
	 */
	FontFace,

	/**
	 * A `UFoliageType` — one entry of a level's foliage palette: a mesh, and the rules for scattering
	 * it. `GC-WORLD-GENERATION-DESIGN.md` §5.1, row G4.
	 *
	 * **`data_asset`'s DOCUMENT ON A CLASS THAT IS NOT A DATA ASSET, and that sentence is the whole
	 * design.** A foliage type declares nothing — no graph, no members, no sub-objects — so its
	 * entire authored content is the property values that differ from its class default, which is
	 * `values { }` and `valueir/1` exactly as an instance's is. What it does NOT share is ancestry:
	 * `UFoliageType` derives from `UObject` (MEASURED, `FoliageType.h:105`), so
	 * `IsProjectableInstanceClass`'s `IsChildOf(UDataAsset)` rule does not reach it and a foliage
	 * type projects NO SIDECAR AT ALL today.
	 *
	 * **It is a kind rather than a widening of that rule** (§5.1): the predicate is decidable from
	 * the class alone without loading the package, and relaxing it to `UObject` would sweep in every
	 * engine asset at once with no way to say which ones project well. So a second named native base
	 * joins it, and the token stays honest about what the asset is.
	 *
	 * **CORE OWNS IT — USER-RULED 2026-08-20: *"foliage itself is core, but foliage placement is
	 * world."*** `GC-PRODUCT-SPLIT.md` listed "Foliage types" under World; the ruling draws the line
	 * at ASSET versus PLACEMENT instead, and the measurement agrees — there is no World half of a
	 * `UFoliageType` to keep, so World owning it would write an `omits: all@world` stub for an asset
	 * Core reads completely. That is the outcome `animation_blueprint` is in Core to avoid. What IS
	 * World's is where the things end up: `omits: foliage_instances` on `kind:level`, and anything
	 * that ever carries it.
	 *
	 * **`UpdateGuid` IS THE TRAP AND IT IS A WRITE PROBLEM, NOT A READ ONE.** It is what tells a
	 * level's painted instances that their type has changed. Keeping it out of the file is free —
	 * it is a bare `UPROPERTY()` with no `CPF_Edit`, and so is `ChangeCount` — but a write that
	 * skipped `PostEditChangeProperty` would leave every placed instance stale against its own
	 * palette with nothing saying so. See `GraphCodeValueWriter.cpp`'s notify pass.
	 */
	FoliageType,

	/**
	 * A `UPhysicalMaterial` — friction, restitution, density and the surface type a trace reports.
	 * Core's, C13's tail.
	 *
	 * **`foliage_type`'s CASE A SECOND TIME: `values { }` on a class that is not a Data Asset.**
	 * `UPhysicalMaterial` derives from `UObject` (MEASURED, `PhysicalMaterial.h:102`), it declares
	 * nothing — no graph, no members, no sub-objects — and its entire authored content is the
	 * property values that differ from its class default. So it takes `data_asset`'s envelope,
	 * `data_asset`'s block and `valueir/1`, and the third named native base joins
	 * `IsValuesProjectableClass` rather than that rule being widened to `UObject`.
	 *
	 * **AND IT IS THE FIRST KIND HERE WITH NO BINARY HALF AND NO OMISSION OF ITS OWN.** A texture, a
	 * sound and a mesh all keep an importer's output out of the document and say so on `omits:`;
	 * this asset is what a person typed and nothing else. `MaterialHandle` is the physics engine's
	 * built copy and is not a `UPROPERTY` at all, so it is not a facet to omit — the codec's own
	 * limits are exactly this kind's limits, which is the sentence `foliage_type` already owns.
	 *
	 * **ANCESTRY RATHER THAN THE EXACT CLASS, and the trap the exact-class kinds have is ABSENT
	 * here.** `texture_2d` and `sound_wave` match a leaf because a descendant of each has a kind of
	 * its own to be stolen — a curve atlas, a MetaSound. Nothing descends from this class at all:
	 * MEASURED 2026-08-21 across `Engine/Source` and `Engine/Plugins`, UE ships ZERO subclasses. It
	 * is `Blueprintable` and concrete, so the set is open and matching the leaf would classify the
	 * corpus that exists and silently stop classifying whatever a project adds — `foliage_type`'s
	 * reasoning exactly, minus the abstraction.
	 *
	 * **`UPhysicalMaterialMask` IS A SIBLING AND IS DELIBERATELY NOT THIS KIND.** It derives from
	 * `UObject` directly (`Runtime/Engine/Classes/PhysicalMaterials/PhysicalMaterialMask.h`), so no
	 * ancestry test here can reach it and none needs to refuse it. It is named anyway so its
	 * absence reads as a decision rather than an oversight ([[an-omission-outlives-its-cause]]): its
	 * authored half is three fields, and the thing it actually IS — a per-pixel map from a source
	 * texture to physical materials — is generated from that texture the way a mesh's vertices are.
	 * Whoever covers it gets a kind and an `import { }`, not a case label here.
	 *
	 * **THE SURFACE NAME IS NOT IN THIS ASSET AND CANNOT BE.** `SurfaceType` is a
	 * `TEnumAsByte<EPhysicalSurface>` whose enumerators are `SurfaceType1 … SurfaceType62`; what
	 * calls one of them *Concrete* is `[/Script/Engine.PhysicsSettings] PhysicalSurfaces` in
	 * `DefaultEngine.ini`. So a document here reads `SurfaceType = SurfaceType3` and is complete —
	 * the mapping is project configuration, a second document, and C13's own row names it as the
	 * other half of this line.
	 *
	 * The three members a reader would expect to see and will not — `DestructibleDamageThresholdScale`
	 * and `PhysicalMaterialProperty`, both deprecated, and the transient `bShowExperimentalProperties`
	 * — drop through `InstancePolicy`'s `CPF_Edit`/`CPF_EditConst` rule without a name being spelled
	 * anywhere. That is the rule working, not a gap.
	 */
	PhysicalMaterial,

	/**
	 * A `UHLODLayer` — what a World Partition map builds its proxies from: the layer TYPE, the
	 * builder settings under it, the cell size and loading range of the grid it generates into, and
	 * the parent it chains to. `GC-WORLD-PARTITION-DESIGN.md` §9.
	 *
	 * **`foliage_type`'s AND `physical_material`'s CASE A THIRD TIME: `values { }` on a class that is
	 * not a Data Asset.** `UHLODLayer` derives from `UObject` (MEASURED,
	 * `Engine/Public/WorldPartition/HLOD/HLODLayer.h:41`), it declares no graph and no members, and
	 * its entire authored content is the property values that differ from its class default. So it
	 * takes `data_asset`'s envelope, `data_asset`'s block and `valueir/1`, and the FOURTH named
	 * native base joins `IsValuesProjectableClass` rather than that rule being widened to `UObject`.
	 *
	 * **IT IS CORE'S, AND THE BOARD ROW THAT ASKED FOR IT SAID WORLD'S.** `GC-PRODUCT-SPLIT.md` §5
	 * files *grids, cells, HLOD layers, data layers, runtime streaming* under World, and that
	 * sentence is about a LEVEL's storage — the runtime hash `omits: partition_hash` declares. The
	 * standalone asset is the other half, and the row one week earlier already answered it: WP row 6
	 * left a `UDataLayerAsset` in Core as `kind:data_asset` and shipped only the read-only
	 * `data_layers { }` projection in World. Same split here, same reason
	 * ([[asset-is-core-placement-is-world]]): Core reads this asset COMPLETELY, so filing the kind
	 * under World would write an `omits: all@world` stub for a document Core can produce whole —
	 * the outcome `foliage_type`'s entry above exists to avoid. What stays World's is where the
	 * layers END UP, which is the level's hash and is still declared rather than carried.
	 *
	 * **ANCESTRY RATHER THAN THE EXACT CLASS, on `physical_material`'s reasoning exactly.** The
	 * class is concrete and `Blueprintable`, and UE ships no subclass of it — so matching the leaf
	 * would classify the corpus that exists and silently stop classifying whatever a project adds.
	 *
	 * **THE ONE MEMBER THAT IS NOT A PLAIN VALUE IS `HLODBuilderSettings`, AND IT IS THE ONE FACET
	 * THIS KIND DECLARES RATHER THAN CARRIES — `omits: hlod_builder_settings`.** It is
	 * `UPROPERTY(VisibleAnywhere, Instanced, NoClear, meta=(EditInline, ShowInnerProperties))`, and
	 * `VisibleAnywhere` sets `CPF_Edit | CPF_EditConst`, so `InstancePolicy` excludes it — measured
	 * 2026-08-22 by the spec, against a first draft of this comment that claimed the opposite
	 * ([[visibleanywhere-admits-computed-fields]]: the flag is right about the POINTER, which the
	 * engine owns and replaces when `LayerType` changes, and silent about the CONTENTS, which
	 * `ShowInnerProperties` makes editable). Closing it means teaching the codec to descend into an
	 * `EditConst` instanced pointer, which is a change every values kind shares — so the gap is
	 * declared, `partition_hash`'s decision one row earlier.
	 *
	 * **AND `bIsSpatiallyLoaded=false` SILENTLY OVERRIDES TWO OTHER MEMBERS — a read trap, not a
	 * write one.** `GetCellSize()` returns 0 and `GetLoadingRange()` returns `WORLD_MAX` when the
	 * flag is off, while the STORED `CellSize` and `LoadingRange` keep whatever was typed. The
	 * document carries the stored values, because they are what the property editor holds and what a
	 * round trip has to return; a file written from the getters would lose an author's numbers the
	 * moment they unticked a box ([[a-query-that-answers-as-close-as-it-got]]).
	 */
	HLODLayer,

	/**
	 * A `UTextureRenderTarget` — a surface the GPU draws into: its dimensions, its pixel format, its
	 * clear colour, its addressing and its mip policy. `GRAPHCODE-CORE-QUEUE.md` §3 C13, the *tail
	 * kinds* board row.
	 *
	 * **THE FIRST KIND WHOSE `omits:` IS GENUINELY EMPTY, and that is the whole shape of it.** Every
	 * other asset with pixels declares the pixels — `texture_2d` writes `omits: source_pixels`
	 * because a mip chain is neither reviewable nor writable as text. A render target HAS no stored
	 * pixels to lose: `UTextureRenderTarget2D::Serialize` adds two version fix-ups and nothing else
	 * (MEASURED 2026-08-23, `TextureRenderTarget2D.cpp:290`), and its `FTextureSource` is empty
	 * because nothing was ever imported. So the settings ARE the asset, and `omits: none` is a
	 * measurement rather than an optimism.
	 *
	 * **`values { }` ON A CLASS THAT IS NOT A DATA ASSET, the fifth named base rather than a fifth
	 * widening** — `foliage_type`, `physical_material` and `hlod_layer`'s case again. It is also the
	 * first values kind that is NOT a bare `UObject`: a `UTextureRenderTarget` is a `UTexture`, so
	 * `kind:texture_2d` is the neighbour it has to stay out of the way of. That neighbour matches
	 * the EXACT class `UTexture2D` and always did, so the two never overlapped; what changes here is
	 * that `RefusalForTextureClass`'s render-target sentence stops pointing at a queue row and names
	 * this kind instead ([[a-refusal-reason-is-a-claim]] — a shipped reason goes stale).
	 *
	 * **AND IT TAKES `values { }` RATHER THAN THE TEXTURE KIND'S `settings { }`, which is a ruling.**
	 * The two blocks are the same machinery — `ExportDeviations` against the class CDO — so the
	 * choice is about what the document is, not what it can carry. `settings { }` is named that way
	 * on `texture_2d` because it sits BESIDE `import { }` and `pipelines { }` and has to say which
	 * half it is; its writer even carries a `bDeferRebuild` flag whose only purpose is sequencing
	 * those two against it. A render target has no source file, no import record and no reimport, so
	 * putting it on that route would hand it a coupling it can never use and split an exact-class
	 * predicate that exists to be exact. One block, one route, and the route already generalised.
	 *
	 * **ANCESTRY FROM THE ABSTRACT BASE, `foliage_type`'s arrangement exactly.** `UTextureRenderTarget`
	 * is `UCLASS(abstract)`; what a project holds is a 2D, a cube, a volume, a 2D array or a canvas
	 * target, and the reflected export covers each one whole because their differences ARE
	 * properties (`SizeZ`, `Slices`, `SampleCount`). Matching the five concrete classes would
	 * classify today's corpus and silently stop classifying whatever arrives next — the failure this
	 * kind exists to fix, since both of THIS project's render targets had no sidecar at all.
	 *
	 * **CREATION IS A PER-CLASS FACTORY TABLE AND A CREATED TARGET DOES NOT START AT ITS CDO.**
	 * `UTextureRenderTargetFactoryNew::FactoryCreateNew` calls `InitAutoFormat(Width, Height)` with
	 * the factory's own 256×256, while the CDO is 1×1 (MEASURED 2026-08-23,
	 * `EditorFactories.cpp:2232` / `TextureRenderTarget2D.cpp:35`) — so a create with no `values { }`
	 * lands at 256×256 and says so, `hlod_layer`'s template finding one row over.
	 */
	RenderTarget,

	/**
	 * A `UDataRegistry` — the lookup table's TABLE OF CONTENTS: which id format it accepts, which
	 * struct its items are, and the ordered list of sources it searches to find one.
	 * `GRAPHCODE-CORE-QUEUE.md` §3 C13, the same *tail kinds* board row as `render_target`.
	 *
	 * **THE FIRST KIND WHOSE CLASS THIS BUILD MAY NOT HAVE, AND IT NEEDS NO BUILD GATE FOR THAT.**
	 * `DataRegistry` is an optional engine plugin, `EnabledByDefault: false`, and it is OFF in this
	 * project. The values route already had the answer: `IsFoliageTypeClass` walks the superclass
	 * chain comparing a spelled PATH, so Core recognises a class it never compiles against. With the
	 * plugin off no asset can carry the class, nothing matches, and the kind is simply inert — no
	 * `#if`, no `.Build.cs` entry, no `WITH_GRAPHCODE_*` define. That is a smaller mechanism than
	 * `GraphCodeBuildGates`, and it is the right one HERE because this kind needs no engine TYPE at
	 * compile time: the whole document is reflected values ([[optional-plugin-gate-already-exists]]
	 * is about linking against a plugin's API, which this does not do).
	 *
	 * **`values { }` ON A CLASS THAT IS NOT A DATA ASSET — the sixth named base.** `UDataRegistry`
	 * derives from `UObject` (MEASURED 2026-08-23, `DataRegistry.h:24`), declares no graph and no
	 * members, and its whole authored content is the seven `EditDefaultsOnly` properties that differ
	 * from its class default. `foliage_type`'s, `physical_material`'s and `hlod_layer`'s case again.
	 *
	 * **THE MEMBER THAT DECIDES WHETHER THIS KIND IS WORTH HAVING IS `DataSources`, AND THE CODEC
	 * CARRIES IT.** It is `UPROPERTY(EditDefaultsOnly, Instanced) TArray<TObjectPtr<UDataRegistry
	 * Source>>` — an instanced-subobject array, which is EXACTLY the shape that made a `UInputAction`
	 * project an empty `values { }` in July and earn a kind of its own. That is no longer true and
	 * the difference is dated: AuthoringCore grew `ExportInstancedSubobjectValue`, and its fixture
	 * `UAuthoringCoreInstancedTestOwner::Items` is this member's shape exactly. So a registry's
	 * sources come out as nested authored objects rather than as bare references, and the reason
	 * `input_action` had to leave this route does not apply here.
	 *
	 * **`RuntimeSources` IS NOT AN OMISSION, it is not authored at all** — `VisibleDefaultsOnly,
	 * Transient`, so `CPF_EditConst` and `CPF_Transient` each drop it on their own. It is the list
	 * `DataSources` is EXPANDED INTO at runtime, rebuilt by `RefreshRuntimeSources()`, which is
	 * `physical_material`'s `MaterialHandle` argument: a built copy of something the document already
	 * carries is not a facet the document lost. So `omits: none`.
	 *
	 * **WHAT IS GENUINELY ELSEWHERE IS THE PROJECT'S REGISTRY CONFIGURATION, and it is a different
	 * document rather than a gap in this one.** `UDataRegistrySettings` — which directories are
	 * scanned for registries, and whether they preload — is `[/Script/DataRegistry.DataRegistry
	 * Settings]` in `DefaultGame.ini`. No tool in this project writes project config at all
	 * (measured 2026-08-23), which is the same open door `physical_material` hit with
	 * `PhysicalSurfaces`. This kind's document is COMPLETE for the asset; the `.ini` half is C13's
	 * still-open configuration line and is named here so the two are not confused.
	 */
	DataRegistry,

	/**
	 * THE TWO METASOUND ASSETS — Core Extension's second and third whole kinds.
	 * `GC-METASOUND-DESIGN.md`, unblocked 2026-09-10 after the 2026-08-19 *not now*.
	 *
	 * **TWO KINDS BECAUSE THE BLOCK SET DIFFERS**, which is `input_action`/`input_mapping_context`'s
	 * ruling word for word. A `UMetaSoundSource` is a `USoundWaveProcedural`, so it carries the whole
	 * `USoundBase` settings surface plus `OutputFormat` and the quality overrides; a `UMetaSoundPatch`
	 * is a bare `UObject` and has no `settings { }` at all. Everything else about them is identical —
	 * one `FMetasoundFrontendDocument` each, the same graph, the same members.
	 *
	 * **BOTH BY EXACT CLASS, AND THIS IS THE OTHER HALF OF A GUARD CORE HAS SHIPPED SINCE
	 * 2026-08-19.** `FGraphCodeSoundProjection::IsProjectableSoundClass` is exact precisely because a
	 * `UMetaSoundSource` derives from `USoundWave` two links down, so an `IsChildOf` rule there would
	 * have Core sweep these assets and write a settings document for each. That guard named this
	 * product and this kind while neither existed; they exist now.
	 *
	 * **SPELLED AS PATHS AND NEVER LINKED, `data_registry`'s arrangement** — see
	 * `GcMetaSoundSourceClassPath` below. `Metasound` is `EnabledByDefault: true` in the engine, so
	 * it is present in most installs; Core still must not take a dependency on it, because the whole
	 * point of the refusal above is that MetaSound belongs to another product.
	 */
	MetaSoundSource,
	MetaSoundPatch,

	/**
	 * A `UMetaHumanCharacter` — GraphCode Move's character kind (`GC-HUMAN-GRAMMAR.md` §3).
	 *
	 * **IT REFLECTS WHAT THE CHARACTER WEARS, AND NOT WHO IT IS**, which is the whole reason this
	 * kind is cheap. The engine's own module boundary falls in the same place the document's does:
	 * `MetaHumanCharacter` is a RUNTIME module holding skin, eyes, makeup, head model, wardrobe and
	 * texture info — all reflected `UPROPERTY`s a projection reads straight off the saved asset —
	 * while the face state, the region transforms and the body measurements live in two EDITOR
	 * modules and are not reachable from a file at all. An earlier draft tried to reflect all three
	 * and hit a problem with no precedent: the same asset projecting differently under `saved` than
	 * under `live` because half of it was structurally unreachable rather than merely unsaved.
	 *
	 * The identity half is AUTHORED instead, in a `.gchuman`, exactly as a `.gcshape` authors the
	 * geometry a static mesh declares on `omits: geometry`. So `omits: identity` here is the same
	 * sentence with a different noun, and `human: "…" program:…` is the same stamp as `shape:`.
	 *
	 * **CORE EXTENSION owns the codec**, not Move — `NEXT.md`'s Advanced section is *the
	 * fifth product, on optional engine plugins*, and MetaHumanCharacter is one exactly as MVVM,
	 * MetaSound and CommonUI are. Core owns this token and the class path (spelled as a literal
	 * below) so an install without that product still classifies the asset and writes an honest
	 * `omits: all@extension` stub rather than leaving every character invisible to version
	 * control.
	 */
	MetaHuman,

	/**
	 * A native `UGroomAsset` built or imported through HairStrands. Core owns the vocabulary and
	 * Core Extension supplies the codec when the optional HairStrands plugin is enabled. Generated
	 * curves remain in `.gcgroom`; this kind reflects their named groups, native summary and recipe
	 * provenance without pretending the curve bulk data round-trips through text.
	 */
	Groom,

	/**
	 * A native `UGroomBindingAsset`. Separate from `groom` because it binds one Groom to one target
	 * and carries different data and omissions. Its heavy root bulk stays native; the `.gc` states
	 * the relationship, build settings, group counts and the `.gcgroom` provenance that owns it.
	 */
	GroomBinding,

	/**
	 * A `UControlRigBlueprint` — GraphCode Rig's graph kind (`GC-RIG-LEDGER.md` §12.10).
	 *
	 * **IT EXISTS BECAUSE THIS BUILD WRITES ONE AND COULD NOT READ IT BACK.** `bulge=` emits a
	 * Control Rig to move the helper bone it makes, and until this kind that asset was the first
	 * thing GraphCode produced that no `.gc` could state — invisible to version control, to
	 * `gc_versions` and to anybody wanting to know what the generator had done.
	 *
	 * **THE RIGVM MODEL IS THE SOURCE OF TRUTH, NOT THE EDGRAPH**, which is why this cannot go
	 * through the Blueprint reader and why refusing it there is correct rather than a gap:
	 * `URigVMEdGraphSchema` derives from `UEdGraphSchema` and not from `UEdGraphSchema_K2`, and the
	 * EdGraph is a VIEW the model generates. Reading the view would make the document depend on
	 * whether an editor had ever been opened on the asset. `ControlRigIR` reads
	 * `URigVMGraph`/`URigVMNode`/`URigVMPin` instead.
	 *
	 * **THE KIND TEST MUST RUN BEFORE THE PLAIN BLUEPRINT ONE.** A `UControlRigBlueprint` IS a
	 * `UBlueprint` — the widget and Animation Blueprint case exactly, and with the same consequence
	 * if the order slips: the asset would classify as `blueprint`, and the Blueprint codec would
	 * then find a graph it has no schema for.
	 *
	 * **GraphCode Rig owns the codec.** Control Rig is an optional engine plugin and Rig is the
	 * product that already declares it — `GraphCodeRig.Build.cs` names `ControlRig`, `RigVM` and
	 * their developer modules to WRITE these assets. Core owns this token and the class path
	 * (spelled as a literal below) so an install without Rig still classifies the asset and writes
	 * an honest stub rather than leaving every generated rig invisible.
	 */
	ControlRig,

	/**
	 * An IMPORT PROPOSAL - the only kind here that describes an asset that does not exist yet.
	 *
	 * Every other kind projects something the project already contains, so its envelope binds to
	 * that asset's bytes. This one is written BEFORE the import, so `bin:` has nothing to digest
	 * and says `none`, and `sig:` cannot be the staleness gate either - the caller is meant to edit
	 * the body between the plan and the import. What must not move is the source file on disk, so
	 * the `import { }` block carries `source_hash` and that is what an apply checks.
	 *
	 * It is therefore NOT a sidecar: nothing writes one on save, the projector does not produce it,
	 * and `gc_sync` has nothing to sync. GC-IMPORT-DESIGN.md.
	 */
	Import,

	/**
	 * Not a kind. The domain's size, so the capability registry can ITERATE the enum instead of
	 * restating it.
	 *
	 * `GraphCode.Projection.KindTokenRoundTrip` spent six kinds asserting a hand-written list whose
	 * own comment claimed that omitting a kind was "a failure rather than a silent pass". It was
	 * not: the list is additive, so `DataTable`, both input kinds and the three curves were added
	 * and the test went on passing over the old nine. A list cannot notice what is missing from it.
	 * This is what lets something walk `0 .. Count` and find out.
	 */
	ProceduralVegetation,
	PcgSettings,
	/**
	 * A `UNiagaraParameterCollection` — GraphCode Move's, beside `niagara_system`
	 * (`GC-MOVE-NIAGARA-PARAMETER-COLLECTION.md`): a namespace, an optional source material collection
	 * and typed parameters with their defaults. Core owns the token and the class path for the same
	 * reason it owns `niagara_system`'s: an install without Move still classifies one and stubs it.
	 */
	NiagaraParameterCollection,
	/**
	 * A `UAnimComposite`, exact class — GraphCode Move's (`GC-MOVE-ANIM-COMPOSITE.md`): one ordered track of
	 * animation segments plus the blocks every `UAnimSequenceBase` kind shares. The montage's sibling under
	 * `UAnimCompositeBase`, which is why the montage and sequence tests stay exact.
	 */
	AnimComposite,
	/**
	 * A `UNiagaraSimCache`, exact class — GraphCode Move's, beside `niagara_system`
	 * (`GC-MOVE-NIAGARA-FIELD-1116-PLAN.md` N19): a recorded run of one system, its capture recipe, frames,
	 * emitters and attributes. The class is the Niagara plugin's own, so Core spells it as a path.
	 */
	NiagaraSimCache,
	Count,
};

/**
 * The two IKRig plugin classes, spelled as PATHS so Core can classify them without depending on the
 * plugin that defines them.
 *
 * `GraphCodeProducts.h`'s standing arrangement is that the vocabulary is Core's and the code is the
 * product's, and a class path is vocabulary. Every other class test in `IsSupportedAssetData` says
 * `X::StaticClass()->GetClassPathName()`, which is the same `FTopLevelAssetPath` these produce —
 * but reaching it that way needs the header, and the header needs a module dependency Core has no
 * other reason to carry. `FAssetData::AssetClassPath` is compared without resolving a class either
 * way, so the two spellings do the identical work.
 *
 * **WHAT A LITERAL LOSES is the compile error when a class is renamed**, so the check moves to
 * where the type CAN be named: `GraphCodeMove`'s tests assert each of these against the real
 * `StaticClass()`. A rename therefore fails a spec instead of silently un-classifying every IK
 * asset in the project.
 */
inline FTopLevelAssetPath GcIkRigClassPath()
{
	return FTopLevelAssetPath(TEXT("/Script/IKRig"), TEXT("IKRigDefinition"));
}

inline FTopLevelAssetPath GcIkRetargeterClassPath()
{
	return FTopLevelAssetPath(TEXT("/Script/IKRig"), TEXT("IKRetargeter"));
}

inline FTopLevelAssetPath GcBehaviorTreeClassPath()
{
	return FTopLevelAssetPath(TEXT("/Script/AIModule"), TEXT("BehaviorTree"));
}

// GraphCode Move's `kind:state_tree`, spelled as a path for the same reason as the kinds above —
// Core classifies a `UStateTree` without depending on the StateTree plugin. Move's tests assert this
// against `UStateTree::StaticClass()` so a rename fails a spec rather than silently un-classifying
// every StateTree asset in the project.
inline FTopLevelAssetPath GcStateTreeClassPath()
{
	return FTopLevelAssetPath(TEXT("/Script/StateTreeModule"), TEXT("StateTree"));
}

// GraphCode Move's `kind:niagara_system`, spelled as a path for the same reason as the IK kinds
// above — Core classifies a `UNiagaraSystem` without depending on the Niagara plugin. Move's tests
// assert this against `UNiagaraSystem::StaticClass()` so a rename fails a spec, not silently.
inline FTopLevelAssetPath GcNiagaraSystemClassPath()
{
	return FTopLevelAssetPath(TEXT("/Script/Niagara"), TEXT("NiagaraSystem"));
}

// GraphCode Move's `kind:niagara_parameter_collection`, path-spelled as `niagara_system` above is.
inline FTopLevelAssetPath GcNiagaraParameterCollectionClassPath()
{
	return FTopLevelAssetPath(TEXT("/Script/Niagara"), TEXT("NiagaraParameterCollection"));
}

// GraphCode Move's `kind:niagara_sim_cache`, path-spelled as `niagara_system` above is.
inline FTopLevelAssetPath GcNiagaraSimCacheClassPath()
{
	return FTopLevelAssetPath(TEXT("/Script/Niagara"), TEXT("NiagaraSimCache"));
}

// GraphCode Core Extension's `kind:metahuman`. Spelled as a path for the reason the kinds above
// are, and here with the force `GcPcgGraphClassPath` has: `MetaHumanCharacter` is an OPTIONAL engine
// plugin that a customer may genuinely not have installed, so Core must classify a
// `UMetaHumanCharacter` and write the `omits: all@extension` stub without the plugin being present
// at all. Core Extension's tests assert this against `UMetaHumanCharacter::StaticClass()` — behind
// `WITH_GRAPHCODE_METAHUMAN`, since that is the only build in which the type can be named — so a
// rename fails a spec rather than silently un-classifying every character in the project.
//
// EXACT CLASS. `UMetaHumanCharacter` derives straight from `UObject` and has no shipped subclass,
// but the exactness is the rule rather than a measurement of today's engine.
inline FTopLevelAssetPath GcMetaHumanClassPath()
{
	return FTopLevelAssetPath(TEXT("/Script/MetaHumanCharacter"), TEXT("MetaHumanCharacter"));
}

/** HairStrands is optional, so Core classifies both generated assets by path without linking it. */
inline FTopLevelAssetPath GcGroomClassPath()
{
	return FTopLevelAssetPath(TEXT("/Script/HairStrandsCore"), TEXT("GroomAsset"));
}

inline FTopLevelAssetPath GcGroomBindingClassPath()
{
	return FTopLevelAssetPath(TEXT("/Script/HairStrandsCore"), TEXT("GroomBindingAsset"));
}

// GraphCode Rig's `kind:control_rig`. Spelled as a path for the reason the kinds above are, and
// with `GcMetaHumanClassPath`'s force: ControlRig is an OPTIONAL engine plugin, so Core must
// classify a `UControlRigBlueprint` and write its stub without the plugin being present at all.
// Rig's tests assert this against `UControlRigBlueprint::StaticClass()` so a rename fails a spec
// rather than silently un-classifying every generated rig in the project.
//
// **`ControlRigDeveloper` AND NOT `ControlRig`, which is the trap in this one line.** The runtime
// module holds `UControlRig`; the asset a person opens is a `UControlRigBlueprint` and that lives
// in the DEVELOPER module. A path naming the obvious module compiles, reads plausibly, and matches
// nothing forever.
//
// EXACT CLASS, and here the exactness has teeth: a `UControlRigBlueprint` IS a `UBlueprint`, so
// `TryGetAssetKind` must test this BEFORE its Blueprint ancestry test or every rig classifies as
// `kind:blueprint`. See the kind comment.
inline FTopLevelAssetPath GcControlRigClassPath()
{
	return FTopLevelAssetPath(TEXT("/Script/ControlRigDeveloper"), TEXT("ControlRigBlueprint"));
}

// GraphCode Move's `kind:level_sequence`. Spelled as a path for the same reason as the kinds above,
// with one difference worth stating: `LevelSequence` is an engine RUNTIME MODULE rather than an
// optional plugin, so Core COULD depend on it. It still does not — the rule here is that Core
// classifies an asset without linking the product that owns its codec, and making an exception for
// the one module that happens to be always-present is how the rule stops being a rule. Move's tests
// assert this against `ULevelSequence::StaticClass()` so a rename fails a spec rather than silently
// un-classifying every cinematic in the project.
inline FTopLevelAssetPath GcLevelSequenceClassPath()
{
	return FTopLevelAssetPath(TEXT("/Script/LevelSequence"), TEXT("LevelSequence"));
}

// GraphCode Core Extension's `kind:env_query`. Spelled as a path for the same reason as the kinds
// above — Core classifies a `UEnvQuery` without depending on AIModule. Core Extension's tests assert
// this against `UEnvQuery::StaticClass()` so a rename fails a spec rather than silently
// un-classifying every EQS asset in the project.
inline FTopLevelAssetPath GcEnvQueryClassPath()
{
	return FTopLevelAssetPath(TEXT("/Script/AIModule"), TEXT("EnvQuery"));
}

// GraphCode World's `kind:pcg_graph`. Spelled as a path for the reason the kinds above are, and here
// the reason has more force than it does for any of them: PCG is `EnabledByDefault: false`, so it is
// the first plugin in this list a project may genuinely not have. Core must classify a `UPCGGraph`
// and write the `omits: all@world` stub without the plugin being present at all.
//
// **THE EXACT CLASS, AND THE EXACTNESS IS LOAD-BEARING.** `UPCGGraphInstance` is a sibling under
// `UPCGGraphInterface`, not a subclass of this — it is `parent:` + overrides, `kind:material_instance`'s
// shape, and its own kind when it lands (§4.3). An ancestor test would swallow it into this one and
// project an instance's empty node list as if the graph had none.
inline FTopLevelAssetPath GcPcgGraphClassPath()
{
	return FTopLevelAssetPath(TEXT("/Script/PCG"), TEXT("PCGGraph"));
}

inline FTopLevelAssetPath GcPcgSettingsClassPath()
{
	return FTopLevelAssetPath(TEXT("/Script/PCG"), TEXT("PCGSettings"));
}

inline FTopLevelAssetPath GcProceduralVegetationClassPath()
{
	return FTopLevelAssetPath(TEXT("/Script/ProceduralVegetation"), TEXT("ProceduralVegetation"));
}

// GraphCode World's `kind:pcg_graph_instance` (§4.3). The SIBLING of the class above under
// `UPCGGraphInterface`, and the reason both are matched exactly rather than by ancestry: an
// `IsInstanceOf` test on the interface would classify an instance as a graph and project its empty
// node list as though the graph it points at had none.
inline FTopLevelAssetPath GcPcgGraphInstanceClassPath()
{
	return FTopLevelAssetPath(TEXT("/Script/PCG"), TEXT("PCGGraphInstance"));
}

// `kind:foliage_type`'s BASE class, and the one entry in this list matched by ANCESTRY rather than
// exactly — G4, §5.1.
//
// **THE ABSTRACT BASE ON PURPOSE.** `UFoliageType` is `UCLASS(abstract)`; what a project actually
// holds is a `UFoliageType_InstancedStaticMesh` or a `UFoliageType_Actor`, and `UFoliageType` is
// `Blueprintable`, so a fourth subclass can arrive from `/Game`. Naming the two concrete engine
// classes would classify today's assets and silently stop classifying tomorrow's — an asset that
// projects no file at all, which is the failure this kind exists to fix.
//
// **The exactness the neighbours need does not apply here**: nothing else in `TryGetAssetKind`
// claims a `UFoliageType`, and no other kind's class derives from one, so an ancestor test has
// nothing to swallow. That is the opposite of `PCGGraph`/`PCGGraphInstance`, which are siblings
// under one interface, and the reason both spellings appear in this file.
//
// Spelled as a path so Core takes no dependency on the Foliage module for a class it only wants to
// recognise — the arrangement every kind above uses, and `FGraphCodeValueProjection` walks the
// superclass chain comparing this against `GetPathName()` exactly as its four refusals already do.
inline FTopLevelAssetPath GcFoliageTypeClassPath()
{
	return FTopLevelAssetPath(TEXT("/Script/Foliage"), TEXT("FoliageType"));
}

// `kind:hlod_layer`'s base class, matched by ANCESTRY like `foliage_type` above.
//
// **NOT SPELLED AS A PATH FOR THE REASON THE NEIGHBOURS ARE, AND THIS IS THE EXCEPTION WORTH
// NAMING.** Every path in this list exists so Core can recognise a class from a module it does not
// compile against — Foliage, PCG, AIModule. `UHLODLayer` is in `Engine`, which Core already has as
// a PUBLIC dependency, so there is nothing to avoid: `FGraphCodeValueProjection` names the real
// class and gets `GcIsBlendSpaceClass`'s trade — a renamed or moved class becomes a compile error
// instead of a string that quietly stops matching ([[case-sensitivity-needs-a-near-miss]]).
//
// The path is written down anyway because the registry fast path compares class path names without
// loading, and one spelling for both directions is what keeps the loaded and unloaded answers from
// disagreeing.
inline FTopLevelAssetPath GcHLODLayerClassPath()
{
	return FTopLevelAssetPath(TEXT("/Script/Engine"), TEXT("HLODLayer"));
}

// `kind:render_target`'s ABSTRACT base class, matched by ancestry like `foliage_type` above — C13.
//
// Named through the real class in `FGraphCodeValueProjection` and spelled here for the registry
// fast path, which is `GcHLODLayerClassPath`'s arrangement above and for its reason: `UTexture
// RenderTarget` is in `Engine`, so there is no module dependency to avoid and the compile error is
// worth having. Two spellings, one truth, and a spec asserts they agree.
inline FTopLevelAssetPath GcRenderTargetClassPath()
{
	return FTopLevelAssetPath(TEXT("/Script/Engine"), TEXT("TextureRenderTarget"));
}

// `kind:data_registry`'s base class, matched by ancestry — C13.
//
// **SPELLED AS A PATH, AND HERE THAT IS NOT A STYLE CHOICE — it is the only option.** The two
// helpers above name real classes because `Engine` is already Core's public dependency. `UData
// Registry` belongs to an OPTIONAL plugin that is `EnabledByDefault: false` and is off in this
// project, so there is no class to name: `FGraphCodeValueProjection` walks the superclass chain
// comparing this, exactly as `GcFoliageTypeClassPath` is walked, and with the plugin absent nothing
// ever matches. That is the whole gate — an inert kind rather than a compiled-out one.
inline FTopLevelAssetPath GcDataRegistryClassPath()
{
	return FTopLevelAssetPath(TEXT("/Script/DataRegistry"), TEXT("DataRegistry"));
}

/**
 * The two MetaSound asset classes, matched by EXACT path — `GC-METASOUND-DESIGN.md` §3.
 *
 * **SPELLED AS PATHS FOR `GcDataRegistryClassPath`'s REASON WITH ONE WORD CHANGED: not because the
 * class may be absent, but because Core must not NAME it.** `Metasound` is `EnabledByDefault: true`,
 * so the classes are usually here — and `FGraphCodeSoundProjection` already refuses a MetaSound by
 * comparing `GetName()` against a prefix, precisely so Core would not link `MetasoundEngine` to
 * spell a refusal about another product's asset. Linking it now to spell a CLASSIFIER would undo
 * that, so these are literals and `GraphCodeCoreExtension`'s specs assert each against the real
 * `StaticClass()` — `GcIkRigClassPath`'s arrangement, and its rename-safety argument unchanged.
 *
 * **EXACT, NEVER AN ANCESTOR WALK, AND THAT IS NOT A MARGIN HERE.** `UMetaSoundSource` derives from
 * `USoundWaveProcedural` derives from `USoundWave`, so a walk would answer `metasound_source` for
 * every ordinary wave's ancestors in the other direction and — worse — the neighbouring exactness in
 * `IsProjectableSoundClass` exists to stop the same collision going the other way. Two exact tests
 * over one class hierarchy is the only arrangement where neither claims the other's assets.
 */
inline FTopLevelAssetPath GcMetaSoundSourceClassPath()
{
	return FTopLevelAssetPath(TEXT("/Script/MetasoundEngine"), TEXT("MetaSoundSource"));
}

inline FTopLevelAssetPath GcMetaSoundPatchClassPath()
{
	return FTopLevelAssetPath(TEXT("/Script/MetasoundEngine"), TEXT("MetaSoundPatch"));
}

/** True for either MetaSound asset class, by EXACT path. Neither resolves nor loads a class. */
inline bool GcIsMetaSoundClassPath(const FTopLevelAssetPath& ClassPath)
{
	return ClassPath == GcMetaSoundSourceClassPath() || ClassPath == GcMetaSoundPatchClassPath();
}

/** True for the two MetaSound kinds. The predicate `GcIsValuesKind`'s comment argues for. */
inline bool GcIsMetaSoundKind(const EGraphCodeAssetKind Kind)
{
	return Kind == EGraphCodeAssetKind::MetaSoundSource || Kind == EGraphCodeAssetKind::MetaSoundPatch;
}

/**
 * Every kind whose asset is a `UBlueprint`.
 *
 * **A PREDICATE RATHER THAN A LIST AT EACH SITE, and that is the whole reason it exists.** Before
 * §11.4b there were three Blueprint kinds and twelve places asking *"is this one of them"* by
 * writing the comparison out — `Kind != Blueprint && Kind != WidgetBlueprint && Kind !=
 * AnimationBlueprint`. Adding a kind to that shape means finding all twelve, and the ones you miss
 * do not fail to compile: `properties` simply reports *"not a Blueprint declaration block"* for an
 * asset that plainly is one. Same silent shape [[gc-level-kind]] recorded for the sweep's class
 * filter, on the kind axis.
 *
 * WIDGET IS IN, because it is a `UBlueprint` and every one of those twelve sites wanted it. The one
 * question that excludes it — `settings { }`, where a Widget Blueprint has its own route — asks for
 * the widget case FIRST and falls through to this, which is how the exclusion stays visible instead
 * of hiding inside a predicate's name.
 */
/**
 * Every kind whose whole projection is `values { }` — the VALUES kinds.
 *
 * **A PREDICATE RATHER THAN THE LIST AT EACH SITE, for `GcIsBlueprintKind`'s reason and on a list
 * that has grown three times in three weeks.** `data_asset` was alone, then `foliage_type` (G4),
 * then `physical_material` (C13), and each arrival meant finding every `A || B` and widening it —
 * two in `GraphCodeApply` alone, one deciding whether `values` is the only block the kind projects
 * and one deciding whether the write route accepts it. **A site somebody misses does not fail to
 * compile:** the kind projects a `values { }` document that the apply route then refuses as though
 * the block name were wrong, which reads as a grammar error rather than as a missing case
 * ([[a-silent-no-op-is-worse-than-a-refusal]]).
 *
 * **THE ROUTE IS STILL SPELLED `DataAsset` AND THIS PREDICATE IS NOT NAMED AFTER IT.** That token
 * is on every report a caller has already seen and does not move; what a fourth kind must not do is
 * make the route's NAME decide which kinds it takes.
 */
inline bool GcIsValuesKind(const EGraphCodeAssetKind Kind)
{
	switch (Kind)
	{
	case EGraphCodeAssetKind::DataAsset:
	case EGraphCodeAssetKind::FoliageType:
	case EGraphCodeAssetKind::PhysicalMaterial:
	case EGraphCodeAssetKind::HLODLayer:
	case EGraphCodeAssetKind::RenderTarget:
	case EGraphCodeAssetKind::DataRegistry:
		return true;
	default:
		return false;
	}
}

/**
 * What a person calls one of the kinds above, for a refusal a person reads.
 *
 * The PROSE word rather than the token, and per kind rather than one shared noun: every sentence
 * these appear in is a claim about what the caller is holding, and printing *Data Asset* at somebody
 * holding an HLOD layer sends them looking for a kind mismatch that is not there
 * ([[a-refusal-reason-is-a-claim]]). Empty for anything that is not a values kind, so a caller that
 * forgot to ask `GcIsValuesKind` first gets nothing to print rather than a wrong word.
 */
inline const TCHAR* GcValuesKindNoun(const EGraphCodeAssetKind Kind)
{
	switch (Kind)
	{
	case EGraphCodeAssetKind::DataAsset:
		return TEXT("Data Asset");
	case EGraphCodeAssetKind::FoliageType:
		return TEXT("foliage type");
	case EGraphCodeAssetKind::PhysicalMaterial:
		return TEXT("physical material");
	case EGraphCodeAssetKind::HLODLayer:
		return TEXT("HLOD layer");
	case EGraphCodeAssetKind::RenderTarget:
		return TEXT("render target");
	case EGraphCodeAssetKind::DataRegistry:
		return TEXT("data registry");
	default:
		return TEXT("");
	}
}

inline bool GcIsBlueprintKind(const EGraphCodeAssetKind Kind)
{
	switch (Kind)
	{
	case EGraphCodeAssetKind::Blueprint:
	case EGraphCodeAssetKind::WidgetBlueprint:
	case EGraphCodeAssetKind::AnimationBlueprint:
	case EGraphCodeAssetKind::BlueprintInterface:
	case EGraphCodeAssetKind::BlueprintFunctionLibrary:
	case EGraphCodeAssetKind::BlueprintMacroLibrary:
		return true;
	default:
		return false;
	}
}

/**
 * The three sub-types alone — a `UBlueprint` whose `BlueprintType` is not `BPTYPE_Normal`.
 *
 * Separate from the predicate above because the two answer different questions and a caller that
 * conflates them gets it wrong in both directions. This one is for the FACTORY and for the
 * assertions about it; that one is for *does this asset have Blueprint blocks*.
 */
inline bool GcIsBlueprintSubTypeKind(const EGraphCodeAssetKind Kind)
{
	return Kind == EGraphCodeAssetKind::BlueprintInterface
		|| Kind == EGraphCodeAssetKind::BlueprintFunctionLibrary
		|| Kind == EGraphCodeAssetKind::BlueprintMacroLibrary;
}

/**
 * A complete GraphCode projection ready to serialize.
 *
 * The individual IR readers own the block bodies. GraphCode supplies the envelope, computes
 * the document digest, normalizes line endings, and writes the projection next to the asset.
 *
 * ONE DOCUMENT, NOT TWO SLABS. This was a header plus IR sections joined by `--`, and the
 * two sections came from two serializers that were never meant to share a file: the asset
 * path appeared three times, the parent class twice, and nothing read a `.gc` back to
 * notice when the three disagreed. Now the envelope states each of those once and every
 * block below it is a named block that declares only what is its own.
 */
struct GRAPHCODE_API FGraphCodeDocument
{
	int32 FormatVersion = 1;
	EGraphCodeAssetKind Kind = EGraphCodeAssetKind::Blueprint;
	FString PackageName;

	/** Stated once here, so no block below repeats it. Optional: a struct or enum has none. */
	FString ParentClassPath;

	/** Optional Behavior Tree blackboard asset path; empty means the tree has no blackboard. */
	FString BlackboardAssetPath;

	/**
	 * Optional StateTree schema class path; empty for every kind but `state_tree`.
	 *
	 * `UStateTreeEditorData::Schema` is `Instanced, NoClear` — a tree without one is not runnable —
	 * and it decides what the tree may contain (including whether a state holds `Tasks` or the lone
	 * `SingleTask`). It is envelope-shaped for the same reason `blackboard:` is on a Behavior Tree:
	 * stated once, consumed at CREATE, compared on edit, never a block. GC-STATE-TREE-DESIGN.md §4.4.
	 */
	FString SchemaClassPath;

	/**
	 * **WHERE A GENERATED MESH CAME FROM** — the `.gcshape` that built it and a digest of the
	 * program, projected as one `shape:` line. Empty for every asset nobody generated.
	 *
	 * **IT IS ENVELOPE-SHAPED FOR `import { }`'S REASON TURNED AROUND** (`GC-SHAPE-AUTHORING-DESIGN.md`
	 * §11, row M8). `import { }` is a BLOCK because its two fields are the visible half of an edit
	 * surface a reimport writes back through; this is provenance and nothing writes it back — the
	 * asset states it, a `.gc` reflects it, and `gc_apply` has no more business setting it than it
	 * has setting `source:`. **A mesh has one or the other, never both**: an imported mesh has no
	 * program and a generated one was never imported.
	 */
	FString ShapeSource;
	FString ShapeProgram;

	/**
	 * **WHERE A BUILT CLIP CAME FROM** — the `.gcmove` that made it and a digest of the program, as one
	 * `move:` line. `shape:`'s ruling for motion (`GC-MOVE-GRAMMAR.md` §0): the keyframes stay on
	 * `omits: bone_animation`, and the program that made them is named instead. Empty for every
	 * animation nobody generated.
	 */
	FString MoveSource;
	FString MoveProgram;

	/**
	 * **WHERE A BUILT CHARACTER CAME FROM** — the `.gchuman` that made it and a digest of the
	 * program, as one `human:` line. `shape:`'s ruling for identity (`GC-HUMAN-GRAMMAR.md` §2.5):
	 * the face state stays on `omits: identity`, and the program that produced it is named instead.
	 * Empty for every character nobody generated.
	 */
	FString HumanSource;
	FString HumanProgram;

	/** The `.gcgroom` recipe provenance on a native Groom or Binding. Read-only in `.gc`. */
	FString GroomSource;
	FString GroomProgram;

	/**
	 * What this projection does NOT carry.
	 *
	 * `covers:` used to sit beside this and was deleted: a covered facet always emits its
	 * block, so the block being there IS the coverage statement, and a hand-written token
	 * list beside it is just a second place to disagree with the body. Absence is the one
	 * thing presence cannot express, which is why this side stayed.
	 *
	 * A completely covered asset writes `omits: none`; the field never disappears.
	 */
	TArray<FString> Omits;
	TArray<FString> IrVersions;

	/**
	 * What an ABSENT property line means. Optional; empty omits the field entirely.
	 *
	 * GC-GRAMMAR-AUDIT.md D6 declined to put this on every `.gc`, and that ruling stands and is the
	 * reason the field is optional rather than required: a Blueprint document mixes facets whose
	 * baselines differ - `properties` against `defaults { }`, `class_defaults` against the parent
	 * CDO, `components` against each template - so one global line would be false for most of them.
	 *
	 * A level has exactly ONE facet, so the line is honest there and load-bearing: it is what tells
	 * a reader apart from a `.gcp`, whose own `baseline: none` means the opposite. The two files
	 * would otherwise be indistinguishable while meaning opposite things (WORLD-GC-DESIGN.md §3).
	 */
	FString Baseline;

	/**
	 * Digest of the `.uasset` this projection was taken from. Optional; empty omits the field.
	 *
	 * HARD BINDING (§14.3). The body digest says the text still describes the asset GraphCode
	 * projected; this says that asset is still the one on disk. Together they let a tool, a
	 * commit hook or CI verify a sidecar without loading the asset — without Unreal at all.
	 *
	 * Empty means "predates the binding, go and re-project", never "unchanged". So it is left
	 * empty whenever the honest answer is unknown: a dirty package's on-disk bytes are not what
	 * the body was read from, and stamping them would be a lie in exactly the direction that
	 * matters.
	 */
	FString BinaryDigest;

	/** Named blocks in document order, each already `name { … }`. */
	TArray<FString> Blocks;

	/** Optional deterministic overrides. Empty values use the current project and engine version. */
	FString SourceName;
	FString SourceVersion;

	/**
	 * `errors { }` — why this document is known not to become an asset, or which of its facets are
	 * not this install's to write. `GC-PRODUCT-PACKAGING-DESIGN.md`.
	 *
	 * **NOT ONE OF `Blocks`, AND THAT IS THE WHOLE REASON IT CAN BE HERE.** `sig:` is taken over the
	 * joined `Blocks` and this is appended after the digest, so a document that gains an annotation
	 * states exactly the signature it would have stated without one. That matches `SignedBody`,
	 * which strips the block before verifying, and it is what lets the note be durable instead of
	 * making every annotated file read as hand-edited (E11.7a).
	 *
	 * Aliases are minted by the renderer, never supplied here.
	 */
	TArray<FGcSidecarError> Errors;
};

/** Pure G0 path, envelope, and transactional-write implementation for GraphCode sidecars. */
class GRAPHCODE_API FGraphCodeSidecarWriter
{
public:
	/** Map /Game/Foo/Asset to the absolute project path Content/Foo/Asset.gc. */
	static bool TryGetSidecarPath(const FString& PackageName, FString& OutPath, FString& OutError);

	/** Assemble the canonical GraphCode v1 text. Produces LF text with exactly one trailing newline. */
	static bool BuildText(const FGraphCodeDocument& Document, FString& OutText, FString& OutError);

	/** Write text through a same-directory temporary file and atomically replace the destination. */
	static bool WriteTextAtomically(const FString& SidecarPath, const FString& Text, FString& OutError);

	/** Atomic LF/UTF-8 write for a generated local `.gcs`; creates no proposal or local history. */
	static bool WriteDerivedSliceAtomically(
		const FString& SlicePath, const FString& Text, FString& OutError);

	/**
	 * The same write, over bytes nobody is allowed to reinterpret.
	 *
	 * WriteTextAtomically normalizes line endings and forces exactly one trailing newline, which is
	 * right for a projection this code produced and wrong for text it is only holding: restoring a
	 * hand edit through it would hand the author back a file that differs from the one they wrote.
	 * So a restore goes through here, and "unchanged" can be asserted byte for byte.
	 */
	static bool WriteBytesAtomically(
		const FString& SidecarPath, TArrayView<const uint8> Bytes, FString& OutError);

	/**
	 * Digest one `.uasset` for FGraphCodeDocument::BinaryDigest.
	 *
	 * Over the file's bytes, which are NOT semantically stable — re-saving an unchanged
	 * Blueprint can move them. So equal means definitely unchanged and different means may have
	 * changed, and a caller that sees a difference re-projects to find out. That is the
	 * conservative direction, and it is the only one safe for a check that can run with no
	 * editor to consult.
	 */
	static bool ComputeBinaryDigest(const FString& AssetFilename, FString& OutDigest, FString& OutError);

	/** The `.uasset` path a package projects from, and whether its on-disk bytes can be trusted. */
	static bool TryGetDigestibleAssetFile(const UPackage* Package, FString& OutFilename);

	/**
	 * The file on disk a package name maps to, `.uasset` OR `.umap`.
	 *
	 * One function because `kind:level` arrived and every caller that hard-coded
	 * GetAssetPackageExtension() silently stopped finding a level - which does not fail, it just
	 * leaves `bin:` off and the fast path permanently declined. The extension is decided by which
	 * file exists, not by the kind, so no caller has to know what it is looking at.
	 */
	static bool TryGetPackageFilename(const FString& PackageName, FString& OutFilename);

	/** Exact byte comparison against the canonical UTF-8-without-BOM representation. */
	static bool FileMatchesText(const FString& SidecarPath, const FString& Text);

	/** The file is Text plus a `seal:` on its first line, and nothing else differs. */
	static bool FileMatchesTextKeepingSeal(const FString& SidecarPath, const FString& Text);

	/** Build and write one complete document. */
	static bool Write(const FGraphCodeDocument& Document, FString& OutPath, FString& OutError);

	static const TCHAR* KindToken(EGraphCodeAssetKind Kind);

	/**
	 * "THIS WRITE KNOWS WHAT IT IS LANDING ON", and it is the exact word `PreserveProposalIfEdited`
	 * turns on: that hook exists to catch a regeneration that is *uninformed* about the proposal it
	 * is about to overwrite. A write that has already established the proposal's fate is not that,
	 * and reporting it as one is a false statement about somebody's data.
	 *
	 * Everywhere else, a projection landing on top of a `merged-from:` file is the loss §4.12.1a
	 * exists to catch, so the writer copies the text to Saved/ and says so at Warning. Inside this
	 * scope that report would be false twice over — nothing is being lost, and the copy would
	 * accumulate one backup per merge — so the preservation is skipped for these paths and no other.
	 *
	 * TWO WRITES QUALIFY, and it took a false toast in front of a person to find the second:
	 *
	 * - **The apply absorbing it** (M2.10) — a whole-document apply that has already checked the
	 *   proposal against its ours-parent and is now putting it into the asset.
	 * - **The panel settling it afterwards** (E10.12) — `SettleAppliedProposal` re-projects a `.gc`
	 *   whose every block the asset ALREADY holds, which happens on the commonest ending of a real
	 *   merge: take `ours` on every decision and the apply writes nothing, so there is no
	 *   re-projection inside the apply's own scope to clear `merged-from:`. Unscoped, that write
	 *   told the user *"saving the asset has regenerated the file"* about a save that never
	 *   happened, and dumped a backup of text the asset already held — once per finished merge.
	 *
	 * It is a STATEMENT, not a mute switch, which is why the path is named rather than passing a
	 * bool: a write informed about A must not silence a write to B that happens underneath it.
	 * Game-thread only, like every apply, and nested scopes restore the outer one.
	 */
	class GRAPHCODE_API FInformedProposalWriteScope
	{
	public:
		explicit FInformedProposalWriteScope(const FString& SidecarPath);
		~FInformedProposalWriteScope();

		FInformedProposalWriteScope(const FInformedProposalWriteScope&) = delete;
		FInformedProposalWriteScope& operator=(const FInformedProposalWriteScope&) = delete;

	private:
		FString Previous;
	};

	/** Whether this exact sidecar is being written right now by something that knows it is a proposal. */
	static bool IsInformedProposalWrite(const FString& SidecarPath);

	/**
	 * "THE ASSET DID NOT CHANGE; THE PROJECTION DID." User-directed 2026-08-06 after a corpus
	 * regeneration put 169 entries in the local history store in one command.
	 *
	 * **IT NO LONGER SUPPRESSES THE ARCHIVE — IT LABELS IT.** Reversed 2026-08-27; the paragraph
	 * below is the original reasoning, kept because it is still half right.
	 *
	 * A sweep — `GcRegenerate`, `gc_sync`, `GcCheck`'s write half — writes because the PROJECTOR
	 * moved, not because anybody edited anything: the predecessor is the same asset spelled by older
	 * code, so archiving it stores a version no person ever made and pushes real recovery points out
	 * under the budget. True of the ASSET. False of the FILE, which is what the store holds: the
	 * older spelling is the only copy of itself, and dropping it left assets nobody had edited
	 * holding one entry forever. The budget pressure it feared was hypothetical — measured at 5.4 MB
	 * of 256 MB with nothing ever pruned. So a body-changing refresh is archived under
	 * `EGcHistoryReason::Refresh`, and `ArchivePredecessor`'s own equal-body and envelope-only
	 * checks are what keep a sweep over an unchanged corpus free.
	 *
	 * **Nothing authored is lost by this, and that is load-bearing rather than hopeful.**
	 * `PreserveProposalIfEdited` runs on the line above and is untouched: a predecessor that fails
	 * to parse, whose `sig:` does not match, or that is a `merged-from:` proposal still goes to
	 * `Saved/GraphCode/Proposals/`.
	 *
	 * **A RUN, NOT A PATH**, which is the one way it differs from `FInformedProposalWriteScope`
	 * above. That scope names its file on purpose, so a write informed about A cannot silence a
	 * write to B underneath it. A corpus sweep writes hundreds of paths it discovers as it goes;
	 * naming them would be a list that goes stale against its own loop. `FGraphCodeGenerator::Run`
	 * and the apply's post-save re-projection are the only two that open it, and the sweep now
	 * drains the save-hook queue BEFORE opening it — those queued packages are a person's saves and
	 * were being written inside the scope, which is how an authored save that landed as a sweep
	 * started lost its predecessor.
	 *
	 * Game-thread only, and nested scopes restore the outer one.
	 */
	class GRAPHCODE_API FProjectionRefreshScope
	{
	public:
		FProjectionRefreshScope();
		~FProjectionRefreshScope();

		FProjectionRefreshScope(const FProjectionRefreshScope&) = delete;
		FProjectionRefreshScope& operator=(const FProjectionRefreshScope&) = delete;

	private:
		bool bPrevious = false;
	};

	/** Whether a projection refresh is running, which decides an entry's reason rather than its existence. */
	static bool IsProjectionRefresh();

	/**
	 * **AN ASSET IS BEING BORN AT THIS PATH, SO NO WRITE TO IT ARCHIVES A PREDECESSOR.**
	 *
	 * A create has nothing to archive by definition — `FGcGreenfield::Create` re-checks that the
	 * path is empty immediately before it builds — and yet it produced history entries, which is the
	 * user's report *"VCS panel shows changed the asset, not created the asset ... looks like an
	 * empty asset before that, not something uncreated"*.
	 *
	 * WHERE THE PHANTOM VERSION CAME FROM. `Create` builds a bare asset and then applies the
	 * document into it, and (as `FInformedProposalWriteScope`'s note in that function records)
	 * *every `ApplyBlock` syncs first, and that sync projects the freshly built (still empty) asset*.
	 * So the sidecar is written mid-birth, and the save at the end of the create then finds that
	 * projection sitting on disk and archives it as *the version before this one*. It is not one. It
	 * is a photograph of scaffolding, and the only reason it ever reached the store is that the
	 * writer cannot see which call it is inside.
	 *
	 * The two candidate predecessors during a create are that scaffolding and the create RECIPE the
	 * document was read from, and neither is a state anybody had. What records a birth is the
	 * `EGcHistoryTransition::Created` tombstone the caller writes before building.
	 *
	 * **A PATH AND NOT A BOOL, FOR THE REASON `FInformedProposalWriteScope` IS ONE and the reason
	 * `FProjectionRefreshScope` warns about draining the save-hook queue first.** A create SAVES,
	 * and a save can drain queued projections for packages that have nothing to do with it. A global
	 * flag would strip those writes of their predecessors — silent history loss for somebody else's
	 * asset, which is the exact defect this is here to end.
	 *
	 * Game-thread only, and nested scopes restore the outer one.
	 */
	class GRAPHCODE_API FAssetCreationScope
	{
	public:
		explicit FAssetCreationScope(const FString& SidecarPath);
		~FAssetCreationScope();

		FAssetCreationScope(const FAssetCreationScope&) = delete;
		FAssetCreationScope& operator=(const FAssetCreationScope&) = delete;

	private:
		FString Previous;
	};

	/** Is THIS sidecar the one an in-flight create is building? Archiving is off for it alone. */
	static bool IsCreatingAsset(const FString& SidecarPath);

	// ---------------------------------------------------------------------------------------
	// THE PROJECTION THIS BUILD LAST HANDED OUT — CP1 fixes item 5.
	//
	// **Local history archives PREDECESSORS, so it holds a revision only once something has
	// REPLACED it.** That is backwards for the base of a three-way merge, which is by definition
	// the revision the caller is still standing on. Measured 2026-08-25 across the walk's own
	// shadow ledger: seven `apply-stale` observations could reach no base, five of them at the
	// DOCUMENT seat, where the `.gc` on disk is the caller's edited text and therefore cannot be
	// the base either.
	//
	// **AND A LEVEL IS WHERE IT BITES, because a level moves with nothing written anywhere.**
	// Measured on `L_HollowVale`: saving `BP_Pudu` with its capsule set to the value its six
	// placed instances already held dropped six lines from the level's projection and moved its
	// `sig` — with `bin:` byte-identical, no `.umap` save and no `.gc` write. So no predecessor
	// was archived, because nothing superseded anything.
	//
	// This is the third carrier, and it is one file per package, overwritten in place: not a
	// version store, not budgeted, not pruned, and never shown to a person. `RunStaleThreeWay`
	// is its only reader and the digest is its whole guard — a slot that does not hash to the
	// signature the patch names is not taken, so a stale or resurrected one is inert rather
	// than wrong.
	//
	// **THE LIMIT, NAMED RATHER THAN HIDDEN**: it remembers what THIS install wrote. A `.gc` that
	// arrived from source control and has never been re-projected here has no slot, and a stale
	// apply against it still falls back on the refusal.
	// ---------------------------------------------------------------------------------------

	/** Where the remembered projection for one package lives, or empty for a name that cannot be one. */
	static FString LastProjectionPathFor(const FString& PackageName);

	/**
	 * Remember canonical projection text as the base a later stale apply may need. The package name
	 * comes out of the DOCUMENT, exactly as the archive site takes it: a `.gc` states the asset it
	 * projects, and reversing the sidecar path would be a second implementation of the mount table.
	 * Silent on anything that is not canonical projection text — a proposal is not a base.
	 */
	static void RememberProjection(const FString& CanonicalText);

	/**
	 * The remembered projection for one package, but only when it hashes to `BodySignature`.
	 * Returns false with `OutText` untouched otherwise, including when there is no slot at all.
	 */
	static bool RecallProjection(
		const FString& PackageName, const FString& BodySignature, FString& OutText);
};
