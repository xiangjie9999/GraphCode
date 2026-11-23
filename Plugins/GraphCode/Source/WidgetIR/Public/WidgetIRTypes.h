#pragma once

#include "CoreMinimal.h"
#include "AuthoringCoreTypes.h"

/**
 * The grammar and semantic version this build reads and writes.
 *
 * 3, since 2026-08-10 (row 11o): `bind` and `unbind` are statements a v2 reader has never seen.
 * It would not misread one - a v2 parser meets `bind W.P = F` and fails on the space inside what
 * it takes for a widget name - but design section 6.1 says a parser refuses a newer version rather
 * than approximating it, and refusing on the header beats refusing on line 40 with a message about
 * identifiers. The bump also moves `ir:` on all 54 widget sidecars, which is what makes gc_sync's
 * fast path decline them instead of trusting a digest taken over a grammar that has moved.
 */
/**
 * 4, since 2026-08-11 (C6): `navigate` is a statement a v3 reader has never seen, and it would fail
 * on the space inside what it takes for a widget name rather than on the header. Design section 6.1
 * says a parser refuses a newer version instead of approximating it, and refusing on line 1 beats
 * refusing on line 40 with a message about identifiers.
 *
 * ONE BUMP FOR THE WHOLE C6 BATCH. The facets closing under it - navigation, orphan resolution, the
 * animation codec - are all new statements in the same zone, and nothing has shipped at 4 yet, so a
 * bump per facet would move `ir:` on all 54 widget sidecars three times to say one thing.
 */
/**
 * 5, since 2026-08-11 (row 11u): `mvvm { }` is a whole nested ZONE a v4 reader has never seen, so it
 * meets a bare `mvvm` line, reads it as a widget declaration missing its `=`, and refuses with a
 * message about identifiers forty lines below the fact. Same rule as every bump above it.
 *
 * NOT FOLDED INTO 4, even though nothing in this project has committed a widgetir/4 sidecar - the
 * corpus is still at 3 - because 4 is what a `gc_read` has been HANDING OUT since C6, and a payload
 * a caller round-tripped through this build carries it. A version already in someone's hands cannot
 * quietly acquire a new grammar; that is the same reason `blueprint_settings` was renamed rather
 * than redefined.
 */
/**
 * 6, since 2026-08-12 (row 11u's last box): `convert=` and `convert_back=` on an `bind` line inside
 * `mvvm { }` name the function a binding converts through, and a v5 reader refuses them BY NAME as
 * unknown specifiers - correctly, and forty lines below the fact. Same rule as every bump above it.
 *
 * NOT FOLDED INTO 5 for the reason 5 was not folded into 4, and this time the corpus proves it
 * rather than the argument: 5 is what the 57-sidecar reprojection COMMITTED, so it is a version in
 * the tree's own hands and cannot quietly acquire a grammar.
 */
/**
 * 7, since 2026-08-21 (Core queue row 20, `UI-AUTHORING-DESIGN.md` §18a): a legacy `bind` line's
 * SOURCE is rooted on `self` — `bind Slider.Value = self.Value` — where a v6 document wrote a bare
 * member. One keyword had two source grammars, MVVM's rooted and this one not, and the destinations
 * already agreed; rooting the legacy half on the word that already meant the widget itself leaves
 * one `bind` grammar for a fourth consumer to inherit.
 *
 * THE ONLY BUMP HERE WHOSE OLDER READER MISREADS RATHER THAN REFUSES, and that is the argument for
 * it rather than against. A v6 parser accepts a multi-segment source already — `bind X.Y = A.B` is
 * a property path on the widget — so it meets `self.Value`, reads a two-segment path, and binds to
 * a member no class has. Every bump above this one could have been caught on line 40; this one
 * could not be caught at all, so it has to be caught on line 1.
 */
#define WIDGETIR_VERSION 7

enum class EWidgetIRAttachment : uint8
{
	Root,
	Children,
	Content,
	NamedSlot,
};

struct WIDGETIR_API FWidgetIRNode
{
	/**
	 * The authored widget name, and the document's only handle on this widget.
	 *
	 * UMG resolves widgets by name and by nothing else: generated variables, designer bindings,
	 * navigation, animation bindings, named-slot fills from derived assets and native BindWidget
	 * contracts all key on this FName. A separate document alias would have been a second identity
	 * for something the engine already identifies, so there is only this one - which also means a
	 * rename is expressed by editing this text, not by calling a rename operation.
	 *
	 * The one exception is the write-only spelling `_`: a widget the payload declines to name, for
	 * which the engine generates one. It never appears in a read.
	 */
	FString Name;

