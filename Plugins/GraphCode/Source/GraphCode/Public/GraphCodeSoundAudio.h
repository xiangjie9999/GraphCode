// The bytes of a sound, in and out — so that a caller can HEAR what a `.gc` describes. C13.
//
// **WHY A KIND NEEDED THIS AT ALL** (user-directed 2026-08-19: *"let gc open download or let ai
// listen to the audio, so ai can judge on it"*). `kind:sound_wave` makes every decision about a
// sound reviewable as text, and none of those decisions answers the only question that finally
// matters — does it sound right. A compression type is a claim about audio nobody can check by
// reading it. This is the sound itself, and it is the audio counterpart of what `gc_render` is for
// pictures: the artifact that closes the loop between authoring a thing and judging it.
//
// **RIFF/WAVE AND NOTHING ELSE, AND IT COSTS NO DEPENDENCY** — the same argument `GraphCodeRenderFilm`
// makes for APNG over a movie container, arrived at independently and for the same reason. UE hands
// back 16-bit PCM through `GetImportedSoundWaveData`; a canonical 44-byte RIFF header in front of it
// is a file every player on every platform opens. No encoder, no plugin, no `ffmpeg`, nothing to
// ship, and lossless because the samples are the imported ones rather than a re-compression.
//
// The header is written rather than copied from the source file on purpose. The source is not
// guaranteed to still exist — `import { }` carries a relative path that may point at another
// machine — and for a multichannel wave the payload is several concatenated RIFF files rather than
// one, so "copy the original bytes" is not a thing that can be done in general. Building the header
// from what the engine reports is the one path that works for every wave.

#pragma once

#include "CoreMinimal.h"

class UObject;
class USoundWave;

struct FGraphCodeSoundAudioResult
{
	bool bSuccess = false;

	/** The complete `.wav` file — header and samples — ready to write or to base64. */
	TArray<uint8> Bytes;

	uint32 SampleRate = 0;
	uint16 Channels = 0;
	/** Seconds, from the sample count rather than from `USoundWave::Duration`. See the .cpp. */
	float Seconds = 0.0f;

	FString Error;
};

// PUBLIC because `gc_open kind:"sound"` lives in UnrealClaude and this is the whole of what it
// needs from GraphCode — the same direction `GraphCodeSearch.h` already faces. Nothing else about
// the kind crosses the module boundary: the projection, the writer and the policy stay Private,
// because a tool has no business knowing how a wave is spelled, only how it sounds.
class GRAPHCODE_API FGraphCodeSoundAudio
{
public:
	/**
	 * A canonical 16-bit PCM RIFF/WAVE file around samples somebody already has.
	 *
	 * Public because the sound specs build their fixture with it: a `USoundWave` with no payload
	 * cannot re-encode, so a spec that exercises the WRITE needs a wave carrying real audio, and
	 * hand-rolling a second header in the test would be a second implementation to disagree with
	 * this one.
	 */
	static TArray<uint8> BuildRiffWave(const TArray<uint8>& Pcm16, uint32 SampleRate,
		uint16 Channels);

	/**
	 * One wave's imported audio as a playable `.wav`.
	 *
	 * Fails with a NAMED reason rather than an empty buffer: a wave whose editor payload was stripped
	 * by a cook, a procedural or MetaSound source that has no imported audio at all, and a wave this
	 * kind does not own are three different answers and a caller acts differently on each.
	 */
	static FGraphCodeSoundAudioResult Export(UObject* Sound);
};
