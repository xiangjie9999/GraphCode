// GraphCode Vision, phase 1 — the camera, and nothing that needs a GPU.
//
// `Docs/GC-VISION-DESIGN.md` is the design. `Docs/GC-VISION-HANDOFF.md` §6 is the build order, and
// it is the reason this file exists before a single pixel does: phase 1 is "the camera, entirely
// headless. No GPU, no editor", and every gate below is asserted under `-nullrhi`.
//
// THE ONE THING TO UNDERSTAND BEFORE READING THE CODE: the hard part of `gc_render` is not the
// render. The engine already ships the preview-scene half — `FThumbnailPreviewScene` and its
// per-kind family are in production behind every Content Browser tile. What it ships in a form the
// design is right to reject is the CAMERA: `FClassActorThumbnailScene` fits a subject by
// sphere radius and zooms by adding centimetres (`ThumbnailHelpers.cpp:1204-1214`), so a tall actor
// is framed by its diagonal and the same `OrbitZoom` means "slightly closer" on a building and
// "inside the mesh" on a doorknob. Everything here is that correction: an eight-corner fit, a
// logarithmic zoom, a box-normalised offset, and an angle chosen by projected geometry.
//
// AND THE SECOND THING: the angle selection is ANALYTIC, not a lattice of rendered masks. The three
// scores the design named — silhouette area, thinness, a front/three-quarter prior — are all
// closed-form on the projected oriented box (`GC-VISION-HANDOFF.md` §4). Eight matrix multiplies per
// candidate, no render target, no readback. That is what moves smart framing out of GPU-gated phase
// 3 and into this file, where a tall actor, a thin plane and a symmetric subject can be fixtured
// without an editor at all. Masks remain the ESCALATION, justified by a measured bad pick
// (`GC-VISION-DESIGN.md` §5.5, phase 3 P3.4) — not by cost.
//
// NOT HERE, ON PURPOSE: output lifetime, destination, templating and `manifest.json`. The handoff
// moves them behind the camera, and §7 item 2 is still an open decision. A request struct that
// guessed it would be the wrong kind of finished.

#pragma once

#include "CoreMinimal.h"

/** What the resolved numbers in a result MEAN. Stated in every result; never inferred by a caller. */
enum class EGraphCodeRenderSpace : uint8
{
	/** An isolated preview world's coordinates. A camera copied from here is not a level camera. */
	PreviewLocal,
	/** The loaded editor world's coordinates. */
	World,
};

/** Which axes `azimuth`/`elevation` orbit around — `GC-VISION-DESIGN.md` §5.1. */
enum class EGraphCodeRenderBasis : uint8
{
	/** The subject's own forward/right/up. `azimuth:0` is its front. The default. */
	Local,
	/** World axes, for an aggregate selection whose "forward" is nobody's. */
	World,
};

enum class EGraphCodeRenderProjection : uint8
{
	Perspective,
	/**
	 * BUILT 2026-08-03. The design refused it until implemented rather than approximating it with a
	 * narrow FOV, and that refusal is now gone.
	 *
	 * The one thing that changes shape: `zoom:0` cannot be a DISTANCE here. Moving an orthographic
	 * camera does not change how large anything is — the ortho WIDTH does — so the fit solves for a
	 * width and the standoff distance becomes bookkeeping. Everything else (azimuth, elevation, the
	 * logarithmic zoom, the offset, the anchor) means exactly what it means in perspective.
	 */
	Orthographic,
};

/** The context the subject is rendered in. The REQUEST chooses it; editor state never does. */
enum class EGraphCodeRenderScene : uint8
{
	Preview,
	LoadedLevel,
};

/**
 * Anchor forms — `GC-VISION-DESIGN.md` §5.2.
 *
 * The camera core resolves the three that are pure geometry. `Component`, `Actor` and `ActorSet`
 * name things only an adapter can look up: the request is checked HERE for being well formed — a
 * name list that is empty or has the wrong count is a refusal no world is needed for — and the
 * level adapter turns the names into a box and a basis before the solver ever sees the view
 * (`GC-VISION-HANDOFF.md` phase 4, P4.2).
 */
enum class EGraphCodeRenderAnchorMode : uint8
{
	/** The centre of the occupying box. */
	Auto,
	/** The basis origin, which for a preview subject is where the actor was spawned. */
	Origin,
	/** An explicit point plus the radius `zoom:0` needs, because a point has no size to fit. */
	Point,
	Component,
	Actor,
	ActorSet,

	/**
	 * `{region:"$Crypt"}` — an AUTHORED region of the level's `.gcw`, §21.13's R6.
	 *
	 * IT IS THE SAME SEAM AS `actor`, one source over: the adapter resolves the name into a box and
	 * a basis and hands the solver an `auto` anchor, so every framing, fitting and scoring answer
	 * downstream is the arithmetic that was already proved. What a region brings is that the box is
	 * **its own oriented prism** — `FGraphCodeRegionSolver::Bounds`, rotated by the region's
	 * `forward:` — rather than an actor's bounds, so *photograph the crypt* frames the room somebody
	 * authored and not the union of whatever happens to be standing in it.
	 */
	Region,
};

/** Stable error/warning tokens. Prose changes; these do not, so tests and callers key on them. */
/**
 * What a Move clip says about the air it is played in (GC-MOVE-INTERACTION-GRAMMAR-PLAN.md §11), spelled once for
 * the build that writes it and the preview that reads it. `Settle` is package metadata on the clip, in seconds;
 * the wind is three float curves on it, the air's velocity in metres a second in the character's component space.
 */
class UAnimSequenceBase;
class UMaterial;

namespace GraphCodeClipAir
{
	inline constexpr const TCHAR* SettleKey = TEXT("GraphCodeMove.Settle");
	inline constexpr const TCHAR* WindCurves[3] = { TEXT("GC_WindX"), TEXT("GC_WindY"), TEXT("GC_WindZ") };
	/** The clip's wind at a moment of it (clamped to the clip), in metres a second; unset when the clip states none. */
	GRAPHCODE_API TOptional<FVector> AirAt(const UAnimSequenceBase* Clip, double Seconds);
}

namespace GraphCodeRenderToken
{
	/** Errors — the request or the subject is unusable and nothing is resolved. */
	inline constexpr const TCHAR* AssetMissing = TEXT("asset_missing");
	inline constexpr const TCHAR* ViewsTooMany = TEXT("views_too_many");
	inline constexpr const TCHAR* ViewNameInvalid = TEXT("view_name_invalid");
	inline constexpr const TCHAR* ViewNameDuplicate = TEXT("view_name_duplicate");
	inline constexpr const TCHAR* DimensionOutOfRange = TEXT("dimension_out_of_range");
	inline constexpr const TCHAR* TotalPixelsExceeded = TEXT("total_pixels_exceeded");
	inline constexpr const TCHAR* CameraAmbiguous = TEXT("camera_ambiguous");
	inline constexpr const TCHAR* FovOutOfRange = TEXT("fov_out_of_range");
	inline constexpr const TCHAR* ZoomOutOfRange = TEXT("zoom_out_of_range");
	inline constexpr const TCHAR* PresetUnknown = TEXT("preset_unknown");
	inline constexpr const TCHAR* MarginOutOfRange = TEXT("margin_out_of_range");
	inline constexpr const TCHAR* ProfileUnknown = TEXT("profile_unknown");
	/**
	 * An orthographic camera whose width nobody supplied — §5.6.
	 *
	 * `projection_unsupported` used to live here and is GONE: orthographic is implemented, so
	 * nothing is left for it to refuse. What replaces it is the same shape as the point anchor's
	 * radius rule — an ABSOLUTE ortho camera frames nothing, so its width cannot be derived and has
	 * to be given. An ortho ORBIT needs no width at all, because `zoom:0` solves for one.
	 */
	inline constexpr const TCHAR* OrthoWidthInvalid = TEXT("ortho_width_invalid");
	/**
	 * The cut STATEMENT is malformed — `GC-SPATIAL-AUTHORING-DESIGN.md` §21.13b.
	 *
	 * An unknown axis letter, a point count that is not two or three, two points that are one point,
	 * three collinear points on a vertical line, or a `below` that is not a positive thickness. All of
	 * them are wrong before any camera exists, which is why they are checked without one.
	 */
	inline constexpr const TCHAR* CutInvalid = TEXT("cut_invalid");
	/**
	 * The cut is a real plane and this view cannot be cut by it.
	 *
	 * Exactly one case, and it is not the one this token was first drafted for: a plane through the
	 * EYE. §21.13b rules that everything between the camera and the plane is discarded, and a camera
	 * lying on its own cut is on both sides of that sentence at once. A plane at any ANGLE to the view
	 * is fine — the projection carries an oblique near plane, so nothing has to be perpendicular.
	 */
	inline constexpr const TCHAR* CutUnresolvable = TEXT("cut_unresolvable");
	/**
	 * A cut in perspective. The near plane a cut becomes lives in the ORTHOGRAPHIC matrix; a
	 * perspective one is built by the engine from a single scalar and carries no plane to replace.
	 * Refused rather than ignored, because a section drawing that silently came back uncut is a
	 * picture of a roof — the exact failure §21.13a exists to remove.
	 */
	inline constexpr const TCHAR* CutProjectionUnsupported = TEXT("cut_projection_unsupported");
	/**
	 * `view_mode:` named a mode this build cannot draw. The refusal lists what it would have taken.
	 *
	 * REFUSED RATHER THAN DEFAULTED, on `preset_unknown`'s rule and not `highlight_nothing_found`'s: a
	 * mistyped mode that quietly rendered `lit` hands back a correct-looking picture of a question
	 * nobody asked, with nothing about it reading as wrong.
	 */
	inline constexpr const TCHAR* ViewModeUnknown = TEXT("view_mode_unknown");
	/**
	 * `inside` on an ORTHOGRAPHIC camera.
	 *
	 * An orthographic picture does not depend on where along its own axis the eye stands, so *stand
	 * inside it* states nothing there and would silently return the outside picture. The orthographic
	 * way to see into something is `cut:`, which removes what is in front of a plane; `inside` is the
	 * perspective one, which stands behind it.
	 */
	inline constexpr const TCHAR* InsideProjectionUnsupported = TEXT("inside_projection_unsupported");
	/** A lens or grade value outside the range the editor's own property panel would accept. */
	inline constexpr const TCHAR* SettingOutOfRange = TEXT("setting_out_of_range");
	/** `settings.from_actor` named something that is not a camera, or is not there at all. */
	inline constexpr const TCHAR* SettingsActorUnresolvable = TEXT("settings_actor_unresolvable");
	inline constexpr const TCHAR* AbsoluteAimMissing = TEXT("absolute_aim_missing");
	inline constexpr const TCHAR* AbsoluteAimAmbiguous = TEXT("absolute_aim_ambiguous");
	inline constexpr const TCHAR* AbsoluteLookAtDegenerate = TEXT("absolute_look_at_degenerate");
	inline constexpr const TCHAR* AnchorUnresolvable = TEXT("anchor_unresolvable");
	inline constexpr const TCHAR* AnchorRadiusInvalid = TEXT("anchor_radius_invalid");
	/**
	 * A named camera actor could not be used — §5.7. Its own token rather than a shared one, because
	 * an anchor and a camera are different request fields: a caller who fixed a spelling should be
	 * told which of the two names was wrong.
	 */
	inline constexpr const TCHAR* CameraActorUnresolvable = TEXT("camera_actor_unresolvable");
	/**
	 * R8's ignore list named an actor this level does not have.
	 *
	 * REFUSED RATHER THAN WARNED, and it is the anchor's argument turned up: an unresolvable anchor
	 * produces no picture, while an unresolvable ignore produces the picture the caller was
	 * specifically avoiding — with the thing they wanted gone still in it, and nothing about the
	 * result reading as wrong.
	 */
	inline constexpr const TCHAR* IgnoreUnresolvable = TEXT("ignore_unresolvable");
	/** Only a level has actors to leave out; a preview world holds the subject and nothing else. */
	inline constexpr const TCHAR* IgnoreNotALevel = TEXT("ignore_not_a_level");
	/** `highlights:` named a type this build does not know — §4.3b. The refusal lists the known set. */
	inline constexpr const TCHAR* HighlightUnknown = TEXT("highlight_unknown");
	/** Only a level has navigation or effects to mark; a preview world holds the subject alone. */
	inline constexpr const TCHAR* HighlightNotALevel = TEXT("highlight_not_a_level");
	/**
	 * ~~A well-formed `highlights:` this build cannot draw.~~ **DORMANT — the overlay draws.**
	 *
	 * It went out, came back, and went again inside one day, which is the part worth keeping. It
	 * returned on a measurement that was real but WRONG, and the correction is worth carrying: two
	 * engine debug materials assigned RAW drew 0 px each against an opaque control that drew, which
	 * was read as *nothing translucent reaches a capture*. **`R15` retracted that 2026-08-18** — a
	 * purpose-built translucent material draws at full AND half opacity. What was true is only that
	 * those two materials are invisible when used that way. At the time
	 * the overlay was ruled HALF TRANSPARENT — so the feature genuinely could not produce a picture
	 * and was answering `navmesh: 1` over frames with no navmesh in them. The user then struck
	 * transparency from the ruling, and an OPAQUE overlay draws.
	 *
	 * **The translucency defect is real and still open**; it simply no longer stands between this
	 * parameter and a picture. The token stays declared because a caller may have branched on the
	 * string, and because this is the second time it has been needed.
	 */
	inline constexpr const TCHAR* HighlightNotImplemented = TEXT("highlight_not_implemented");
	/**
	 * A highlight type resolved to nothing in this level — no navmesh built, no emitters placed.
	 *
	 * A WARNING AND NOT AN ERROR, which is the opposite of `ignore_unresolvable` one field over, and
	 * the asymmetry is the point: an unresolvable ignore produces the picture the caller was
	 * specifically avoiding, while an empty highlight produces a picture that is *correct* — there
	 * really is no navmesh there. The caller is told because *nothing was drawn* and *nothing is
	 * there* are the same image and not the same fact.
	 */
	inline constexpr const TCHAR* HighlightNothingFound = TEXT("highlight_nothing_found");
	/**
	 * A marked subject declared no size of its own and was drawn at the default radius — §4.3b's
	 * fourth ruling, which requires the result to say WHICH.
	 *
	 * **ITS OWN TOKEN AND NOT `highlight_nothing_found`'s**, though both are warnings on the same
	 * line of the same call: *nothing is there* and *something is there and its size is a
	 * convention* are different facts, and a caller that branches on the string is entitled to tell
	 * them apart. A default radius is not a measurement.
	 */
	inline constexpr const TCHAR* HighlightDefaultSize = TEXT("highlight_default_size");
	inline constexpr const TCHAR* BasisMismatch = TEXT("basis_mismatch");
	inline constexpr const TCHAR* BasisNonUniformScale = TEXT("basis_non_uniform_scale");
	inline constexpr const TCHAR* SourceUnsupported = TEXT("source_unsupported");
	/** The asset is not a kind this has an adapter for, or is one it cannot use. */
	inline constexpr const TCHAR* SubjectUnsupported = TEXT("subject_unsupported");
	/** No RHI, or the render itself failed. Distinct from a request that was wrong. */
	inline constexpr const TCHAR* CannotRender = TEXT("cannot_render");
	/**
	 * A level capture changed something about the editor's own session. An ERROR and never a
	 * warning: by the time this can be reported the user's camera has already moved, and the whole
	 * point of the feature was not borrowing it.
	 */
	inline constexpr const TCHAR* EditorStateMoved = TEXT("editor_state_moved");

