// Copyright OraStudio. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"

/**
 * MOVE-PARAM1 SLICE 1: THE POSE PANEL'S CONTRACT (Docs/GC-MOVE-POSE-PANEL-DESIGN.md "Version 2"; Docs/GC-MOVE-POSE-PANEL-INTEGRATION.md section 0).
 *
 * Two things, nothing else:
 *  1. FGcMovePosePanelModel: what the panel DRAWS. Plain data, no logic. Whoever hosts the panel fills it (the parameter pass reads a real .gcmove into
 *     it); the panel only reads it.
 *  2. IGcMovePosePanelHost: what the panel CALLS. The panel never parses, builds, poses or saves anything itself; every act of the person is one call
 *     here. After any change the host broadcasts OnModelChanged() and the panel re-reads Model() whole.
 *
 * A panel is named by its Path, a row by its Name inside that panel: every host call takes (PanelPath, RowName).
 */

/** What kind of control a row is. */
enum class EGcMovePoseRowKind : uint8
{
	/** A number: slider and number box. */
	Number,
	/** true / false: a check box. Text is "true" or "false". */
	Switch,
	/** One word of a few (hand=right, frame=character): a small choice field. The words are in Choices (only the words the preset's body compares
	 *  the value to); an EMPTY Choices list = the word is shown as text, not editable. */
	Word,
	/** A place on the body (`to=$Tip`): shown as its Text, not editable in slice 1. */
	Place,
	/** ROUND 4 (timing): a RESULT the last build printed (the action's length, a stage's seconds, the point's top speed): Name is what it is, Text the
	 *  number with its unit, read-only, plain small text. Tip is the build's own note it was read from; bStopped marks a note the build said as a WARNING. */
	Result,
};

/** Who decides a row's value. */
enum class EGcMovePoseRowState : uint8
{
	/** A ROOT panel's own value (the animation's or the opened preset's): always live, no override box. */
	Own,
	/** A child's value not written: it follows its parent (FollowsWhat says what). Greyed, not draggable; moves when the parent moves. */
	Follows,
	/** A child's value the person ticked and wrote: live, his number, written at the call. */
	Overridden,
	/** A value not asked (none) with nothing to follow: an empty box and a dash. Ticking it gives it a value (the host picks the start). */
	NotAsked,
	/** A child's value that is a plain number not the person's: written by the PARENT's body at its call of the child (`$Back(lean=20)` inside a
	 *  library preset), or the child's plain default. Drawn like Follows (greyed, not draggable, tick to override) but it does NOT move with the
	 *  parent; FollowsWhat says where the number comes from ("written in Punch: 20", "default 10"). */
	Fixed,
};

/** The work's state, said on the panel's one state line. */
enum class EGcMovePoseWork : uint8
{
	/** Nothing changed since the file was opened or saved. */
	Clean,
	/** Values changed, nothing built yet. */
	Dragging,
	/** Apply is building the trial: Progress 0..1 and WorkLine. */
	Building,
	/** Apply is queued behind another build (WorkLine may say whose). Apply reads Cancel. */
	Waiting,
	/** A trial is built and playable; the saved animation is untouched. */
	Trial,
	/** Written to the file and the animation overridden. */
	Saved,
	/** The build refused; WorkLine is its sentence. Any earlier trial is no longer playable (ruling 7). */
	Refused,
};

/** One row: a name, a control, a number. */
struct FGcMovePoseRow
{
	/** The value's name as written in the preset (`lean`, `curl`, `hand`). */
	FString Name;
	EGcMovePoseRowKind Kind = EGcMovePoseRowKind::Number;
	/** Word rows: the words that may be chosen, in order. */
	TArray<FString> Choices;

	/** The value as text: a number as the build would print it ("0.62"), a word ("right"), "true"/"false"; empty when NotAsked. */
	FString Text;
	/** The value as a number (Number rows; Switch rows 1/0). Meaningless when NotAsked. */
	double Value = 0.0;

