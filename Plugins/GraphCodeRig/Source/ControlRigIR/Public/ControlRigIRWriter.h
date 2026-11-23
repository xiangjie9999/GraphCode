// Copyright (c) 2026. Licensed under the MIT License.
#pragma once

#include "CoreMinimal.h"
#include "ControlRigIRTypes.h"

class UControlRigBlueprint;

namespace ControlRigIR
{
	struct CONTROLRIGIR_API FGraphEdit
	{
		FControlRigIRDocument Document;
		TArray<FString> DeleteVariables;
		TMap<FString, TArray<FString>> DeleteNodes;
		/** Internal function-body replacement, never inferred from a public patch. */
		TSet<FString> ReplaceFunctions;
		bool bPropertiesSupplied = false;
		bool bGraphsSupplied = false;
	};

	/** Parse the complete edit before any native controller is touched. */
	CONTROLRIGIR_API bool ParseGraph(const FString& Body, FGraphEdit& Out, FString& Error);
	CONTROLRIGIR_API bool ParseProperties(const FString& Body, FGraphEdit& Out, FString& Error);
	CONTROLRIGIR_API FString VariableStatement(const FControlRigIRVariable& Variable);
	/** Changes only the supplied graph block. Prepared hierarchy and unmentioned patch entries survive. */
	CONTROLRIGIR_API bool WriteGraph(UControlRigBlueprint* Rig, const FString& Body, bool bSnapshot,
		bool bDryRun, TArray<FString>& Removed, FString& Error);
	/** Properties and graph are validated and committed together when both are supplied. */
	CONTROLRIGIR_API bool WriteBlocks(UControlRigBlueprint* Rig, const TMap<FString, FString>& Bodies,
		bool bSnapshot, bool bDryRun, TArray<FString>& Removed, FString& Error);
	/** Replace only these function bodies, preserving all other graphs and existing members.
	 * Callers retain function identity and signatures. Variables in Document are additions/edits. */
	CONTROLRIGIR_API bool WriteFunctions(UControlRigBlueprint* Rig, const FControlRigIRDocument& Document,
		bool bDryRun, FString& Error);
	CONTROLRIGIR_API bool Compile(UControlRigBlueprint* Rig, FString& Error);
}
