// Copyright GraphCode Sdn Bhd. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * ONE CURVE OF NUMBERS, FILLED IN THE SAME WAY BY EVERY FORMAT THAT WRITES ONE.
 *
 * `.gcshape`'s `profile()` reads it across an outline or along a travel; `.gcmove`'s reads it across
 * a clip (`GC-MOVE-GRAMMAR.md` §4). They are one object — stops at `t` from 0 to 1 and a smooth fill
 * between them — so the fill is written once: two copies of an interpolator drift, and drift here is
 * a shape and a motion disagreeing about what one profile means.
 *
 * Header-only and templated on the stop, so each format keeps its own stop type (Shape's carries a
 * percent flag) and a per-vertex call allocates nothing. A stop is anything with `.T` and `.Value`.
 */
namespace AuthoringCurve
{
	/**
	 * The value at `InT` of stops sorted by `.T`.
	 *
	 * **SMOOTH MEANS MONOTONE CUBIC (PCHIP), AND THE CHOICE IS NOT COSMETIC.** The tangents are the
	 * weighted harmonic mean of the neighbouring secants and are ZEROED wherever the slope changes
	 * sign, so the curve never leaves the interval its neighbouring stops define. A Catmull-Rom through
	 * the same stops overshoots, and every profile scales something physical — a radius that dips below
	 * zero between two gentle stops is a shape that turns inside out.
	 *
	 * **OPEN, the default:** `InT` clamps to 0..1, and `0.0` and `1.0` may be left out — the nearest
	 * declared value holds to the end rather than being extrapolated off it. Two stops are a line.
	 *
	 * **LOOPED:** the domain wraps, so `t = 1` IS `t = 0` and the last stop's segment runs on into the
	 * first. Every stop's tangent reads BOTH neighbours across the seam, which is what lets a motion
	 * carry through it instead of the curve flattening there. The caller refuses stops outside [0, 1)
	 * and two stops at one `t`, which is the seam stated twice.
	 */
	template <typename StopType>
	double Evaluate(const TArray<StopType>& Stops, const double InT, const bool bLinear, const bool bLoop)
	{
		const int32 Count = Stops.Num();
		if (Count == 0)
		{
			return 0.0;
		}
		if (Count == 1)
		{
			return Stops[0].Value;
		}

		// A stop past either end of the array, one period round when looped and clamped when open.
		auto TAt = [&Stops, Count, bLoop](const int32 Index) -> double
		{
			if (!bLoop)
			{
				return Stops[FMath::Clamp(Index, 0, Count - 1)].T;
			}
			const int32 Wrapped = ((Index % Count) + Count) % Count;
			return Stops[Wrapped].T + FMath::FloorToDouble(static_cast<double>(Index) / Count);
		};
		auto ValueAt = [&Stops, Count, bLoop](const int32 Index) -> double
		{
			return bLoop ? Stops[((Index % Count) + Count) % Count].Value
				: Stops[FMath::Clamp(Index, 0, Count - 1)].Value;
		};

		double T = InT;
		int32 Left = 0;
		if (bLoop)
		{
			T = InT - FMath::FloorToDouble(InT);
			Left = Count - 1;
			for (int32 Index = 0; Index < Count; ++Index)
			{
				if (Stops[Index].T > T)
				{
					Left = Index - 1;
					break;
				}
			}
		}
		else
		{
			T = FMath::Clamp(InT, 0.0, 1.0);
			if (T <= Stops[0].T)
			{
				return Stops[0].Value;
			}
			if (T >= Stops.Last().T)
			{
				return Stops.Last().Value;
			}
			int32 Right = 1;
			while (Right < Count && Stops[Right].T < T)
			{
				++Right;
			}
			Left = Right - 1;
		}

		const double Span = TAt(Left + 1) - TAt(Left);
		if (Span <= UE_DOUBLE_SMALL_NUMBER)
		{
			return ValueAt(Left + 1);
		}
		const double U = (T - TAt(Left)) / Span;

		// An open two-stop curve is a straight line either way; a looped one is not, since both stops
		// then have neighbours on each side and the tangents read them.
		if (bLinear || (!bLoop && Count == 2))
		{
			return FMath::Lerp(ValueAt(Left), ValueAt(Left + 1), U);
		}

		auto Secant = [&TAt, &ValueAt](const int32 Index)
		{
			const double D = TAt(Index + 1) - TAt(Index);
			return D > UE_DOUBLE_SMALL_NUMBER ? (ValueAt(Index + 1) - ValueAt(Index)) / D : 0.0;
		};
		auto Tangent = [&TAt, &Secant, Count, bLoop](const int32 Index)
		{
			// An open curve's end stop has one neighbour, so its tangent is that one secant.
			if (!bLoop && Index <= 0)
			{
				return Secant(0);
			}
			if (!bLoop && Index >= Count - 1)
			{
				return Secant(Count - 2);
			}
			const double Before = Secant(Index - 1);
			const double After = Secant(Index);
			const double SpanBefore = TAt(Index) - TAt(Index - 1);
			const double SpanAfter = TAt(Index + 1) - TAt(Index);
			// **TWO STOPS AT ONE `t` ARE A STEP, AND A STEP ENDS THE CURVE ON EACH SIDE**
			// (`GC-SHAPE-AXIS-SLOPES-DESIGN.md`, user-ruled 2026-10-08). So each side of it takes the
			// end stop's tangent above — its one real secant — instead of reading the zero-width
			// segment as a flat and arriving at the step level. A curve with no repeated `t` never
			// reaches these two lines.
			if (!bLoop && SpanBefore <= UE_DOUBLE_SMALL_NUMBER)
			{
				return After;
			}
			if (!bLoop && SpanAfter <= UE_DOUBLE_SMALL_NUMBER)
			{
				return Before;
			}
			if (Before * After <= 0.0)
			{
				return 0.0;
			}
			const double Weight = 2.0 * SpanAfter + SpanBefore;
			const double WeightNext = SpanAfter + 2.0 * SpanBefore;
			return (Weight + WeightNext) / (Weight / Before + WeightNext / After);
		};

		const double M0 = Tangent(Left) * Span;
		const double M1 = Tangent(Left + 1) * Span;
		const double U2 = U * U;
		const double U3 = U2 * U;
		return (2.0 * U3 - 3.0 * U2 + 1.0) * ValueAt(Left)
			+ (U3 - 2.0 * U2 + U) * M0
			+ (-2.0 * U3 + 3.0 * U2) * ValueAt(Left + 1)
			+ (U3 - U2) * M1;
	}
}
