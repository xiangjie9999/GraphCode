// Copyright GraphCode Sdn Bhd. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"

/**
 * **THE PRODUCT'S OWN LOG, AND IT IS DECLARED RATHER THAN STATIC SO THE BUILDER CAN REACH IT.** It
 * was `DEFINE_LOG_CATEGORY_STATIC` in the module's cpp, which is one file's private category — fine
 * for the registration error it was written for, and unreachable from `GraphCodeShapeBuild.cpp`,
 * where the numbers a measurement wants are actually known.
 *
 * **`Verbose` IS WHERE A MEASUREMENT LIVES AND `Log` IS WHERE A REFUSAL DOES.** A per-program
 * diagnostic said at `Log` would land in every editor's log for every build, which is the noise a
 * corpus sweep has to read past; asked for with `-LogCmds="LogGraphCodeShape Verbose"` it costs
 * nothing until somebody wants it.
 */
DECLARE_LOG_CATEGORY_EXTERN(LogGraphCodeShape, Log, All);

/**
 * GraphCode Shape — the sixth product. `.gcshape`, a program that builds a mesh asset from nothing.
 *
 * `GC-SHAPE-GRAMMAR.md` is the grammar and it is canonical; `GC-SHAPE-AUTHORING-DESIGN.md` §15 is
 * the build order this module is step M0 of.
 *
 * **THE PRODUCT IS ITS OWN PLUGIN BECAUSE THE FILE IS ITS OWN FILE.** A `.gc` is REFLECTIVE — it
 * projects an asset that already exists — and a `.gcshape` is DECLARATIVE: nothing exists until it
 * runs. `GC-PRODUCT-SPLIT.md` §5.2 holds the boundary from the other side: the `.gcshape` decides
 * shape and slots, and `kind:static_mesh`'s `.gc` decides what the asset wears and how it builds.
 *
 * **M0 REGISTERS AND REFUSES, and that is the whole of it.** `splines { }` once shipped a writer
 * with no route for a day because its gate called the writer directly — a block ships when it is
 * REACHABLE and DECLARED, not when its writer passes, and a whole product has the same failure mode
 * one layer up. So the seam is bound now, by a builder that says the evaluator is not here yet, and
 * M1 replaces the body without touching the route.
 */
class FGraphCodeShapeModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