	/** The slider's ends (the value's x_min / x_max as they come to). Without a range the panel shows a number box only. */
	bool bHasRange = false;
	double Min = 0.0;
	double Max = 1.0;

	EGcMovePoseRowState State = EGcMovePoseRowState::Follows;
	/** Follows / Fixed / Overridden: what it follows or where its number comes from, for the hover tip ("Stance.lean", "curl", "0.95 * (1 - (1 - lean/100)^1.74)"). */
	FString FollowsWhat;
	/** The value's description from the preset, for the hover tip on its name. May be empty. */
	FString Tip;

	/** False: the override tick is drawn but cannot be ticked (a not-asked value whose start the host cannot give). */
	bool bCanTick = true;

	/** The build stopped a joint (or left a chain short) on this value: a small warning mark on the row, the build's sentence as its tip.
	 *  Which BONES are at a stop (drawn in the viewport) is the host's business; the row carries only this mark and the sentence. */
	bool bStopped = false;
	FString StopSaid;

	/** A saved override whose name names nothing any more (the preset lost the value): drawn struck, only untick is offered. */
	bool bOrphan = false;

	/** ROUND 5: a Result row from a build of OTHER values than those on screen (changed since): drawn dimmed until Apply builds them. */
	bool bStale = false;

	/** CODER-NOTES-6 (the Timing group's rows of every kind): a row that is NOT a value of the clip ("timed by"):
	 *  no override tick is drawn (its column stays empty so names align); its control writes through the host as any row's does. */
	bool bNoTick = false;

	/** RULING 106 SLICE 1: the joints this value moves that a bone override of the pose replaces ("upperarm_r, lowerarm_r"): the row says
	 *  "overridden" and the tip names them. Empty: none. */
	FString OverriddenBy;
	/** Ruling 107 (the Timing group in words a person understands): the name DRAWN for the row when it is set ("Stance to Windup", "Fastest at"); Name stays
	 *  the value's name every host call takes. Empty: Name is drawn. */
	FString Label;
	/** Ruling 107: a short result drawn right of the control, quiet (a stage's seconds from the last build, "0.360 s, bound by speed"). Empty: nothing. Dimmed
	 *  with bStale. */
	FString Beside;
	/** Ruling 107: the tip's second line, drawn grey under Tip: where the value is saved (Tip says what it does to the motion). Empty: none. */
	FString TipWhere;
	/** UX ROUND 4 (the owner, 20 Nov: "anything modified compared to the save file, show it as modified, and also, have a revert button"): what is
	 *  written for this value differs from the SAVED file on disk (its number at its call, a bone override's word, a timing value at the build line). A
	 *  value that only moves with its parent is not modified. SavedText: the saved file's own text for it, bSavedWritten false when the saved file
	 *  does not write it (reverting unwrites it: it follows again). */
	bool bModified = false;
	bool bSavedWritten = false;
	FString SavedText;
};

/**
 * One collapsible panel: a preset as called at its place in the tree. The tree is FLAT, depth first: a panel's children follow it and carry its
 * index as Parent. Panels[0] is the root (the animation or the opened preset). Same preset called twice = two panels ("Hand, right", "Hand, left").
 */
struct FGcMovePosePanel
{
	/** As shown in the header ("Hand, right"). */
	FString Name;
	/** From the root, dotted, using the call's label where the parent calls one preset twice: "" for the root, "Hand", "ArmReach.Forearm", "HandR".
	 *  Until call labels exist, a second call of one preset in one parent is "Punch.Hand#2" (an id only; its Name reads "Hand, 2"). */
	FString Path;
	/** Index of the parent panel in FGcMovePosePanelModel::Panels; INDEX_NONE for the root. */
	int32 Parent = INDEX_NONE;
	/** 0 for the root, 1 for its children ... (saves the panel a walk). */
	int32 Depth = 0;
	/** False: this child is not used at these values (its call sits under an `if` not taken). Drawn dimmed; its rows cannot be ticked. */
	bool bLive = true;
	/** The preset's description, for the hover tip on the header. May be empty. */
	FString Tip;
	/** Ruling 107: the header tip's second line, grey (where the panel's values are saved). Empty: none. */
	FString TipWhere;
	TArray<FGcMovePoseRow> Rows;