	/** Output — where the picture goes. `GC-VISION-DESIGN.md` §3, ruled 2026-08-03. */
	inline constexpr const TCHAR* OutputPathMissing = TEXT("output_path_missing");
	inline constexpr const TCHAR* OutputAbsoluteRefused = TEXT("output_absolute_refused");
	inline constexpr const TCHAR* OutputTraversal = TEXT("output_traversal");
	inline constexpr const TCHAR* OutputNameInvalid = TEXT("output_name_invalid");
	inline constexpr const TCHAR* OutputContentRefused = TEXT("output_content_refused");
	inline constexpr const TCHAR* OutputTemplateMissingView = TEXT("output_template_missing_view");
	/**
	 * §11's frames would collapse into one file, and it is `{view}`'s rule one dimension down.
	 *
	 * Its own token rather than `output_template_missing_view`'s because the fix is a different
	 * character: a caller told *add {view}* who is rendering one view of twenty-four frames would add
	 * the wrong field and get the same refusal back.
	 */
	inline constexpr const TCHAR* OutputTemplateMissingFrame = TEXT("output_template_missing_frame");
	inline constexpr const TCHAR* OutputTemplateUnresolved = TEXT("output_template_unresolved");
	inline constexpr const TCHAR* OutputExists = TEXT("output_exists");

	/**
	 * §11's time span is malformed — `to` before `at`, `fps` and `count` together, a non-positive
	 * rate, a count below one, or a moment outside the stated hour.
	 *
	 * All of them are string-and-arithmetic facts about the request, so all of them are refused before
	 * a subject is loaded, in company with every other check in `ValidateRequest`.
	 */
	inline constexpr const TCHAR* TimeInvalid = TEXT("time_invalid");
	/** More frames than `FGraphCodeRenderTime::MaxFrames`. A stated budget, refused rather than cut. */
	inline constexpr const TCHAR* FramesTooMany = TEXT("frames_too_many");

	/** Warnings — the view resolved, and something about it is worth saying out loud. */
	inline constexpr const TCHAR* BoundsSubstituted = TEXT("bounds_substituted");

	/**
	 * The framed region states no `height:`, so it is a COLUMN (§21.5) and its prism is flat.
	 *
	 * A WARNING AND NOT A SUBSTITUTION. The file says the room has a floor and does not say where its
	 * ceiling is; inventing one would frame a volume nobody authored, and the honest picture is of
	 * everything the `.gcw` actually knows about. The caller is told so they can state a height or
	 * accept a plan view.
	 */
	inline constexpr const TCHAR* RegionIsColumn = TEXT("region_is_column");
	/**
	 * The engine's framing whitelist knew none of this actor's components, so the subject was
	 * measured by the design's broader rule instead — every visible, non-hidden, non-editor
	 * primitive. Said out loud because the two rules can disagree, and because the whitelist is what
	 * every Content Browser tile is framed by.
	 */
	inline constexpr const TCHAR* BoundsWidened = TEXT("bounds_widened");
	/**
	 * T-6 — the anchor draws nothing itself, so the subject was measured from the actors ATTACHED
	 * below it.
	 *
	 * A ROOT WITH NO MESH IS AN ORDINARY SHAPE, not a mistake: `BP_Cabinet` and
	 * `BP_CompartmentRoot` are both mesh-less parents whose children carry every part. Framing a
	 * 100 cm cube at such an anchor is a picture of nothing, and a positive `zoom` on that cube then
	 * puts the camera inside whatever stands next to it — two wasted renders before the substitution
	 * warning was understood.
	 *
	 * Said out loud rather than done silently, because the box is then somebody else's: the caller
	 * asked for one actor and is being shown the extent of its descendants.
	 */
	inline constexpr const TCHAR* BoundsFromChildren = TEXT("bounds_from_children");
	inline constexpr const TCHAR* OutputContentImported = TEXT("output_content_imported");
	inline constexpr const TCHAR* OutputRenamedUnique = TEXT("output_renamed_unique");
	/** The pixels arrived before the shaders did. Not "wrong" — "do not trust these yet". */
	inline constexpr const TCHAR* SubjectIncomplete = TEXT("subject_incomplete");
	inline constexpr const TCHAR* ElevationClamped = TEXT("elevation_clamped");
	/**
	 * A6, 2026-10-21: a moving render anchored on a bone or socket moved its camera with it, frame by frame, turning no
	 * more than the first frame did. Said out loud because the reported camera is the first frame's.
	 */
	inline constexpr const TCHAR* CameraFollowed = TEXT("camera_followed");
	/**
	 * N14 (GC-MOVE-NIAGARA-FIELD-1116-PLAN.md), 2026-11-16: a level view with `simulate:true` and `at:` reset and
	 * advanced the open level's active Niagara components to that moment. Said out loud, with the count, because it is
	 * the one thing a level capture moves; `camera_followed`'s kind of note (it DID happen), never `time_not_seekable`,
	 * which stays for what was not advanced.
	 */
	inline constexpr const TCHAR* EffectsWarmedUp = TEXT("effects_warmed_up");
	/**
	 * A named camera's own aspect ratio is not the picture's. Said out loud because the vertical
	 * field of view then differs from what that camera's viewfinder frames, which is a difference a
	 * caller can fix by asking for a view whose width:height matches the camera's.
	 */
	inline constexpr const TCHAR* CameraAspectDiffers = TEXT("camera_aspect_differs");
	inline constexpr const TCHAR* DistanceClamped = TEXT("distance_clamped");
	inline constexpr const TCHAR* CameraInsideBounds = TEXT("camera_inside_bounds");
	/**
	 * `inside` has no fitted distance to divide, so `zoom` moved the LENS — and it ran out of lens.
	 *
	 * Zoom's definition is *logarithmic in projected size*, which perspective spends on a distance and
	 * orthographic on a width; an interior camera has neither, so it spends it on the field of view.
	 * Same definition, third arithmetic — and unlike the other two this one has a hard end at
	 * `MinFov`/`MaxFov`. Said out loud because a silent clamp reads as a zoom that did nothing.
	 */
	inline constexpr const TCHAR* InsideZoomClamped = TEXT("inside_zoom_clamped");
	inline constexpr const TCHAR* SubjectCropped = TEXT("subject_cropped");

	/** §11: a span stated neither `fps` nor `count`, so `DefaultCount` moments were taken. */
	inline constexpr const TCHAR* FrameCountDefaulted = TEXT("frame_count_defaulted");
	/**
	 * §11: the span is not a whole number of frames at the stated rate, so it ends early.
	 *
	 * The rate is what a caller stated and the span is what gives — the reasoning is on
	 * `FGraphCodeRenderTime::Fps`. This says where it actually stopped, because *ends at 2.0* and
	 * *ends at 2.05* are the same picture count and not the same animation.
	 */
	inline constexpr const TCHAR* FrameSpanTruncated = TEXT("frame_span_truncated");
	/**
	 * §11.6: `video:"mp4"` was asked for where no MP4 can be written, so an APNG was written instead.
	 *
	 * A WARNING AND NEVER AN ERROR, for the reason the contact sheet's failure is one: the frames are
	 * the expensive part, they are already rendered and already correct, and refusing the whole
	 * capture over the container would throw away everything that worked. The animation still exists
	 * and still plays — it is a different file under a different extension, which is exactly what the
	 * warning says, so a caller who wanted the `.mp4` path knows why it is not there.
	 *
	 * The two reasons it fires are not the same problem: this machine has no Media Foundation (a
	 * Windows "N" edition, or not Windows at all), which the caller cannot fix, and an odd pixel
	 * dimension, which the caller can fix by asking for an even one.
	 */
	inline constexpr const TCHAR* VideoFormatUnavailable = TEXT("video_format_unavailable");
	/**
	 * §11: something in this picture could not be posed at the requested moment.
	 *
	 * A WARNING AND NEVER AN ERROR, which is the same asymmetry `highlight_nothing_found` sits on: the
	 * view family's clock is set for every subject, so the moment is always honoured for materials and
	 * anything else reading engine time, and this reports only the components that additionally needed
	 * posing and could not be. A refusal would throw away a picture that is correct about most of
	 * itself; silence would let an unmoving mesh read as an animation that does not move.
	 */
	inline constexpr const TCHAR* TimeNotSeekable = TEXT("time_not_seekable");
	/**
	 * §11.10: this picture was SEEKED and the subject holds a node that only moves when it is ticked.
	 *
	 * **THE PICTURE IS WRONG IN A WAY THAT LOOKS LIKE A BROKEN RIG, WHICH IS WHY IT IS WORTH A TOKEN
	 * OF ITS OWN.** A seek runs an animation's evaluate half and not its update half, so a node that
	 * integrates sits at its rest state while every bone around it moves — and the skin bridging the
	 * two is stretched into thin fins and split by dark slits. **Measured 2026-09-18 on an elephant
	 * carrying seven `jiggle=` springs**: at a full 90 degree fold the seeked capture tore across the
	 * belly and the SAME moment under `simulate:true` was clean. Two readers in one session called a
	 * correct rig broken from that picture, and the second was the user.
	 *
	 * **THE TEST IS THE ENGINE'S OWN AND CANNOT GO STALE:** `FAnimNode_Base::NeedsDynamicReset()` is
	 * virtual, defaults false, and is overridden true by exactly the nodes that carry simulation
	 * state. Nothing here enumerates node types, so a node the engine adds tomorrow answers for
	 * itself — which is the rule `gc_apply`'s settings gate states for the same reason.
	 *
	 * A WARNING AND NEVER AN ERROR: the geometry, the camera and every other subject in the frame are
	 * right, and `simulate:true` costs a replay the caller may not want for a rest-pose check.
	 */
	inline constexpr const TCHAR* SimulationNotAdvanced = TEXT("simulation_not_advanced");
	/**
	 * `simulate:true` ran hair that has nothing to collide with: Unreal's hair collides only with a
	 * skeletal mesh's physics asset, so on a static target or one without bodies it falls through the
	 * head. A warning, since the fall is still the asset's own physics and may be all that was asked.
	 */
	inline constexpr const TCHAR* SimulationWithoutCollision = TEXT("simulation_without_collision");
	/**
	 * §6.5: `kind:"thumbnail"` asked for a size the renderer would not draw at.
	 *
	 * `width`/`height` are a BOX on this route rather than a promise, and that is the engine's rule
	 * and not ours: `ThumbnailTools::RenderThumbnail` asks a `UTextureThumbnailRenderer` how big its
	 * subject WANTS to be and scales that down inside the box, preserving aspect. So a 512x512
	 * request on a 2048x1024 texture draws 512x256 — the whole texture at its own shape, which is
	 * the right picture and the wrong dimensions to have promised. Said out loud with both numbers,
	 * because a caller comparing two captures by size would otherwise be comparing the box.
	 */
	inline constexpr const TCHAR* ThumbnailSizeAdjusted = TEXT("thumbnail_size_adjusted");
	/**
	 * A preview subject's Nanite meshes were drawn from their FALLBACK mesh. User-reported 2026-09-12.
	 *
	 * Not a degradation notice, which is how it reads at first: the alternative is not full Nanite
	 * detail, it is ROOT PAGES — a preview scene is built, drawn once and destroyed inside one call,
	 * and Nanite geometry arrives over several of the engine's own frames, so nothing a capture
	 * contains is ever streamed in. Said out loud with the component count because a caller
	 * comparing a preview against the same asset in a LEVEL is comparing two representations, and
	 * only one of them is the one the game ships. `DrawNaniteFromFallback` carries the measurement.
	 */
	inline constexpr const TCHAR* NaniteFallback = TEXT("nanite_fallback");
	/**
	 * A Nanite preview was rendered from a transient raster mesh rebuilt from its highest-detail
	 * editor source. This is the faithful one-call route: material slots and UVs are preserved while
	 * the asset, editor world, and global Nanite settings remain untouched.
	 */
	inline constexpr const TCHAR* NaniteSourceGeometry = TEXT("nanite_source_geometry");
	/** Orthographic rasterization inflates sub-pixel strand geometry in world space. */
	inline constexpr const TCHAR* ThinGeometryOrthographic = TEXT("thin_geometry_orthographic");
	/**
	 * R1 (RENDER-READY1, GC-GAP-REPORT-1117-PLAN.md), 17 Nov: after a kept frame a mesh slot's material still had no
	 * complete shader map, so that slot drew the engine's default material (a grey grid) and not its own. Names the
	 * component, the slot and the material; readiness is then not `ready`.
	 */
	inline constexpr const TCHAR* FallbackMaterial = TEXT("fallback_material");
	/**
	 * R1 (G120): after the Blueprint's BeginPlay, the body an `animation:` clip was put on no longer plays that clip (its
	 * own Anim Blueprint, another clip or mode took it back). Said by component, animation mode and anim class: the
	 * picture shows the Blueprint's own animation.
	 */
	inline constexpr const TCHAR* AnimationNotPlayed = TEXT("animation_not_played");
	/**
	 * R1 (G120): an `animation:` render over a span whose played body moved no bone, under half a centimetre from the
	 * view's first frame (the pelvis or the root's first child, the feet, a followed bone). A WARNING AND NEVER A
	 * REFUSAL: a standing clip is legal. Names the component, its animation mode and anim class.
	 */
	inline constexpr const TCHAR* AnimationNoMotion = TEXT("animation_no_motion");

