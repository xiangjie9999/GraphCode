#pragma once

#include "CoreMinimal.h"
#include "Internationalization/LocKeyFuncs.h"

/**
 * Case-sensitive `FString` keys, for anything whose key is an authored NAME.
 *
 * **`TSet<FString>` and `TMap<FString, …>` FOLD CASE.** `FString` hashes through
 * `Strihash_DEPRECATED` and compares with an `operator==` that ignores case, so `SkyLight` and
 * `Skylight` are ONE key — and both are real component names in this project's own
 * `Content/Default.gc`. A container that cannot hold them apart cannot report that a document
 * names two things, cannot tell a rename from a no-op, and cannot offer "did you mean" either,
 * because it never sees the second candidate.
 *
 * Borrowed from `Internationalization/LocKeyFuncs.h` rather than reimplemented: a localisation key
 * needs exactly this — an `FString` key matched with `ESearchCase::CaseSensitive` over a hash that
 * does not fold — and duplicating a hash function to rename it buys nothing.
 *
 * Use these for names and property paths. NOT for free text: deduplicating compiler messages by
 * folding case is harmless, and tightening it would change diagnostics for no reason.
 */
using FAuthoringNameSet = TSet<FString, FLocKeySetFuncs>;

template <typename ValueType>
using TAuthoringNameMap =
	TMap<FString, ValueType, FDefaultSetAllocator, FLocKeyMapFuncs<ValueType>>;

/** One canonical reflected property deviation. Order is significant. */
struct AUTHORINGCORE_API FAuthoringProperty
{
	FString Path;
	FString Value;

	bool operator==(const FAuthoringProperty& Other) const
	{
		// Case-sensitive on BOTH halves. The path is an identity, and the value is authored text —
		// a display string going from `Open` to `open` is an edit, and `==` would call it no change.
		return Path.Equals(Other.Path, ESearchCase::CaseSensitive)
			&& Value.Equals(Other.Value, ESearchCase::CaseSensitive);
	}
};

enum class EAuthoringDiagnosticSeverity : uint8
{
	Info,
	Warning,
	Unsupported,
	Error,
};

struct AUTHORINGCORE_API FAuthoringDiagnostic
{
	EAuthoringDiagnosticSeverity Severity = EAuthoringDiagnosticSeverity::Info;
	FString Path;
	FString Message;
};

namespace AuthoringCore
{
	/**
	 * Double-quoted string with the escaping shared by every authoring DSL: `\\`, `\n`, `\r`,
	 * `\t`, `\'` and `\"`, which is the engine's own `ReplaceCharWithEscapedChar` table and the
	 * one every other language spells the same way.
	 */
	AUTHORINGCORE_API FString Quote(const FString& Value);

	/**
	 * Undo the escaping inside a literal - the CONTENTS, without the quotes.
	 *
	 * **THIS EXISTS BECAUSE `FString::ReplaceEscapedCharWithChar` IS NOT AN INVERSE.** It applies
	 * the escape table by `ReplaceInline`, whose SearchCase defaults to `IgnoreCase`, so the `\t`
	 * rule matches `\T` - and `"C:\\Temp\\Ore.png"`, the ordinary spelling of a Windows path, comes
	 * back as `C:` + TAB + `emp\Ore.png`. Measured 2026-08-27 by the first spec that put a path
	 * through the string codec. `\N`, `\R`, `\'` and `\"` fail the same way.
	 *
	 * One left-to-right pass, case-sensitive, and no re-scan of what it has already written - which
	 * is the only shape that cannot corrupt, since a replacement can never be re-matched.
	 *
	 * AN UNKNOWN ESCAPE KEEPS BOTH CHARACTERS. `Quote` escapes every backslash it writes, so a lone
	 * `\` before anything else can only be hand-written text - and a person who typed
	 * `C:\Temp\Ore.png` without doubling the slashes meant the path. Dropping the backslash is the
	 * one reading that silently destroys data.
	 */
	AUTHORINGCORE_API FString Unescape(const FString& Escaped);

