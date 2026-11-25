// Copyright GraphCode Sdn Bhd. All Rights Reserved.
//
// THE ONE PLACE GraphCode asks which engine it is compiled against (Docs/GC-ENGINE-VERSIONS-PLAN.md §1).
// Call sites never test the engine version: they call a function here, and each function picks the
// spelling that engine has. One source tree builds for UE 5.6, 5.7 and 5.8.
//
// Templates on the object's pointer type (raw, TObjectPtr, TStrongObjectPtr all spell `->`), so this header pulls in no UMG/Font/Mesh headers of its own: each
// function is checked where it is called, against the includes that caller already has. Only names
// used without a dependent object (the RHI globals, the quality-level enum) are included here.
//
// Add a function when an engine renames or reshapes an API; delete its 5.6 branch the day 5.6 support
// ends. Never test UE_VERSION_* outside this file; a feature an engine lacks is gated at its entry with
// GRAPHCODE_ENGINE_AT_LEAST (below) and refuses with NeedsNewerEngine.
#pragma once

#include "CoreMinimal.h"
#include "Misc/EngineVersionComparison.h"
#include "RHIFeatureLevel.h"
#include "RHIShaderPlatform.h"
#include "UObject/ObjectMacros.h"
#if UE_VERSION_OLDER_THAN(5, 7, 0)
#include "SceneTypes.h"
#endif
#if UE_VERSION_OLDER_THAN(5, 8, 0)
#include "RenderResource.h"
#else
#include "HAL/IConsoleManager.h"
#include "UObject/UnrealType.h"
#endif

class FFXSystemInterface;

// UE_FORCEINLINE_HINT arrived in 5.7 (HAL/Platform.h: FORCEINLINE, or inline in debug builds).
#ifndef UE_FORCEINLINE_HINT
#define UE_FORCEINLINE_HINT FORCEINLINE
#endif

/**
 * A FEATURE that one engine lacks (not a renamed API: those get a function below) is compiled only
 * where it exists, behind this one macro so every such gate is one search away:
 *
 *     #if GRAPHCODE_ENGINE_AT_LEAST(5, 7)
 *         ... the feature ...
 *     #else
 *         OutError = GraphCodeEngineCompat::NeedsNewerEngine(TEXT("cloth property editing"), 5, 7);
 *         return false;
 *     #endif
 *
 * The verb then refuses by name, as it does for an optional plugin the project has not enabled.
 */
#define GRAPHCODE_ENGINE_AT_LEAST(Major, Minor) UE_VERSION_NEWER_THAN_OR_EQUAL(Major, Minor, 0)

namespace GraphCodeEngineCompat
{
	/** "<Feature> needs Unreal Engine 5.7 or later; this editor is 5.6." */
	inline FString NeedsNewerEngine(const TCHAR* Feature, const int32 Major, const int32 Minor)
	{
		return FString::Printf(TEXT("%s needs Unreal Engine %d.%d or later; this editor is %d.%d."),
			Feature, Major, Minor, ENGINE_MAJOR_VERSION, ENGINE_MINOR_VERSION);
	}

	// ---- UFont: 5.7 made CompositeFont private behind accessors ---------------------------------

	template <typename TFontPtr>
	const auto& CompositeFont(const TFontPtr& Font)
	{
#if UE_VERSION_NEWER_THAN_OR_EQUAL(5, 7, 0)
		return Font->GetInternalCompositeFont();
#else
		return Font->CompositeFont;
#endif
	}

	template <typename TFontPtr>
	auto& MutableCompositeFont(const TFontPtr& Font)
	{
#if UE_VERSION_NEWER_THAN_OR_EQUAL(5, 7, 0)
		return Font->GetMutableInternalCompositeFont();
#else
		return Font->CompositeFont;
#endif
	}

	// ---- UStaticMesh: Nanite settings and customized collision became accessors in 5.7 ---------

	template <typename TMeshPtr>
	decltype(auto) NaniteSettings(const TMeshPtr& Mesh)
	{
#if UE_VERSION_NEWER_THAN_OR_EQUAL(5, 7, 0)
		return Mesh->GetNaniteSettings();
#else
		return (Mesh->NaniteSettings);
#endif
	}