	/** `token: message`, which is what everything in this feature emits. */
	GRAPHCODE_API FString Line(const TCHAR* Token, const FString& Message);
	/** Whether a token appears in a list of such lines. The tokens are the contract. */
	GRAPHCODE_API bool Has(const TArray<FString>& Lines, const TCHAR* Token);
}

/**
 * Named views — `GC-VISION-DESIGN.md` §5.1, "aliases only".
 *
 * They lower to ordinary azimuth/elevation and the result returns the LOWERED numbers, so a caller
 * who asked for `top` gets back something they can adjust by ten degrees. Six names, exactly the
 * six the design lists; a seventh would be a second camera system arriving one alias at a time.
 */
namespace GraphCodeRenderPreset
{
	/** True if `Name` is one of the six, with its angles. Case-insensitive. */
	GRAPHCODE_API bool Lower(const FString& Name, double& OutAzimuth, double& OutElevation);
	/** The six, comma-separated, for a refusal that says what it would have accepted. */
	GRAPHCODE_API FString Known();
}

/**
 * WHAT THE RENDERER DOES WITH THE SCENE — `view_mode:`, user-asked 2026-08-24.
 *
 * **THIS IS §6.2'S RULING HONOURED, not a new idea beside it.** The design has said since it was
 * written that *`unlit` is a second profile only if it is implemented as an explicit profile, never
 * as an undocumented show-flag difference*. Every word of that survives. What changed is that the
 * thing being named turned out not to be a profile.
 *
 * **IT IS A VIEW FIELD AND NOT A PROFILE NAME, because it is orthogonal to what a profile decides.**
 * `neutral` fixes the world a picture is taken in — the lighting rig, the background, the margin,
 * the field of view — and this fixes what the renderer draws of that world. Folding the two together
 * multiplies out: seven modes times every future profile, each needing a name, and still no way to
 * say *these two views of one subject, one lit and one wireframe* in a single call. It sits on the
 * view beside `Cut` for the reason `Cut` is there — it says what this picture is a picture OF.
 *
 * **EVERY MODE IS THE ENGINE'S OWN `EViewModeIndex`, through `ApplyViewMode` and
 * `EngineShowFlagOverride`** — the two calls `FEditorViewportClient` itself makes — and never a
 * hand-picked set of show flags. That is what makes `unlit` here and Unlit in the viewport's own menu
 * the same picture, and it is why `unlit` is actually unlit at all: `ApplyViewMode` alone does NOT
 * clear `Lighting` for `VMI_Unlit` (`ShowFlags.cpp:394-432`), `EngineShowFlagOverride` does
 * (`:548-573`), and a hand-rolled `SetLighting(false)` would have been a third thing that merely
 * looks like it. IT IS ALSO NOT SUFFICIENT — see `ApplyViewModeMaterialOverrides`, and the list of
 * withdrawals below, for what a family alone does not carry.
 *
 * **FOUR OF THE ENGINE'S MODES ARE DELIBERATELY ABSENT, on a measurement of its source rather than a
 * taste**: `VMI_Clay`, `VMI_Zebra`, `VMI_FrontBackFace` and `VMI_RandomColor` make
 * `EngineShowFlagOverride` write the GLOBAL `r.Nanite` console variable, restoring it only on a later
 * call that names some other mode (`ShowFlags.cpp:579-607`). A capture that changes how the user's
 * own editor renders is the one thing this whole feature exists not to be. The debug-view-shader
 * family — shader complexity, quad overdraw, the texture-accuracy modes — is absent for a weaker
 * reason: it needs a shader path no offscreen family here has been measured to take. Both sets can
 * arrive the day a probe measures one, and a name added later is additive.
 *
 * **THREE MORE WERE WRITTEN, RENDERED, LOOKED AT, AND TAKEN BACK OUT.** All three are in the
 * viewport's own menu, all three set their show flags correctly from here, and none of them survives
 * an OFFSCREEN family — which is the difference between a mode and a mode-shaped hole. Recorded
 * rather than quietly omitted, because each one is a day somebody else would otherwise spend:
 *
 * - **`VMI_Lit_Wireframe` ASSERTS.** `Assertion failed: ViewFamilyData [MeshEdgesRendering.cpp:431]`
 *   — `ComposeMeshEdges` finding no extension data where `RenderMeshEdges` should have left some.
 *   The pre-pass that creates it hangs off a scene view extension registered under
 *   `FApp::CanEverRender() && !FApp::IsUnattended()` (`Renderer.cpp:125`). **So mesh edges exist for
 *   an attended editor and are a hard crash in every `-unattended` one** — the automation gate, every
 *   commandlet, and the headless `UnrealEditor-Cmd` this feature was designed to run in
 *   (`GC-VISION-HANDOFF.md` phase 0).
 * - **`VMI_Wireframe` RENDERS A SOLID WHITE PLATE.** No edges, no subject, no background: 512×512 of
 *   white, looked at rather than inferred from a digest. The engine's own note says the
 *   editor-primitive composition pass *is also used when rendering VMI_WIREFRAME in order to use
 *   MSAA* (`PostProcessCompositeEditorPrimitives.cpp:257`), and a game-flag family does not run that
 *   pass. Switching `CompositeEditorPrimitives` on was drafted and never measured — so what is known
 *   here is the white plate, and nothing at all about the fix.
 * - **`VMI_ReflectionOverride` RENDERS BLACK** in the `neutral` preview world, which is correct and
 *   useless: it makes every surface a mirror, and that world has its sky sphere hidden and no
 *   reflection capture, so there is nothing to be a mirror OF. It could be a real mode for a LEVEL
 *   capture, where there is a sky — that has NOT been measured, and shipping it on the strength of
 *   the argument would be advertising a picture nobody has taken.
 *
 * **THE PROBE FOUND ALL OF THIS, and only because it compares digests PAIRWISE.** `detail_lighting`
 * and `lighting_only` first came back BYTE-IDENTICAL to `lit`: every other check passed, and the
 * pictures were the lit one. `ApplyViewModeMaterialOverrides` carries what was missing.
 */
enum class EGraphCodeRenderViewMode : uint8
{
	/** The default, and the only picture this tool has ever taken. `VMI_Lit`. */
	Lit,
	/** Base colour and nothing else — no lights, no shadows, no fog. `VMI_Unlit`. */
	Unlit,
	/** Lighting on a neutral material — the shape, without the albedo. `VMI_Lit_DetailLighting`. */
	DetailLighting,
	/** The lighting alone, every material's colour taken out. `VMI_LightingOnly`. */
	LightingOnly,
	/** Lit, the subject's surface a checker on its first UV set (user 2026-10-18: "Rendering with checkerboard will be better?"): skin that stretches bends
	 * the squares, and skin that tears breaks them. Not an engine view mode: a transient material put on the subject's mesh for the capture alone. */
	Checker,
};

namespace GraphCodeRenderViewMode
{
	/** True if `Name` is one of the four. Case-insensitive; `detail_lighting` also spells `detail lighting`. */
	GRAPHCODE_API bool Lower(const FString& Name, EGraphCodeRenderViewMode& OutMode);
	/** The token a mode is spelled with — what a manifest, a result and a refusal all write. */
	GRAPHCODE_API const TCHAR* Name(EGraphCodeRenderViewMode Mode);
	/** The four, comma-separated, for a refusal that says what it would have accepted. */
	GRAPHCODE_API FString Known();
	/** The material `checker` dresses a subject in (null outside the editor); exposed so a test can read its usage flags. */
	GRAPHCODE_API class UMaterial* CheckerMaterial();
}

/**
 * `highlights:` — the TYPES a picture marks, `GC-VISION-DESIGN.md` §4.3b, user-ruled 2026-08-17.
 *
 * **TYPES AND NOT ACTOR NAMES**, which is the user's own word: *"types of volume, like nav mesh or
 * niagara."* That is the one thing about this parameter that is not `ignore:`'s shape, and the
 * difference is the point — a caller marking navigation does not know, and should not have to know,
 * which actors carry it.
 *
 * ~~**EXACTLY THE TWO THE USER NAMED, and a third would be invention.**~~ **SIX SINCE 2026-08-25,
 * AND THAT IS THE MECHANISM WORKING RATHER THAN BEING OVERRIDDEN.** The vocabulary was left short on
 * purpose and grows when the user names more — *"should improve to add ability to show many volume
 * right? like collision etc."* and *"should also be able to render invisible things, like camera"*
 * are the two rulings that added the last four. An unknown type still refuses and says what it would
 * have accepted, so growing the list is additive and never silently reinterprets an old request.
 */
namespace GraphCodeRenderHighlight
{
	inline constexpr const TCHAR* Regions = TEXT("regions");
	inline constexpr const TCHAR* Splines = TEXT("splines");
	/** The navmesh SURFACE — what navigation built, which is not the volume somebody drew. */
	inline constexpr const TCHAR* NavMesh = TEXT("navmesh");
	/** Niagara emitters, marked for PLACEMENT rather than appearance (the torch argument). */
	inline constexpr const TCHAR* Niagara = TEXT("niagara");
	/** SIMPLE collision shapes, per primitive. USER-ASKED 2026-08-25; §12.2. */
	inline constexpr const TCHAR* Collision = TEXT("collision");
	/**
	 * Every `AVolume` brush — trigger, physics, post-process, blocking, nav bounds.
	 *
	 * **NOT THE SAME QUESTION AS `collision` and never merged with it** (§12.2). A trigger volume
	 * with its collision profile set to overlap appears under both, and that is correct: one answers
	 * *what would this block or overlap*, the other *where is the room somebody drew*. A
	 * PostProcessVolume answers only the second, because it has no collision at all.
	 */
	inline constexpr const TCHAR* Volumes = TEXT("volumes");
	/**
	 * Every `UCameraComponent` — its editor body mesh, plus the frustum it is pointing.
	 *
	 * **A CAMERA IS NOT A BILLBOARD, measured 2026-08-25** (§12.3c). It carries a real `UStaticMesh`
	 * on a proxy component and a `UDrawFrustumComponent`, so it is triangles and takes `collision`'s
	 * road rather than the sprite one below.
	 */
	inline constexpr const TCHAR* Camera = TEXT("camera");
	/**
	 * Every `UBillboardComponent` — a light, a player start, an audio actor, a reflection capture,
	 * a note. **USER-RULED 2026-08-25 to draw UNREAL'S OWN ICON**: *"if unreal real icon is
	 * available then use unreal icon, why not"* (§12.3d).
	 *
	 * This population is DOUBLY invisible: a visualisation component is dropped from `actors { }`,
	 * `components { }` and every `.gcp` by [[decoration-is-not-authored-state]], and it draws no
	 * pixels in a capture either. Until this token existed, the only thing that named a
	 * PlayerStart was the frame legend, with a rectangle and nothing in it.
	 */
	inline constexpr const TCHAR* Billboard = TEXT("billboard");
	/** The pose view: an animation's or skeletal mesh's bones, where each points (Docs/GC-MOVE-SEE-AND-POINT-PLAN.md §3.7). */
	inline constexpr const TCHAR* Bones = TEXT("bones");
	/** The clip trajectory: each selected bone's path over the whole clip, a dot per sample (plan §3.9). */
	inline constexpr const TCHAR* Trails = TEXT("trails");
	/**
	 * A face's named points, drawn and labelled ON the face by the subject that knows them (a MetaHuman:
	 * its fit points and Move handles), so an author sees what each name means before measuring the
	 * matching spot on a reference. USER-ASKED 2026-10-19. A subject that knows no such points refuses it.
	 */
	inline constexpr const TCHAR* Landmarks = TEXT("landmarks");

	/** True if `Type` is one of the known types. Case-insensitive; whitespace trimmed. */
	GRAPHCODE_API bool IsKnown(const FString& Type);
	/** The canonical spelling of a known type, for echoing back what a request meant. */
	GRAPHCODE_API FString Canonical(const FString& Type);
	/** The known types, comma-separated, for a refusal that says what it would have accepted. */
	GRAPHCODE_API FString Known();
}

/**
 * Where the camera aims. `offset` moves the AIM POINT, not the box (`GC-VISION-DESIGN.md` §5.3),
 * so `zoom:0` still fits the whole original box around the shifted aim — which means an offset
 * pushes the camera OUTWARD rather than cropping something silently.
 */
struct GRAPHCODE_API FGraphCodeRenderOffset
{
	FVector Value = FVector::ZeroVector;

	/**
	 * true multiplies each component by the corresponding local HALF-EXTENT, so `[0,0,1]` aims at
	 * the top face of any subject and the same number behaves on a prop and a building. false is
	 * the explicit `{"value":[0,0,20],"unit":"cm"}` spelling, for when centimetres are meant.
	 */
	bool bBoxNormalised = true;

	bool IsZero() const { return Value.IsNearlyZero(); }
};

struct GRAPHCODE_API FGraphCodeRenderAnchor
{
	EGraphCodeRenderAnchorMode Mode = EGraphCodeRenderAnchorMode::Auto;

	/** `Point` only. */
	FVector Point = FVector::ZeroVector;
	/** `Point`, and `Component` with a `Bone`; required for both: `zoom:0` cannot fit a box whose size was never
	 * supplied, and a bone or socket is a point. */
	double Radius = 0.0;