	/**
	 * The inverse of Quote: `"a \"b\" c"` back to `a "b" c`.
	 *
	 * ONLY A WELL-FORMED LITERAL IS ACCEPTED - opening quote, closing quote, and no unescaped
	 * quote between them - and false means "this text is not a literal", never an error. That is
	 * what lets a caller keep the raw text as its own value, which every sidecar written before
	 * the codec quoted its strings depends on: `Name: string = Main` still reads as `Main`.
	 *
	 * The one text this cannot separate is a legacy value that WAS `"Left"`, quote characters and
	 * all, written raw. It now reads as `Left` - and that is the reading the format wants, since
	 * a value carrying its own quotes was unwritable and unreadable both ways
	 * ([[a-reader-that-spells-owes-the-writer-an-inverse]]).
	 */
	AUTHORINGCORE_API bool ParseQuoted(const FString& Text, FString& OutValue);

	/**
	 * An engine identifier - a widget name, a named slot, a property path segment, an enum entry -
	 * spelled so it survives a round trip through a grammar that uses (), [], = and , itself.
	 * Bare when it can be, backtick-quoted when it cannot: `Handle (Pull)`.
	 *
	 * Backticks are lexical only, so a quoted identifier is still an identifier and not a string
	 * literal. UE permits spaces and parentheses in display names and enum entries, so this is not
	 * a corner case.
	 *
	 * This is the one implementation shared by BlueprintIR, WidgetIR, SchemaCode and world
	 * snapshots, so the same authored name cannot acquire a different spelling by asset kind.
	 */
	AUTHORINGCORE_API FString QuoteIdentifier(const FString& Name);

	/** Inverse of QuoteIdentifier. Accepts a bare or backtick-quoted identifier. */
	AUTHORINGCORE_API bool ParseIdentifier(const FString& Text, FString& OutName, FString& OutError);

	/** One `rename <Old> -> <New>` directive, as a payload asked for it. */
	struct FAuthoringRename
	{
		FString Old;
		FString New;
	};

	/** The one spelling of the arrow, so nine blocks cannot each pick their own. */
	inline constexpr const TCHAR* RenameArrow = TEXT("->");

	/**
	 * Read the operand of a `rename` directive: `<Old> -> <New>`, both through ParseIdentifier.
	 *
	 * SHARED BECAUSE THE ALTERNATIVE IS NINE COPIES. `delete `'s four existing parsers already
	 * disagree — two go through ParseIdentifier, one does on a single name, and `functions { }`
	 * takes the rest of the line raw — so a caller cannot tell which of four spellings a block
	 * wants. A verb added block by block acquires the same divergence by construction.
	 *
	 * The arrow is what separates two operands where `delete` has one. A name needing spaces is
	 * backtick-quoted as everywhere else, so the arrow is not load-bearing for parsing a quoted
	 * name; it is load-bearing for the bare spelling a caller actually types, and it is what makes
	 * a refusal able to say which side it means.
	 */
	AUTHORINGCORE_API bool ParseRenameDirective(const FString& Operand, FAuthoringRename& OutRename,
		FString& OutError);

	/**
	 * Take a block body's own directive lines out of it: `delete <Name>` and `rename <Old> -> <New>`.
	 * Returns the body with those lines removed, which is what the block's entry parser then reads.
	 *
	 * SHARED BECAUSE THE EXTRACTION DIVERGES AS EASILY AS THE PARSE DID. §6.7 moved the operand parse
	 * here and left five callers each finding the lines their own way; the five positional rename
	 * sites need exactly this walk, and two of them live in BlueprintIR, which cannot see GraphCode's
	 * copy. One implementation is the only way the same payload cannot mean different things by block.
	 *
	 * Two things it gets right that a `StartsWith` over every line does not:
	 *
	 * - **Only at the block's own depth.** A `delete` inside a graph's braces is not this token, and
	 *   reading it here would take a statement out of a payload without telling anyone.
	 * - **The wrapper is optional**, because `UnwrapBlock` accepts a payload with its `name { … }`
	 *   around it or without. With the wrapper the block's own depth is 1, and reading directives at
	 *   depth 0 found none at all - measured by a spec that fed it the wrapped form, not reasoned about.
	 *
	 * A malformed directive is an ERROR rather than a line handed on to the entry parser: left in
	 * place it comes back as "could not parse the block", which sends the caller looking at their
	 * entries for a defect in one directive line.
	 */
	AUTHORINGCORE_API FString SplitBlockDirectives(const FString& BlockBody, const FString& BlockName,
		TArray<FString>& OutDeleted, TArray<FAuthoringRename>& OutRenames, TArray<FString>& OutErrors);

