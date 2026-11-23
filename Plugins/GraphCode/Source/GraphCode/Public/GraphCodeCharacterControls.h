// Copyright OraStudio. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "GraphCodeRigProgram.h"

class USkeleton;

/** A calibrated output at control weights zero and one. Angles are rest-relative degrees,
 * translations are local centimetres, and curves use their native units. */
struct GRAPHCODE_API FGcCharacterChannel
{
	FString Target;
	FString Channel;
	double Start = 0;
	double End = 0;
	bool IsCurve() const { return Channel == TEXT("curve"); }
	bool IsTranslation() const { return Channel.StartsWith(TEXT("move")); }
	FString Address() const { return IsCurve() ? TEXT("curve.") + Target : Target + TEXT(".") + Channel; }
};

struct GRAPHCODE_API FGcCharacterControl
{
	FString Name;
	TArray<FGcCharacterChannel> Channels;
	int32 Line = 0;
};

/** A contact point in the skeletal component's reference space. Rig resolves Point from points{}. */
struct GRAPHCODE_API FGcCharacterContact
{
	FString Name;
	FString Chain;
	FString Point;
	FVector At = FVector::ZeroVector;
	/** The bone the point rides (`bone=foot_r`); empty is the chain's goal bone. Only plant(..., roll=) reads a point on an inner bone. */
	FString Bone;
	/** `[radius=, cone=]` (2026-10-05, GC-MOVE-ARM-LADDER-PLAN.md 9.14): the pad is the patch of the chain's end skin within
	 * PadRadius cm of the point whose drawn normals lie within PadCone degrees of the point's own. Both 0: a point. */
	double PadRadius = 0.0;
	double PadCone = 0.0;
	int32 Line = 0;
};

/** A region of skin (`.gcrig` regions { }): where its centre lies on the skin, in reference component space. A .gcmove
 * reads the skin there as it is in a pose, so `contact($Gun.Web, on=web_fold_r)` follows the skin as a thumb opens. */
struct GRAPHCODE_API FGcCharacterRegion
{
	FString Name;
	FVector At = FVector::ZeroVector;
};

/** Legacy participating chains, or named available contacts; neither prescribes joint angles. */
struct GRAPHCODE_API FGcCharacterGrip
{
	FString Name;
	TArray<FString> Chains;
	TArray<FString> Contacts;
	TArray<FString> Required;
	int32 Line = 0;
};

/** A joint's reference motion, `.gcrig` `velocity:`/`accel:`/`ramp:` (header) and `[velocity=, accel=, ramp=]` (joint):
 * this character's normal full effort, 100%, in deg/s, deg/s^2 and seconds. Unset is unknown, never defaulted.
 * `[twist_velocity=, twist_accel=]` (joint only, 2026-11-20): the same for the joint's turn ABOUT ITS OWN LENGTH alone, where a
 * joint's twist is measured far faster than its swing (a shoulder's internal rotation, Docs/Research/joint-speed-and-acceleration-
 * references.md section 4). Unset, the twist is measured against Velocity / Accel with the rest of the turn, exactly as before. */
struct GRAPHCODE_API FGcCharacterMotion
{
	FString Bone;   // empty for the file-wide default
	TOptional<double> Velocity, Accel, Ramp;
	TOptional<double> TwistVelocity, TwistAccel;
	bool IsSet() const { return Velocity.IsSet() || Accel.IsSet() || Ramp.IsSet() || TwistVelocity.IsSet() || TwistAccel.IsSet(); }
	bool HasTwist() const { return TwistVelocity.IsSet() || TwistAccel.IsSet(); }
};

/** The whole body's reference motion, `.gcrig` `body_accel:`/`body_speed:` (header only; the body is not a joint): how hard
 * this character's feet speed its body up or brake it, cm/s^2, and its normal full-effort running speed, cm/s
 * (GC-MOVE-ROOT-STATE-PLAN.md, MOVE-ROOT1 R4). References, not maximums; unset is unknown, never defaulted. */
struct GRAPHCODE_API FGcCharacterBody
{
	TOptional<double> Accel, Speed;
	bool IsSet() const { return Accel.IsSet() || Speed.IsSet(); }
};

