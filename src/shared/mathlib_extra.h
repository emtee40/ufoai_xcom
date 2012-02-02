/**
 * @file mathlib_extra.h
 * @brief Special, additional math algorithms for floating-point values
 *
 * These are special, custom functions for influencing floating-point variables,
 * Usually with values that start and end either between 0.f and 1.f or
 * -1.f and +1.f.  They can push variables toward or away from the
 * minimum and maxumium possible values in a non-linear, curved shape.
 * Unless stated otherwise, "fpVal" is the original value to be altered, and
 * "mEffect" is a value between 0.f and 1.f that affects how strong the curve is,
 * with 1.f having the most effect.  Some functions also have a "cntPnt" (Center Point)
 * value that can be given, a point that the fpVal curves toward or away from.
 *
 * NOTE: A special .PNG file will also be available that will show
 * these functions on a graph, to help in choosing which one is most
 * suitable to use.  (Please see "mathlib_extra.png")
 */

/*
Copyright (C) 2002-2011 UFO: Alien Invasion.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#ifndef MATHLIB_EXTRA_H
#define MATHLIB_EXTRA_H

#include "ufotypes.h"
#include "mathlib.h"

/**
 * @brief This "DENORM" is for avoiding denormal-related issues when values equal to a
 * perfect 0.0f are passed to some functions, which could otherwise
 * cause very high CPU use (slow things down), or sometimes even
 * break stuff (error, crash, etc.)  Also used to avoid divide-by-zero issues.
 *
 * The "Inv" (Inverted) value is for avoiding similar issues when a variable can almost reach
 * 1.0f but should not hit it perfectly.
 */
#ifndef DENORM
#define DENORM 0.0000000001
#endif
#ifndef DENORM_INV
#define DENORM_INV (1.0 - (DENORM))
#endif

/**
 * @brief This takes a floating-point variable and, if it happens to have a value
 * of perfect 0.0f, will gently nudge it UP a little to avoid denormal or divide-by-zero problems.
 * @param[in] fpVal Floating-point variable that should be checked, to make sure it
 * isn't a perfect zero.
 * @return fpVal The Floating-pont variable, with any corrections.
 */
#ifndef ChkDNorm
#define ChkDNorm(fpVal) ((fpVal) == 0.0 ? (DENORM) : (fpVal) )
#endif

/**
 * @brief This takes a floating-point variable and, if it happens to have a value
 * of perfect 1.0f, will gently nudge it DOWN a little to avoid denormal or divide-by-zero problems.
 * @param[in] fpVal Floating-point variable that should be checked, to make sure it
 * isn't a perfect 1.0f.
 * @return fpVal The Floating-pont variable, with any corrections.
 */
#ifndef ChkDNorm_Inv
#define ChkDNorm_Inv(fpVal) ( ((fpVal) == 1.0) ? (DENORM_INV) : (fpVal) )
#endif

/** fpVal = 0.0f to 1.0f for these functions */

/* Basic curves */
double FpCurveUp(double fpVal, double mEffect);
double FpCurveDn(double fpVal, double mEffect);

/* "RS" = "Rapid Start", curve has a different, skewed shape */
double FpCurveUpRs(double fpVal, double mEffect);
double FpCurveDnRs(double fpVal, double mEffect);

/** These next functions can curve up OR down, depending on the
 * starting value of "x" and the center mid-point, which
 * by default is 0.5f.  The "_u_" in the function name
 * means "unsigned" because all the values are still between
 * 0.0f and 1.0f, while later functions with "_s_" will work
 * with values between -1.0f and +1.0f.
 */
/* "cntPnt" = center point, function curves "fpVal" TOWARD "cntPnt" */
double FpCurve1D_u_in(double fpVal, double mEffect, double cntPnt);
/* "cntPnt" = center point, function curves "fpVal" AWAY FROM "cntPnt" */
double FpCurve1D_u_out(double fpVal, double mEffect, double cntPnt);

/** fpVal = -1.0f to 1.0f for these functions */

/* Function curves "fpVal" AWAY FROM 0.0, toward 1.0f or -1.0f */
double FpCurve1D_s_out(double fpVal, double mEffect);

/** These are floating-point curves where the input can start at 0.f but
* has no upper limit.  The output will always be within 0.f and +- 1.f.
* They may be used for newer alien interest code, air combat, and other stuff.
* If they need to be used often, the (float) based functions should of course be
* faster than the ones that use (double)s.
*/
float XMath_CurveUnlFixed_f(const float inpVal, const float hard);
double XMath_CurveUnlFixed_d(const double inpVal, const double hard);