	/** `Component`, `Actor`, `ActorSet` — carried so the refusal can name what was asked for. `Component` may leave it
	 * empty when it names a `Bone`: then the one component of the preview that has that bone or socket. */
	TArray<FString> Names;

	/** `Component` only: a bone or socket on it to aim at (MOVE-FIX1 (9), 2026-10-05: a trigger finger could not be
	 * looked at, because a preview recentres its actor and no point a caller knew in the character's own space landed
	 * on it). Empty: the component's own bounds. Framed at the view's first moment; over a span the camera follows
	 * it (A6, 2026-10-21, `CameraFollowed`). */
	FString Bone;
};

/**
 * The ordinary camera: three subject-relative numbers and an optional offset.
 *
 * Every field is an OVERRIDE on the automatic choice, not a member of an all-fields-required
 * struct. An unset `Azimuth` with a set `Elevation` means "pick the best azimuth AT this
 * elevation" — the lattice is restricted, not abandoned, which is the only reading of §5's
 * "missing azimuth or elevation keeps the internally selected value" that stays useful.
 */
struct GRAPHCODE_API FGraphCodeRenderOrbit
{
	/**
	 * One of `GraphCodeRenderPreset::Known()`. It fills whichever of the two angles below the
	 * caller did not give, so `{preset:"top", zoom:0.5}` means what it looks like, and an explicit
	 * angle beside a preset still wins — every field here is an override.
	 */
	TOptional<FString> Preset;

	TOptional<double> Azimuth;
	TOptional<double> Elevation;

	/** Logarithmic in projected size: `distance = fit / 2^zoom`. Omitted is 0, which is the fit. */
	double Zoom = 0.0;

	/**
	 * STAND AT THE AIM POINT AND LOOK OUT — the interior view, user-asked 2026-08-24 as *"zoom inside
	 * to see the inner"*.
	 *
	 * **THE ORBIT ALREADY REACHED THE INSIDE AND COULD NOT LOOK AROUND WHEN IT GOT THERE.** `zoom:8`
	 * divides the fitted distance by 256 and `camera_inside_bounds` has warned about the result since
	 * the solver shipped — so the camera has always been able to stand in the middle of a tower. It
	 * then aimed at the aim point, eleven centimetres away, and photographed the empty air at the
	 * middle of the room. Every wall was behind it. What was missing was never the position.
	 *
	 * **SO IT IS ONE BOOLEAN AND NOT A FOURTH CAMERA FORM.** Everything an orbit already carries keeps
	 * its meaning: `offset` puts the eye somewhere in the box in half-extents, so `[0,0,0.5]` stands
	 * halfway up; `anchor` chooses which box; `basis` chooses whose axes. Only two things change, and
	 * both are inversions of the same fact — the eye is AT the aim rather than a fitted distance from
	 * it, and it faces OUT rather than in.
	 *
	 * **THE INVARIANT THAT FIXES WHICH WAY *OUT* IS: THE SAME AZIMUTH SHOWS YOU THE SAME FACE.**
	 * `azimuth:0` frames the subject's own front from outside, and `azimuth:0, inside:true` looks at
	 * that same front from within. Pointing the camera along the orbit direction instead — down the ray
	 * from the aim towards where the camera would have stood — reads as the tidier arithmetic and makes
	 * one number mean two opposite pictures.
	 *
	 * **AND `zoom` STILL MEANS PROJECTED SIZE**; there is simply no distance left to spend it on, so it
	 * moves the lens. `FGraphCodeRenderRequest::InsideFovDegrees` is where `zoom:0` starts, and it is
	 * NOT the profile's 35: a 35 mm-equivalent lens standing in a room photographs a patch of wall.
	 * An explicit `fov` still wins, exactly as it does everywhere else.
	 *
	 * **WHAT IT CANNOT DO IS SEE INSIDE A SOLID.** A closed mesh's faces point outward, so from within
	 * it there is nothing facing the eye and the picture is empty — correctly. `inside` is for what has
	 * an inside: a room, a shelled tower, a level. To look into something solid, `cut:` it.
	 */
	bool bInside = false;

	FGraphCodeRenderOffset Offset;
	FGraphCodeRenderAnchor Anchor;

	/** Unset takes the basis the adapter built the subject in, and the result reports which. */
	TOptional<EGraphCodeRenderBasis> Basis;

	/**
	 * Unset is perspective. Orthographic needs no width here — `zoom:0` fits one to the box and
	 * `zoom` scales it, so the whole grammar keeps working and only the meaning of "fit" changes.
	 */
	TOptional<EGraphCodeRenderProjection> Projection;
};

/**
 * The escape hatch — `GC-VISION-DESIGN.md` §5.6. Deliberately verbose, deliberately not the
 * adjustment path. `Location` pairs with exactly one of `Rotation` or `LookAt`.
 */
struct GRAPHCODE_API FGraphCodeRenderAbsoluteCamera
{
	FVector Location = FVector::ZeroVector;
	TOptional<FRotator> Rotation;
	TOptional<FVector> LookAt;

	/** Unset takes the profile's FOV, so an absolute shot does not silently change perspective. */
	TOptional<double> Fov;

	EGraphCodeRenderProjection Projection = EGraphCodeRenderProjection::Perspective;
	/** Orthographic only, and REQUIRED there: an absolute camera fits nothing to derive it from. */
	double OrthoWidth = 0.0;
};

/**
 * Temporary adjustments to a named camera — `GC-VISION-DESIGN.md` §5.7.
 *
 * THE TRIPOD CONVENTION, stated because it is the only part a caller can be surprised by. The three
 * distances move the camera along the axes it had BEFORE it was turned — you push the tripod, then
 * you pan it — and the three angles are added to its rotation as Euler degrees, which in Unreal
 * means yaw about world up, pitch about the yawed right axis and roll about the resulting forward.
 * That is what a viewport does when you drag it, and it means `yaw:5` on a level camera never tilts
 * the horizon.
 *
 * Every one of these is applied to a COPY. Nothing here writes the actor, and the level capture's
 * before/after check would fail the whole render if it did.
 */
struct GRAPHCODE_API FGraphCodeRenderCameraAdjust
{
	/** Camera-local right, in centimetres. */
	double Truck = 0.0;
	/** Camera-local up, in centimetres. */
	double Pedestal = 0.0;
	/** Camera-local forward, in centimetres. */
	double Dolly = 0.0;

	double Yaw = 0.0;
	double Pitch = 0.0;
	double Roll = 0.0;

	bool IsZero() const
	{
		return Truck == 0.0 && Pedestal == 0.0 && Dolly == 0.0
			&& Yaw == 0.0 && Pitch == 0.0 && Roll == 0.0;
	}
};

/**
 * A camera somebody already placed — §5.7, and a level's third camera form.
 *
 * The handle is the actor's INTERNAL name, matching level `.gc` and `.gcp`. The Outliner label is
 * not identity: two actors may carry one label, and `GetActorLabel` is editor-only.
 */
struct GRAPHCODE_API FGraphCodeRenderNamedCamera
{
	FString ActorName;
	FGraphCodeRenderCameraAdjust Adjust;
};

/**
 * The lens and the grade, separately from where the camera stands — `GC-VISION-DESIGN.md` §5.9.
 *
 * THE POINT IS THE SPLIT. A named camera actor answers "what does that camera see"; this answers
 * "what would this shot look like THROUGH that camera" — the same focal length, exposure and white
 * balance, from an angle the caller chose. Before it existed, borrowing a project's look meant
 * accepting its position too, and the two have nothing to do with each other.
 *
 * Precedence, stated once because everything here is an override: the profile supplies the default,
 * `FromActor` replaces what it carries, and an explicit field below wins over both. Every field is
 * optional and an omitted one changes nothing — a request that sets none of this is byte-identical
 * to one that has no settings block at all.
 */
struct GRAPHCODE_API FGraphCodeRenderSettings
{
	/**
	 * Take the lens and post-process from this camera actor and NOT its pose. Level only: there is
	 * no camera actor in an isolated preview world to borrow from.
	 */
	TOptional<FString> FromActor;

	/** Horizontal, degrees. Applies to every camera form, which the profile's FOV could not. */
	TOptional<double> Fov;
	TOptional<EGraphCodeRenderProjection> Projection;
	/** Orthographic only. On an orbit this overrides the fit, the way an absolute camera does. */
	TOptional<double> OrthoWidth;

	/** Exposure compensation in STOPS: +1 is one stop brighter. The editor's own EV slider. */
	TOptional<double> Exposure;
	/** White balance in kelvin. Unset leaves the scene's own; 6500 is neutral daylight. */
	TOptional<double> Temperature;
	/** White balance tint, green/magenta, in [-1, 1]. */
	TOptional<double> Tint;

	bool IsSet() const
	{
		return FromActor.IsSet() || Fov.IsSet() || Projection.IsSet() || OrthoWidth.IsSet()
			|| Exposure.IsSet() || Temperature.IsSet() || Tint.IsSet();
	}

	/** Whether anything here has to reach the post-process chain rather than the camera. */
	bool TouchesGrade() const
	{
		return Exposure.IsSet() || Temperature.IsSet() || Tint.IsSet();
	}

	/** UE's own editor ranges, so a refusal matches what the property panel would accept. */
	static constexpr double MinTemperature = 1500.0;
	static constexpr double MaxTemperature = 15000.0;
	static constexpr double MaxAbsTint = 1.0;
	static constexpr double MaxAbsExposure = 15.0;
};

/**
 * Fixed lighting, background, exposure, show flags and FOV, named as one thing.
 *
 * A profile is the only place a rendering constant is allowed to live. The design's rule is that
 * `unlit` may exist as an explicit profile and must never exist as an undocumented show-flag
 * difference; the same rule is why FOV is here and not a loose request field.
 */
struct GRAPHCODE_API FGraphCodeRenderProfile
{
	FString Name = TEXT("neutral");

	/** HORIZONTAL field of view, matching Unreal. The vertical follows from the view's aspect. */
	double FovDegrees = 35.0;

	/**
	 * The fraction of each frame half-axis kept empty at `zoom:0`. 0.05 leaves a 5% border, so a
	 * fitted subject does not touch the edge of its own picture.
	 */
	double Margin = 0.05;

	static FGraphCodeRenderProfile Neutral() { return FGraphCodeRenderProfile(); }
	/** Known names, so an unknown one refuses instead of quietly rendering the default. */
	static bool IsKnownName(const FString& Name);
};

/**
 * How a cut plane was STATED — `GC-SPATIAL-AUTHORING-DESIGN.md` §21.13b.
 *
 * Three forms and not three features: they are the three amounts of information a plane needs, and
 * the COUNT OF POINTS is what chooses the convention that closes the rest.
 */
enum class EGraphCodeRenderCutForm : uint8
{
	/** `z=90`. Fully determined: the axis is the normal and the number is the offset. */
	Axis,
	/** Two points. The line through them, in the plane PERPENDICULAR TO THE VIEW. */
	TwoPoints,
	/**
	 * Three points. The plane through all three — and when they are COLLINEAR, the VERTICAL plane
	 * through that line, which is the only way to say *vertical, whatever the camera is doing*.
	 * Collinear is a legal input here and not a degenerate one (§21.13b, user-ruled).
	 */
	ThreePoints,
};

/**
 * The SECTION CUT, as asked for — §21.13a. *A top-down picture of a building is a picture of its
 * roof*, so every storey but the top one is unauthorable from vision until this exists.
 *
 * IT IS A PLACE, NEVER A CLIP DISTANCE. `z=90` is ninety centimetres up the world's own z axis, and
 * the solver turns it into the plane the projection starts at. A caller typing a near-plane distance
 * would be typing a number about the camera, which is the arithmetic §1 exists to remove.
 *
 * WHICH HALF GOES IS NOT A CONVENTION AND IS NEVER STATED: everything between the camera and the
 * plane is discarded, because a cut belongs to a view and a view has an eye. So the sign of a stated
 * axis or the winding of three points cannot mean anything, and nothing here carries a side.
 */
struct GRAPHCODE_API FGraphCodeRenderCut
{
	EGraphCodeRenderCutForm Form = EGraphCodeRenderCutForm::Axis;

	/** `Axis` only: X, Y or Z. */
	EAxis::Type Axis = EAxis::Z;
	/** `Axis` only: where along it, in centimetres. */
	double Value = 0.0;

	/** `TwoPoints`/`ThreePoints`: world-space, in centimetres. */
	TArray<FVector> Points;

	/**
	 * How far PAST the cut the picture sees, measured perpendicular to the cut — so a stairwell
	 * opening does not show the storey underneath through it. Unset sees to the far plane.
	 *
	 * NOT `depth:`, ruled before it was built: Z3's frame legend already spends that word on every
	 * actor's view-space distance, and one request and its answer would then carry two unrelated
	 * meanings of one word.
	 */
	TOptional<double> Below;
};

/**
 * A cut RESOLVED against a view: the world plane the picture starts at, and the slab it keeps.
 *
 * On the camera rather than beside it, because it has to reach the projection matrix — and through
 * that matrix, the deprojection. The picture and the pick then cannot disagree about where the
 * storey starts, which is the whole claim §21.13a makes for a cut over an exclude list.
 */
struct GRAPHCODE_API FGraphCodeRenderCutPlane
{
	/**
	 * Unit normal of the plane `dot(Normal, p) = Offset`, in world space.
	 *
	 * ITS SIGN CARRIES NOTHING. The kept half is the one the camera is NOT in, decided when the plane
	 * reaches view space — so a caller cannot state a side and a reader must not read one.
	 */
	FVector Normal = FVector::UpVector;
	double Offset = 0.0;

	/** The kept slab's thickness, perpendicular to the plane. `below`, or the camera's own far. */
	double Depth = 0.0;
};