/** A declared animation default. Rotation is a local offset from the unchanged native bind. */
struct GRAPHCODE_API FGcCharacterRest
{
	FString Bone;
	TOptional<double> Fold, Side;
	FQuat Rotation = FQuat::Identity;
	/** Degrees about the bone's own length, stated or worked out by the rig (`rest=(twist=forward)`); the Rotation carries it either way. */
	TOptional<double> Twist;
};

/** A morph whose weight a joint's fold sets at runtime (`.gcrig` `morph(follows=elbow_L.fold, range=)`, GC-MORPH-DESIGN.md §3.5):
 * its owner is the joint, so it has no control, and a clip's drive() naming its curve is refused naming the joint. */
struct GRAPHCODE_API FGcCharacterFollowed
{
	FString Curve;
	/** The joint and its freedom, `elbow_L.fold`. */
	FString Joint;
};

/** Shared authoring contract. Rig owns calibration; Move consumes it without depending on Rig.
 * Records live in editor metadata and never introduce a runtime GraphCode dependency. */
struct GRAPHCODE_API FGcCharacterControls
{
	TArray<FGcCharacterControl> Controls;
	TArray<FGcCharacterContact> Contacts;
	TArray<FGcCharacterGrip> Grips;
	TArray<FGcCharacterRegion> Regions;
	TArray<FGcCharacterRest> Rest;
	/** Correctives: curves a joint's fold sets at runtime, which no control may play. */
	TArray<FGcCharacterFollowed> Followed;
	const FGcCharacterFollowed* FindFollowed(const FString& Curve) const;
	/** This character's normal full effort (GC-MOVE-KINEMATIC-DIAGNOSTICS-PLAN.md): the file-wide default, and one entry per
	 * joint that states any key. A reference to measure a clip against, never a limit. Rig fills both in one function
	 * (`GraphCodeRigControls::MotionOf`), for a built skeleton and for a `controls:` adapter alike. */
	FGcCharacterMotion MotionDefault;
	TArray<FGcCharacterMotion> Motion;
	/** The body's own references, filled by the same `MotionOf`. One per character, so no per-bone list. */
	FGcCharacterBody Body;
	/** A joint's reference, per key: its own value where it states one, else the default's. A key neither states is unset. */
	FGcCharacterMotion ReferenceOf(const FString& Bone) const;
	/** What a `controls:` adapter's joints declare, as its build would record them, and the mesh its places were read
	 * from (GC-MOVE-METAHUMAN-BODY-ADAPTER-PLAN.md item 2). Empty for a controls-only adapter; never written to, nor
	 * read from, a skeleton — `FGcRigAxisRecord` is that record's own carrier. */
	FGcRigAxisRecord Axes;
	FString Mesh;
	const FGcCharacterContact* FindContact(const FString& Name) const;
	const FGcCharacterRegion* FindRegion(const FString& Name) const;
	static bool ParseContact(const FString& Name, const FString& Value, FGcCharacterContact& Out,
		FString& Error, bool bResolvedRecord = false);
	/** Empty/all or explicit name list. Automatic selection is deliberately not implemented here. */
	bool SelectContacts(const FGcCharacterGrip& Grip, const FString& Selection,
		TArray<FGcCharacterContact>& Out, FString& Error) const;
	const FGcCharacterGrip* FindGrip(const FString& Name) const;
	static bool ParseGrip(const FString& Name, const FString& Value, FGcCharacterGrip& Out, FString& Error);
	const FGcCharacterControl* Find(const FString& Name) const;
	static bool ParseControl(const FString& Name, const FString& Value, FGcCharacterControl& Out, FString& Error);
	static FString ToText(const FGcCharacterControls& Record);
	static bool FromText(const FString& Text, FGcCharacterControls& Out, FString& Error);
	static bool Read(const UObject* Skeleton, FGcCharacterControls& Out, FString& Error);
	static void Write(UObject* Skeleton, const FGcCharacterControls& Record);
	static bool Axis(const USkeleton& Skeleton, const FGcCharacterChannel& Channel, FVector& Out, FString& Error);

	/** Optional product seam for an explicitly referenced .gcrig adapter. No asset is changed. */
	using FReader = TDelegate<bool(const FString&, FGcCharacterControls&, FString&)>;
	static void RegisterReader(FReader Reader);
	static void UnregisterReader();
	static bool ReadAdapter(const FString& Path, FGcCharacterControls& Out, FString& Error);
};
