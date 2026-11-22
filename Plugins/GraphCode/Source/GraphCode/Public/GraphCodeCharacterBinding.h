// Copyright OraStudio. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "UObject/StrongObjectPtr.h"

class USkeletalMesh;
class UPhysicsAsset;
class USceneComponent;

/** A named native component, including attachments which do not themselves carry a skeleton. */
struct GRAPHCODE_API FGcCharacterPart
{
	FString Name;
	FString Parent;
	FName Socket;
	FString ClassPath;
	FTransform Relative = FTransform::Identity;
	USkeletalMesh* Mesh = nullptr;
	FString AnimationClass;
	FString PostProcessClass;
	bool bPostProcessDisabled = false;
};

/** Read-only authoring binding to an existing assembly. The Blueprint remains the authority;
 * no copied skeleton, constructed actor, service request or runtime GraphCode object is needed. */
struct GRAPHCODE_API FGcCharacterBinding
{
	TStrongObjectPtr<UObject> Source;
	TArray<FGcCharacterPart> Parts;

	static bool Read(const FString& Path, FGcCharacterBinding& Out, FString& Error);
	static bool Read(UObject* Asset, FGcCharacterBinding& Out, FString& Error);
	/** An omitted name is unambiguous only when exactly one skeletal part exists. */
	const FGcCharacterPart* Select(const FString& Name, FString& Error) const;
	/** Effective native component asset, including an assembly's PhysicsAssetOverride. */
	UPhysicsAsset* PhysicsFor(const FGcCharacterPart& Part) const;
	/** OBJ1t (Docs/GC-MOVE-DRAWN-BODY-PLAN.md): the parts a `.gcmove`'s `worn:` names as drawn over Part, posed by its bones (leader pose, or an
	 * AnimBP copying its pose) and drawn at its origin. Each must be a skeletal-mesh part on Part's skeleton, attached under Part with no socket
	 * and an identity relative transform at every step of that chain; anything else is refused by name, as an offset follower would need its
	 * component transform applied and is not guessed. That the part really follows Part is the author's statement: templates are read here,
	 * never construction scripts, so an AnimBP's copying cannot be checked. */
	bool Worn(const FGcCharacterPart& Part, TConstArrayView<FString> Names, TArray<const FGcCharacterPart*>& Out, FString& Error) const;
	/** The skeletal parts attached under Part that Named does not hold: on Part's skeleton (Same) or another (Other). Reported, never read. */
	void Undeclared(const FGcCharacterPart& Part, TConstArrayView<FString> Named, TArray<const FGcCharacterPart*>& Same,
		TArray<const FGcCharacterPart*>& Other) const;
	/** Read the native component template without running construction scripts. */
	USceneComponent* ComponentFor(const FString& Name) const;
	/** Selected component, its attachment ancestry, mesh and skeleton rest pose, and calibration. */
	FString Signature(const FGcCharacterPart& Part) const;
	FString Describe() const;
};