	/**
	 * The same walk, also collecting the directives ONE LEVEL IN, keyed by the entry whose braces
	 * they sat in.
	 *
	 * **§6.3's "its scope is the braces it sits in" is a rule about nesting, not only about which
	 * block you are in.** A timeline's TRACKS live inside the timeline's own braces, so a track
	 * rename is spelled there and nowhere else:
	 *
	 *     timelines {
	 *       rename Spin -> Rotate                  // the timeline
	 *       Rotate = Timeline(length=3.0) {
	 *         rename Interpolation -> Curve        // a track of that timeline
	 *         Curve = FloatTrack(...)
	 *       }
	 *     }
	 *
	 * The two levels cannot be flattened into one list: a track name is unique only within its
	 * timeline, so `rename Interpolation -> Curve` means nothing without the entry that owns it.
	 *
	 * The entry's name is read off the line that opened the braces - everything before its `=`, or
	 * before the `{` when the entry has no value - through `ParseIdentifier`, so a quoted name works
	 * here exactly as it does anywhere else.
	 */
	AUTHORINGCORE_API FString SplitBlockDirectives(const FString& BlockBody, const FString& BlockName,
		TArray<FString>& OutDeleted, TArray<FAuthoringRename>& OutRenames,
		TMap<FString, TArray<FAuthoringRename>>& OutEntryRenames, TArray<FString>& OutErrors);

	/**
	 * The same walk for a PROPERTY BAG, which reads one more token: `reset <Path>`. C23 §3.
	 *
	 * **A bag's entries ARE its properties**, so the identity question §3 asks of every writable
	 * block lands on a path rather than on a name, and `reset` is what patch says instead of
	 * `delete`: a reflected property cannot be removed, only returned to its baseline, which is
	 * where omission ends under snapshot. The token is not invented here - `components { }` has
	 * spelled an inherited override's removal `reset <Name>` since C4.
	 *
	 * `delete` and `rename` still come back so the caller can refuse them BY NAME. Left in the body
	 * they reach the property parser as a line with no `=` and come back as *"expected Path =
	 * Value"*, which sends a caller looking at their properties for a defect in one directive.
	 *
	 * The operand is a property PATH, so `Movement.MaxSpeed` and `Tints[3]` are ordinary operands;
	 * only the characters `QuoteIdentifier` escapes need backticks.
	 */
	AUTHORINGCORE_API FString SplitPropertyBagDirectives(const FString& BlockBody,
		const FString& BlockName, TArray<FString>& OutReset, TArray<FString>& OutDeleted,
		TArray<FAuthoringRename>& OutRenames, TArray<FString>& OutErrors);

	/**
	 * Every contradiction between a payload's rename tokens, its deletes, and the two name sets.
	 *
	 * `Noun` is what the block calls one entry - "graph", "component", "variable" - and appears in
	 * the messages. Returns false and fills `OutErrors` when the payload cannot be read as written.
	 *
	 * The rules, and they are the same for every block because they are about the payload rather
	 * than about what is being renamed:
	 *
	 * - `<Old>` must name something live, and must NOT still be in the payload. Both readings of a
	 *   name that is being moved AND kept are wrong.
	 * - `<New>` must be in the payload, because omission still answers the other question: under
	 *   patch a missing entry is a truncated payload, under snapshot it is a deletion, and neither
	 *   is what the token said.
	 * - `<New>` must not already be live, UNLESS it is itself the `<Old>` of another token in the
	 *   same payload. That is a SWAP, which this token makes expressible for the first time and
	 *   which a caller must therefore apply through temporary names.
	 * - A name may be the `<Old>` of one token only, the `<New>` of one token only, and may not be
	 *   both deleted and renamed.
	 */
	AUTHORINGCORE_API bool ValidateRenameDirectives(const TArray<FAuthoringRename>& Renames,
		const TArray<FString>& LiveNames, const TArray<FString>& PayloadNames,
		const TArray<FString>& Deletes, const TCHAR* Noun, TArray<FString>& OutErrors);

