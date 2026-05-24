/*
 * intrin.h shim for Xbox XDK (RXDK)
 *
 * The XDK's include path replaces the normal MSVC paths, so the standard
 * MSVC <intrin.h> is not found. This shim provides the minimal subset
 * needed by Opus (ecintrin.h uses _BitScanReverse for CLZ).
 *
 * The Xbox CPU is a Pentium III (x86), so BSR is available.
 */
#ifndef _INTRIN_H_SHIM
#define _INTRIN_H_SHIM

/* MSVC built-in - should still work even without the header on MSVC */
unsigned char _BitScanReverse(unsigned long *_Index, unsigned long _Mask);
unsigned char _BitScanForward(unsigned long *_Index, unsigned long _Mask);

#pragma intrinsic(_BitScanReverse)
#pragma intrinsic(_BitScanForward)

#endif /* _INTRIN_H_SHIM */
