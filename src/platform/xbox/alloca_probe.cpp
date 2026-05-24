/*
 * alloca_probe.cpp - Stack probe stub for Xbox XDK (RXDK)
 *
 * MSVC emits __alloca_probe_16 for large stack frame allocations.
 * The XDK libcmt.lib does not export this symbol.
 * We provide a minimal x86 implementation as a naked function.
 */

extern "C" __declspec(naked) void __cdecl _alloca_probe_16(void)
{
    __asm
    {
        push    ecx
        lea     ecx, [esp + 8]
        sub     ecx, eax
        and     ecx, 0xFFFFFFF0
        xchg    ecx, esp
        mov     eax, [ecx]
        push    eax
        ret
    }
}

/*==========================================================================
  Xbox Debug CRT stubs
  libcmtd.lib references HeapValidate (Win32 API not on Xbox)
  d3d8d.lib references XDebugWarning/XDebugError (xbdm, not always linked)
  Provide no-op stubs so debug builds link cleanly.
==========================================================================*/
extern "C" {

    /* HeapValidate - Win32 heap validation, not available on Xbox kernel */
    int __stdcall HeapValidate(void* hHeap, unsigned long dwFlags, const void* lpMem)
    {
        (void)hHeap; (void)dwFlags; (void)lpMem;
        return 1; /* always report heap as valid */
    }

    /* XDebugWarning / XDebugError - Xbox debug output, provided by xbdm.lib
       Stub here in case xbdm is not linked */
    void __cdecl XDebugWarning(const char* fmt, ...)
    {
        (void)fmt;
    }

    void __cdecl XDebugError(const char* fmt, ...)
    {
        (void)fmt;
    }

} /* extern "C" */