	/** True when applying these renames in order would collide, so temporary names are needed. */
	AUTHORINGCORE_API bool RenamesNeedTemporaries(const TArray<FAuthoringRename>& Renames);

	/**
	 * THE ONE INFERENCE THAT SURVIVES, and only under `snapshot`. Exactly one vanished name against
	 * exactly one arrived name is paired and both lists are emptied; anything else is left alone.
	 *
	 * Under `patch` a caller can write a token, so nothing is guessed and `UnpairedNamesRefusal`
	 * below asks which they meant. Under `snapshot` the document is a PROJECTION - what a merge and
	 * a restore produce - and no projection contains a token, so the only alternatives are this
	 * pairing or delete-plus-create. Delete-plus-create mints a fresh identity and breaks every
	 * asset that bound to the old one, which is worse than a guess that refuses the moment it has
	 * two candidates.
	 *
	 * User-directed, 2026-08-08. THE DISTINCTION IS NOT PATCH-VERSUS-SNAPSHOT, it is whether a
	 * guess can be silently WRONG: this one cannot, because it declines the moment either side has
	 * more than one member. Positional pairing - leftovers matched by their order in the block -
	 * can be, so it is removed rather than kept here, in every mode.
	 */
	AUTHORINGCORE_API bool PairSingleRename(TArray<FString>& InOutMissing,
		TArray<FString>& InOutFresh, FAuthoringRename& OutRename);

	/**
	 * The refusal that REPLACES a rename heuristic, worded once for every block that had one.
	 *
	 * What a payload with one vanished name and one arrived name used to get was a guess. Letting
	 * it fall through to delete+add instead would be strictly worse than the guess - a fresh GUID,
	 * every reference broken, nothing logged - so what replaces it is a refusal that names both
	 * candidates and both readings. Empty when there is nothing unaccounted for.
	 */
	AUTHORINGCORE_API FString UnpairedNamesRefusal(const TArray<FString>& Missing,
		const TArray<FString>& Fresh, const TCHAR* BlockName, const TCHAR* Noun);

	/**
	 * Split on a delimiter only at top level, respecting (), [], {}, "strings" and `identifiers`.
	 * Backticks have to be opaque here or a quoted identifier containing the delimiter would be
	 * torn in half by the very splitter the quoting exists to survive.
	 */
	AUTHORINGCORE_API bool SplitTopLevel(const FString& Text, TCHAR Delimiter,
		TArray<FString>& OutParts, FString& OutError);

	/** Parse a typed tuple `(value1,value2)` while preserving each value's source spelling. */
	AUTHORINGCORE_API bool ParseTupleList(const FString& Text,
		TArray<FString>& OutItems, FString& OutError);

	/** Render a typed tuple in its canonical compact spelling: `(value1,value2)`. */
	AUTHORINGCORE_API FString RenderTupleList(const TArray<FString>& Items);

	/**
	 * The text between `Open` at OpenIndex and its matching `Close`, skipping "strings" and
	 * `identifiers`. OutCloseIndex is where the matching close was found.
	 */
	AUTHORINGCORE_API bool ExtractBalanced(const FString& Text, int32 OpenIndex, TCHAR Open,
		TCHAR Close, FString& OutInside, int32& OutCloseIndex, FString& OutError);

	/** One source line, or several joined into one statement, with the line it started on. */
	struct FAuthoringLogicalLine
	{
		FString Text;
		int32 Number = 0;

		/**
		 * The last source line the statement occupies. Equals `Number` when nothing wrapped.
		 *
		 * With `Number` it is a span, which is what lets a caller recover the statement's ORIGINAL
		 * bytes — indentation included — from the body it parsed. `SourceLines` holds the same lines
		 * trimmed, which is right for deciding layout and wrong for reproducing it.
		 */
		int32 LastNumber = 0;