	/** ROUND 4 (timing): a top panel whose rows carry the override tick all the same (the Timing group: ticked = written at the build line's call,
	 *  unticked = the clip's own default). False: only a child's rows have the tick. */
	bool bTicks = false;
	/** ROUND 4: one plain line under the rows ("This action's timing is written as fixed numbers."), and a button beside it when ButtonText is set; the
	 *  button calls IGcMovePosePanelHost::PressPanelButton(Path). Both empty: nothing is drawn. */
	FString Line;
	FString ButtonText;
	/** UX round 4: the modified values in this group and the groups under it ("Back · 2 changed"); RevertWhyNot: why the group's values cannot be
	 *  written back from here (the tab's write guard), empty when they can. */
	int32 Changed = 0;
	FString RevertWhyNot;
};

/** Everything the panel draws. */
/** UX ROUND 3 (the owner, 20 Nov: "should have a button like show in editor for the animations isn't it? Or else how do I know where's my animation"):
 *  one animation of the chosen pose on disk (or where it will be), with Show in Content Browser and Open beside it. */
struct FGcMovePoseBuilt
{
	/** What it is, plainly: "This pose", "The motion", "The animation", "Copy for Clothed MetaHuman". */
	FString What;
	/** The asset's long package name (`/Game/.../A_PresetSlash_Manny`): where it is, or where the build writes it. */
	FString Package;
	/** It is on disk: Show and Open are on. */
	bool bBuilt = false;
	/** REVIEW-UXR3 3: a build that rewrites it is running: the row is grey, Show and Open off until it is done (a copy is never rewritten by a build). */
	bool bWaiting = false;
	/** Built: where, in plain words ("Saved A_PresetSlash_Manny in /Game/GraphCodeExamples/PresetSlash_20261118"). Not built: why the buttons are off and
	 *  what to do ("Not built yet. Press Build now."). */
	FString Line;
};

struct FGcMovePosePanelModel
{
	/** The open file (a long package name, `/Game/.../Tune6`), empty when none. */
	FString File;
	/** Files the host offers in the picker (long package names). May be empty: the field also takes a typed name. ROUND 5 R7: every `.gcmove` in
	 *  the picker's scope (PickScope), sorted by path; the search filters them (GcMovePosePick::Filter). */
	TArray<FString> Files;
	/** ROUND 5 R7 (the owner: "when there's many animations it will be hard to find?"): the files opened in this tab, newest first (kept across
	 *  sessions), the files of the open file's folder, the scope ("" = the whole project), and whether the background scan has finished. */
	TArray<FString> RecentFiles;
	TArray<FString> FolderFiles;
	FString PickScope;
	bool bFilesReady = false;
	/** The animation (clip) being edited, as named in the file ("KZ6", "ClawSlash"). */
	FString Animation;

	/** The animation's poses in order, plain names ("stance", "wind-up", "contact"). A stance has one. */
	TArray<FString> Poses;
	/** One entry per Poses entry, or empty: entries with the SAME group number are one pose used twice (one thing to edit: choosing either
	 *  lights both). Empty = every entry is its own pose. */
	TArray<int32> PoseGroups;
	/** Which pose the tree below is, and the character shows. INDEX_NONE when none. */
	int32 ChosenPose = INDEX_NONE;
	/** True while the whole animation plays (Play becomes Stop). */
	bool bPlaying = false;
	/** Play plays the trial (true) or the saved animation (false). */
	bool bPlaysTrial = false;

	/** A built trial exists (Refused leaves none). */
	bool bHasTrial = false;
	/** The trial is older than the values on screen ("Dragging, trial is older" against "Dragging, nothing built"). */
	bool bTrialStale = false;

