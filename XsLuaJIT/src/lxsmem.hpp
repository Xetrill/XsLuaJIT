#ifndef lxsmem_hpp
#define lxsmem_hpp 1

#include "lxsext.h"
#include "lxs_def.h"


//==============================================================================

size_t lxs_mnext_pow2(size_t v)
{
    --v;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    return ++v;
}

size_t lxs_mgrow(lua_State* const L,
                 size_t current_cap,
                 size_t new_cap,
                 bool force, /*= false*/
                 double growth_factor/* = LUAXS_STR_GROWTH_FACTOR*/)
{
    lxs_assert(L, L);
    lxs_assert(L, growth_factor > 1.0);

    if (!force && current_cap < new_cap)
    {
        new_cap = lxs_mnext_pow2(STATIC_CAST(size_t,
            new_cap * growth_factor)
        );
    }

    if (new_cap >= ((STATIC_CAST(size_t, (~STATIC_CAST(size_t, 0))) - 2) >> 1))
        lxs_error(L, "buffer way too big (%u)", new_cap);

    return new_cap;
}

//==============================================================================


#endif // lxsmem_hpp