/*==========================================================================
  zelda_rtl_xbox.c - Replaces src/main.c for the Xbox build and provides
  Xbox-specific zelda_rtl fixes. Compiled ONLY for Xbox.

  Equivalent of smw_rtl_xbox.c in the SMW port:
  - Owns global state that main.c normally owns (g_renderer_funcs, etc.)
  - Provides XboxZeldaInit/Frame entry points called by xbox_main.cpp
  - HDMA writable shadow buffers (XDK linker .rdata corruption fix)
  - Per-frame PPU widescreen safety hook
==========================================================================*/

#include <stdio.h>
extern int __cdecl DbgPrint(const char* format, ...);
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "snes/ppu.h"
#include "types.h"
#include "variables.h"
#include "zelda_rtl.h"
#include "zelda_cpu_infra.h"
#include "config.h"
#include "assets.h"
#include "load_gfx.h"
#include "util.h"
#include "audio.h"
#include "features.h"

/* XDK */
void __stdcall Sleep(unsigned long dwMilliseconds);
int  __cdecl  _snprintf(char*, size_t, const char*, ...);
extern void Xbox_EnsureSaveDir(void);
extern void Xbox_ResolveSavePaths(void);
extern void Xbox_Audio_StartThread(void);
extern int  Xbox_D3D_GetDispMode(void);
extern bool Xbox_Menu_IsGodHealth(void);
extern const uint8 kMaxBombsForLevel[];
extern const uint8 kMaxArrowsForLevel[];
extern bool Xbox_Menu_IsGodMagic(void);
extern bool Xbox_Menu_IsGodBombs(void);
extern bool Xbox_Menu_IsGodArrows(void);
extern bool Xbox_Menu_IsGodRupees(void);
extern void Xbox_Config_Load(void);

/* -----------------------------------------------------------------------
   Global state - owned here since main.c is excluded on Xbox
----------------------------------------------------------------------- */
struct RendererFuncs  g_renderer_funcs;
int                   g_ppu_render_flags = 0;
int                   g_snes_width = 256;
int                   g_snes_height = 224;

extern const struct RendererFuncs kXboxRendererFuncs;

/* -----------------------------------------------------------------------
   HDMA writable shadow buffers
   kMapModeHdma0/1/kAttractIndirectHdmaTab are static in zelda_rtl.c
   so we define our own copies with the same values.
   AT_WORD(x) = (x)&0xff, (x)>>8
----------------------------------------------------------------------- */
uint8_t  g_xbox_MapModeHdma0_buf[7] = { 0xf0, 0x27, 0xdd, 0xf0, 0x07, 0xde, 0 };
uint8_t  g_xbox_MapModeHdma1_buf[7] = { 0xf0, 0xe7, 0xde, 0xf0, 0xc7, 0xdf, 0 };
uint8_t  g_xbox_AttractHdma_buf[7] = { 0xf0, 0x00, 0x1b, 0xf0, 0xe0, 0x1b, 0 };

extern const uint16_t kMapMode_Zooms1[240];
extern const uint16_t kMapMode_Zooms2[240];
uint16_t g_xbox_Zooms1_buf[240];
uint16_t g_xbox_Zooms2_buf[240];

static bool s_hdma_inited = false;

static void InitHdmaBufs(void) {
    if (s_hdma_inited) return;
    memcpy(g_xbox_Zooms1_buf, kMapMode_Zooms1, sizeof(g_xbox_Zooms1_buf));
    memcpy(g_xbox_Zooms2_buf, kMapMode_Zooms2, sizeof(g_xbox_Zooms2_buf));
    s_hdma_inited = true;
}

/* -----------------------------------------------------------------------
   main.c function stubs (main.c excluded, config_xbox.c has config stubs)
----------------------------------------------------------------------- */
void NORETURN Die(const char* error) {
    char buf[256];
    _snprintf(buf, sizeof(buf), "zelda3 FATAL: %s\n", error);
    DbgPrint("%s", buf);
    for (;;) Sleep(2000);
}

void Xbox_SetPpuRenderFlags(int flags) { g_ppu_render_flags = flags; }
int  Xbox_GetPpuRenderFlags(void) { return g_ppu_render_flags; }