		/**
		 * The trimmed source lines this statement was joined from — one entry when nothing wrapped.
		 *
		 * Kept because WRAPPING IS NOT DERIVABLE FROM CONTENT, and that outlived the rule that caused
		 * it. No projector wraps any more (`EFieldLayout`, user ruling 2026-08-18) — but every `.gc`
		 * committed before the ruling does, in three different layouts, and a consumer that must
		 * reproduce the bytes it READ cannot recompute which one it is looking at. It reproduces this
		 * instead of guessing. Reading wrapped text is therefore permanent, not transitional.
		 */
		TArray<FString> SourceLines;
	};

	/**
	 * A block body as STATEMENTS rather than lines: comments and blanks dropped, a wrapped property
	 * list rejoined, a trailing `@tag[…]` decoration folded onto the statement it decorates, and a
	 * lone brace kept as its own entry.
	 *
	 * Shared because `components { }`, `actors { }` and anything else using rule 7's
	 * `Name = Type(props) { children }` shape all have to undo the SAME layout decision — the
	 * wrapping `AppendFields` used to apply when a property list did not fit on a line. It no longer
	 * applies to anything being WRITTEN, and undoing it is still required for everything already
	 * written. Two parsers with two copies of this is how one of them ends up not handling a
	 * five-line entry.
	 */
	AUTHORINGCORE_API TArray<FAuthoringLogicalLine> CollectLogicalLines(const FString& Body);

	/** One `@tag[…]` decoration, in the order it was written. */
	struct FAuthoringTag
	{
		FString Name;
		FString Inside;

		/** Written across lines in the source. See `FAuthoringLogicalLine::SourceLines`. */
		bool bWrapped = false;
	};

	/**
	 * Which container spelling produced an entry.
	 *
	 * `GC-GRAMMAR-AUDIT.md` §1 concluded three statement shapes carry every asset kind. `slot` is the
	 * counter-example it did not have: `widgets { }` writes `slot Body { … }` for a named slot — a
	 * container with a NAME, no type token and no property list — which rule 7 cannot express, and it
	 * appears in 24 of this project's 50 committed widget blocks.
	 *
	 * A form, not a synthetic type token, because the two differ in what may be written on them: a
	 * named slot can never carry properties or decorations, and a writer that only knew `TypeToken`
	 * was empty would have to guess whether that meant "a slot" or "a malformed declaration".
	 */
	enum class EAuthoringEntryForm : uint8
	{
		/** Rule 7: `Name = Type(properties)`, optional `@tag[…]`, optional `{ children }`. */
		Declaration,

		/** `slot Name { child }`. Holds exactly one entry and nothing else. */
		NamedSlot,
	};

	/** One `Name = Type(properties) { children }` entry — rule 7, at any depth. */
	struct FAuthoringObjectNode
	{
		FString Name;
		FString TypeToken;
		TArray<FAuthoringProperty> Properties;
		TArray<FAuthoringTag> Tags;
		TArray<FAuthoringObjectNode> Children;

		EAuthoringEntryForm Form = EAuthoringEntryForm::Declaration;

		/** The entry's own property list was written across lines. Tags carry their own flag. */
		bool bWrapped = false;

		/** The source line the entry started on, for an error a reader can locate. */
		int32 Line = 0;

		/**
		 * The last source line it spans: its closing brace, or the end of its own statement.
		 *
		 * With `Line` it is a span, which is what lets a caller ask *which entity owns this line* —
		 * the same question `FGraphCodeBlock::FirstLine`/`LastLine` answers one level up. Without it,
		 * a line can only be attributed to the last entry that STARTED before it, which puts every
		 * closing brace on the wrong entity.
		 */
		int32 EndLine = 0;
	};

