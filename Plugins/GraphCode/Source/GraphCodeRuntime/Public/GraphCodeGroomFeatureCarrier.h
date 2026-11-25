// Copyright GraphCode Sdn Bhd. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "Engine/AssetUserData.h"

#include "GraphCodeGroomFeatureCarrier.generated.h"

/**
 * One named GC Groom attachment frame stored on the generated Groom asset.
 *
 * ReferenceTransform is always in target-mesh reference space. A socket on rigidly skinned
 * support additionally stores the same frame relative to its owning bone, allowing an ordinary
 * scene component attachment to follow animation without sampling Groom simulation or Niagara.
 */
USTRUCT(BlueprintType)
struct GRAPHCODERUNTIME_API FGraphCodeGroomFeatureSocket
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GraphCode|Groom")
	FName Name = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GraphCode|Groom")
	FTransform ReferenceTransform = FTransform::Identity;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GraphCode|Groom")
	bool bBoneBound = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GraphCode|Groom")
	FName ParentBone = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GraphCode|Groom")
	FTransform BoneRelativeTransform = FTransform::Identity;

	/** Effective support weight carried by ParentBone; 1 means the support is rigidly skinned. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GraphCode|Groom")
	double ParentBoneWeight = 0.0;

	/** Stable authoring identity used for diagnostics and rebuild comparison, not attachment lookup. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GraphCode|Groom")
	FString InvocationIdentity;
};

/** Cooked feature-socket payload owned by a generated Groom through AssetUserData. */
UCLASS(BlueprintType)
class GRAPHCODERUNTIME_API UGraphCodeGroomFeatureData : public UAssetUserData
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GraphCode|Groom")
	TArray<FGraphCodeGroomFeatureSocket> Sockets;

	const FGraphCodeGroomFeatureSocket* FindSocket(FName Name) const;
};

/**
 * Runtime, Niagara-free attachment carrier for GC Groom feature sockets.
 *
 * Assign the generated Groom asset to FeatureSource and attach this component to the same target
 * mesh component as the Groom. Static/reference-space sockets follow the carrier. Rigid-bone sockets
 * query the attached skinned component's current bone transform. This deliberately does not claim to
 * follow the final per-frame simulated Groom guides.
 */
UCLASS(ClassGroup=(GraphCode), BlueprintType, meta=(BlueprintSpawnableComponent))
class GRAPHCODERUNTIME_API UGraphCodeGroomFeatureCarrierComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	UGraphCodeGroomFeatureCarrierComponent();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="GraphCode|Groom",
		meta=(AllowedClasses="/Script/HairStrandsCore.GroomAsset"))
	TObjectPtr<UObject> FeatureSource = nullptr;

	UFUNCTION(BlueprintCallable, Category="GraphCode|Groom")
	void SetFeatureSource(UObject* InFeatureSource);

	/** Checked alternative to GetSocketTransform: false distinguishes an unknown or unbound socket. */
	UFUNCTION(BlueprintPure, Category="GraphCode|Groom")
	bool FindFeatureSocketTransform(FName SocketName, FTransform& OutWorldTransform) const;

	virtual FTransform GetSocketTransform(FName InSocketName,
		ERelativeTransformSpace TransformSpace = RTS_World) const override;
	virtual bool DoesSocketExist(FName InSocketName) const override;
	virtual bool HasAnySockets() const override;
	virtual void QuerySupportedSockets(TArray<FComponentSocketDescription>& OutSockets) const override;

private:
	const UGraphCodeGroomFeatureData* GetFeatureData() const;
	bool ResolveSocketWorldTransform(FName SocketName, FTransform& OutWorldTransform) const;
};
