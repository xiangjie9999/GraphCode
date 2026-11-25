// Copyright GraphCode Sdn Bhd. All Rights Reserved.
//
// Delta files — `GC-TERRAIN-DESIGN.md` §6 and row L4. The blob a `sculpt(source=file'…')` reads,
// the map that turns it into centimetres, and the hash that says which ground it was cut from.
//
// **THE BLOB IS THE ENGINE'S OWN `uint16` AND NOT OUR CENTIMETRES, and that is the whole decision.**
// The user's requirement is that a capture applies back PERFECTLY, and a landscape stores height as
// a 16-bit value: written out as centimetres and read back it would round twice, so the ground would
// come home near where it started rather than on it. Stored verbatim, a capture goes out and comes
// back on the identical integers — the quantiser's `round()` recovers exactly the value that
// produced the centimetre, because that centimetre came from the same linear map.
//
// New sculpts use lossless 16-bit grayscale PNG. Existing raw little-endian uint16 files still
// read and merge in their original format. The height map and base hash remain on the .gcw line.

#pragma once

#include "CoreMinimal.h"

struct FGcTerrainField;

/**
 * What one `sculpt(…)` line says about its file — enough to read the blob without opening it, and
 * enough to refuse it by name when it does not fit.
 *
 * ON THE INSTRUCTION RATHER THAN IN THE BLOB, so the document is self-describing: `git log` shows
 * the range changing, a reviewer can see a delta was cut against a different base, and a file that
 * has gone missing is still fully described by the line that wanted it.
 */
struct GRAPHCODE_API FGcTerrainDeltaSource
{
	/** `source=file'Deltas/Human_01.png'` — relative to the .gcw, or rooted under /Game. */
	FString Path;

	/** `size=(505, 505)` — samples across and up; must match the PNG dimensions or raw length. */
	int32 SizeX = 0;
	int32 SizeY = 0;

	/**
	 * `range=(Min=…, Max=…)` — the world centimetres that stored 0 and stored 65535 mean.
	 *
	 * **ONE SPELLING FOR BOTH NATURES OF FILE**, which is why it is a range and not a `zero=`/`step=`
	 * pair. A whole ground captured off a landscape has a range straddling the actor's own Z; one
	 * edit layer's contribution has a range straddling nothing at all. The arithmetic is identical
	 * and only the two numbers differ, so there is one field and no mode.
	 */
	double RangeMin = 0.0;
	double RangeMax = 0.0;

	/**
	 * `base=<md5>` — the ground this was cut from, as `HashField` measured it. Empty states none.
	 *
	 * **OPTIONAL, BECAUSE A FILE THAT WAS NEVER CAPTURED HERE HAS NO BASE TO STAMP.** A DEM was cut
	 * from the Earth. Requiring one would make every hand-authored line start with a digest nobody
	 * can compute, and inventing one would make the check that matters — *this patch has been moved
	 * onto ground that changed underneath it* — pass for the wrong reason.
	 */
	FString Base;

	double Step() const { return (RangeMax - RangeMin) / 65535.0; }

	int32 Samples() const { return SizeX * SizeY; }
};

class GRAPHCODE_API FGraphCodeTerrainDelta
{
public:
	/**
	 * The ground a delta was cut from, as 32 lowercase hex digits.
	 *
	 * **HEIGHTS ARE ROUNDED TO THE WHOLE CENTIMETRE BEFORE HASHING, and that is a ruling rather than
	 * an implementation detail.** The hash exists to answer *has the ground under this patch
	 * changed?*, and a base that differs by a hundredth of a centimetre is the same ground by any
	 * reading an author would recognise. Hashing the raw doubles instead would make the answer
	 * depend on the last bit of an accumulated sum, so re-ordering two lines that do not overlap —
	 * which changes nothing anybody can see — would report every delta below them as rebased. The
	 * cost is stated rather than hidden: two grounds a hundredth of a centimetre apart hash the
	 * same, and a difference at the rounding boundary flips on nothing.
	 *
	 * **THE GEOMETRY IS IN IT TOO.** The same heights over a different origin, or at a different
	 * sample spacing, are not the same ground — a patch cut from one and dropped on the other lands
	 * somewhere else entirely, and heights alone would call that a match.
	 */
	static FString HashField(const FGcTerrainField& Field);

	/**
	 * `source=` against the directory the `.gcw` lives in.
	 *
	 * **REFUSES AN ABSOLUTE PATH AND REFUSES `..`**, by name. A `.gcw` is committed and travels
	 * between machines, so a path that reaches outside the document's own folder is one that works
	 * for its author and for nobody else — and a document that can read any file on the disk is a
	 * document worth being careful about.
	 */
	static bool Resolve(const FString& SourceDir, const FString& Relative, FString& OutFull,
		FString& OutError);

	/**
	 * `file'…'` — a value naming a file outside the document, the sibling of `obj'…'`.
	 *
	 * **ONE READER FOR EVERY LINE THAT NAMES A FILE** — a terrain `sculpt(…)` and a painted forest's
	 * `foliage(source=…)` — so the mark means one thing wherever it appears. `Example` is the whole
	 * phrase the refusal offers after *One is*, so each caller's sentence stays its own.
	 */
	static bool ReadFileRef(const FString& Value, const TCHAR* Example, FString& OutPath,
		FString& OutError);

	/**
	 * Read a blob and map it to world centimetres, one per sample, row-major.
	 *
	 * Accepts 16-bit grayscale PNG or legacy raw uint16. Refuses mismatched dimensions/length,
	 * color PNGs and other bit depths; no color conversion or resampling is performed.
	 * With OutImageSize supplied, a PNG may omit both source dimensions (zero); its own dimensions
	 * are returned. This stamp mode also accepts 8-bit grayscale PNG, expanded exactly to uint16.
	 * Recorded sculpts still declare and validate their exact grid size and require 16-bit data.
	 */
	static bool Load(const FString& FullPath, const FGcTerrainDeltaSource& Source,
		TArray<double>& OutCentimetres, FString& OutError, FIntPoint* OutImageSize = nullptr);

	/** The inverse: stored values straight to disk, making the directory if it is not there. */
	static bool Save(const FString& FullPath, const TArray<uint16>& Stored,
		int32 SizeX, int32 SizeY, FString& OutError);
};