	/**
	 * A block body as a tree of rule-7 entries.
	 *
	 * Purely syntactic: it does not know which tags are legal, what a type token resolves to, or
	 * what nesting means — those belong to the block's own writer, which is the only thing that
	 * knows. `components { }`, `actors { }` and `import { }` are one shape and differ entirely in
	 * what they do with it.
	 *
	 * `slot Name` is the one KEYWORD it recognises, because a keyword that opens braces is syntax in
	 * the way `delete Foo` is not: it changes what the following `{` attaches to. It is admitted at
	 * every depth rather than only where widgets use it, on the same reasoning as the `@` sigil —
	 * matching a shape costs one branch, and matching a block name would put the caller's vocabulary
	 * in the shared parser.
	 *
	 * @param OutDirectives  depth-0 lines that are not entries — `delete Foo` and its kin. Returned
	 *                       rather than refused, because a keyword is the caller's vocabulary.
	 *
	 * NOTE `GraphCodeActorWriter` and `GraphCodeLevelWriter` still carry their own copies of this
	 * walk. They predate it and are covered by their own specs; migrating them is worth doing and is
	 * not worth doing in the same change that introduced the shared version.
	 */
	AUTHORINGCORE_API bool ParseObjectTree(const FString& Body, TArray<FAuthoringObjectNode>& OutNodes,
		TArray<FAuthoringLogicalLine>& OutDirectives, FString& OutError);

	/** Parse a canonical `Name=Value, ...` property list without interpreting value types. */
	AUTHORINGCORE_API bool ParsePropertyList(const FString& Text,
		TArray<FAuthoringProperty>& OutProperties, FString& OutError);

	/**
	 * A block body of `Path = Value` STATEMENTS, one per logical line — the shape `settings { }`
	 * has on every kind that projects one. `ParsePropertyList`'s sibling: that one reads a
	 * comma-separated list written inside `Type(…)`, this one reads a block.
	 *
	 * Splits on the FIRST `=` and rejoins the rest, because a value may contain one and rejoining
	 * is the only way to keep `ImportKeyField = "a=b"` meaning what it says.
	 *
	 * Neither ordered nor deduplicated: the caller's `ImportDeviations` applies these in the order
	 * they were written, and for a repeated path last-writer-wins is what the details panel does.
	 * `class_defaults { }` wants the opposite on both counts and so keeps its own parser.
	 */
	AUTHORINGCORE_API bool ParsePropertyBody(const FString& Body,
		TArray<FAuthoringProperty>& OutProperties, FString& OutError);

	/**
	 * The first property at `Path`, or null. **CASE-SENSITIVE** — every hand-written copy of this
	 * used `FString::operator==`, which folds, so it answered `SkyLight` with `Skylight`.
	 *
	 * First rather than only: `ParsePropertyBody` does not deduplicate, so a body may legitimately
	 * carry a path twice.
	 */
	AUTHORINGCORE_API const FAuthoringProperty* FindProperty(
		const TArray<FAuthoringProperty>& Properties, const FString& Path);

	/**
	 * The one candidate that equals `Name` apart from case, for a lookup that already missed.
	 *
	 * **This is what makes case-sensitivity safe rather than merely correct.** Once names are
	 * matched exactly, a mis-cased name stops resolving — and a writer whose next move is "then it
	 * must be new" would CREATE a second thing beside the one the author meant. Asking this before
	 * treating a miss as a create turns a silent duplicate into a sentence naming the real spelling.
	 *
	 * Empty when nothing is close, and **empty when more than one candidate matches**: with
	 * `SkyLight` and `Skylight` both present, `skylight` has no single correction and guessing one
	 * would pick by array order. The caller says "no such name" in that case, which is true.
	 */
	AUTHORINGCORE_API FString FindNameIgnoringCase(
		const TArray<FString>& Candidates, const FString& Name);

	/** Render properties in their stored canonical order. */
	AUTHORINGCORE_API FString RenderPropertyList(const TArray<FAuthoringProperty>& Properties);

	/** Platform-independent BLAKE3 digest over UTF-8 canonical content. */
	AUTHORINGCORE_API FString ContentSignature(const FString& Namespace, const FString& CanonicalContent);

	/**
	 * LF, whatever the file arrived as. Every authored text projection is canonically LF, so a
	 * digest taken before this ran would disagree with itself between a Windows and a Unix checkout.
	 */
	AUTHORINGCORE_API FString NormalizeNewlines(FString Text);
}
