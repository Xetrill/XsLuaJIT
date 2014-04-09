#pragma once
#ifndef xscpu_h
#define xscpu_h

#define WINVER 0x501      // XP, 2003
#define WIN32_LEAN_AND_MEAN
#define NOGDICAPMASKS     // CC_*, LC_*, PC_*, CP_*, TC_*, RC_
#define NOVIRTUALKEYCODES // VK_*
#define NOWINMESSAGES     // WM_*, EM_*, LB_*, CB_*
#define NOWINSTYLES       // WS_*, CS_*, ES_*, LBS_*, SBS_*, CBS_*
#define NOSYSMETRICS      // SM_*
#define NOMENUS           // MF_*
#define NOICONS           // IDI_*
#define NOKEYSTATES       // MK_*
#define NOSYSCOMMANDS     // SC_*
#define NORASTEROPS       // Binary and Tertiary raster ops
#define NOSHOWWINDOW      // SW_*
#define OEMRESOURCE       // OEM Resource values
#define NOATOM            // Atom Manager routines
#define NOCLIPBOARD       // Clipboard routines
#define NOCOLOR           // Screen colors
#define NOCTLMGR          // Control and Dialog routines
#define NODRAWTEXT        // DrawText() and DT_*
#define NOGDI             // All GDI defines and routines
#define NOKERNEL          // All KERNEL defines and routines
#define NOUSER            // All USER defines and routines
#define NONLS             // All NLS defines and routines
#define NOMB              // MB_* and MessageBox()
#define NOMEMMGR          // GMEM_*, LMEM_*, GHND, LHND, associated routines
#define NOMETAFILE        // typedef METAFILEPICT
#define NOMINMAX          // Macros min(a,b) and max(a,b)
#define NOMSG             // typedef MSG and associated routines
#define NOOPENFILE        // OpenFile(), OemToAnsi, AnsiToOem, and OF_*
#define NOSCROLL          // SB_* and scrolling routines
#define NOSERVICE         // All Service Controller routines, SERVICE_ equates, etc.
#define NOSOUND           // Sound driver routines
#define NOTEXTMETRIC      // typedef TEXTMETRIC and associated routines
#define NOWH              // SetWindowsHook and WH_*
#define NOWINOFFSETS      // GWL_*, GCL_*, associated routines
#define NOCOMM            // COMM driver routines
#define NOKANJI           // Kanji support stuff.
#define NOHELP            // Help engine interface.
#define NOPROFILER        // Profiler interface.
#define NODEFERWINDOWPOS  // DeferWindowPos routines
#define NOMCX             // Modem Configuration Extensions
#include <windows.h>

#include <stdint.h>
#include <intrin.h> // __cpuid
#include <string.h> // memcmp

#include "lxs_def.h"


//==============================================================================




//==============================================================================

#define _XS_CPU_VENDOR_LEN 12

// @see https://en.wikipedia.org/wiki/CPUID
static bool _xs_cpu_getvendor(char* vendor)
{
    HKEY  key = NULL;
    DWORD len = 0;
    //char  buf[_XS_CPU_VENDOR_LEN];

    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE,
                      "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0",
                      0,
                      KEY_READ,
                      &key) != ERROR_SUCCESS)
        goto L_ERR;
    if (RegQueryValueExA(key,
                         "VendorIdentifier",
                         NULL,
                         NULL,
                         (LPBYTE)/*buf*/vendor, &len) != ERROR_SUCCESS)
        goto L_ERR;
    RegCloseKey(key);

    if (len != _XS_CPU_VENDOR_LEN)
        return false;

    //memcpy(vendor, buf, _XS_CPU_VENDOR_LEN);
    return true;

L_ERR:
    if (key != NULL)
        RegCloseKey(key);
    return false;
}


//==============================================================================

#define _XS_WIN_CMP_VENDOR(VENDOR)                                             \
    char vendor[_XS_CPU_VENDOR_LEN];                                           \
    return (                                                                   \
        _xs_cpu_getvendor(vendor) &&                                           \
        memcmp(VENDOR, vendor, 12) == 0                                        \
    ) ? true : false;

static bool xs_cpu_isintel(void) { _XS_WIN_CMP_VENDOR("GenuineIntel"); }
static bool xs_cpu_isamd(void)   { _XS_WIN_CMP_VENDOR("AuthenticAMD"); }


//==============================================================================

// TODO use a bitset instead
static bool _cpu_flags[] = {
    false, // SSE3 Instructions
    false, // SSE3 Supplemental
    false, // SSE4 Extensions
    false, // SSE4.1 Extensions
    false, // SSE4.2 Extensions
    false, // SSE4A Extensions
    false, // Misaligned SSE
    false, // MMX Extensions
    false, // 3DNow! Extensions
    false, // 3DNow! Instructions
    false
};

static void xs_cpu_getinfo(void)
{
    uint32_t ids, exIds;
    int32_t info[4] = { -1 };

    if (_cpu_flags[XS_CPU__MAX])
        return;

    __cpuid(info, 0);
    ids = info[0];

    __cpuid(info, 0x80000000);
    exIds = info[0];

    if (ids >= 1)
    {
	    __cpuid(info, 1);
	    _cpu_flags[XS_CPU_SSE3I] = (info[2] & 0x1);
	    _cpu_flags[XS_CPU_SSE3S] = (info[2] & 0x200);
	    _cpu_flags[XS_CPU_SSE41] = (info[2] & 0x80000);
	    _cpu_flags[XS_CPU_SSE42] = (info[2] & 0x100000);
    }

    if (exIds >= 0x80000001)
    {
        __cpuid(info, 0x80000001);
        _cpu_flags[XS_CPU_SSE4A]          = (info[2] & 0x40);
        _cpu_flags[XS_CPU_MISALIGNED_SSE] = (info[2] & 0x80);
        _cpu_flags[XS_CPU_MMX]            = (info[3] & 0x40000);
        _cpu_flags[XS_CPU_3DNOW_EXT]      = (info[3] & 0x40000000);
        _cpu_flags[XS_CPU_3DNOW]          = (info[3] & 0x80000000);
    }

    _cpu_flags[XS_CPU__MAX] = true;
}

static bool xs_cpu_supports(enum XS_CPU flag)
{
    assert(flag >= 0 && flag < XS_CPU__MAX);
    return _cpu_flags[flag];
}


//==============================================================================

#undef _XS_CPU_VENDOR_LEN
#undef _XS_WIN_CMP_VENDOR

#endif // xscpu_h