/**
 * Which container `video:` writes, when it writes one at all — `GC-VISION-DESIGN.md` §11.6.
 *
 * **A WORD RATHER THAN A SECOND BOOLEAN**, because the two formats are alternatives and never both:
 * a request that set `video:true` and `mp4:true` would have to mean something, and every meaning is
 * worse than being unable to say it. `video:true` is still APNG, so the word is additive and no
 * existing call changes what it writes.
 *
 * The formats are NOT interchangeable and the choice is real:
 *
 * - `Apng` is lossless, is a valid PNG so it opens as a still anywhere, needs no encoder at all, and
 *   works on every platform. It is large, and nothing will scrub it.
 * - `Mp4` is H.264 in a container a person can scrub, seek and drop into anything. It is lossy, and
 *   it is WINDOWS ONLY — it is the system's own Media Foundation encoder, which is what makes it
 *   free of any dependency to ship, and which is also why it does not exist off Windows.
 *
 * An `Mp4` asked for where it cannot be written FALLS BACK TO `Apng` with a warning rather than
 * failing the capture. The frames were the expensive part and they are already correct; throwing
 * them away over the container would be the wrong trade, and it is the same call §11 makes when a
 * contact sheet fails to assemble.
 */
enum class EGraphCodeRenderVideoFormat : uint8
{
	/** No animation. The default: `video` is for a person, and most captures are not. */
	None,
	/** `<view>.anim.png`. */
	Apng,
	/** `<view>.mp4`. */
	Mp4,
};

/**
 * WHEN this picture is of — `GC-VISION-DESIGN.md` §11, user-asked 2026-08-18.
 *
 * **A CAPTURE HAS ALWAYS HAD A TIME AND HAS NEVER SAID WHAT IT WAS.** The view family was built with
 * `FGameTime::GetTimeSinceAppStart()`, so a panning material, a world-position-offset wobble or
 * anything else reading the engine clock rendered at whatever moment the call happened to land on.
 * Two captures of one subject were therefore never comparable, and nothing in the result said so.
 * That is the same non-determinism §4.3 records for effects, one level lower down and affecting
 * every subject rather than the animated ones.
 *
 * **TIME IS A DIMENSION INSIDE A VIEW AND NOT A SECOND VIEW**, which is the one structural decision
 * here. Frames of an animation share a camera exactly — that is what makes them frames rather than
 * pictures — so spelling them as views would make the caller repeat the camera N times, would spend
 * `MaxViews` (eight) on two seconds of walk cycle, and would let two "frames" disagree about where
 * they were shot from. One view, N moments, one camera.
 *
 * **THE SPAN IS FOUR OPTIONAL SCALARS AND NOT A BLOCK.** `at` alone is a moment; `at`+`to` is a span;
 * `fps` or `count` says how densely it is sampled. Nothing here is required and a view that states
 * none of it renders exactly as it did before this existed.
 */
struct GRAPHCODE_API FGraphCodeRenderTime
{
	/** The moment, in SECONDS of subject time. `0` is the start of whatever the subject animates. */
	double At = 0.0;

	/** The last moment, inclusive. Unset means `At` is the only one. */
	TOptional<double> To;

	/**
	 * Frames per second across the span. Mutually exclusive with `Count`.
	 *
	 * **THE RATE IS KEPT EXACTLY AND THE SPAN IS WHAT GIVES**, which is a decision and not a rounding
	 * detail: a caller who said 24 fps is describing playback, and satisfying `to` by shipping 23.8 fps
	 * instead would make the finished animation play at the wrong speed for a reason nothing reports.
	 * So a span that is not a whole number of frames stops at the last whole one and the result says
	 * which moment it actually ended at.
	 */
	TOptional<double> Fps;

	/**
	 * Total frames across the span, both ends inclusive. Mutually exclusive with `Fps`.
	 *
	 * The other way round from `Fps`: here the ENDS are exact and the rate is whatever falls out, which
	 * is what a caller wants when the span is the point (*eight pictures across this door opening*) and
	 * playback speed is not.
	 */
	TOptional<int32> Count;

	/**
	 * Also write the frames tiled into ONE image — the contact sheet.
	 *
	 * **DEFAULTED ON FOR A SPAN, AND THAT IS WHAT MAKES THIS FEATURE USABLE IN ONE CALL.** A result
	 * returns inline image content only when it holds exactly one picture (§3), so a twenty-four frame
	 * request without a sheet hands its caller twenty-four paths and no pixels — and the caller of this
	 * tool is usually an agent that reads images, cannot play a video, and pays a round trip per file.
	 * The sheet is the one image that shows the motion, so it is what comes back inline.
	 *
	 * Unset means *on when there is more than one frame*. `false` turns it off; `true` on a single
	 * frame is honoured and produces a one-cell sheet, because refusing it would be a rule to remember
	 * for no gain.
	 */
	TOptional<bool> bSheet;

	/**
	 * Also write an ANIMATION of the frames — the thing a person can actually play.
	 *
	 * `EGraphCodeRenderVideoFormat` carries which container and why there are two. `video:true` on
	 * the wire still means `Apng`, which is the no-dependency, lossless, plays-anywhere one;
	 * `video:"mp4"` asks for H.264 through the system encoder, which is what a person who wants to
	 * scrub actually wants and which exists only on Windows.
	 *
	 * Off by default: it is for a person, and most captures are not.
	 */
	EGraphCodeRenderVideoFormat Video = EGraphCodeRenderVideoFormat::None;

	bool IsVideo() const { return Video != EGraphCodeRenderVideoFormat::None; }

	/** Opt in to BeginPlay and world ticks for an isolated Actor Blueprint. Requires at. */
	bool bSimulate = false;
	/** Native collision and rigid-body physics in that Blueprint world. Requires simulate. */
	bool bPhysics = false;
	static constexpr double MaxSimulationSeconds = 60.0;
	static constexpr double SimulationStepSeconds = 1.0 / 60.0;

	/**
	 * Whether the request stated a time at all — set by whoever parsed it.
	 *
	 * It exists because `At` DEFAULTS TO A LEGAL VALUE. Every other optional thing on a view is a
	 * `TOptional` that answers this by itself, but zero is a perfectly good moment to ask for, so
	 * *unstated* and *`at:0`* are the same struct and have to be told apart by a flag. They are not the
	 * same request: the first keeps the pre-§11 wall clock and the second pins the subject to zero.
	 */
	bool bStated = false;

	bool IsSet() const { return bStated; }

	/** A span longer than an hour is a request nobody meant to type. */
	static constexpr double MaxAbsSeconds = 3600.0;
	static constexpr double MinFps = 0.01;
	static constexpr double MaxFps = 240.0;
	/** Five seconds at 24 fps. A stated ceiling, so the machine is not what enforces the limit. */
	static constexpr int32 MaxFrames = 120;
	/** A span with neither `fps` nor `count`: eight, the same number of pictures `MaxViews` allows. */
	static constexpr int32 DefaultCount = 8;
};

/**
 * The moments one view resolved to, and how they were arrived at.
 *
 * ANSWERED BACK RATHER THAN INFERRED FROM THE REQUEST, for the reason `Ignored` and `Highlighted`
 * are: a caller who asked for 12 fps across 2.05 seconds and got 25 frames ending at 2.0 cannot tell
 * that from the request, and two captures are only comparable if each says which moment it took.
 */
struct GRAPHCODE_API FGraphCodeRenderTimeline
{
	/** Seconds, ascending, always at least one entry. A view with no time resolves to `{0}`. */
	TArray<double> Moments;

	/** The effective rate. Zero for a single frame, which has no rate. */
	double Fps = 0.0;

	/** Whether the request named a time. False means nothing about the capture changed. */
	bool bStated = false;

	bool bSheet = false;

	/**
	 * The container that will actually be written, which is NOT always the one asked for: an `Mp4`
	 * requested where Media Foundation cannot be reached resolves to `Apng` and says so in a warning.
	 * Resolved here, before a single frame renders, so the PLAN names the right file — a downgrade
	 * decided at assembly time would have written `.mp4` into the manifest and an APNG into it.
	 */
	EGraphCodeRenderVideoFormat Video = EGraphCodeRenderVideoFormat::None;
	bool bSimulate = false;
	bool bPhysics = false;

	int32 Num() const { return Moments.Num(); }
	bool IsMultiFrame() const { return Moments.Num() > 1; }
	bool IsVideo() const { return Video != EGraphCodeRenderVideoFormat::None; }
};

/** One requested picture. Exactly one camera form, or none of them for the automatic default. */
struct GRAPHCODE_API FGraphCodeRenderView
{
	/** A safe path component: it becomes a filename. Empty means the single default view. */
	FString Name = TEXT("main");

	int32 Width = 1024;
	int32 Height = 1024;

	TOptional<FGraphCodeRenderOrbit> Orbit;
	TOptional<FGraphCodeRenderAbsoluteCamera> Absolute;
	/** §5.7. Level only: there is no camera actor in an isolated preview world to name. */
	TOptional<FGraphCodeRenderNamedCamera> Named;

	/** The lens and grade, which are the same question whichever of the three forms is above. */
	FGraphCodeRenderSettings Settings;

	/**
	 * §21.13a's section cut. On the VIEW and not in `Settings`, because it is neither the lens nor
	 * the grade: it says what this picture is a picture OF, which is the same question the camera
	 * form answers and belongs beside it.
	 */
	TOptional<FGraphCodeRenderCut> Cut;

	/**
	 * What the renderer draws of the scene — `EGraphCodeRenderViewMode` carries the whole argument.
	 *
	 * Beside `Cut` and inheriting across views exactly as it does: *all of these in wireframe* is one
	 * sentence, and a view that wants its own says so.
	 */
	EGraphCodeRenderViewMode ViewMode = EGraphCodeRenderViewMode::Lit;

	/**
	 * R8's second half — actors this picture LEAVES OUT, by internal name. Level renders only.
	 *
	 * **IT EXISTS TO CLOSE AN ASYMMETRY, not to add a capability**: `screen_to_world` has taken an
	 * `IgnoreActors` list since it shipped, and the render had no actor filter of any kind — so what
	 * a trace stepped over was still in the picture, and a pick could answer past something the
	 * caller could plainly see. Beside the cut, because it answers the cut's question about the cases
	 * a PLANE cannot reach: a suspended ceiling belonging to the storey underneath, a fog volume, an
	 * overhanging roof (§21.13a, §21.13d).
	 *
	 * **A HIDDEN ACTOR IS HIDDEN EVERYWHERE THIS CAPTURE ANSWERS** — out of the pixels, out of the
	 * frame legend, out of the occlusion traces that legend measures, and out of a later
	 * `screen_to_world` through the same capture, which reads the list back off the manifest. A
	 * picture that omits something and a legend or a pick that still names it would be a worse
	 * asymmetry than the one this closes.
	 *
	 * It hides per PRIMITIVE at the view, so nothing about the world moves and nothing is unloaded.
	 */
	TArray<FString> Ignore;

	/**
	 * §4.3b's `highlights:` — types this picture MARKS, half transparent, over the ordinary image.
	 *
	 * **BESIDE `Ignore` AND NOT INSIDE IT, though they are close relatives.** Both say what this
	 * picture contains that a game camera would not, and both are per view. They are separate fields
	 * because they take different things — one names actors, the other names types — and folding two
	 * vocabularies into one list would make `["Torch_02", "niagara"]` legal and meaningless.
	 *
	 * Empty is the default and costs nothing: §4.3b rules the feature OFF unless asked for.
	 */
	TArray<FString> Highlights;

	/**
	 * THE POSE VIEW'S SELECTION (`Docs/GC-MOVE-SEE-AND-POINT-PLAN.md` §3.7, user 2026-10-15: *"we should also be able to ask for
	 * render of the bones direction"*): what a `bones` highlight draws on an animation or skeletal mesh. Set only when `Highlights`
	 * holds `bones`; written as `{type:"bones", of:[...], draw:[...], rig:"..."}` or the bare `"bones"`.
	 */
	struct FBones
	{
		/** Bone names and the rig's chain names; empty is the default selection the bone reader chooses. */
		TArray<FString> Of;
		bool bRay = true;
		bool bAxes = false;
		bool bRange = false;
		/** The reach bubble (plan §3.2, F5 v1): a sphere round each selected chain's first joint, its radius the chain laid out. */
		bool bBubble = false;
		/** The wanted aim, in the body's words a `.gcmove` writes (`(forward, down=55)`), marked on a limb end's direction sphere (§3.13). */
		FString Aim;
		/** The key's reach [share=] (0 none): the combined bubble tries each hand place with the trunk turned toward it as a build turns a
		 * reach's trunk by that share (§3.17 item 3). Needs Aim. */
		double Share = 0.0;
		/** The key this bubble places (BUBBLE-GRIP1, 2026-10-23; the user: "make the bubble match it"): `<.gcmove document>:<clip>`. Its reach's
		 * [share=] (where share: is not given), its balance and its plants give every place the build's own hip rise or sink (W6), and an ask's
		 * answer is the build's: the key dry-run with its reach pointed at the asked place, nothing saved. Empty: neither. Needs Aim. */
		FString Key;
		/** The ask's build check only: the line (`frame(at=..., toward=...)`) the key's reach is pointed at for TryPlaces[0]. */
		FString BuildLine;
		/** Given by an ask searching again (MOVE-ACTION1 W10 G15): where the key's build left the hips with this limb at its last answer. The
		 * dry run without the limb is skipped and the sphere stands round the joint carried that far. Also given to every ask of a picture
		 * with key: as that picture's own dry run without the limb found it, where its sphere stands (BUBBLE-SPEED1): the key is not built
		 * again for it. */
		FVector Settle = FVector::ZeroVector;
		bool bSettleGiven = false;
		/** The bubble's size, centimetres from the limb's first joint (0: the bone's own length). One sphere that big, bright where the hand
		 * can be (with the aim held, given Aim), dark where it cannot (plan §3.18 v6, §3.19; user 2026-10-16). */
		double Size = 0.0;
		/** Or the size by word (user 2026-10-16: "select either current arm to hand distanced sphere or a fully extend sphere"): "current", as
		 * far as the chain's end is from the joint now; "full", the chain laid out. Empty: Size. */
		FString SizeWord;
		/** An IK rig to read the joints' words and ranges from; empty means the asset's own build provenance. */
		FString Rig;
		/**
		 * TRAILS' GRAPH (Docs/GC-MOVE-SEE-AND-POINT-PLAN.md §3.22; user 2026-10-17: "So author can really see the jitterness", "And not only
		 * velocity but acceleration", "Author should be able to choose bone, joint, chain etc"): what to plot for each selected bone over the
		 * clip, velocity beside acceleration: "place" (where it goes, cm/s), "turn" (how fast it turns, deg/s), "joint" (its three angles in the
		 * rig's words), "centre" (the centre of mass). Empty: no graph. Trails only.
		 */
		TArray<FString> Graph;
		/**
		 * THE ASK TRIES ITS OWN PLACES (Docs/GC-MOVE-SEE-AND-POINT-PLAN.md §3.23; user 2026-10-22: "so next query can query about the relative
		 * angle + distance without needing to render again"): hand places, in the posed component's space, that the combined bubble tries
		 * instead of its sphere, answered in the reading's AimPlaces in this order. No sphere, direction sphere or ring is read. Empty: the sphere.
		 */
		TArray<FVector> TryPlaces;
		/** The ask's line search (BUBBLE-SPEED1): from this index on, TryPlaces are a line of which only the first that holds is read; the
		 * places after it are not tried against the body. INDEX_NONE: every place is tried. */
		int32 TryUntilHolds = INDEX_NONE;
		/** A ring point's ask (BUBBLE-GRIP1, the lead's ruling 2026-10-26): per place, the way the held frame's +Z (its edge) faces, square to
		 * Aim, in the character's space - the roll asked, not the build's. With bTryAtKeysPlace and Key, the one place tried is where the key's
		 * reach puts the grip (its own at=), and the key is built with toward= the aim and up= TryUps[0] there. */
		TArray<FVector> TryUps;
		bool bTryAtKeysPlace = false;
	};
	FBones Bones;
	/** The clip trajectory's selection (`{type:"trails", of:[...]}`, plan §3.9): the same names; `draw` does not apply. */
	FBones Trails;