float XMath_CurveUnlScaled_f(const float inpVal, const float hard, const float scale);
double XMath_CurveUnlScaled_d(const double inpVal, const double hard, const double scale);

/**
 * @brief An RC-style low-pass filter structure.
 *
 * Although filters similar to this are used in audio DSP on audio samples, this
 * structure is adapted a bit so it can be used for smooth transitions of values for all
 * kinds of purposes, including angles, vectors, etc.  An example use would be for air combat,
 * so that when an aircraft tries to change direction in mid-air, it doesn't change all the way
 * to a new, intended direction of movement instantaneously (it has intertia and air drag, etc.),
 * but instead gradually changes velocity to eventually reach a new target direction of movement.
 * Note that this filter should not have its pass rate changed frequently, if you need a filter that
 * has a rapidly changing pass rate, please consider the xMathRcDynBufferF_s filter.
 */
typedef struct xMathRcBufferF_s {
	float rate;			/**< How many times things are ran (ticks) per second. */
	float passRate;		/**< Determines how much affect is applied during a tick(), can be from 0.f to 1.f. */
	float input;		/**< When it comes time for a tick(), this is the input value that will be used. */
	float buffer;		/**< Buffer value which should not be discarded between ticks. */
	float passFactorA;	/**< Used during a tick() to calculate the end-result value. */
	float passFactorB;	/**< Used during a tick() to calculate the end-result value. */
} xMathRcBufferF_t;
void XMath_RcBuffInit(xMathRcBufferF_t *rcbuff, const float inpRate, const float inpPass);
void XMath_RcBuffInput(xMathRcBufferF_t *rcbuff, const float inpVal);
float XMath_RcBuffGetOutput(const xMathRcBufferF_t *rcbuff);
void XMath_RcBuffTick(xMathRcBufferF_t *rcbuff);
void XMath_RcBuffForceBuffer(xMathRcBufferF_t *rcbuff, const float inpForceVal);
void XMath_RcBuffChangeRate(xMathRcBufferF_t *rcbuff, const float inpRate);
void XMath_RcBuffChangePassRate(xMathRcBufferF_t *rcbuff, const float inpPass);

/**
 * @brief An RC-style low-pass filter structure supporting a dynamic pass rate that can change frequently.
 *
 * Although filters similar to this are used in audio DSP on audio samples, this
 * structure is adapted a bit so it can be used for smooth transitions of values for all
 * kinds of purposes, including angles, vectors, etc.  An example use would be for air combat,
 * so that when an aircraft tries to change direction in mid-air, it doesn't change all the way
 * to a new, intended direction of movement instantaneously (it has intertia and air drag, etc.),
 * but instead gradually changes velocity to eventually reach a new target direction of movement.
 * Note that if you want a more simple filter that doesn't change pass rate too often, you may want to
 * consider the xMathRcBufferF_s filter instead.
 */
typedef struct xMathRcDynBufferF_s {
	float rate;			/**< How many times things are ran (ticks) per second. */
	float passRateHigh;	/**< Determines how much affect is applied during a tick(), can be from 0.f to 1.f. This is the maximum pass rate. */
	float passRateLow;	/**< Determines how much affect is applied during a tick(), can be from 0.f to 1.f. This is the minimum pass rate. */
	float input;		/**< When it comes time for a tick(), this is the input value that will be used. */
	float buffer;		/**< Buffer value which should not be discarded between ticks. */
	float passLevel;	/**< Value from 0.f to 1.f that determines how 'open' the filter is, or where the pass rate is between min and max pass rates. */
	float passFactorAhi;	/**< Used during a tick() to calculate the end-result value. (Max pass rate.) */
	float passFactorBhi;	/**< Used during a tick() to calculate the end-result value. (Max pass rate.) */
	float passFactorAlo;	/**< Used during a tick() to calculate the end-result value. (Min pass rate.) */
	float passFactorBlo;	/**< Used during a tick() to calculate the end-result value. (Min pass rate.) */
	float passFactorAcurrent;	/**< Used during a tick() to calculate the end-result value. (Current pass rate, as set by "passLevel" value.) */
	float passFactorBcurrent;	/**< Used during a tick() to calculate the end-result value. (Current pass rate, as set by "passLevel" value.) */
} xMathRcDynBufferF_t;
void XMath_RcDynBuffInit(xMathRcDynBufferF_t *rcbuff, const float inpRate, const float inpPassMin, const float inpPassMax);
void XMath_RcDynBuffInput(xMathRcDynBufferF_t *rcbuff, const float inpVal);
void XMath_RcDynBuffSetDynamic(xMathRcDynBufferF_t *rcbuff, const float dynVal);

#endif
