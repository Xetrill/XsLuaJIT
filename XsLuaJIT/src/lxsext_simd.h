#ifndef lxsext_simd_h
#define lxsext_simd_h

#include "xssemath.h"

#ifdef lua_number2int
# undef lua_number2int
#endif
#ifdef luai_numadd
# undef luai_numadd
#endif
#ifdef luai_numsub
# undef luai_numsub
#endif
#ifdef luai_nummul
# undef luai_nummul
#endif
#ifdef luai_numdiv
# undef luai_numdiv
#endif
//#ifdef luai_nummod
//# undef luai_nummod
//#endif
//#ifdef luai_num
//# undef luai_num
//#endif

#define lua_number2int(i,d) __asm fld d __asm fisttp i

// TODO use Agners vectorclass library; use Agners asmlib library otherwise.
#define luai_numadd(a,b)    (_xs_sse2_add((a), (b)))
#define luai_numsub(a,b)    (_xs_sse2_sub((a), (b)))
#define luai_nummul(a,b)    (_xs_sse2_mul((a), (b)))
#define luai_numdiv(a,b)    (_xs_sse2_div((a), (b)))
//#define luai_nummod(a,b)    ((a) - floor((a)/(b))*(b))
//#define luai_numpow(a,b)    (pow(a,b))
//#define luai_numunm(a)      (-(a))
//#define luai_numeq(a,b)     ((a)==(b))
//#define luai_numlt(a,b)     ((a)<(b))
//#define luai_numle(a,b)     ((a)<=(b))
//#define luai_numisnan(a)    (!luai_numeq((a), (a)))

#endif // lxsext_simd_h