	FString ClassPath;
	FString ClassToken;
	bool bVariable = false;

	/** State owned by the widget itself, compacted against its class default object. */
	TArray<FAuthoringProperty> Properties;

	/** State owned by the relation between this widget and its parent. */
	TArray<FAuthoringProperty> SlotProperties;
	EWidgetIRAttachment Attachment = EWidgetIRAttachment::Children;
	FString NamedSlot;

	/** Ordered authored children. A named-slot child carries NamedSlot on the child. */
	TArray<FWidgetIRNode> Children;

	/** True for the `_` spelling: construct a fresh widget and let the engine name it. */
	bool IsAnonymous() const { return Name == TEXT("_"); }
};

/**
 * One member on the way to a binding's source.
 *
 * A member is a property or a function, and canonical text spells the difference with `()`, so
 * `Value` is a property read and `OnGetUserMenuContent()` is a call. The engine keeps the same
 * distinction per segment (`FEditorPropertyPathSegment::IsProperty`), and it is not decoration:
 * the compiler binds a function directly and reaches a property through a generated accessor.
 */
struct WIDGETIR_API FWidgetIRBindingMember
{
	FString Name;
	bool bFunction = false;

	bool operator==(const FWidgetIRBindingMember& Other) const
	{
		return bFunction == Other.bFunction && Name.Equals(Other.Name, ESearchCase::CaseSensitive);
	}
};

/**
 * One designer property binding: `UWidgetBlueprint::Bindings`, the Designer's Bind dropdown.
 *
 * IDENTITY IS `<Widget>.<Property>` AND THE ENGINE SAYS SO. `FDelegateEditorBinding::operator==`
 * compares `ObjectName` and `PropertyName` and nothing else, with the reason in its own comment:
 * *"the function is irrelevant since you're only allowed to bind a property on an object to a
 * single function"*. So the source is the VALUE of this entry, which is the whole point of the
 * shape - the block this replaced printed a COUNT, and a count moves when a binding is added or
 * removed and stands perfectly still when one is repointed at a different function.
 *
 * Source is a chain because the engine's is: `FEditorPropertyPath` is an ordered list of segments,
 * and binding through a sub-object or a struct member produces more than one. The flat
 * `FunctionName`/`SourceProperty`/`Kind` trio is the last segment restated, and `MemberGuid` is
 * that segment's guid - all of which this document derives rather than carries, because a name
 * that resolves inside the file is the identity everywhere else in the language.
 */
struct WIDGETIR_API FWidgetIRBinding
{
	/** `ObjectName` - a widget in THIS asset's tree; the compiler resolves it there and nowhere else. */
	FString WidgetName;

	/** `PropertyName` - the bindable property or delegate on that widget. */
	FString PropertyName;

	/** Ordered members from the widget's owner class to the bound member. Never empty in a read. */
	TArray<FWidgetIRBindingMember> Source;

	/** The engine's own equality: the key, without the value. */
	bool SameTarget(const FWidgetIRBinding& Other) const
	{
		return WidgetName.Equals(Other.WidgetName, ESearchCase::CaseSensitive)
			&& PropertyName.Equals(Other.PropertyName, ESearchCase::CaseSensitive);
	}

	bool operator==(const FWidgetIRBinding& Other) const
	{
		return SameTarget(Other) && Source == Other.Source;
	}

	FString Key() const { return WidgetName + TEXT(".") + PropertyName; }
};

/**
 * One `navigate <Widget>.<Key> = <value>` entry: `UWidget::Navigation`, the Designer's Navigation
 * panel.
 *
 * IDENTITY IS `<Widget>.<Key>`, and the key is a SLOT on that widget's `UWidgetNavigation` rather
 * than a direction, because the object holds seven authored answers and only six of them are
 * directions. `Up`/`Down`/`Left`/`Right`/`Next`/`Previous` each carry an `FWidgetNavigationData`;
 * `Routing` carries `EWidgetNavigationRoutingPolicy`. Splitting those into two statement families
 * would have spelled one object's state two ways, and the reader that has to reassemble a
 * `UWidgetNavigation` needs them together anyway.
 *
 * `WidgetToFocus` is ONE engine field with TWO meanings - its own comment says "either the widget to
 * focus, OR the name of the function to call" - and `Rule` is what disambiguates them. The grammar
 * keeps that disambiguation visible rather than restating it: `explicit(Other)` names a widget and
 * `custom(Fn)` names a function, so a reader never has to hold a rule in mind to know what the
 * argument is. That is also why there is no separate `Target` and `Function` field here.
 */