	/** The viewport's "as written / as played" switch: shown only when this is true; bAsPlayed is its position. */
	bool bCanShowAsPlayed = false;
	bool bAsPlayed = false;

	/** The chosen pose's tree (see FGcMovePosePanel). Empty when no file is open. */
	TArray<FGcMovePosePanel> Panels;

	EGcMovePoseWork Work = EGcMovePoseWork::Clean;
	/** Building: 0..1 (Waiting is its own state, not Progress 0). */
	float Progress = 0.f;
	/** Building: what it is doing ("solving contact 3 of 6"). Waiting: whose build it waits behind. Refused: the build's sentence. Otherwise optional. */
	FString WorkLine;

	/** ROUND 2 (the owner, 17 Nov: "have UI to show it's loading the pose"): the host is about to solve, or is solving, a pose that takes a while
	 *  (over 0.25 s, or not yet known): PosingLine ("Posing KContact... (about 2.7 s)") shows on the state line and in the viewport until it is done.
	 *  The solve runs on a later frame, after this has been drawn. */
	bool bPosing = false;
	FString PosingLine;
	/** A short line for the viewport while a value of a pose too slow to follow is dragged ("Let go to pose"); empty otherwise. */
	FString HintLine;
	/** The viewport's Select rig toggle (ruling 105; off by default): the joints the pose uses drawn and picked; a selection filters Panels. */
	bool bShowRig = false;
	/** ROUND 3, the scrubber under the viewport: the animation on the body is held still (bPlaying stays true while paused). */
	bool bPaused = false;
	/** ROUND 4 (timing): the pose strip ends with one more chip, "Timing" (a keys file with an action beside it built from these keys); while it is
	 *  chosen (bTimingChosen) the tree is that action's Timing group and no pose chip is lit. */
	bool bHasTimingChip = false;
	bool bTimingChosen = false;

	bool bCanUndo = false;
	bool bCanRedo = false;
	/** Which buttons are on (the host decides, per the state table of the design, Version 3 section 4). */
	bool bCanPlay = false;
	bool bCanApply = false;
	bool bCanSave = false;
	bool bCanDiscard = false;
	/** RULING 106 SLICE 1: "Convert to presets" writes the chosen pose again as preset calls and bone overrides (a trial on screen; Apply builds it, Save writes it). */
	bool bCanToPresets = false;
	/** The owner, 19 Nov ("why is there button to convert to preset when there's already preset?"): the button is shown only on a key that has something
	 * to convert, a `source=` frame or a direct pose line (swing, aim, shift) other than a bone override; a key of preset calls only has none. */
	bool bShowToPresets = false;
	/** The owner, 19 Nov ("it should be loading the animation, not rebuilding it every time"; REVIEW-LOAD-NOT-BUILD.md): the tab opens the saved
	 * animation, never builds it. When the saved one is not what the file builds now (the file, a presets: library, the skeleton or a face's body
	 * changed since, or it was never built), StaleLine says so ("The saved animation is older than the file") and Build now builds it from the file. */
	FString StaleLine;
	bool bCanBuildNow = false;
	/** POSE TAB UX STEP 3 (Docs/GC-MOVE-POSE-UX.md 3.3; the lead, 19 Nov): a warning that matters, shown on its pose in one plain sentence. Only four
	 * kinds: a prop held in an open hand, a planted foot off the floor, a hold missed by more than 1 cm, a refusal. PoseWarnings is parallel to Poses
	 * (that pose's sentences, one a line; empty: none), Warnings the chosen pose's, one sentence each. */
	TArray<FString> PoseWarnings;
	TArray<FString> Warnings;
	/** UX round 2 (PANEL-RETEST-1119 item 4): why the stage is empty, in plain words and what to do ("This file names no character to show ..."); empty
	 *  when a character stands on it. The viewport says it over the empty stage. */
	FString StageLine;
	/** UX round 3 (the owner, 20 Nov): the chosen pose's animations, each with Show in Content Browser and Open: its own clip, the motion played from it
	 *  (the action beside a keys file, or a timed clip of the same file), and every copy Save for <character> wrote beside them. */
	TArray<FGcMovePoseBuilt> Built;
};