	/**
	 * §11's `at:` / `to:` / `fps:` — WHEN this view is of, and how many moments of it.
	 *
	 * On the view beside `Cut` and for the same reason `Cut` is: it says what this picture is a picture
	 * OF. A cut answers *which part of the world*, this answers *which instant of it*, and both are
	 * questions the camera form is already the third of.
	 */
	FGraphCodeRenderTime Time;

	double AspectRatio() const
	{
		return Height > 0 ? static_cast<double>(Width) / static_cast<double>(Height) : 1.0;
	}
};

enum class EGraphCodeRenderLifetime : uint8
{
	/** A short-lived cache under `Saved/GraphCode/renders/.transient/<capture>/`. The default. */
	Transient,
	/** The caller's own destination, kept until the caller removes it. */
	Persistent,
};

enum class EGraphCodeRenderOverwrite : uint8
{
	/** Refuse rather than replace something that already exists. The default. */
	Refuse,
	Replace,
	/** Write alongside under a suffixed name, and say which name it used. */
	Unique,
};

/**
 * Where the picture goes — `GC-VISION-DESIGN.md` §3, ruled complete rather than trimmed 2026-08-03.
 *
 * THE RULE THE WHOLE STRUCT FOLLOWS: make the surprising thing explicit rather than impossible.
 * Nothing here lands anywhere by accident — omitting the block gives one transient image and costs
 * no request tokens — and every destination that would surprise somebody needs a word from the
 * caller rather than a refusal from us. The caller knows what the picture is for.
 */
struct GRAPHCODE_API FGraphCodeRenderOutput
{
	EGraphCodeRenderLifetime Lifetime = EGraphCodeRenderLifetime::Transient;

	/**
	 * A directory, or a PNG template. Empty with `Transient` is the cache; empty with `Persistent`
	 * refuses, because "keep this" without saying where is not an instruction.
	 *
	 * Template fields: `{asset}`, `{kind}`, `{view}`, `{capture}`, `{date}`. A template that writes
	 * more than one view without `{view}` refuses BEFORE anything renders — one file per view is
	 * the point, and finding out afterwards means the GPU work is already spent.
	 */
	FString Path;

	EGraphCodeRenderOverwrite Overwrite = EGraphCodeRenderOverwrite::Refuse;

	/** Accept an absolute path. Relative paths resolve from the project directory. */
	bool bExternal = false;

	/**
	 * Accept a path under `Content/`.
	 *
	 * Unreal auto-imports a PNG there, which does not merely discover the file — it CREATES a
	 * `UTexture2D` that then wants a sidecar and enters the corpus. That is a real thing to want and
	 * a worse thing to do by accident, so it takes a word, and the result says that it happened.
	 */
	bool bContent = false;
};

/**
 * The two values that would otherwise be a hidden clock.
 *
 * `{capture}` and `{date}` are the only non-deterministic things in a resolved path, so they are
 * passed IN rather than read from `FDateTime::Now()` inside the resolver. That is what makes the
 * whole output layer testable without freezing time, and it is the same reason the camera takes a
 * subject instead of finding one.
 */
struct GRAPHCODE_API FGraphCodeRenderStamp
{
	FString CaptureId;
	/** `YYYY-MM-DD`. */
	FString Date;

	static FGraphCodeRenderStamp Now();
};

/** One FRAME's resolved destination. Nothing is written yet; this is the plan. */
struct GRAPHCODE_API FGraphCodeRenderFile
{
	FString ViewName;
	/** Absolute, normalised, and checked. */
	FString Path;

	/**
	 * §11. Zero-based within the view, and `Seconds` is the moment it is of.
	 *
	 * A single-frame view is index 0 at second 0 and keeps the filename it always had — `<view>.png`,
	 * with no index in it. The suffix appears only once there is more than one frame to tell apart,
	 * which is what keeps every destination this tool has ever written byte-identical.
	 */
	int32 FrameIndex = 0;
	double Seconds = 0.0;
};

/** A derived image assembled from a view's frames, rather than rendered. §11. */
struct GRAPHCODE_API FGraphCodeRenderDerivedFile
{
	FString ViewName;
	FString Path;
	/** Contact sheet only: the grid it was tiled into. */
	int32 Columns = 0;
	int32 Rows = 0;
};

struct GRAPHCODE_API FGraphCodeRenderOutputPlan
{
	EGraphCodeRenderLifetime Lifetime = EGraphCodeRenderLifetime::Transient;
	FString CaptureId;

	/** Absolute. Nothing has created it. */
	FString Directory;
	/** One per (view, frame), in view order then frame order. */
	TArray<FGraphCodeRenderFile> Files;
	/** §11's contact sheets, at most one per view. */
	TArray<FGraphCodeRenderDerivedFile> Sheets;
	/** §11's animated PNGs, at most one per view. */
	TArray<FGraphCodeRenderDerivedFile> Videos;
	FString ManifestPath;

	/**
	 * §3: inline image content is returned when the request produced exactly ONE image.
	 *
	 * §11 widened what that sentence counts without changing it. It used to mean one view, because a
	 * view was one picture; it now means one PICTURE, so a single view of twenty-four frames does not
	 * qualify — and its contact sheet does, which is the whole reason the sheet defaults on.
	 */
	bool bInlineImage = false;

	/** The view whose SHEET is returned inline, when frames made the frames themselves too many. */
	FString InlineSheetView;

	/** Every file this plan will write, frames and derived alike. The overwrite check reads it. */
	int32 TotalFiles() const { return Files.Num() + Sheets.Num() + Videos.Num(); }

	TArray<FString> Warnings;
};

/**
 * The whole request, provider-neutral: no JSON, no MCP, no UObject.
 */
struct GRAPHCODE_API FGraphCodeRenderRequest
{
	/** Always explicit. There is no "active asset" fallback, in any phase. */
	FString AssetPath;

	/**
	 * Optional static-mesh geometry drawn over a mesh preview after the subject is placed, using its
	 * vertex colours and ignoring depth. This is how a rig diagnostic can sit on the REAL mesh rather
	 * than replacing it with a wire cage. It is preview-only: a level is a world the user owns, and a
	 * read-only render does not spawn helpers into it.
	 */
	FString OverlayAssetPath;

	/**
	 * Optional clip played on a skeletal mesh the subject brings with it — a Groom's target, so hair is
	 * seen on a head that moves. A stated moment poses it and `simulate:true` plays it from zero under
	 * the hair physics. Preview only: the mesh and the clip are never written.
	 */
	FString AnimationAssetPath;

	/** The GraphCode kind token, for `{kind}`. Empty is fine until a template asks for it. */
	FString Kind;

	FGraphCodeRenderOutput Output;

	/**
	 * `live` only. `saved` can be added when it is PROVEN to unload or isolate a dirty live object;
	 * until then it would silently render the wrong revision, so it refuses by name.
	 */
	FString Source = TEXT("live");

	EGraphCodeRenderScene Scene = EGraphCodeRenderScene::Preview;
	FGraphCodeRenderProfile Profile;
	TArray<FGraphCodeRenderView> Views;

	/** Returns the absolute camera and scoring inline as well as in the manifest. */
	bool bDetailsFull = false;

	static constexpr int32 MaxViews = 8;
	static constexpr int32 MinDimension = 1;
	static constexpr int32 MaxDimension = 4096;
	/** Eight 2048² views, or two 4096². Eight 4K views cannot be asked for at all. */
	static constexpr int64 MaxTotalPixels = 33554432;
	static constexpr double MinFov = 5.0;
	static constexpr double MaxFov = 170.0;
	/** ±8 is a 256× change in projected size in either direction. Beyond it the fit is noise. */
	static constexpr double MaxAbsZoom = 8.0;
	/**
	 * Where `zoom:0` starts for an interior camera — `FGraphCodeRenderOrbit::bInside`.
	 *
	 * WIDER THAN THE PROFILE'S 35 ON PURPOSE, and stated here rather than buried in the solver because
	 * a rendering constant that is not written down is a hidden variable — the same rule that put the
	 * profile's own numbers on a struct. 90° horizontal is roughly what an interior photograph is taken
	 * with, and for the same reason: standing in a room, a narrow lens frames a patch of wall.
	 */
	static constexpr double InsideFovDegrees = 90.0;
	/** Short of ±90°, per §5.1, so the up axis never degenerates. */
	static constexpr double MaxAbsElevation = 89.0;
	/** Empty or invalid bounds become this cube, and say so. */
	static constexpr double SubstituteBoxHalfExtent = 50.0;
};

/**
 * The subject, as the adapter measured it.
 *
 * `Box` is axis-aligned IN THE ORBIT BASIS, not in render space, and `BasisToRender` places it.
 * That split is what lets one camera implementation serve a preview world (identity), a level actor
 * (the actor's transform) and an aggregate world-axis selection (a translation), and lets all of
 * them be fixtured here without an editor.
 */
struct GRAPHCODE_API FGraphCodeRenderSubject
{
	FBox Box = FBox(ForceInit);

	/**
	 * Orbit basis → render space. Rotation, translation and UNIFORM scale only: a non-uniform
	 * scale would make "the camera is 3 metres from the anchor" false along one axis, and the
	 * adapter has the geometry to fold that scale into `Box` before it gets here.
	 */
	FTransform BasisToRender = FTransform::Identity;

	EGraphCodeRenderSpace Space = EGraphCodeRenderSpace::PreviewLocal;
	EGraphCodeRenderBasis Basis = EGraphCodeRenderBasis::Local;
};

/** The camera, fully resolved, in render space. This is manifest data, not the compact reply. */
struct GRAPHCODE_API FGraphCodeRenderCamera
{
	FVector Location = FVector::ZeroVector;
	FRotator Rotation = FRotator::ZeroRotator;
	/** Horizontal, and meaningless when the projection is orthographic. */
	double Fov = 35.0;
	double AspectRatio = 1.0;
	EGraphCodeRenderProjection Projection = EGraphCodeRenderProjection::Perspective;

	/**
	 * Orthographic only: the world-space width the picture covers. The height follows from the
	 * aspect, exactly as the vertical FOV does in perspective — one convention, both projections.
	 */
	double OrthoWidth = 0.0;
	/**
	 * The ortho depth range, relative to the camera. Defaulted to the engine's own
	 * `DEFAULT_ORTHONEARPLANE`/`DEFAULT_ORTHOFARPLANE`, so a camera GraphCode computed behaves like
	 * an untouched Camera component — the near plane is BEHIND the camera on purpose, which is what
	 * lets a 2D scene put sprites on both sides of it.
	 */
	double OrthoNear = -768.0;
	double OrthoFar = 2097152.0;

	/**
	 * §21.13a's section cut, resolved. Unset is the ordinary picture, and then `OrthoNear`/`OrthoFar`
	 * above are the whole depth range exactly as they always were.
	 *
	 * SET, IT REPLACES BOTH — and the projection it builds is a strict GENERALISATION of the one it
	 * replaces rather than a second path: a plane perpendicular to the view reduces, term for term, to
	 * the engine's own `FReversedZOrthoMatrix`. That is why an oblique cut costs no accuracy and why
	 * the perpendicular case cannot drift away from the uncut picture.
	 */
	TOptional<FGraphCodeRenderCutPlane> Cut;

	/** Repeated here so a camera can never be copied out of a result without its space. */
	EGraphCodeRenderSpace Space = EGraphCodeRenderSpace::PreviewLocal;
};

/** The compact pose — paste-ready on the next call, which is the whole point of §4.1. */
struct GRAPHCODE_API FGraphCodeRenderPose
{
	double Azimuth = 0.0;
	double Elevation = 0.0;
	double Zoom = 0.0;
	FGraphCodeRenderOffset Offset;
	EGraphCodeRenderBasis Basis = EGraphCodeRenderBasis::Local;

	/**
	 * Whether this pose was taken from within the subject.
	 *
	 * IN THE POSE AND NOT ONLY IN THE REQUEST, because the pose is the thing a caller pastes back with
	 * one number changed. Dropping it would turn *ten degrees to the left, still inside* into a shot
	 * from outside the building, which is the one edit this compact form exists to make safe.
	 */
	bool bInside = false;

	/** Positive zoom may crop on purpose. Reported, never treated as a framing failure. */
	bool bCropped = false;
};