struct WIDGETIR_API FWidgetIRNavigation
{
	/** The widget whose `Navigation` object this entry belongs to. */
	FString WidgetName;

	/** `Up` `Down` `Left` `Right` `Next` `Previous` `Routing` - canonical, in that order. */
	FString Key;

	/**
	 * For a direction: `escape` `stop` `wrap` `invalid`, or `explicit` / `custom` / `customBoundary`
	 * with `Argument` set. For `Routing`: the `EWidgetNavigationRoutingPolicy` name.
	 */
	FString Rule;

	/** The widget name for `explicit`, the function name for `custom`/`customBoundary`, else empty. */
	FString Argument;

	bool SameTarget(const FWidgetIRNavigation& Other) const
	{
		return WidgetName.Equals(Other.WidgetName, ESearchCase::CaseSensitive)
			&& Key.Equals(Other.Key, ESearchCase::CaseSensitive);
	}

	bool operator==(const FWidgetIRNavigation& Other) const
	{
		return SameTarget(Other)
			&& Rule.Equals(Other.Rule, ESearchCase::CaseSensitive)
			&& Argument.Equals(Other.Argument, ESearchCase::CaseSensitive);
	}

	/** Not `Key()`, which would shadow the field of that name; the identity is still the pair. */
	FString TargetKey() const { return WidgetName + TEXT(".") + Key; }
};

/**
 * One `unbind <Widget>.<Property>`: apply-only intent, never produced by a read.
 *
 * The two names are kept apart rather than joined into one string because the dot is grammar. A
 * key held as text would have to be split again to be quoted, and a split on the first dot is a
 * guess about which side a name with a dot in it belongs to.
 */
struct WIDGETIR_API FWidgetIRUnbind
{
	FString WidgetName;
	FString PropertyName;

	FString Key() const { return WidgetName + TEXT(".") + PropertyName; }
};

/** Which side of the view an MVVM path starts from - `EMVVMBlueprintFieldPathSource`. */
enum class EWidgetIRMvvmSource : uint8
{
	None,
	Widget,
	ViewModel,
	SelfContext,
};

/**
 * One end of an MVVM binding: where the path starts, and the members walked from there.
 *
 * THE ROOT IS A NAME EVEN THOUGH THE ENGINE'S IS A GUID, and that is the whole reason this type
 * exists rather than the engine's struct being projected field by field.
 * `FMVVMBlueprintPropertyPath` holds `ContextId` - the viewmodel's `ViewModelContextId` - so the
 * authored reference is to an identity nothing in the file spells. A `.gc` that printed that guid
 * would name something no reader could resolve and no author could type, which is the rule
 * [[gc-names-must-resolve-in-the-file]] states: the guid is re-derived from the viewmodel declared
 * in this same block, exactly as row 11o re-derives a binding's `MemberGuid` from its member name.
 */
struct WIDGETIR_API FWidgetIRMvvmPath
{
	EWidgetIRMvvmSource Source = EWidgetIRMvvmSource::None;

	/** The viewmodel's `ViewModelName`, the widget's name, or empty for `self`. */
	FString Root;

	/** Members walked from the root. `Account.Profile.Name` is two. */
	TArray<FString> Fields;

	bool operator==(const FWidgetIRMvvmPath& Other) const
	{
		return Source == Other.Source
			&& Root.Equals(Other.Root, ESearchCase::CaseSensitive)
			&& Fields == Other.Fields;
	}

	/** `Account.DisplayName`, `TitleText.Text`, `self.IsBusy` - the text form, both directions. */
	FString Render() const;
};

/**
 * One viewmodel a view declares: an element of `UMVVMBlueprintView::AvailableViewModels`.
 *
 * A NEW ENTITY FAMILY RATHER THAN A RELATION, which is what the retired `[provider=mvvm]` spelling
 * missed (`UI-AUTHORING-DESIGN.md` §11.1a). A legacy binding's source is a member of a class the
 * document already names; an MVVM binding's source is normally one of these, and nothing in a
 * widget document declared it before.
 *
 * IDENTITY IS `Name`. The engine also carries `ViewModelContextId`, and that guid is derived here
 * rather than written: `ViewModelName` is the property the compiler generates on the widget, so it
 * is already the handle every binding, getter and setter resolves through.
 */