void Xbox_Reapply_Config(void) {
    int dm = Xbox_D3D_GetDispMode();
    if (dm == 2) {
        g_config.extended_aspect_ratio = 71;
        g_config.features0 |= kFeatures0_ExtendScreen64 | kFeatures0_WidescreenVisualFixes;
    }
    else {
        g_config.extended_aspect_ratio = 0;
        g_config.features0 &= ~(kFeatures0_ExtendScreen64 | kFeatures0_WidescreenVisualFixes);
    }
    g_ppu_render_flags &= ~kPpuRenderFlags_4x4Mode7;
    uint8 elr = g_config.extended_aspect_ratio;
    if (elr > kPpuExtraLeftRight) elr = kPpuExtraLeftRight;
    g_zenv.ppu->extraLeftRight = elr;
    g_snes_width = g_config.extended_aspect_ratio * 2 + 256;
    g_snes_height = g_config.extend_y ? 240 : 224;
    g_wanted_zelda_features = g_config.features0;
}

/* -----------------------------------------------------------------------
   Asset loading (Xbox paths)
----------------------------------------------------------------------- */
const uint8* g_asset_ptrs[kNumberOfAssets];
uint32        g_asset_sizes[kNumberOfAssets];

static void LoadAssets(void) {
    size_t length = 0;
    uint8* data = ReadWholeFile("D:\\Media\\zelda3_assets.dat", &length);
    if (!data) {
        size_t bps_len, src_len;
        uint8* bps = ReadWholeFile("D:\\Media\\zelda3_assets.bps", &bps_len);
        if (!bps) Die("Failed to read D:\\Media\\zelda3_assets.dat");
        uint8* src = ReadWholeFile("D:\\Media\\zelda3.sfc", &src_len);
        if (!src) Die("Missing file: D:\\Media\\zelda3.sfc");
        data = ApplyBps(src, src_len, bps, bps_len, &length);
        if (!data) Die("Unable to apply zelda3_assets.bps.");
    }
    static const char kAssetsSig[] = { kAssets_Sig };
    if (length < 16 + 32 + 32 + 8 + kNumberOfAssets * 4 ||
        memcmp(data, kAssetsSig, 48) != 0 ||
        *(uint32*)(data + 80) != kNumberOfAssets)
        Die("Invalid assets file");
    uint32 offset = 88 + kNumberOfAssets * 4 + *(uint32*)(data + 84);
    for (size_t i = 0; i < kNumberOfAssets; i++) {
        uint32 size = *(uint32*)(data + 88 + i * 4);
        offset = (offset + 3) & ~3;
        if ((uint64)offset + size > length) Die("Assets file corruption");
        g_asset_sizes[i] = size;
        g_asset_ptrs[i] = data + offset;
        offset += size;
    }
}

MemBlk FindInAssetArray(int asset, int idx) {
    return FindIndexInMemblk((MemBlk) { g_asset_ptrs[asset], g_asset_sizes[asset] }, idx);
}

/* -----------------------------------------------------------------------
   Draw frame
----------------------------------------------------------------------- */
static void DrawFrame(void) {
    int scale = PpuGetCurrentRenderScale(g_zenv.ppu, g_ppu_render_flags);
    uint8* pixels = 0;
    int pitch = 0;
    g_renderer_funcs.BeginDraw(g_snes_width * scale, g_snes_height * scale, &pixels, &pitch);
    ZeldaDrawPpuFrame(pixels, pitch, g_ppu_render_flags);
    g_renderer_funcs.EndDraw();
}

/* -----------------------------------------------------------------------
   Per-frame hook - HDMA init + PPU widescreen safety reset
----------------------------------------------------------------------- */
static void FrameHook(void) {
    InitHdmaBufs();
    if (g_zenv.ppu && g_zenv.ppu->extraLeftRight == 0) {
        g_zenv.ppu->extraLeftCur = 0;
        g_zenv.ppu->extraRightCur = 0;
        g_zenv.ppu->extraBottomCur = 0;
    }
}

/* -----------------------------------------------------------------------
   zelda_ppu_write / zelda_ppu_write_word - Release only.
   In Debug, zelda_rtl.c emits the symbol (inline is not inlined).
   In Release, MSVC inlines them away so we provide the definition here.
----------------------------------------------------------------------- */
#ifdef NDEBUG
void zelda_ppu_write(uint32_t adr, uint8_t val) {
    ppu_write(g_zenv.ppu, (uint8)adr, val);
}

void zelda_ppu_write_word(uint32_t adr, uint16_t val) {
    zelda_ppu_write(adr, (uint8_t)val);
    zelda_ppu_write(adr + 1, (uint8_t)(val >> 8));
}
#endif /* NDEBUG */

