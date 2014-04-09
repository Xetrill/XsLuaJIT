#ifndef leastl_hpp
#define leastl_hpp 1

#define EASTL_USER_DEFINED_ALLOCATOR 1
#define EASTL_ALLOCATOR_DEFAULT_NAME "LuaAllocator"
#define EASTLAllocatorType LuaAllocator::allocator
#define EASTLAllocatorDefault LuaAllocator::GetDefaultAllocator

#define EASTL_ASSERT_ENABLED LUAXS_DEBUG

#define EASTL_NAME_ENABLED 0
#define EASTL_ALLOCATOR_COPY_ENABLED 1
#define EASTL_RTTI_ENABLED 0
#define EASTL_EXCEPTIONS_ENABLED 0
#define EASTL_STRING_OPT_LENGTH_ERRORS 0
#define EASTL_STRING_OPT_RANGE_ERRORS 0
#define EASTL_STRING_OPT_ARGUMENT_ERRORS 0

#ifdef __cplusplus
extern "C" {
#endif
#include "lxsext.h"
#if LUAXS_EASTL_LUAM_MALLOC
#include "lmem.h"
#endif
#ifdef __cplusplus
};
#endif

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <eabase/eabase.h>
#include <eastl/internal/config.h>



namespace LuaAllocator
{
    class allocator
    {
    public:
        EASTL_ALLOCATOR_EXPLICIT inline allocator(
            const char* EASTL_NAME(name) = EASTL_ALLOCATOR_DEFAULT_NAME)
#if EASTL_NAME_ENABLED
            : name_(name)
#endif
        {
        }

        inline allocator(const allocator& EASTL_NAME(alloc))
#if EASTL_NAME_ENABLED
            : name_(alloc.name_)
#endif
        {
        }

        inline allocator(const allocator&, const char* EASTL_NAME(name))
#if EASTL_NAME_ENABLED
            : name_(name ? name : EASTL_ALLOCATOR_DEFAULT_NAME)
#endif
        {
        }

        //----------------------------------------------------------------------

        inline allocator& operator=(const allocator& EASTL_NAME(alloc))
        {
#if EASTL_NAME_ENABLED
            name_ = alloc.name_;
#endif
            return *this; // All considered equal since they're global malloc/free
        }



        //----------------------------------------------------------------------

        inline void* allocate(size_t n, int flags = 0) const
        {
            //CDBG(NULL, "eastl allocation of %u bytes", n);

#if LUAXS_EASTL_LUAM_MALLOC
            return luaM_malloc(lxs_mt(), n);
#else   // LUAXS_EASTL_LUAM_MALLOC

            return _aligned_malloc(n, 8); // since this allocator API doesn't
            // differantiate between aligned/offset allocations and releases
            // but the CRT does, we'll just use an aligned malloc with the
            // platform default alignment.

#endif  // LUAXS_EASTL_LUAM_MALLOC
        }


        inline void* allocate(size_t n,
                              size_t alignment,
                              size_t offset,
                              int flags = 0) const
        {
            //CDBG(NULL,
            //     "eastl allocation of %u bytes (align: %u, offset: %u)",
            //     n, alignment, offset
            //);

#if LUAXS_EASTL_LUAM_MALLOC
            CWRN(NULL,
                 "unsupported aligned malloc (size: %u, align: %u, offset: %u)",
                 n, alignment, offset);
            return luaM_malloc(lxs_mt(), n);
#else   // LUAXS_EASTL_LUAM_MALLOC

            return _aligned_offset_malloc(n, alignment, offset);

#endif  // LUAXS_EASTL_LUAM_MALLOC
        }


        inline void deallocate(void* p, size_t n) const
        {
            //CDBG(NULL, "eastl deallocation of %u bytes (%p)", n, p);

#if LUAXS_EASTL_LUAM_MALLOC
            luaM_freemem(lxs_mt(), p, n);
#else   // LUAXS_EASTL_LUAM_MALLOC

            _aligned_free(p);

#endif  // LUAXS_EASTL_LUAM_MALLOC
        }



        //----------------------------------------------------------------------

        inline const char* get_name() const
        {
#if EASTL_NAME_ENABLED
            return name_;
#else
            return EASTL_ALLOCATOR_DEFAULT_NAME;
#endif
        }

        inline void set_name(const char* EASTL_NAME(name))
        {
#if EASTL_NAME_ENABLED
            name_ = name;
#endif
        }

    private:
#if EASTL_NAME_ENABLED
        const char* name_;
#endif
    }; // cs allocator


    // EASTL expects us to define these operators (allocator.h L103)
    static bool operator==(const allocator& a, const allocator& b)
    {
        return &a == &b || memcmp(&a, &b, sizeof(allocator)) == 0;
    }

    static bool operator!=(const allocator& a, const allocator& b)
    {
        return !(a == b);
    }


    // Defines the EASTL API glue, so we can set our allocator as the global default allocator
    static allocator  g_defaultAllocator;
    static allocator* gp_defaultAllocator = &g_defaultAllocator;

    static allocator* GetDefaultAllocator()
    {
        return gp_defaultAllocator;
    }

    static allocator* SetDefaultAllocator(allocator* pNewAlloc)
    {
        allocator* pOldAlloc = gp_defaultAllocator;
        gp_defaultAllocator = pNewAlloc;
        return pOldAlloc;
    }
}; // ns LuaAllocator



// EASTL also wants us to define this (see string.h line 197)
static int Vsnprintf8(char8_t* dest,
                      size_t dest_size,
                      const char8_t* fmt,
                      va_list args)
{
    return _vsnprintf(dest, dest_size, fmt, args);
}

#endif // leastl_hpp