struct WIDGETIR_API FWidgetIRMvvmViewModel
{
	/** `ViewModelName` - the generated property, and this entry's identity. */
	FString Name;

	/** `NotifyFieldValueClass` as a path. Never a bare class name: it resolves outside this asset. */
	FString ClassPath;

	/** `EMVVMBlueprintViewModelContextCreationType` - Manual, CreateInstance, and three more. */
	FString CreationType;

	/** `GlobalViewModelIdentifier`, and only meaningful under `GlobalViewModelCollection`. */
	FString GlobalIdentifier;

	/** `ViewModelPropertyPath`, and only meaningful under `PropertyPath`. */
	FString PropertyPath;

	bool bCreateSetter = false;
	bool bCreateGetter = true;
	bool bOptional = false;
	bool bExposeInstanceInEditor = false;
	bool bGlobalCollectionUpdate = false;
	bool bUseAsInterface = false;

	/**
	 * This viewmodel carries a `Resolver` or an `InstancedViewModel` this document does not.
	 *
	 * Both are instanced polymorphic UObjects a project subclasses, so their state is not reachable
	 * from reflection over a closed struct the way every other field here is. Recorded per entry
	 * rather than as one document-wide flag because `omits:` answers for the FILE and this answers
	 * *which viewmodel* - the difference between "something was dropped" and a line you can look at.
	 */
	bool bHasUnprojectedObject = false;

	bool SameTarget(const FWidgetIRMvvmViewModel& Other) const
	{
		return Name.Equals(Other.Name, ESearchCase::CaseSensitive);
	}
};

/**
 * The function a binding converts through, in one direction.
 *
 * PROJECTED SINCE 2026-08-12, and the design said it might never be - `UI-AUTHORING-DESIGN.md`
 * §11.1b's second open box. What re-opened it was reading the type rather than reasoning about it:
 * that box rests on *"it wraps either a function reference or a generated GRAPH, and a graph is
 * BlueprintIR's"*, and `UMVVMBlueprintViewConversionFunction` says of its own graph
 * *"generated on load/compile and is not saved"* - `bWrapperGraphTransient` defaults TRUE. So the
 * authored state is a REFERENCE plus the pin values that feed it, and the graph is derived from
 * both. A derived thing is not a second owner; it is the thing this document does not have to carry.
 *
 * WHAT IS STILL NOT CARRIED IS THE PINS, and that is why `mvvm_instanced_objects` narrows rather
 * than leaves. A `FMVVMBlueprintPin` holds a property path OR a literal default that may be an
 * FText or a UObject pointer, per pin, on a function whose parameter list the document does not
 * describe - so it is its own increment and is named per binding rather than counted per file.
 */
struct WIDGETIR_API FWidgetIRMvvmConversion
{
	/**
	 * The function's own path - `/Script/Engine.KismetTextLibrary:Conv_DoubleToText`, or the path of
	 * a `UK2Node` subclass when the conversion is a node rather than a call.
	 *
	 * THE SHAPE IS THE DISCRIMINATOR AND IT IS TOTAL, so there is no second token saying which of the
	 * two a caller wrote. A UFunction is always a member of a class, so its path always carries the
	 * `:` that separates the two - including one defined on this very asset
	 * (`/Game/UI/WBP_Menu.WBP_Menu_C:MakeTitle`) - and a UClass path never can.
	 */
	FString Function;

	bool IsSet() const { return !Function.IsEmpty(); }

	bool operator==(const FWidgetIRMvvmConversion& Other) const
	{
		return Function.Equals(Other.Function, ESearchCase::CaseSensitive);
	}
};

/**
 * One binding of `UMVVMBlueprintView::Bindings`.
 *
 * TWO ENDS, NOT ONE. `FMVVMBlueprintViewBinding` holds `SourcePath` and `DestinationPath` whatever
 * the mode is - even `TwoWay` keeps the roles - so the document keeps them too, and `Mode` states
 * the traffic over them rather than the direction being spelled twice. This is the difference from
 * the legacy family, where the target is the key and the source is the value.
 */