/** One candidate's closed-form score. Recorded so a bad pick can be argued with numbers. */
struct GRAPHCODE_API FGraphCodeRenderAngleScore
{
	double Azimuth = 0.0;
	double Elevation = 0.0;

	/**
	 * How much of the subject this direction shows: its projected silhouette area over the largest
	 * silhouette area any direction could produce, in [0,1].
	 *
	 * NOT the fraction of the frame the subject covers. That reading is the obvious one and it is
	 * wrong in a way the fixtures caught: the fit makes every view fill its frame on the limiting
	 * axis, so frame-fraction is maximised by the most COMPACT projection — which means looking
	 * straight down the longest axis. It frames a sofa end-on and a lamppost from above.
	 */
	double Coverage = 0.0;
	/** Shorter ÷ longer principal extent of that silhouette, in [0,1]. 0 is a line. */
	double Aspect = 0.0;
	/** The front/three-quarter prior, in [0,1]. */
	double Prior = 0.0;
	double Total = 0.0;
};

/**
 * Which camera actor a picture came from, and what was done to it — §5.7.
 *
 * The adjusted absolute camera is in `FGraphCodeRenderViewResult::Camera` and in the manifest, so
 * repeating a shot never depends on the camera actor still being where it was. This is the other
 * half of that: what it was BEFORE, so a reader can tell a moved camera from a changed adjustment.
 */
struct GRAPHCODE_API FGraphCodeRenderNamedCameraResult
{
	FString ActorName;
	/** `CameraComponent` or `CineCameraComponent`. Reported, because they are not the same lens. */
	FString ComponentClass;
	/** The component's own horizontal FOV, before the picture's aspect was applied. */
	double BaseFov = 0.0;
	/** The component's own aspect ratio, which the requested view's need not match. */
	double BaseAspectRatio = 0.0;
	bool bPostProcessApplied = false;

	/**
	 * True when the picture was taken THROUGH this camera; false when only its lens and grade were
	 * borrowed and the pose came from somewhere else (§5.9). One flag, because the two read
	 * identically in a result otherwise and mean completely different pictures.
	 */
	bool bPoseFromActor = true;

	FGraphCodeRenderCameraAdjust Adjust;
};

struct GRAPHCODE_API FGraphCodeRenderAngleReport
{
	FGraphCodeRenderAngleScore Winner;
	int32 CandidatesEvaluated = 0;
	/** The best few, for the manifest. Not the whole lattice: it is a thousand rows of nothing. */
	TArray<FGraphCodeRenderAngleScore> Runners;
	static constexpr int32 MaxRunners = 5;
};

struct GRAPHCODE_API FGraphCodeRenderViewResult
{
	FString Name;
	int32 Width = 0;
	int32 Height = 0;

	/**
	 * What the renderer drew, carried back so the manifest and the reply state it.
	 *
	 * A picture and its mode are not separable after the fact — a wireframe render and a subject with
	 * no surfaces look identical, and so do `unlit` and a level whose lights were never built.
	 */
	EGraphCodeRenderViewMode ViewMode = EGraphCodeRenderViewMode::Lit;

	/**
	 * Unset for an absolute camera, and that is the point: azimuth/elevation/zoom are orbit facts,
	 * and a struct of zeroes would read as "azimuth 0" to anyone who did not check a flag.
	 */
	TOptional<FGraphCodeRenderPose> Pose;
	FGraphCodeRenderCamera Camera;

	/** Set only when the camera was a named level camera. */
	TOptional<FGraphCodeRenderNamedCameraResult> NamedCamera;

	/** The box actually fitted, after any substitution, in basis space. */
	FBox FittedBox = FBox(ForceInit);
	/** Orbit only: the aim point after `offset`, in basis space. */
	FVector Aim = FVector::ZeroVector;
	/** Orbit only: the distance before `zoom` divided it. `zoom:0` means this is the distance. */
	double FitDistance = 0.0;

	/** Set only when the automatic selection chose the angle. */
	TOptional<FGraphCodeRenderAngleReport> Angles;

	/**
	 * What this view actually left out — the resolved internal names, R8's ignore list.
	 *
	 * ANSWERED BACK RATHER THAN ASSUMED FROM THE REQUEST, and it reaches the manifest, because a
	 * picture that is missing something has to say so: a reader who did not make the call cannot tell
	 * *there is no roof here* from *the roof was hidden* by looking.
	 */
	TArray<FString> Ignored;

	/**
	 * §4.3b's `highlights:` — what this picture actually MARKED, as `type: n subjects`.
	 *
	 * ANSWERED BACK RATHER THAN ECHOED FROM THE REQUEST, for the same reason `Ignored` is: a reader
	 * who did not make the call cannot tell *this level has no navmesh* from *the overlay failed* by
	 * looking at the picture, and a request field repeated back would say nothing about either.
	 */
	TArray<FString> Highlighted;

	/**
	 * POINT AND ASK (Docs/GC-MOVE-SEE-AND-POINT-PLAN.md §3.15): each direction sphere this picture drew (the pose view's bubble), written
	 * to the manifest so `mode:"ask"` meets it before the mesh behind it.
	 */
	struct FBubble
	{
		/** `direction` (a limb end's: degrees past a stop), `aim` (a chain's facing, the gaze: degrees short), `reach` (a limb's first joint:
		 * a point inside is a PLACE, answered against the net in Reach) or `joint` (a drawn bone's small target: its place). */
		FName Kind;
		FName Bone;
		/** In the picture's own space, as its camera is. */
		FVector Centre = FVector::ZeroVector;
		double Radius = 0.0;
		/** From that space into the body's words (left +X, forward +Y, up +Z): the posed component's rotation, inverted. */
		FQuat ToBody = FQuat::Identity;
		/** Per direction of the sphere's grid (rows of up from -85 every 5 degrees, 72 columns round every 5), degrees past a stop; -1 none. */
		TArray<float> Excess;
		/** Per direction of that grid, why the arm meets the body where the joints hold it (empty where clear or not tried): dark, as the build refuses it. */
		TArray<FString> Blocked;
		/** A reach bubble's net: per direction of its grid (rows of up from -84 every 12 degrees, 30 columns round every 12), how far the end
		 * goes with every joint in range, cm; 0 where it goes no way. */
		TArray<float> Reach;
		/** The component's origin in the picture's space: a place's `at=` is measured from it, as a .gcmove frame's is. */
		FVector Origin = FVector::ZeroVector;
		/** The combined bubble (plan §3.17 item 1): the wanted aim as written, the hand places tried (the character's space) and at each how
		 * far past a stop holding it needs (0 in range, negative no place). */
		FString Aim;
		TArray<FVector> AimPlaces;
		TArray<float> AimExcess;
		/** At each place, the excess per joint of AimJoints (upper, middle, end), and why an arm in range there meets the body (empty: clear). */
		TArray<FName> AimJoints;
		TArray<FVector3f> AimShares;
		TArray<FString> AimBlocked;
		/** The whole chain at each place that holds (plan §3.19 step 3): its joints, their axes' words (three each), and per place per joint
		 * the three angles as [readings] reads them. */
		TArray<FName> AimPoseJoints;
		TArray<FString> AimPoseWords;
		TArray<FVector3f> AimPose;
		/** A direction sphere's roll ring (plan §3.20 step 5): the ups square to the wanted aim (the body's words), per up the degrees past
		 * holding the edge that way needs (0 in range, negative no place), and per up the excess per joint of RollJoints. */
		TArray<FVector> RollUps;
		TArray<float> RollExcess;
		TArray<FVector3f> RollShares;
		TArray<FName> RollJoints;
		/** Per up, why the arm meets the body with the edge that way (empty where clear or not tried). */
		TArray<FString> RollBlocked;
		/**
		 * WHAT A REACH BUBBLE'S PLACES ARE TRIED AGAIN FROM (§3.23; user 2026-10-22: "Then could query again for that extension"): the posed mesh,
		 * its pose (the component's space, every bone), the bones request the bubble was drawn from and the chain laid out from Bone (cm), so
		 * the ask tries any place again, exactly, with no new picture. Empty on every other bubble.
		 */
		FString Mesh;
		TArray<FTransform> Space;
		FGraphCodeRenderView::FBones Request;
		double ReachRadius = 0.0;
	};
	TArray<FBubble> Bubbles;

	/**
	 * §11's resolved moments — WHEN each of this view's pictures was taken, in seconds.
	 *
	 * ANSWERED BACK RATHER THAN ECHOED, like `Ignored` and `Highlighted` above it, and here the gap
	 * between request and result is arithmetic rather than resolution: `fps` and `count` both describe
	 * a sampling and neither states the moments, so a caller comparing two captures has nothing to
	 * compare on until this says what they were.
	 */
	FGraphCodeRenderTimeline Timeline;

	TArray<FString> Warnings;
};

/**
 * The camera half of `gc_render`. Every function here is pure arithmetic over the structs above:
 * no world, no RHI, no asset, and therefore the whole of it is asserted under `-nullrhi`.
 */
class GRAPHCODE_API FGraphCodeRenderCameraSolver
{
public:
	/**
	 * The views this request actually renders. An empty list is not an error — §4 says an omitted
	 * `views` renders one view named `main`, and the default belongs here so every caller agrees.
	 */
	static TArray<FGraphCodeRenderView> EffectiveViews(const FGraphCodeRenderRequest& Request);

	/** Caps, names, view identity and camera-form exclusivity. Touches no subject. */
	static bool ValidateRequest(const FGraphCodeRenderRequest& Request, TArray<FString>& OutErrors);

	/** One view against one subject. Validation failures are errors; framing facts are warnings. */
	static bool ResolveView(const FGraphCodeRenderRequest& Request, const FGraphCodeRenderView& View,
		const FGraphCodeRenderSubject& Subject, FGraphCodeRenderViewResult& OutResult,
		TArray<FString>& OutErrors);

	/**
	 * §5.7's adjustments, on a copy. The tripod convention is on `FGraphCodeRenderCameraAdjust`.
	 *
	 * Pure, and separate from the actor lookup on purpose: WHERE the camera is comes from a world
	 * only an adapter can read, and what happens to it afterwards is six numbers of arithmetic that
	 * can be fixtured under `-nullrhi` like everything else here.
	 */
	static FGraphCodeRenderCamera ApplyAdjust(const FGraphCodeRenderCamera& Base,
		const FGraphCodeRenderCameraAdjust& Adjust);

	/**
	 * A view whose camera is a named actor the adapter has already resolved into `Base`.
	 *
	 * It takes neither a subject nor the request, and both absences are the point rather than
	 * omissions: a named camera frames whatever it is pointing at, so there is no box to fit and no
	 * pose to return, and its field of view is the lens somebody chose rather than the profile's.
	 */
	static bool ResolveNamedView(const FGraphCodeRenderView& View,
		const FGraphCodeRenderCamera& Base, FGraphCodeRenderViewResult& OutResult,
		TArray<FString>& OutErrors);

	/**
	 * The analytic angle choice — silhouette coverage, principal-axis aspect, front prior.
	 *
	 * Depends on the subject's SHAPE and nothing else: no FOV, no aspect ratio, no margin, no aim.
	 * `zoom:0` fits the whole box whatever the angle, so the framing constants cannot change which
	 * angle is best, and an offset aim moves the same silhouette rather than a different one.
	 *
	 * `FixedAzimuth`/`FixedElevation` restrict the lattice rather than replacing it, so a caller
	 * who pinned one number still gets the best value for the other.
	 */
	static FGraphCodeRenderAngleScore SelectAngle(const FBox& Box,
		const TOptional<double>& FixedAzimuth, const TOptional<double>& FixedElevation,
		FGraphCodeRenderAngleReport& OutReport);

	/**
	 * The eight-corner fit: the nearest distance at which every corner of the box is still inside
	 * both FOVs, with the margin preserved. A sphere-radius fit is what this replaces.
	 */
	static double FitDistance(const FBox& Box, const FVector& Aim, double Azimuth, double Elevation,
		double FovDegrees, double AspectRatio, double Margin);

	/**
	 * The same eight corners, for the projection where distance is not the answer.
	 *
	 * Orthographic framing has no depth term at all: a corner is inside the picture when its LATERAL
	 * offset is under half the ortho width and its VERTICAL offset under half the ortho height. So
	 * the fit is a max over the corners rather than a max over demands, and it needs no FOV.
	 */
	static double FitOrthoWidth(const FBox& Box, const FVector& Aim, double Azimuth, double Elevation,
		double AspectRatio, double Margin);

	/** Camera basis for an orbit pose: forward looks AT the aim, roll is always zero. */
	static void OrbitAxes(double Azimuth, double Elevation, FVector& OutForward, FVector& OutRight,
		FVector& OutUp);

	/** `[-180,180]`, so two spellings of the same angle never read as two different poses. */
	static double NormalizeAzimuth(double Azimuth);

	/**
	 * The two matrices a picture is actually taken through — `GC-SPATIAL-AUTHORING-DESIGN.md` §4b.
	 *
	 * **These are SHARED with the renderer's own `AddView`, not a second construction of the same
	 * thing.** S1v exists to turn a pixel back into a world ray, and a deprojection built from its
	 * own copy of these is correct only until one of the two copies changes — at which point the ray
	 * lands somewhere plausible and nothing fails. The renderer calls these; so does the
	 * deprojection; there is one reversed-Z assumption and one axis swap in the module.
	 *
	 * Still pure arithmetic, so they stay in this class and stay provable under `-nullrhi`.
	 */
	/**
	 * §21.13a's cut, lowered onto a camera that is already resolved — and it has to be already
	 * resolved, because *between the camera and the plane* is not a question a request can answer.
	 *
	 * A no-op when the view carries no cut, which is what lets every exit call it unconditionally.
	 */
	static bool ApplyCut(const FGraphCodeRenderView& View, FGraphCodeRenderCamera& InOutCamera,
		TArray<FString>& OutErrors);

	static FMatrix ViewRotationMatrix(const FGraphCodeRenderCamera& Camera);
	static FMatrix ProjectionMatrix(const FGraphCodeRenderCamera& Camera, int32 Width, int32 Height);