	template <typename TMeshPtr, typename TSettings>
	void SetNaniteSettings(const TMeshPtr& Mesh, const TSettings& Settings)
	{
#if UE_VERSION_NEWER_THAN_OR_EQUAL(5, 7, 0)
		Mesh->SetNaniteSettings(Settings);
#else
		Mesh->NaniteSettings = Settings;
#endif
	}

	template <typename TMeshPtr>
	bool CustomizedCollision(const TMeshPtr& Mesh)
	{
#if UE_VERSION_NEWER_THAN_OR_EQUAL(5, 7, 0)
		return Mesh->GetCustomizedCollision();
#else
		return Mesh->bCustomizedCollision;
#endif
	}

	template <typename TMeshPtr>
	void SetCustomizedCollision(const TMeshPtr& Mesh, const bool bCustomized)
	{
#if UE_VERSION_NEWER_THAN_OR_EQUAL(5, 7, 0)
		Mesh->SetCustomizedCollision(bCustomized);
#else
		Mesh->bCustomizedCollision = bCustomized;
#endif
	}

	// ---- UEditableTextBox: WidgetStyle became accessors in 5.7 ----------------------------------

	template <typename TBoxPtr>
	decltype(auto) EditableTextBoxStyle(const TBoxPtr& Box)
	{
#if UE_VERSION_NEWER_THAN_OR_EQUAL(5, 7, 0)
		return Box->GetWidgetStyle();
#else
		return (Box->WidgetStyle);
#endif
	}

	template <typename TBoxPtr, typename TStyle>
	void SetEditableTextBoxStyle(const TBoxPtr& Box, const TStyle& Style)
	{
#if UE_VERSION_NEWER_THAN_OR_EQUAL(5, 7, 0)
		Box->SetWidgetStyle(Style);
#else
		Box->WidgetStyle = Style;
#endif
	}

	// ---- UMaterialInterface: 5.7 keys resources and used textures by shader platform -------------

	/** The resource the running RHI renders this material with (5.6 asked by feature level). */
	template <typename TMaterialPtr>
	auto* MaterialResource(const TMaterialPtr& Material)
	{
#if UE_VERSION_NEWER_THAN_OR_EQUAL(5, 7, 0)
		return Material->GetMaterialResource(GMaxRHIShaderPlatform);
#else
		return Material->GetMaterialResource(GMaxRHIFeatureLevel);
#endif
	}

	/** Every texture the material uses at any quality level (5.7's defaulted overload). */
	template <typename TMaterialPtr, typename TTextureArray>
	void UsedTextures(const TMaterialPtr& Material, TTextureArray& OutTextures)
	{
#if UE_VERSION_NEWER_THAN_OR_EQUAL(5, 7, 0)
		Material->GetUsedTextures(OutTextures);
#else
		Material->GetUsedTextures(OutTextures, EMaterialQualityLevel::Num, true, GMaxRHIFeatureLevel, true);
#endif
	}

	/** Compile the resource's shaders for the running RHI now (5.6 named the platform explicitly). */
	template <typename TResourcePtr, typename TPrecompileMode>
	bool CacheShaders(const TResourcePtr& Resource, const TPrecompileMode Mode)
	{
#if UE_VERSION_NEWER_THAN_OR_EQUAL(5, 7, 0)
		return Resource->CacheShaders(Mode);
#else
		return Resource->CacheShaders(GMaxRHIShaderPlatform, Mode);
#endif
	}

	// ---- FFXSystemInterface: 5.7 deferred teardown, 5.8 shared ownership -------------------------

	/**
	 * An FX system this code created and owns. 5.8's Create returns a TSharedRef and the creator holds it
	 * (as UWorld does); earlier engines return a raw pointer. Hand the scene RawFXSystem(Owned).
	 */
#if UE_VERSION_NEWER_THAN_OR_EQUAL(5, 8, 0)
	using FOwnedFXSystem = TSharedPtr<FFXSystemInterface>;
#else
	using FOwnedFXSystem = FFXSystemInterface*;
#endif

	template <typename TFXSystem = FFXSystemInterface, typename TFeatureLevel, typename TScene>
	FOwnedFXSystem CreateFXSystem(const TFeatureLevel FeatureLevel, TScene* Scene)
	{
		return TFXSystem::Create(FeatureLevel, Scene);
	}

