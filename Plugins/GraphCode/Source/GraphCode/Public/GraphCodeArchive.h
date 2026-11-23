// Archives as import sources — `Docs/GC-IMPORT-DESIGN.md` §7.
//
// A `.zip` or a `.7z` is not a format Interchange translates; it is a BOX of files that are. So
// nothing here knows anything about importing. It answers three questions — is this path an
// archive, what is inside it, and where did it land when unpacked — and `FGraphCodeImport` decides
// what to do with the answers.
//
// THE EXTRACTOR IS `bsdtar`, WHICH IS LIBARCHIVE, AND THAT IS ONE MECHANISM RATHER THAN FOUR.
// Windows 10 1803+ and macOS ship it as `tar`; it reads zip, 7z, tar and the compressed tars from
// the same code path, so supporting a second container format costs nothing here. The engine
// carries `libzip` (ZIP only) and no 7z reader at all, so using it would have meant one library for
// `.zip`, a second for `.7z`, and a hand-written 7z header parser nobody should write. GraphCode
// already shells out to `git` for the whole VCS surface, and this is the same trade with the same
// refusal shape: probe once, and name the missing tool rather than failing obscurely.
//
// It is READ-ONLY on the archive. Nothing here writes into one, and extraction only ever writes
// under the project's `Intermediate/`, which is disposable by definition.

#pragma once

#include "CoreMinimal.h"

class GRAPHCODE_API FGraphCodeArchive
{
public:
	/**
	 * Is this path an archive rather than a file to import?
	 *
	 * BY EXTENSION, DELIBERATELY, and it is the one question that cannot be asked of the extractor.
	 * "Try to list it and see" would hand every unreadable `.fbx` to libarchive and let a false
	 * positive decide the whole route; the caller needs a decidable answer before anything runs.
	 * Whether an archive that CLAIMS to be one can actually be read is a separate question, and
	 * `List` is what answers it.
	 */
	static bool IsArchivePath(const FString& Path);

	/** Every extension `IsArchivePath` accepts, lowercase and without the dot, for a refusal to quote. */
	static TArray<FString> ArchiveExtensions();

	/**
	 * The members of an archive, as relative paths with forward slashes and no trailing-slash
	 * directory entries.
	 *
	 * Fails when the extractor is missing or cannot read this container, and says which.
	 */
	static bool List(const FString& ArchiveFile, TArray<FString>& OutMembers, FString& OutError);

	/**
	 * Unpack the WHOLE archive into `DestinationDirectory`, which is created if absent.
	 *
	 * WHOLE, NEVER THE SELECTED MEMBERS, and that is not laziness. An FBX names its textures by
	 * relative path, so importing one member out of a pack whose `Textures/` folder was left in the
	 * box produces a mesh with missing materials and no error — the failure is silent and it is
	 * discovered days later. Unpacking everything costs disk in `Intermediate/` and nothing else.
	 */
	static bool ExtractAll(const FString& ArchiveFile, const FString& DestinationDirectory,
		FString& OutError);

	/**
	 * Where an archive unpacks: `Intermediate/GraphCode/ImportStaging/<name>-<hash>`.
	 *
	 * KEYED ON THE ARCHIVE'S OWN HASH, so a plan and the import that follows it resolve to the same
	 * directory without carrying the path between the two calls, and two versions of one archive
	 * cannot land on top of each other. Under `Intermediate/` because that is the directory the
	 * project already treats as disposable — nothing here belongs in `Content/` or in git.
	 */
	static FString StagingDirectory(const FString& ArchiveFile, const FString& ArchiveHash);

	/**
	 * The extractor's own version line, or empty when there is none to run.
	 *
	 * Exposed so a refusal can quote what it found rather than asserting absence, and so a spec can
	 * skip itself on a machine without libarchive instead of failing there.
	 */
	static FString ExtractorVersion();
};
