#pragma once
/*==========================================================================
  xbox_platform.h  -  zelda3 OG Xbox port (RXDK / native XDK, no SDL)
  Graphics  = Direct3D 8
  Audio     = DirectSound8 (XAudio)
  Input     = XInput (original Xbox controller)

  Required build environment:
    RXDK by Team Resurgent  https://github.com/Team-Resurgent/RXDK
    Visual Studio 2019 or newer
    XDK 5849.17 (or compatible)

  This header is included ONLY by the xbox_*.cpp files (C++ compilation).
  main.c uses xbox_main_fwd.h instead (no XDK COM headers).
==========================================================================*/

/* XTL is the master Xbox include - pulls in d3d8.h, d3dx8.h, dsound.h.
   This header is ONLY included by xbox_*.cpp files (compiled as C++).
   main.c uses xbox_main_fwd.h instead to avoid C++ COM header issues. */
#include <xtl.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

   /* -----------------------------------------------------------------------
      Display constants
      SNES output is up to 512x240 (or 512x224 without extend_y).
      We allocate the texture at the maximum and blit a centred quad.
      Xbox display: 1280x720 (720p) requested - falls back to 480p if not supported.
   ----------------------------------------------------------------------- */
#define ZELDA_TEX_WIDTH     512
#define ZELDA_TEX_HEIGHT    480
#define XBOX_DISPLAY_WIDTH  640
#define XBOX_DISPLAY_HEIGHT 480

   /* -----------------------------------------------------------------------
      Audio constants  (must match zelda3's DSP output)
   ----------------------------------------------------------------------- */
#define AUDIO_SAMPLE_RATE   44100
#define AUDIO_CHANNELS      2
#define AUDIO_BITS          16
   /* frames per push - (534 * 44100) / 32000 = 735 */
#define AUDIO_FRAMES_PER_BLOCK  735
#define AUDIO_BYTES_PER_BLOCK   (AUDIO_FRAMES_PER_BLOCK * AUDIO_CHANNELS * (AUDIO_BITS/8))
/* Circular buffer = 4 blocks */
#define AUDIO_BUF_BLOCKS    4
#define AUDIO_BUF_BYTES     (AUDIO_BUF_BLOCKS * AUDIO_BYTES_PER_BLOCK)

/* -----------------------------------------------------------------------
   Controller trigger digital threshold  (0-255)
----------------------------------------------------------------------- */
#define XBOX_TRIGGER_THRESH  64

/* -----------------------------------------------------------------------
   Global D3D8 objects  (defined in xbox_d3d.cpp)
----------------------------------------------------------------------- */
extern LPDIRECT3D8              g_pD3D;
extern LPDIRECT3DDEVICE8        g_pDevice;
extern LPDIRECT3DTEXTURE8       g_pFrameTex;
extern LPDIRECT3DVERTEXBUFFER8  g_pQuadVB;

/* -----------------------------------------------------------------------
   Global audio objects  (defined in xbox_audio.cpp)
----------------------------------------------------------------------- */
extern LPDIRECTSOUND8      g_pDSound;
extern LPDIRECTSOUNDBUFFER g_pStreamBuf;
extern DWORD               g_dsWritePos;
extern DWORD               g_dsBufSize;

/* -----------------------------------------------------------------------
   Function prototypes - all with C linkage so main.c (compiled as C)
   can call them from the xbox_*.cpp implementations.
----------------------------------------------------------------------- */
#ifdef __cplusplus
extern "C" {
#endif

    /* D3D */
    HRESULT Xbox_D3D_Init(void);
    void    Xbox_D3D_Shutdown(void);
    void    Xbox_D3D_BeginDraw(int width, int height, uint8_t** pixels, int* pitch);
    void    Xbox_D3D_EndDraw(void);
    /* Audio */
    HRESULT Xbox_Audio_Init(void);
    void    Xbox_Audio_Shutdown(void);
    /* Input */
    void     Xbox_Input_Init(void);
    void     Xbox_Input_Poll(void);
    uint16_t Xbox_Input_GetJoypad(void);
    void     Xbox_Input_SuppressUntilRelease(void);
    void     Xbox_Input_Shutdown(void);

    /* Filesystem */
    void   Xbox_EnsureSaveDir(void);

    /* RendererFuncs table */
    extern const struct RendererFuncs kXboxRendererFuncs;

    /* Audio */
    void Xbox_Audio_PushSilence(void);
    void Xbox_Audio_Resume(void);

    /* Toast */
    bool        Xbox_Menu_HasToast(void);
    void        Xbox_Menu_DrawToast(void);
    int         Xbox_Menu_GetToastTimer(void);
    const char* Xbox_Menu_GetToastMsg(void);

#ifdef __cplusplus
} /* extern "C" */
#endif

/* -----------------------------------------------------------------------
   Exported from main.c for use by xbox_main.cpp
----------------------------------------------------------------------- */
#ifdef __cplusplus
extern "C" {
#endif
    void     XboxZeldaInit(void);
    void     XboxZeldaFrame(uint16_t buttons);
    int      XboxGetSnesWidth(void);
    int      XboxGetSnesHeight(void);
#ifdef __cplusplus
}
#endif