	inline FFXSystemInterface* RawFXSystem(const FOwnedFXSystem& FXSystem)
	{
#if UE_VERSION_NEWER_THAN_OR_EQUAL(5, 8, 0)
		return FXSystem.Get();
#else
		return FXSystem;
#endif
	}

	/** Release an FX system this code created; destruction completes later on the render thread. */
	template <typename TFXSystem = FFXSystemInterface>
	void ReleaseFXSystem(FOwnedFXSystem& FXSystem)
	{
		if (!FXSystem)
		{
			return;
		}
#if UE_VERSION_NEWER_THAN_OR_EQUAL(5, 7, 0)
		TFXSystem::MarkPendingKill(RawFXSystem(FXSystem));
#else
		TFXSystem::Destroy(RawFXSystem(FXSystem));
#endif
		FXSystem = nullptr;
	}

	// ---- Mip-level fading: a RenderCore global until 5.8, removed in 5.8 -------------------------

	/** Hold mip fading off for a scope when bOff, so a streamed mip is drawn whole (nothing to hold on 5.8). */
	struct FScopedMipLevelFadingOff
	{
		explicit FScopedMipLevelFadingOff(const bool bOff)
		{
#if UE_VERSION_OLDER_THAN(5, 8, 0)
			Saved = GEnableMipLevelFading;
			if (bOff)
			{
				GEnableMipLevelFading = -1.0f;
			}
#endif
		}
		~FScopedMipLevelFadingOff()
		{
#if UE_VERSION_OLDER_THAN(5, 8, 0)
			GEnableMipLevelFading = Saved;
#endif
		}
		FScopedMipLevelFadingOff(const FScopedMipLevelFadingOff&) = delete;
		FScopedMipLevelFadingOff& operator=(const FScopedMipLevelFadingOff&) = delete;
	private:
#if UE_VERSION_OLDER_THAN(5, 8, 0)
		float Saved = 0.0f;
#endif
	};

	// ---- FMeshDescriptionToDynamicMesh: compacted polygon-group ids arrived in 5.7 ---------------

	/**
	 * Convert with material IDs numbered 0..N-1 in polygon-group order, even when the description's
	 * group ids have gaps. 5.7 does it in the converter; on 5.6 the converter writes the raw group id
	 * as the material id, so the same compaction (FMeshDescriptionToDynamicMesh.cpp in 5.7) is applied
	 * to the result afterwards.
	 */
	template <typename TConverter, typename TDescription, typename TDynamicMesh>
	void ConvertWithCompactedPolygonGroups(TConverter& Converter, const TDescription* Description,
		TDynamicMesh& Out, const bool bCopyTangents)
	{
#if UE_VERSION_NEWER_THAN_OR_EQUAL(5, 7, 0)
		Converter.bUseCompactedPolygonGroupIDValues = true;
		Converter.Convert(Description, Out, bCopyTangents);
#else
		Converter.Convert(Description, Out, bCopyTangents);
		const auto& Groups = Description->PolygonGroups();
		if (Groups.Num() == Groups.GetArraySize() || !Out.HasAttributes() || !Out.Attributes()->HasMaterialID())
		{
			return;
		}
		TArray<int32> Compacted;
		Compacted.Init(INDEX_NONE, Groups.GetArraySize());
		int32 Section = 0;
		for (const auto GroupID : Groups.GetElementIDs())
		{
			Compacted[GroupID.GetValue()] = Section++;
		}
		auto* MaterialIDs = Out.Attributes()->GetMaterialID();
		for (const int32 Triangle : Out.TriangleIndicesItr())
		{
			int32 Raw = 0;
			MaterialIDs->GetValue(Triangle, &Raw);
			if (Compacted.IsValidIndex(Raw) && Compacted[Raw] != INDEX_NONE)
			{
				const int32 Mapped = Compacted[Raw];
				MaterialIDs->SetValue(Triangle, &Mapped);
			}
		}
#endif
	}

	// ---- FStringTable::SetSourceString: editor builds of 5.8 take dev notes too ----------------------