DECLARE_MULTICAST_DELEGATE(FGcMovePosePanelChanged);

/**
 * What the panel calls. Implemented by the tab that hosts the panel (the pose view) and by the panel's demo (a fake). Every call may change the model;
 * the host then broadcasts OnModelChanged(). Calls naming a panel or row that does not exist are ignored by the host.
 */
class IGcMovePosePanelHost
{
public:
	virtual ~IGcMovePosePanelHost() = default;

	/** The model as it stands now. The panel re-reads it on every OnModelChanged(). */
	virtual const FGcMovePosePanelModel& GetModel() const = 0;
	/** Broadcast after ANY change of the model (the panel's own calls included). */
	virtual FGcMovePosePanelChanged& OnModelChanged() = 0;

	/** Open a .gcmove (long package name). The host fills Animation, Poses, chooses the first pose and fills Panels. */
	virtual void OpenFile(const FString& File) = 0;
	/** Put the character in that pose; the tree becomes that pose's. */
	virtual void ChoosePose(int32 Index) = 0;
	/** Play the whole animation (the trial if one is built, else the saved one); called again while playing, stop. */
	virtual void Play() = 0;

	/** The pointer rests on a live row: the host draws that row's bones faintly. EndHover when it leaves. */
	virtual void HoverRow(const FString& PanelPath, const FString& Row) = 0;
	virtual void EndHover() = 0;

	/** The person pressed a slider (or began a typed entry) on a live row: the host keeps the start for undo and shows that row's rig. */
	virtual void BeginDrag(const FString& PanelPath, const FString& Row) = 0;
	/** While dragging: the new value. The host re-poses and recomputes the rows that follow. Work becomes Dragging. */
	virtual void DragValue(const FString& PanelPath, const FString& Row, double Value) = 0;
	/** Released: one undo step from BeginDrag's value to the last DragValue's. The rig of the row is hidden again. */
	virtual void EndDrag(const FString& PanelPath, const FString& Row) = 0;

	/** Tick (true) = the person's own value from now (starts at the value it showed); untick (false) = give it back to the parent / to none. */
	virtual void SetOverride(const FString& PanelPath, const FString& Row, bool bOverride) = 0;
	/** A Word row's new word, or a Switch row's "true"/"false". One undo step. */
	virtual void SetWord(const FString& PanelPath, const FString& Row, const FString& Word) = 0;

	/** Build a trial of the whole animation (Building -> Trial, or Refused). */
	virtual void Apply() = 0;
	/** Stop a build in progress or waiting (Building / Waiting -> back to the state before Apply). */
	virtual void Cancel() = 0;
	/** Write the ticked values into the file and override the animation (-> Saved). */
	virtual void Save() = 0;
	/** Drop every change since the last save and any trial (-> Clean). */
	virtual void Discard() = 0;

	/** The viewport's switch: show the pose as played (true) or as written (false). */
	virtual void SetAsPlayed(bool bAsPlayed) = 0;

	/** RULING 106 SLICE 1, "Convert to presets": the chosen pose written again as preset calls with values and a bone override per bone still off (the
	 *  whole-key writer); shown on the body at once, one undo step, built by Apply and written by Save. Not pure: the demo has no presets. */
	virtual void ToPresets() {}
	/** Build the file as saved (the owner, 19 Nov: the stale saved animation's "Build now"); nothing is written to the file. */
	virtual void BuildNow() {}
	/** UX round 4 (the owner, 20 Nov): one value back to the saved file's (FGcMovePoseRow::SavedText), or every modified value of a group and the groups
	 *  under it; each press one undo step, the pose solved again as after a drag. */
	virtual void RevertRow(const FString& PanelPath, const FString& Row) {}
	virtual void RevertPanel(const FString& PanelPath) {}
	/** UX round 3 (the owner, 20 Nov): Built[Index] in Unreal's Content Browser (its folder opened, the asset selected), and opened in Unreal's animation
	 *  editor. Nothing while it is not built. */
	virtual void ShowBuiltInBrowser(int32 Index) {}
	virtual void OpenBuiltAsset(int32 Index) {}

