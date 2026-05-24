/*
 * xdk_crt_shim.h - CRT shims for Xbox XDK (RXDK)
 * Included automatically via stdbool.h which every TU includes.
 */
#ifndef XDK_CRT_SHIM_H
#define XDK_CRT_SHIM_H

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stddef.h>
#include <malloc.h>
#include <stdlib.h>

 /* assert - include assert.h first then override to no-op.
    zelda_rtl.h includes assert.h after stdbool.h, which would re-enable
    the real assert. We force-include assert.h here then unconditionally
    redefine it as a no-op so the later include is a no-op (guard fires). */
#include <assert.h>
#ifdef assert
#  undef assert
#endif
#define assert(x) ((void)(x))

    /* __attribute__ - GCC extension, empty on MSVC */
#ifndef __attribute__
#  define __attribute__(x)
#endif

/* alloca */
#ifndef alloca
#  define alloca _alloca
#endif

/* snprintf / vsnprintf */
#ifndef snprintf
static __inline int snprintf(char* buf, size_t n, const char* fmt, ...) {
    int r; va_list ap; va_start(ap, fmt);
    r = _vsnprintf(buf, n, fmt, ap); va_end(ap);
    if (r < 0 || (size_t)r >= n) buf[n - 1] = 0;
    return r;
}
#endif
#ifndef vsnprintf
static __inline int vsnprintf(char* buf, size_t n, const char* fmt, va_list ap) {
    int r = _vsnprintf(buf, n, fmt, ap);
    if (r < 0 || (size_t)r >= n) buf[n - 1] = 0;
    return r;
}
#endif

/* strdup */
#ifndef strdup
static __inline char* strdup(const char* s) {
    size_t n = strlen(s) + 1; char* p = (char*)malloc(n);
    if (p) memcpy(p, s, n); return p;
}
#endif

/* _countof */
#ifndef _countof
#  define _countof(a) (sizeof(a)/sizeof(*(a)))
#endif


/* CLAMP16 macro needed by dsp.c */
#ifndef CLAMP16
#  define CLAMP16(io) { if ((int16_t)(io) != (io)) (io) = ((io) >> 31) ^ 0x7FFF; }
#endif

/* FORCEINLINE */
#ifndef FORCEINLINE
#  define FORCEINLINE __forceinline
#endif

/* uint64 - needed by ppu.c */
#ifndef uint64
typedef unsigned __int64 uint64;
#endif


/* __alloca_probe_16 - stack probe emitted by MSVC for large stack frames.
   XDK libcmt.lib does not export this. Map it to _chkstk which XDK provides. */
#pragma comment(linker, "/alternatename:___alloca_probe_16=__chkstk")


   /* -----------------------------------------------------------------------
      File path redirector for Xbox
      Maps "saves/X" directly to resolved UDATA container paths.

      saves/sram.dat      -> {sram_dir}sram.dat
      saves/sram.bak      -> {sram_dir}sram.bak
      saves/zelda3.ini    -> {sram_dir}zelda3.ini
      saves/save1.sav     -> {state_dir[0]}state.dat  (Slot 1)
      saves/save2.sav     -> {state_dir[1]}state.dat  (Slot 2)
      ...
      saves/save10.sav    -> {state_dir[9]}state.dat  (Slot 10)
   ----------------------------------------------------------------------- */
#if defined(XBOX_PORT) && !defined(__cplusplus)
#ifndef _XBOX_PATH_REDIR_H
#define _XBOX_PATH_REDIR_H

extern const char* Xbox_GetSramDir(void);
extern const char* Xbox_GetStateDir(int slot0based);
extern const char* Xbox_GetStateDirCreate(int slot0based);

static __inline void _xbox_map_path(char* out, size_t n, const char* path, int writing) {
    if (!path) { out[0] = 0; return; }
    const char* rel = NULL;
    if (strncmp(path, "saves/", 6) == 0) rel = path + 6;
    else if (strncmp(path, "saves\\", 6) == 0) rel = path + 6;
    else { _snprintf(out, n, "%s", path); return; }

    /* save1.sav=Slot1, save2.sav=Slot2 ... save10.sav=Slot10 */
    if (strncmp(rel, "save", 4) == 0 && rel[4] >= '0' && rel[4] <= '9') {
        int slot = 0;
        const char* p = rel + 4;
        while (*p >= '0' && *p <= '9') slot = slot * 10 + (*p++ - '0');
        if (strncmp(p, ".sav", 4) == 0) {
            /* slot is 1-based (save1.sav=slot1), dir array is 0-based */
            const char* dir = writing ? Xbox_GetStateDirCreate(slot - 1)
                : Xbox_GetStateDir(slot - 1);
            if (dir) _snprintf(out, n, "%sstate.dat", dir);
            else     out[0] = 0; /* state doesn't exist - fopen will return NULL */
            return;
        }
    }
    /* Everything else (sram.dat, sram.bak, zelda3.ini) -> sram UDATA dir */
    _snprintf(out, n, "%s%s", Xbox_GetSramDir(), rel);
}

static __inline FILE* _xbox_fopen(const char* path, const char* mode) {
    int writing = mode && (mode[0] == 'w' || mode[0] == 'a');
    char buf[512]; _xbox_map_path(buf, sizeof(buf), path, writing);
    if (!buf[0]) return NULL;
    return fopen(buf, mode);
}
static __inline int _xbox_rename(const char* o, const char* nw) {
    char bo[512], bn[512];
    _xbox_map_path(bo, sizeof(bo), o, 1);
    _xbox_map_path(bn, sizeof(bn), nw, 1);
    if (!bo[0] || !bn[0]) return -1;
    return rename(bo, bn);
}

#define fopen  _xbox_fopen
#define rename _xbox_rename

#endif /* _XBOX_PATH_REDIR_H */
#endif /* XBOX_PORT && !__cplusplus */

#endif /* XDK_CRT_SHIM_H */