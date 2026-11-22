// Copyright OraStudio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/** Shared scalar syntax. The caller resolves names in its own typed scope. */
namespace GraphCodeScalarExpression
{
	using FResolveName = TFunctionRef<bool(const FString& Name, double& Out, FString& OutError)>;

	/**
	 * `$Name(t)` — a name read AT a value (2026-10-30, the car's warp). Shape answers it for a curve:
	 * `$Roof(point.t.x)` is the profile `Roof` at that `t`. A caller that passes none refuses the form
	 * by name, so Groom and every older reader keep exactly the language they had.
	 */
	using FResolveCall = TFunctionRef<bool(const FString& Name, double Argument, double& Out, FString& OutError)>;

	/**
	 * **`dist($Hole)` — HOW FAR THE CALLER'S POINT IS FROM ANOTHER SHAPE (G29, 2026-11-20; the name is the agent's, for the
	 * owner to change).** The evaluator knows no point and no shape, so the handle is passed through as written (`$Hole`)
	 * and the caller answers. `bSigned` is the word `signed` (`dist($Hole, signed)`): the caller refuses it where its
	 * target has no inside. A caller that passes none refuses the form by name, as a missing `FResolveCall` does.
	 */
	using FResolveQuery = TFunctionRef<bool(const FString& Function, const FString& Handle, bool bSigned,
		double& Out, FString& OutError)>;

	/** One `dist($Hole)` a program reads, in text order. */
	struct FQueryRead
	{
		FString Function;
		FString Handle;
		bool bSigned = false;
	};

	/**
	 * **A FORMULA READ ONCE, EVALUATED MANY TIMES (G29 item 5, 2026-11-20).** The text is walked once into a postfix
	 * program in the exact order the recursive reader evaluated it: every name read, every curve read, every check and
	 * every refusal sits where the reader met it, so running the program gives bit for bit the reader's number and the
	 * reader's first sentence. `Evaluate(Text, …)` is `Compile` then a run; a caller that reads one formula at many
	 * points (a `warp`) compiles once and keeps the program.
	 */
	struct GRAPHCODE_API FProgram
	{
		enum class EOp : uint8
		{
			Number,     // push Value
			Name,       // resolve Names[A], check it is finite, push
			CallCheck,  // refuse when no curve reader was passed: before the argument, where the reader refused
			Call,       // pop the argument, read the curve Names[A] there, check it is finite, push
			Function,   // pop B arguments, run function A, push
			Query,      // read Queries[A] at the caller's point (refused with Messages[B] when no reader), push
			Negate,
			Add,
			Subtract,
			Multiply,
			Divide,
			Fail,       // a reader's refusal at this place: Messages[A], kept if a sentence is already held
			Trailing,   // text after the expression: Messages[A], replacing any sentence already held
		};

		struct FOp
		{
			EOp Kind = EOp::Number;
			int32 A = 0;
			int32 B = 0;
			double Value = 0.0;
		};

		/** The text as the reader saw it (trimmed): the refusals quote it. */
		FString Text;
		TArray<FOp> Ops;
		/** Every distinct `$name` the program reads, plain or at a value, with its `$`. */
		TArray<FString> Names;
		TArray<FString> Messages;
		/** Every `dist($Hole)` read, in text order, for a caller that prepares its targets once. */
		TArray<FQueryRead> Queries;
		/** The most values on the stack at once. */
		int32 Depth = 0;
	};

	/**
	 * A slot resolver: `Names[Index]` of the program it is run with. A caller that classifies each name once (a `warp`'s
	 * point words, its `with=` values) answers by index instead of comparing strings at every point.
	 */
	using FResolveSlot = TFunctionRef<bool(int32 NameIndex, double& Out, FString& OutError)>;
	/** A query resolver by slot: `Queries[Index]` of the program it is run with. */
	using FResolveQuerySlot = TFunctionRef<bool(int32 QueryIndex, double& Out, FString& OutError)>;

	GRAPHCODE_API bool IsNumericLiteral(const FString& Text);
	GRAPHCODE_API bool IsExpression(const FString& Text);
	/** Whether a character continues a name, including a qualified-name separator. */
	GRAPHCODE_API bool NameContinuesAt(const FString& Text, int32 At);
	GRAPHCODE_API bool Evaluate(const FString& Text, FResolveName Resolve, double& Out, FString& OutError);
	GRAPHCODE_API bool Evaluate(const FString& Text, FResolveName Resolve, FResolveCall ResolveCall,
		double& Out, FString& OutError);
	GRAPHCODE_API bool Evaluate(const FString& Text, FResolveName Resolve, FResolveCall ResolveCall,
		FResolveQuery ResolveQuery, double& Out, FString& OutError);

	/** The text read once. It never fails: a refusal is compiled to the place the reader met it. */
	GRAPHCODE_API TSharedRef<const FProgram> Compile(const FString& Text);
	/** A null `ResolveCall` refuses `$Name(t)` by name, exactly as the text overload without one. */
	GRAPHCODE_API bool Evaluate(const FProgram& Program, FResolveName Resolve, const FResolveCall* ResolveCall,
		double& Out, FString& OutError);
	/** The same with names answered by slot; a null `ResolveQuery` refuses `dist()` by name. */
	GRAPHCODE_API bool EvaluateSlots(const FProgram& Program, FResolveSlot Resolve, const FResolveCall* ResolveCall,
		double& Out, FString& OutError);
	GRAPHCODE_API bool EvaluateSlots(const FProgram& Program, FResolveSlot Resolve, const FResolveCall* ResolveCall,
		const FResolveQuerySlot* ResolveQuery, double& Out, FString& OutError);

	GRAPHCODE_API bool MentionsAny(const FString& Text, const TSet<FString>& Names);
	/** The same walk, for a caller whose names compare by its own rule rather than `TSet`'s. */
	GRAPHCODE_API bool MentionsAny(const FString& Text, TFunctionRef<bool(const FString&)> IsNamed);
	GRAPHCODE_API FString SubstituteNames(const FString& Text,
		TFunctionRef<const FString*(const FString&)> Lookup);
}