struct WIDGETIR_API FWidgetIRMvvmBinding
{
	/** `DestinationPath` - normally a widget property. */
	FWidgetIRMvvmPath Destination;

	/** `SourcePath` - normally a viewmodel member. */
	FWidgetIRMvvmPath Source;

	/** `EMVVMBindingMode`. Not defaulted away: the mode is the binding's meaning. */
	FString Mode;

	/** `OverrideExecutionMode`, empty unless `bOverrideExecutionMode` - absence is the override. */
	FString ExecutionMode;

	bool bEnabled = true;
	bool bCompile = true;

	/**
	 * The function the value passes through on its way from the SOURCE to the destination - the
	 * forward direction, which is the one a `OneWayToDestination` binding runs.
	 */
	FWidgetIRMvvmConversion Convert;

	/**
	 * And on the way back, which only a `TwoWay` or `OneWayToSource` binding ever runs.
	 *
	 * TWO FIELDS RATHER THAN ONE WITH A DIRECTION, because the engine holds two: a two-way binding
	 * converts through a DIFFERENT function each way, and a document with one slot could not say so.
	 */
	FWidgetIRMvvmConversion ConvertBack;

	/**
	 * A conversion on this binding carries pin values or a saved wrapper graph this document does not.
	 *
	 * Narrowed 2026-08-12 from *"a conversion function sits here and is not projected"*: which
	 * function is now stated, and this says the remaining sliver is on THIS binding rather than
	 * leaving `omits:` to answer for the whole file.
	 */
	bool bHasUnprojectedConversionState = false;

	bool operator==(const FWidgetIRMvvmBinding& Other) const
	{
		return Destination == Other.Destination
			&& Source == Other.Source
			&& Mode.Equals(Other.Mode, ESearchCase::CaseSensitive)
			&& ExecutionMode.Equals(Other.ExecutionMode, ESearchCase::CaseSensitive)
			&& bEnabled == Other.bEnabled
			&& bCompile == Other.bCompile
			&& Convert == Other.Convert
			&& ConvertBack == Other.ConvertBack;
	}
};

struct WIDGETIR_API FWidgetIRDocument
{
	int32 Version = WIDGETIR_VERSION;
	FString AssetPath;
	FString AssetKind;
	FString ParentClass;
	FString Signature;

	/**
	 * Asset-level authoring settings - palette category, tick policy, thumbnail settings. Authored
	 * state that belongs to the asset rather than to any widget, so it is neither a widget entity
	 * nor a relation, and omitting it was silent loss.
	 */
	TArray<FAuthoringProperty> DesignSettings;

	TOptional<FWidgetIRNode> Root;

	/**
	 * Content this asset authors into named slots of the tree it INHERITS. A Widget Blueprint
	 * deriving from another may own no root at all and put its entire UI here - a third of this
	 * project's UI assets are that shape, and reading only from the root reported them as empty.
	 * Each entry is a subtree root whose NamedSlot names the inherited slot it fills.
	 */
	TArray<FWidgetIRNode> NamedSlotRoots;

	/**
	 * Widgets this asset OWNS that its tree does not reach: `orphan <Name> = <Class>(…)`.
	 *
	 * STATED, NOT COMMENTED, and that is the whole reason this field exists. An orphan used to be a
	 * `# UNSUPPORTED:` line, which is exactly what row 11o closed `widget_bindings` for — a
	 * diagnostic is a comment, and `gc_apply` does not parse comments. A model asked to clean up an
	 * asset could read about the orphan and had no handle to act on.
	 *
	 * The second reason is the one that makes it a correctness fix rather than an ergonomic one:
	 * under `snapshot` the payload IS the whole tree, so an orphan nobody saw is swept as a
	 * deletion. As a line in the file, it is in front of whoever edits that file.
	 *
	 * NOT AUTHORABLE. Keeping one of these lines in a payload is refused: an orphan is a state to
	 * resolve, and this is not a notation for preserving one. The two legal answers are to move the
	 * declaration into the tree, which adopts it, or to `delete` it. Both are grammar that already
	 * existed — what was missing was somewhere for a parser to learn the widget is there at all.
	 *
	 * ROOTS ONLY. An unparented panel carries its children, so it is one entry and not one per
	 * stranded widget: adopting or deleting a panel moves its subtree with it.
	 */
	TArray<FWidgetIRNode> Orphans;