	/** `ViewMatrix * ProjectionMatrix`, inverted — what a deprojection needs and nothing more. */
	static FMatrix InverseViewProjection(const FGraphCodeRenderCamera& Camera, int32 Width,
		int32 Height);

	/**
	 * One normalized top-left image coordinate to a world-space ray.
	 *
	 * This is `FSceneView::DeprojectScreenToWorld`'s arithmetic with its pixel truncation removed —
	 * that helper serves a mouse cursor and rounds to a whole pixel, which makes the picture very
	 * slightly asymmetric about its own centre and caps precision at one pixel. The implementation
	 * says which two lines differ and why.
	 *
	 * The origin is an OUTPUT rather than the camera's location, and that is not a convenience: a
	 * perspective ray starts at the camera, but every orthographic pixel has its own near-plane
	 * origin and shares only the direction. Returning `Camera.Location` for both would be right in
	 * exactly the projection that needs it least.
	 *
	 * `false` when the coordinates are outside `[0,1]` or the view rectangle is empty; the caller
	 * refuses rather than clamping, because a clamped pixel is a different pixel.
	 */
	static bool DeprojectNormalized(const FGraphCodeRenderCamera& Camera, int32 Width, int32 Height,
		double NormalizedX, double NormalizedY, FVector& OutOrigin, FVector& OutDirection);

	/**
	 * A world point to the normalized top-left image coordinate it lands on — the other direction,
	 * and the primitive the frame legend is made of (`GC-SPATIAL-AUTHORING-DESIGN.md` §20.22, Z3).
	 *
	 * THROUGH THE SAME TWO MATRICES, for exactly the reason stated above them. A legend built from
	 * its own projection would agree with the picture until one of the two drifted, and then it would
	 * name an actor at a pixel that shows something else — which is worse than no legend, because a
	 * caller then points at it and gets a confident deprojection of the wrong surface.
	 *
	 * `OutDepth` IS THE VIEW-SPACE DEPTH AND IT IS WHY THIS RETURNS ONE. A perspective divide by a
	 * negative W mirrors a point BEHIND the camera onto a perfectly plausible pixel in front of it,
	 * and an orthographic projection has no W to inspect at all — so *is this thing even in front of
	 * me* cannot be recovered from the coordinates afterwards. Positive is in front.
	 *
	 * The coordinates are NOT clamped and are routinely outside `[0,1]`: an actor off to the left of
	 * the frame has a real position in image space and a caller cropping to the frame wants to know
	 * how far out it is. `false` only when the view rectangle is empty.
	 */
	static bool ProjectNormalized(const FGraphCodeRenderCamera& Camera, int32 Width, int32 Height,
		const FVector& World, double& OutNormalizedX, double& OutNormalizedY, double& OutDepth);
};

/**
 * §11's time half — the moments one view is of, and nothing else.
 *
 * Its own class beside the camera solver rather than a method on it, because it shares nothing with
 * the camera: a moment has no bearing on where the eye is, and a pose has no bearing on when. Pure
 * arithmetic over the request, so the whole of it is asserted under `-nullrhi` exactly as the camera
 * is — a span that does not divide evenly, a count that collapses, a ceiling, and the defaulting rule
 * are all provable with no world, no subject and no GPU.
 */
class GRAPHCODE_API FGraphCodeRenderTimeSolver
{
public:
	/**
	 * One view's stated time to the list of moments it renders.
	 *
	 * ALWAYS PRODUCES AT LEAST ONE MOMENT, including for a view that states no time at all — that
	 * resolves to a single unstated moment, and `Timeline.bStated` is what the renderer reads to
	 * decide whether to pin the clock or leave it where it has always been. So every caller can loop
	 * over `Moments` unconditionally and the single-picture path stays the same code.
	 */
	static bool Resolve(const FGraphCodeRenderView& View, FGraphCodeRenderTimeline& OutTimeline,
		TArray<FString>& OutErrors, TArray<FString>& OutWarnings);

	/** How many frames this view will render, without resolving them. The pixel budget reads it. */
	static int32 FrameCount(const FGraphCodeRenderView& View);
};

/**
 * §11's DERIVED images — the contact sheet and the animated PNG.
 *
 * Neither of them renders anything. A sheet is a memcpy of pixels that already exist and an APNG is
 * the frames' own compressed payloads re-chunked, so the whole of this is byte arithmetic with no
 * world, no subject and no GPU — which is why it is here beside the camera and the output resolver
 * rather than buried in the renderer, and why every claim it makes is provable under `-nullrhi`.
 *
 * **THAT MATTERS MORE FOR THE APNG THAN FOR ANYTHING ELSE IN THIS FEATURE.** A malformed chunk
 * length or a wrong CRC produces a file that is written, has bytes, has a plausible size, and shows
 * nothing when somebody eventually opens it — which is the exact failure §4.2's white level capture
 * was, and it went five runs undetected there. So the bytes are asserted rather than the write.
 */
class GRAPHCODE_API FGraphCodeRenderFilm
{
public:
	/**
	 * The grid a sheet of `Frames` cells is tiled into.
	 *
	 * SHARED BY THE PLAN AND THE BLIT rather than computed in both: the resolver names the file and
	 * records its grid before anything renders, and the renderer writes pixels into it afterwards. Two
	 * copies of "near-square" would agree until one of them was tuned, and the failure would be a
	 * sheet whose last row is off the bottom of its own image.
	 *
	 * Wider than tall, because pictures usually are: a sheet that is square in CELLS is a tall thin
	 * image somebody has to scroll.
	 */
	static void Grid(int32 Frames, int32& OutColumns, int32& OutRows);

	/** One frame into its cell of a sheet buffer the caller allocated. */
	static void Tile(const TArray<FColor>& Frame, int32 FrameWidth, int32 FrameHeight, int32 Cell,
		int32 Columns, TArray<FColor>& InOutSheet, int32 SheetWidth);

	/**
	 * N encoded PNGs into one animated PNG, at `Fps`.
	 *
	 * Every frame must share the first one's `IHDR`, which they do by construction — one render
	 * target, one size, one format, for every frame of a view.
	 */
	static bool Animate(const TArray<TArray64<uint8>>& Frames, double Fps, TArray64<uint8>& OutBytes,
		FString& OutError);

	/**
	 * A PNG's chunk types in order, with every CRC verified — the reader `Animate` is asserted with.
	 *
	 * Exposed rather than kept private because a spec that checked `Animate`'s output with its own
	 * parser would be checking two implementations against each other, and a shared bug in the two
	 * would be invisible. This is the same parser `Animate` reads its input through, so a spec that
	 * uses it is asserting on the bytes rather than on an opinion about them.
	 */
	static bool Chunks(const TArray64<uint8>& Png, TArray<FString>& OutTypes, FString& OutError);
};

/**
 * §11.6's OTHER container — H.264 in an MP4, written by the operating system's own encoder.
 *
 * **NOTHING IS SHIPPED FOR THIS.** The engine's own `MovieRenderPipelineMP4Encoder` proves the
 * shape: its entire Windows dependency is three delay-loaded system DLLs (`mfplat`, `mfreadwrite`,
 * `mfuuid`), because Windows Media Foundation already contains both an H.264 encoder and an MP4
 * muxer. There is no `ffmpeg`, no `ThirdParty/` binary and no licence to carry — which is what makes
 * this affordable for a plugin that ships on Fab, and it is the fact that overturned §11.7's "an
 * `mp4` would add an encoder dependency". That sentence was true of `ffmpeg` and false of the system.
 *
 * Epic's encoder is NOT reused directly, and the reason is coupling rather than taste:
 * `FMoviePipelineMP4Encoder` lives in that plugin's `Private/`, so it cannot be linked, and the
 * module that exports it drags `MovieRenderPipelineCore` and `OpenColorIO` behind it and would make
 * MovieRenderPipeline a hard plugin dependency of GraphCode. This is the same ~10 Media Foundation
 * calls written against the same system libraries, with its file kept as the correctness reference.
 *
 * **IT STREAMS.** `Open`, then one `Frame` per rendered frame, then `Close`. Nothing is accumulated:
 * the APNG path has to hold every frame's compressed payload because an APNG *is* those payloads,
 * but a sink writer consumes a frame and forgets it, so an MP4 of the 120-frame ceiling costs one
 * frame of memory rather than a hundred and twenty.
 *
 * Windows only. `UnsupportedReason` is asked BEFORE the plan names a file, so a capture that cannot
 * have an MP4 writes an APNG under the right name instead of writing the wrong bytes under `.mp4`.
 */
class GRAPHCODE_API FGraphCodeRenderMovie
{
public:
	/**
	 * Why this machine cannot write an MP4 of this size, or empty if it can.
	 *
	 * A STRING RATHER THAN A BOOL because every caller that asks also has to explain itself in a
	 * warning, and a caller that reconstructs the reason from `false` would reconstruct it wrongly:
	 * "not supported on this platform" and "1023 is an odd number" are not the same problem and the
	 * second one the user can actually fix.
	 *
	 * The size is part of the question because H.264 is 4:2:0 — chroma is half resolution in both
	 * axes, so an odd width or height has no legal encoding. `gc_render` will take any size it is
	 * asked for, so this is reachable rather than theoretical.
	 */
	static FString UnsupportedReason(int32 Width, int32 Height);

	FGraphCodeRenderMovie();
	~FGraphCodeRenderMovie();

	FGraphCodeRenderMovie(const FGraphCodeRenderMovie&) = delete;
	FGraphCodeRenderMovie& operator=(const FGraphCodeRenderMovie&) = delete;

	/** Start the file. Creates it, so a failure here happens before any frame is spent. */
	bool Open(const FString& Path, int32 Width, int32 Height, double Fps, FString& OutError);

	/**
	 * One frame, top-down BGRA — exactly what a render target readback already is.
	 *
	 * No copy of the pixels outlives the call. `Pixels` must hold `Width * Height` entries.
	 */
	bool Frame(const TArray<FColor>& Pixels, FString& OutError);

	/**
	 * Finish the file and release the encoder.
	 *
	 * **THE MOOV ATOM IS WRITTEN HERE AND NOWHERE ELSE.** An MP4 abandoned without this is a file
	 * with bytes, a plausible size and no index — the §4.2 failure again — so `Close` is called on
	 * every path out, including the failing ones, and the destructor calls it if nothing else did.
	 */
	bool Close(FString& OutError);

	bool IsOpen() const;

private:
	/**
	 * Pimpl, and not for tidiness: Media Foundation's headers drag `windows.h` with them, and this
	 * is a public header that most of the module includes. The COM types stay in the .cpp.
	 */
	struct FImpl;
	TUniquePtr<FImpl> Impl;
};

/**
 * Where every picture in one request goes, resolved before any of them is rendered.
 *
 * Separate from the camera because it shares nothing with it, and separate from the renderer
 * because it touches no pixels: this is string and path arithmetic, so a traversal, a `Content/`
 * write and a name collision are all provable under `-nullrhi` — which is where a path bug should
 * be caught, rather than after the GPU work is spent.
 */
class GRAPHCODE_API FGraphCodeRenderOutputResolver
{
public:
	/**
	 * Resolve the destination of every view. Creates no directory and writes no file.
	 *
	 * `Overwrite::Unique` and `Overwrite::Refuse` do consult the filesystem — they have to know
	 * what is already there — so the plan is a snapshot, and phase 2 writes promptly after taking
	 * it rather than holding it.
	 */
	static bool Resolve(const FGraphCodeRenderRequest& Request,
		const TArray<FGraphCodeRenderView>& Views, const FGraphCodeRenderStamp& Stamp,
		FGraphCodeRenderOutputPlan& OutPlan, TArray<FString>& OutErrors);

	/** The `.transient` cache root, absolute. Exposed so a cleanup pass cannot guess it wrong. */
	static FString TransientRoot();

	/** How many transient captures survive a prune. See `PruneTransient`. */
	static constexpr int32 TransientKeep = 256;

	/**
	 * Keep the `TransientKeep` most recent captures under `.transient` and delete the rest.
	 *
	 * **C24.10, and it is a missing `if` rather than a policy.** `TransientRoot()` has carried the
	 * comment *"exposed so a cleanup pass cannot guess it wrong"* since the root was introduced, and
	 * the cleanup pass was never written — so a bucket the format itself calls transient held **2,812
	 * captures and 71 MB, six days old**, measured 2026-08-09. Nothing reported it and `.gitignore`
	 * hid it ([[ignored-content-is-invisible-loss]]).
	 *
	 * **RUNS BEFORE THE CAPTURE IT IS MAKING ROOM FOR**, which is why it needs no exemption for the
	 * current capture id: the new directory does not exist yet, so it cannot be pruned by its own
	 * write. The alternative — prune after, skipping the id just written — is one more thing to get
	 * wrong for no gain.
	 *
	 * **THE CAP IS COUNT, NOT AGE, AND THAT IS DELIBERATE.** An age cap deletes the only capture of
	 * somebody who renders once a week; a count cap bounds the store whatever the usage pattern. 256
	 * because `mode:"screen_to_world"` traces from a capture ALREADY TAKEN, so the window has to
	 * outlast a working session comfortably rather than merely be finite.
	 *
	 * **CAPTURES WRITTEN IN THE SAME SECOND ARE NOT ORDERABLE, measured rather than assumed:** six
	 * written 60 ms apart carried TWO distinct timestamps, because the mtime available here has about
	 * one-second resolution. Ties are therefore broken arbitrarily. That is harmless at a cap of 256 —
	 * a tie only decides anything at the boundary, which is 256 captures back — but it is stated so
	 * nobody later reads this as a total order and builds on it.
	 *
	 * Returns how many capture directories it removed. Never touches `.probe`, which is a different
	 * bucket with a different owner.
	 *
	 * **`Root` EXISTS FOR THE SPEC AND THE REASON IS THE FEATURE ITSELF.** A prune spec pointed at the
	 * real root would delete the user's captures to prove it can — a test whose setup is the damage
	 * it is checking for. Empty means `TransientRoot()`, which is what every caller passes.
	 */
	static int32 PruneTransient(int32 KeepMostRecent = TransientKeep, const FString& Root = FString());
};
