/*==========================================================================
  xbox_savegame.cpp - UDATA save path resolution for Zelda ALTTP X

  At startup, resolves the SRAM/INI UDATA container via XCreateSaveGame.
  State containers are resolved lazily on first save (write creates,
  read returns NULL if not found - no empty folders created).

  fopen redirector in xdk_crt_shim.h maps saves/X directly to UDATA.
  No TDATA staging. No intermediate copies. Straight to UDATA.

  Dashboard shows:
    "Zelda ALTTP - Game Save"   sram.dat + zelda3.ini
    "Zelda ALTTP - State 1"     state.dat  (created on first save to slot 1)
    "Zelda ALTTP - State 2"     state.dat  (created on first save to slot 2)
    ...
==========================================================================*/
#include "xbox_platform.h"
#include <stdio.h>
#include <string.h>

static char           s_sram_dir[MAX_PATH] = "";
static char           s_state_dir[10][MAX_PATH];
static const char* kSaveImageSrc = "D:\\Media\\saveimage.xbx";
static const char* kSaveImageMeta = "saveimage.xbx";
static const wchar_t* kSramName = L"Zelda ALTTP - Game Save";
static const wchar_t* kStateNameFmt = L"Zelda ALTTP - State %d";

/* -----------------------------------------------------------------------
   ResolveContainer - open or create a UDATA save game container.
   createIfMissing=true  -> OPEN_ALWAYS  (creates if not found)
   createIfMissing=false -> OPEN_EXISTING (returns false if not found)
----------------------------------------------------------------------- */
static bool ResolveContainer(const wchar_t* name, char* dirOut, size_t dirSize, bool createIfMissing)
{
    DWORD disp = createIfMissing ? OPEN_ALWAYS : OPEN_EXISTING;
    DWORD ret = XCreateSaveGame("U:\\", name, disp, 0, dirOut, (UINT)dirSize);

    if (ret != ERROR_SUCCESS) {
        /* Only log unexpected failures - missing container on read is normal */
        if (createIfMissing) {
            char dbg[128];
            _snprintf(dbg, sizeof(dbg), "zelda3: XCreateSaveGame('%S') failed: %lu\n", name, ret);
            OutputDebugStringA(dbg);
        }
        return false;
    }

    /* Ensure trailing backslash */
    size_t len = strlen(dirOut);
    if (len > 0 && dirOut[len - 1] != '\\') { dirOut[len] = '\\'; dirOut[len + 1] = '\0'; }

    /* Copy save thumbnail and write info.txt for easy identification via FTP */
    char path[MAX_PATH];
    _snprintf(path, sizeof(path), "%s%s", dirOut, kSaveImageMeta);
    CopyFile(kSaveImageSrc, path, FALSE);

    _snprintf(path, sizeof(path), "%sinfo.txt", dirOut);
    HANDLE h = CreateFile(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h != INVALID_HANDLE_VALUE) {
        char txt[128]; DWORD wr = 0;
        _snprintf(txt, sizeof(txt), "Save: %S\r\nDir:  %s\r\n", name, dirOut);
        WriteFile(h, txt, (DWORD)strlen(txt), &wr, NULL);
        CloseHandle(h);
    }
    return true;
}

/* -----------------------------------------------------------------------
   Xbox_ResolveSavePaths - call once at startup before any file I/O.
   Resolves the SRAM/INI container. State containers are lazy.
----------------------------------------------------------------------- */
extern "C" void Xbox_ResolveSavePaths(void)
{
    if (!ResolveContainer(kSramName, s_sram_dir, sizeof(s_sram_dir), true)) {
        OutputDebugStringA("zelda3: Failed to create SRAM UDATA container\n");
        return;
    }
    for (int i = 0; i < 10; i++) s_state_dir[i][0] = '\0';

    char dbg[128];
    _snprintf(dbg, sizeof(dbg), "zelda3: SRAM/INI UDATA: %s\n", s_sram_dir);
    OutputDebugStringA(dbg);
}

/* -----------------------------------------------------------------------
   Path accessors used by xdk_crt_shim.h redirector
----------------------------------------------------------------------- */
extern "C" const char* Xbox_GetSramDir(void) { return s_sram_dir; }

static const char* GetStateDirInternal(int slot0based, bool create)
{
    if (slot0based < 0 || slot0based > 9) return NULL;

    if (s_state_dir[slot0based][0] == '\0') {
        wchar_t name[64];
        _snwprintf(name, 64, kStateNameFmt, slot0based + 1);
        if (!ResolveContainer(name, s_state_dir[slot0based], sizeof(s_state_dir[slot0based]), create))
            return NULL;
    }
    return s_state_dir[slot0based];
}

/* Read - returns NULL if container doesn't exist (won't create it) */
extern "C" const char* Xbox_GetStateDir(int slot0based) {
    return GetStateDirInternal(slot0based, false);
}

/* Write - creates container on first save to this slot */
extern "C" const char* Xbox_GetStateDirCreate(int slot0based) {
    return GetStateDirInternal(slot0based, true);
}