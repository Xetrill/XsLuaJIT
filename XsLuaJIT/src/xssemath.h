#pragma once
#ifndef xsssemath_h
#define xsssemath_h

#include "lxs_def.h"

#include <emmintrin.h> // SSE2
#include <smmintrin.h> // SSE4



/******************************************************************************
 * SSE2
 ******************************************************************************/

XS_AINLINE static double _xs_sse2_min(double lhs, double rhs)
{
    _mm_store_sd(&lhs, _mm_min_sd(_mm_set_sd(lhs), _mm_set_sd(rhs)));
    return lhs;
}

XS_AINLINE static double _xs_sse2_max(double lhs, double rhs)
{
    _mm_store_sd(&lhs, _mm_max_sd(_mm_set_sd(lhs), _mm_set_sd(rhs)));
    return lhs;
}

XS_AINLINE static double _xs_sse2_clamp(double val, double minval, double maxval)
{
    _mm_store_sd(&val, _mm_min_sd(_mm_max_sd(_mm_set_sd(val),
        _mm_set_sd(minval)), _mm_set_sd(maxval)));
    return val;
}

XS_AINLINE static double _xs_sse2_add(double lhs, double rhs)
{
    _mm_store_sd(&lhs, _mm_add_sd(_mm_set_sd(lhs), _mm_set_sd(rhs)));
    return lhs;
}

XS_AINLINE static double _xs_sse2_sub(double lhs, double rhs)
{
    _mm_store_sd(&lhs, _mm_sub_sd(_mm_set_sd(lhs), _mm_set_sd(rhs)));
    return lhs;
}

XS_AINLINE static double _xs_sse2_mul(double lhs, double rhs)
{
    _mm_store_sd(&lhs, _mm_mul_sd(_mm_set_sd(lhs), _mm_set_sd(rhs)));
    return lhs;
}

XS_AINLINE static double _xs_sse2_div(double lhs, double rhs)
{
    _mm_store_sd(&lhs, _mm_div_sd(_mm_set_sd(lhs), _mm_set_sd(rhs)));
    return lhs;
}

XS_AINLINE static double _xs_sse2_sqrt(double val)
{
    _mm_store_sd(&val, _mm_sqrt_sd(_mm_set_sd(0.0), _mm_set_sd(val)));
    return val;
}


/******************************************************************************
 * SSE3
 ******************************************************************************/


//XS_AINLINE static __m128 _xs_sse3_floor(const __m128& a)
//{
//    __m128 v0   = _mm_setzero_ps();
//    __m128 v1   = _mm_cmpeq_ps(v0,v0);
//    __m128 vd5  = *(__m128*)&_mm_slli_epi32(*(__m128i*)&v1, 26);
//    vd5         = *(__m128*)&_mm_srli_epi32(*(__m128i*)&vd5, 2); // make a 0.5f value
//    __m128 vad5 = _mm_sub_ps(a, vd5);
//    __m128i i   = _mm_cvtps_epi32(vad5);
//    __m128 r    = _mm_cvtepi32_ps(i);
//    return r;
//}
//
//XS_AINLINE static __m128 _xs_sse3_ceil(const __m128& a)
//{
//    __m128 v0   = _mm_setzero_ps();
//    __m128 v1   = _mm_cmpeq_ps(v0,v0);
//    __m128 vd5  = *(__m128*)&_mm_slli_epi32(*(__m128i*)&v1, 26);
//    vd5         = *(__m128*)&_mm_srli_epi32(*(__m128i*)&vd5, 2); // make a 0.5f value
//    __m128 vad5 = _mm_add_ps(a, vd5);
//    __m128i i   = _mm_cvtps_epi32(vad5);
//    __m128 r    = _mm_cvtepi32_ps(i);
//    return r;
//}
//
//XS_AINLINE static __m128 _xs_sse3_round(const __m128& a)
//{
//    __m128i i = _mm_cvtps_epi32(a);
//    __m128 r  = _mm_cvtepi32_ps(i);
//    return r;
//}
//
//XS_AINLINE static __m128 _xs_sse3_mod(const __m128& a, const __m128& aDiv)
//{
//    __m128 c      = _mm_div_ps(a,aDiv);
//    __m128i i     = _mm_cvttps_epi32(c);
//    __m128 cTrunc = _mm_cvtepi32_ps(i);
//    __m128 base   = _mm_mul_ps(cTrunc, aDiv);
//    __m128 r      = _mm_sub_ps(a, base);
//    return r;
//}


/******************************************************************************
 * SSE4
 ******************************************************************************/

XS_AINLINE static double _xs_sse41_round(double val)
{
    _mm_store_sd(&val,
        _mm_round_sd(
            _mm_set_sd(0.0),
            _mm_set_sd(val),
            _MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC
        )
    );
    return val;
}

XS_AINLINE static double _xs_sse41_ceil(double val)
{
    _mm_store_sd(&val,
        _mm_round_sd(
            _mm_set_sd(0.0),
            _mm_set_sd(val),
            _MM_FROUND_TO_POS_INF | _MM_FROUND_NO_EXC
        )
    );
    return val;
}

XS_AINLINE static double _xs_sse41_floor(double val)
{
    _mm_store_sd(&val,
        _mm_round_sd(
            _mm_set_sd(0.0),
            _mm_set_sd(val),
            _MM_FROUND_TO_NEG_INF | _MM_FROUND_NO_EXC
        )
    );
    return val;
}

XS_AINLINE static double _xs_sse41_mod(double lhs, double rhs)
{
    // a - floor(a / b) * b
    return _xs_sse2_sub(
        lhs,
        _xs_sse41_floor(
            _xs_sse2_mul(
                _xs_sse2_div(
                    lhs,
                    rhs
                ),
                rhs
            )
        )
    );
}


//==============================================================================

#endif // xsssemath_h