	/** Set an entry's source string (replacing the entry, as every engine's SetSourceString does). */
	template <typename TTablePtr, typename TKey>
	void SetStringTableSource(const TTablePtr& Table, const TKey& Key, const FString& Source)
	{
#if UE_VERSION_NEWER_THAN_OR_EQUAL(5, 8, 0) && WITH_EDITORONLY_DATA
		Table->SetSourceString(Key, Source, FString());
#else
		Table->SetSourceString(Key, Source);
#endif
	}

	// ---- TMulticastScriptDelegate: 5.8 renamed ProcessMulticastDelegate to ProcessDelegate -------------
	// (5.8's deprecated forwarder omits the template argument and cannot compile, so 5.8 calls the new name.)

	template <typename TDelegatePtr>
	void ProcessMulticastDelegate(const TDelegatePtr& Delegate, void* Parameters)
	{
#if UE_VERSION_NEWER_THAN_OR_EQUAL(5, 8, 0)
		Delegate->template ProcessDelegate<UObject>(Parameters);
#else
		Delegate->template ProcessMulticastDelegate<UObject>(Parameters);
#endif
	}

	// ---- ALandscape edit layers: GetLayerCount is gone in 5.8 ----------------------------------------

	template <typename TLandscapePtr>
	int32 LandscapeEditLayerCount(const TLandscapePtr& Landscape)
	{
#if UE_VERSION_NEWER_THAN_OR_EQUAL(5, 8, 0)
		return Landscape->GetEditLayersConst().Num();
#else
		return Landscape->GetLayerCount();
#endif
	}

	// ---- UPCGSubgraphSettings: GetOriginalSubgraphInterface arrived in 5.7 ---------------------------
	// Before it, the plain accessor answers the authored graph; the two differ only while a dynamic
	// subgraph is running.

	template <typename TSettingsPtr>
	auto OriginalSubgraphInterface(const TSettingsPtr& Settings)
	{
#if UE_VERSION_NEWER_THAN_OR_EQUAL(5, 7, 0)
		return Settings->GetOriginalSubgraphInterface();
#else
		return Settings->GetSubgraphInterface();
#endif
	}

	// ---- UPCGNode::IsHidden: nodes became hideable in 5.7 (a 5.6 node is never hidden) -------------

	template <typename TNodePtr>
	bool IsPcgNodeHidden(const TNodePtr& Node)
	{
#if UE_VERSION_NEWER_THAN_OR_EQUAL(5, 7, 0)
		return Node->IsHidden();
#else
		(void)Node;
		return false;
#endif
	}

	// ---- AWaterZone::GetRenderTargetResolution: not exported from Water in 5.8 -----------------------
	// 5.8 moved the getter out of line without WATER_API, so a call from another module does not link.
	// The 5.8 branch returns what that getter returns: the RenderTargetResolution property, clamped by
	// r.Water.WaterInfo.RenderTargetResolutionMax when that is above zero (WaterZoneActor.cpp in 5.8).

	template <typename TZonePtr>
	FIntPoint WaterZoneRenderTargetResolution(const TZonePtr& Zone)
	{
#if UE_VERSION_NEWER_THAN_OR_EQUAL(5, 8, 0)
		FIntPoint Resolution(0, 0);
		if (const FStructProperty* Property = FindFProperty<FStructProperty>(Zone->GetClass(), TEXT("RenderTargetResolution")))
		{
			Resolution = *Property->ContainerPtrToValuePtr<FIntPoint>(&*Zone);
		}
		const IConsoleVariable* Max = IConsoleManager::Get().FindConsoleVariable(TEXT("r.Water.WaterInfo.RenderTargetResolutionMax"));
		const int32 Cap = Max ? Max->GetInt() : 0;
		if (Cap > 0)
		{
			Resolution = FIntPoint(FMath::Clamp(Resolution.X, 0, Cap), FMath::Clamp(Resolution.Y, 0, Cap));
		}
		return Resolution;
#else
		return Zone->GetRenderTargetResolution();
#endif
	}

	// ---- FObjectPostSaveContext: IsFromAutoSave arrived in 5.7 -----------------------------------

	template <typename TSaveContext>
	bool IsFromAutoSave(const TSaveContext& Context)
	{
#if UE_VERSION_NEWER_THAN_OR_EQUAL(5, 7, 0)
		return Context.IsFromAutoSave();
#else
		return (Context.GetSaveFlags() & SAVE_FromAutosave) != 0;
#endif
	}
}