/* -----------------------------------------------------------------------
   XboxZeldaInit - called once from xbox_main.cpp
----------------------------------------------------------------------- */
void XboxZeldaInit(void) {
    InitHdmaBufs();

    g_config.new_renderer = true;
    g_config.no_sprite_limits = true;
    g_config.enhanced_mode7 = true;
    g_config.enable_audio = true;
    g_config.extended_aspect_ratio = 0;
    g_config.extend_y = false;

    /* Mount HDD and resolve UDATA paths FIRST - before any file I/O */
    Xbox_EnsureSaveDir();
    Xbox_ResolveSavePaths();

    LoadAssets();
    Xbox_Config_Load();

    {
        int dm = Xbox_D3D_GetDispMode();
        if (dm == 2) {
            g_config.extended_aspect_ratio = 71;
            g_config.features0 |= kFeatures0_ExtendScreen64 | kFeatures0_WidescreenVisualFixes;
        }
        else {
            g_config.extended_aspect_ratio = 0;
            g_config.features0 &= ~(kFeatures0_ExtendScreen64 | kFeatures0_WidescreenVisualFixes);
        }
    }

    g_ppu_render_flags = kPpuRenderFlags_NewRenderer
        | (g_config.no_sprite_limits ? kPpuRenderFlags_NoSpriteLimits : 0)
        | (g_config.extend_y ? kPpuRenderFlags_Height240 : 0);

    ZeldaInitialize();

    {
        uint8 elr = g_config.extended_aspect_ratio;
        if (elr > kPpuExtraLeftRight) elr = kPpuExtraLeftRight;
        g_zenv.ppu->extraLeftRight = elr;
    }
    g_snes_width = g_config.extended_aspect_ratio * 2 + 256;
    g_snes_height = g_config.extend_y ? 240 : 224;
    g_wanted_zelda_features = g_config.features0;

    ZeldaEnableMsu(g_config.enable_msu);
    ZeldaSetLanguage(g_config.language);

    /* SRAM - fopen redirector maps saves/sram.dat -> UDATA directly */
    ZeldaReadSram();

    g_renderer_funcs = kXboxRendererFuncs;
    Xbox_Audio_StartThread();
}

void XboxZeldaFrame(uint16_t buttons) {
    FrameHook();
    ZeldaRunFrame((int)buttons);

    /* God mode - frozen value pattern from original port.
       Track the highest seen value, restore it every frame. */
    {
        static uint8  s_frozenHealth = 0;
        static uint8  s_frozenMagic = 0;
        static uint8  s_frozenBombs = 0;
        static uint8  s_frozenArrows = 0;
        static uint16 s_frozenRupees = 0;

        if (Xbox_Menu_IsGodHealth()) {
            if (link_health_current > s_frozenHealth) s_frozenHealth = link_health_current;
            if (s_frozenHealth == 0) s_frozenHealth = link_health_capacity;
            link_health_current = s_frozenHealth;
            link_give_damage = 0;
        }
        else { s_frozenHealth = 0; }

        if (Xbox_Menu_IsGodMagic()) {
            if (link_magic_power > s_frozenMagic) s_frozenMagic = link_magic_power;
            if (s_frozenMagic == 0) s_frozenMagic = 0x80;
            link_magic_power = s_frozenMagic;
        }
        else { s_frozenMagic = 0; }

        if (Xbox_Menu_IsGodBombs()) {
            if (link_item_bombs > s_frozenBombs) s_frozenBombs = link_item_bombs;
            if (s_frozenBombs == 0) {
                s_frozenBombs = kMaxBombsForLevel[link_bomb_upgrades];
                link_bomb_filler = s_frozenBombs;
            }
            link_item_bombs = s_frozenBombs;
        }
        else { s_frozenBombs = 0; }

        if (Xbox_Menu_IsGodArrows()) {
            if (link_num_arrows > s_frozenArrows) s_frozenArrows = link_num_arrows;
            if (s_frozenArrows == 0) {
                s_frozenArrows = kMaxArrowsForLevel[link_arrow_upgrades];
                link_arrow_filler = s_frozenArrows;
            }
            link_num_arrows = s_frozenArrows;
        }
        else { s_frozenArrows = 0; }

        if (Xbox_Menu_IsGodRupees()) {
            if (link_rupees_goal > s_frozenRupees) s_frozenRupees = link_rupees_goal;
            if (s_frozenRupees == 0) s_frozenRupees = 100;
            link_rupees_goal = s_frozenRupees;
            link_rupees_actual = s_frozenRupees;
        }
        else { s_frozenRupees = 0; }
    }

    DrawFrame();

}

void XboxZeldaFrameLogicOnly(uint16_t buttons) {
    FrameHook();
    ZeldaRunFrame((int)buttons);
}

int XboxGetSnesWidth(void) { return g_snes_width; }
int XboxGetSnesHeight(void) { return g_snes_height; }