	/** Widget names this payload removes. Omission alone is never deletion. */
	TArray<FString> DeleteNames;

	/**
	 * Widget renames this payload STATES, C23 §6 step 5.
	 *
	 * Before this the only evidence of a rename was a name that vanished and a name that arrived at
	 * the same tree position, so a widget renamed AND moved in one payload read as a delete plus a
	 * create — a fresh identity, and every binding, navigation entry and `BindWidget` pointing at
	 * the old one broken with nothing logged. Position pairing survives for the payload that cannot
	 * carry a token (a projection under `snapshot`); a stated rename is what a caller writes and it
	 * needs no position at all.
	 */
	TArray<AuthoringCore::FAuthoringRename> Renames;

	/**
	 * Designer property bindings, sorted by `<Widget>.<Property>`.
	 *
	 * Sorting is lossless here where it would not be in the tree: only one binding may exist per
	 * target, the compiler iterates the array without depending on its order, and the engine's
	 * own read of `NamedSlotBindings` is sorted for the same reason - a canonical file must not
	 * move because a map or an add-order did.
	 */
	TArray<FWidgetIRBinding> Bindings;

	/**
	 * Explicit navigation rules, sorted by `<Widget>.<Key>`.
	 *
	 * Document-level and not a member of `FWidgetIRNode`, for the reason `Bindings` is: an
	 * `explicit` rule names a widget that may be declared far later in the tree, so these lines can
	 * only be written once the whole tree exists. Sorting is lossless because a
	 * `UWidgetNavigation` has one answer per key and no order of its own - the six directions are
	 * six named fields on a struct, not an array anyone iterates.
	 *
	 * A DEFAULT rule is absent rather than present-and-default. `EUINavigationRule::Escape` is the
	 * engine's default for every direction and `AcceptFocus` for the policy, and a widget whose
	 * navigation object is entirely default answers `UWidgetNavigation::IsDefaultNavigation()` -
	 * which is what the reader asks, rather than comparing seven fields itself.
	 */
	TArray<FWidgetIRNavigation> Navigation;

	/**
	 * `<Widget>.<Property>` keys this payload unbinds. The `delete` of the binding family, and
	 * omission alone is never removal - under `bOmissionIsDeletion` the payload IS the set and
	 * this list is refused, exactly as a `delete` line is.
	 */
	TArray<FWidgetIRUnbind> Unbinds;

	/**
	 * The viewmodels this view declares, in the engine's own array order.
	 *
	 * NOT SORTED, and that is the one place this family differs from `Bindings` and `Navigation`.
	 * Those two are sorted because the engine's order is incidental - a map iteration, or six named
	 * fields on a struct. `AvailableViewModels` is an authored array the Designer's list reorders,
	 * and `UMVVMBlueprintView::RemoveViewModels` takes indices into it, so its order is state. A
	 * canonical form that sorted it would show no diff when somebody reordered the list and would
	 * silently reorder it back on the first write.
	 */
	TArray<FWidgetIRMvvmViewModel> ViewModels;

	/**
	 * MVVM bindings, sorted by destination then source.
	 *
	 * Sorted where the viewmodels are not, because `Bindings` carries its own `BindingId` and the
	 * compiler resolves each entry independently - there is no index into this array that means
	 * anything to an author. The pair is not a unique key the way a legacy binding's target is, so
	 * the sort is a canonical ORDER and not an identity; what identity a written binding gets is
	 * the writer's question and is deliberately not answered here.
	 */
	TArray<FWidgetIRMvvmBinding> MvvmBindings;

	/**
	 * This asset holds an MVVM view whose events or conditions are not projected.
	 *
	 * `Events` and `Conditions` are instanced object arrays on the same footing as `Resolver`: open
	 * to subclassing, so unreachable from a closed-struct walk. Document-level because they hang off
	 * the view rather than off any one viewmodel or binding.
	 */
	bool bMvvmHasUnprojectedObjects = false;

	TArray<FAuthoringDiagnostic> Diagnostics;
};

struct WIDGETIR_API FWidgetIRReadOptions
{
	/** False is the normal compact form: only deviations from the class/slot defaults. */
	bool bIncludeDefaults = false;

	/**
	 * Prove every value's engine text round-trips before emitting it. Off for ordinary reads
	 * because it constructs a scratch value per leaf; a write path turns it on.
	 */
	bool bVerifyValues = false;
};