	virtual void Undo() = 0;
	virtual void Redo() = 0;

	// ROUND 3 (the owner, 17 Nov: "right click to copy ... let's say the head, to be used in the previous stance too"). Not pure: a host without
	// them (the demo) copies nothing.
	/** The values written at a panel's call, as the call's own text (`$Head(turn=-5, nod=5)`; a child's by its path, `Look.x=1`). Empty: none. */
	virtual FString CopyPanel(const FString& PanelPath) { return FString(); }
	/** Write a copied call's values at this panel's call (the same preset only). One undo step. False with why in the model's WorkLine. */
	virtual bool PastePanel(const FString& PanelPath, const FString& CallText) { return false; }
	/** The same panel's values written at the same call of another pose of the file (by index into Poses). One undo step. */
	virtual bool CopyPanelToPose(const FString& PanelPath, int32 Pose) { return false; }

	// ROUND 4 (the owner, 17 Nov: "let users able to control the timing on the panel as well"). Not pure: the demo has no timing.
	/** The strip's "Timing" chip: the tree becomes the action's Timing group (ChoosePose goes back to a pose). */
	virtual void ChooseTiming() {}
	/** The button a panel draws beside its Line (FGcMovePosePanel::ButtonText). */
	virtual void PressPanelButton(const FString& PanelPath) {}

	// ROUND 5 R7, the file picker. Not pure: the demo has no files.
	/** The picker opened: the host lists the project's `.gcmove` files (once, in the background) and fills Files when done. */
	virtual void ScanFiles() {}
	/** The folder the search looks in (`/Game/...`), or "" for the whole project; remembered per project. */
	virtual void SetPickScope(const FString& Folder) {}
};

/** ROUND 5 R7: the picker's search, shared by the panel, the host's watcher and the specs. */
namespace GcMovePosePick
{
	/** Every word of Query (split at blanks, any order, any case) is in the file's package path (its folder or its name). An empty query matches all. */
	inline bool Matches(const FString& Package, const FString& Query)
	{
		TArray<FString> Words;
		Query.ParseIntoArrayWS(Words);
		for (const FString& Word : Words) if (!Package.Contains(Word, ESearchCase::IgnoreCase)) return false;
		return true;
	}

	/** The files matching Query, best first: every word in the file's NAME before a match through its folder; then shorter names; at most Max. */
	inline TArray<FString> Filter(const TArray<FString>& Files, const FString& Query, const int32 Max = 200)
	{
		TArray<FString> Words;
		Query.ParseIntoArrayWS(Words);
		TArray<TPair<int32, FString>> Ranked;
		for (const FString& File : Files)
		{
			if (!Matches(File, Query)) continue;
			int32 Slash = INDEX_NONE;
			const FString Name = File.FindLastChar(TEXT('/'), Slash) ? File.Mid(Slash + 1) : File;
			bool bInName = true;
			for (const FString& Word : Words) bInName &= Name.Contains(Word, ESearchCase::IgnoreCase);
			Ranked.Emplace((bInName ? 0 : 100000) + Name.Len(), File);
		}
		Ranked.StableSort([](const TPair<int32, FString>& A, const TPair<int32, FString>& B) { return A.Key < B.Key; });
		TArray<FString> Out;
		for (int32 Index = 0; Index < Ranked.Num() && Out.Num() < Max; ++Index) Out.Add(Ranked[Index].Value);
		return Out;
	}
}
