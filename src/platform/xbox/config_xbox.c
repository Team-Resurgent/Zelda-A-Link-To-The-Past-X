/*==========================================================================
  config_xbox.c - Replaces src/config.c for Xbox build.
  config.c unconditionally includes SDL.h for keyboard mapping.
  On Xbox we don't need keyboard mapping - controller handled by xbox_input.cpp.
  This file provides g_config, INI save/load, and required function stubs.
  Pattern matches config_xbox.c in the SMW port.
==========================================================================*/
#include "xbox_platform.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "config.h"
#include "features.h"

/* Config INI now stored in UDATA alongside SRAM.
   "saves/zelda3.ini" is remapped by xdk_crt_shim.h to the UDATA sram container. */
static const char* get_config_path(void) {
    return "saves/zelda3.ini";
}

extern void Xbox_D3D_SetDispMode(int v);
extern int  Xbox_D3D_GetDispMode(void);

/* -----------------------------------------------------------------------
   g_config - owned here since config.c is excluded from Xbox build
----------------------------------------------------------------------- */
Config g_config;

/* -----------------------------------------------------------------------
   config.c function stubs
----------------------------------------------------------------------- */
void ParseConfigFile(const char *filename) { (void)filename; }
int  FindCmdForSdlKey(int code, int mod) { (void)code; (void)mod; return 0; }
int  FindCmdForGamepadButton(int button, unsigned int modifiers) { (void)button; (void)modifiers; return 0; }

/* -----------------------------------------------------------------------
   Config accessors - used by zelda_rtl_xbox.c
----------------------------------------------------------------------- */
int  Xbox_Cfg_GetEnhancedMode7(void)    { return g_config.enhanced_mode7 ? 1 : 0; }
int  Xbox_Cfg_GetNoSpriteLimits(void)   { return g_config.no_sprite_limits ? 1 : 0; }
int  Xbox_Cfg_GetExtendedAspect(void)   { return g_config.extended_aspect_ratio; }
int  Xbox_Cfg_GetExtendY(void)          { return g_config.extend_y ? 1 : 0; }
unsigned int Xbox_Cfg_GetFeatures(void) { return g_wanted_zelda_features; }
void Xbox_Cfg_SetEnhancedMode7(int v)   { g_config.enhanced_mode7 = v != 0; }
void Xbox_Cfg_SetNoSpriteLimits(int v)  { g_config.no_sprite_limits = v != 0; }
void Xbox_Cfg_SetExtendedAspect(int v)  { g_config.extended_aspect_ratio = (uint8)v; }
void Xbox_Cfg_SetExtendY(int v)         { g_config.extend_y = v != 0; }
void Xbox_Cfg_SetFeatures(unsigned int v) {
    if (!g_config.extended_aspect_ratio)
        v &= ~(kFeatures0_ExtendScreen64 | kFeatures0_WidescreenVisualFixes);
    g_config.features0 = v;
    g_wanted_zelda_features = v;
}

/* -----------------------------------------------------------------------
   INI Save
----------------------------------------------------------------------- */
void Xbox_Config_Save(void) {
    OutputDebugStringA("zelda3: Config saving\n");
    FILE *f = fopen(get_config_path(), "w");
    if (!f) { OutputDebugStringA("zelda3: Config FAILED to open for write\n"); return; }

    unsigned int feat = Xbox_Cfg_GetFeatures() & ~0x401u;

    fprintf(f, "[General]\r\n# Xbox zelda3 config - edit via in-game menu (L3)\r\n\r\n");
    fprintf(f, "[Graphics]\r\n");
    fprintf(f, "ExtendedAspectRatio = %s\r\n", Xbox_Cfg_GetExtendedAspect() ? "16:9" : "4:3");
    fprintf(f, "ExtendY = %d\r\n",             Xbox_Cfg_GetExtendY() ? 1 : 0);
    fprintf(f, "NewRenderer = 1\r\n");
    fprintf(f, "DisplayMode = %d\r\n",         Xbox_D3D_GetDispMode());
    fprintf(f, "EnhancedMode7 = %d\r\n",       Xbox_Cfg_GetEnhancedMode7() ? 1 : 0);
    fprintf(f, "NoSpriteLimits = %d\r\n",      Xbox_Cfg_GetNoSpriteLimits() ? 1 : 0);
    fprintf(f, "\r\n[Features]\r\n");
    fprintf(f, "DimFlashes = %d\r\n",              (feat & 0x10000) ? 1 : 0);
    fprintf(f, "DisableLowHealthBeep = %d\r\n",    (feat & 0x40)    ? 1 : 0);
    fprintf(f, "SwitchLR = %d\r\n",               (feat & 0x2)     ? 1 : 0);
    fprintf(f, "MirrorToDarkworld = %d\r\n",       (feat & 0x8)     ? 1 : 0);
    fprintf(f, "TurnWhileDashing = %d\r\n",        (feat & 0x4)     ? 1 : 0);
    fprintf(f, "BreakPotsWithSword = %d\r\n",      (feat & 0x20)    ? 1 : 0);
    fprintf(f, "CollectItemsWithSword = %d\r\n",   (feat & 0x10)    ? 1 : 0);
    fprintf(f, "SkipIntroOnKeypress = %d\r\n",     (feat & 0x80)    ? 1 : 0);
    fprintf(f, "ShowMaxItemsInYellow = %d\r\n",    (feat & 0x100)   ? 1 : 0);
    fprintf(f, "CarryMoreRupees = %d\r\n",         (feat & 0x800)   ? 1 : 0);
    fprintf(f, "MoreActiveBombs = %d\r\n",         (feat & 0x200)   ? 1 : 0);
    fprintf(f, "MiscBugFixes = %d\r\n",            (feat & 0x1000)  ? 1 : 0);
    fprintf(f, "GameChangingBugFixes = %d\r\n",    (feat & 0x4000)  ? 1 : 0);
    fprintf(f, "CancelBirdTravel = %d\r\n",        (feat & 0x2000)  ? 1 : 0);
    fprintf(f, "\r\n");
    fclose(f);
    OutputDebugStringA("zelda3: Config saved OK\n");
}

/* -----------------------------------------------------------------------
   INI Load
----------------------------------------------------------------------- */
void Xbox_Config_Load(void) {
    OutputDebugStringA("zelda3: Config loading\n");
    FILE *f = fopen(get_config_path(), "r");
    if (!f) { OutputDebugStringA("zelda3: Config not found - using defaults\n"); return; }
    OutputDebugStringA("zelda3: Config found - reading\n");

    char line[256];
    unsigned int feat = 0;
    int enhanced_mode7   = Xbox_Cfg_GetEnhancedMode7();
    int no_sprite_limits = Xbox_Cfg_GetNoSpriteLimits();
    int extended_aspect  = 0;
    int extend_y         = 0;

    while (fgets(line, sizeof(line), f)) {
        char *nl = line + strlen(line) - 1;
        while (nl >= line && (*nl == '\r' || *nl == '\n')) *nl-- = 0;
        if (line[0] == '#' || line[0] == '[' || line[0] == 0) continue;
        char *eq = strchr(line, '=');
        if (!eq) continue;
        *eq = 0;
        char *key = line, *val = eq + 1;
        while (*key == ' ') key++;
        char *ke = key + strlen(key) - 1;
        while (ke > key && *ke == ' ') *ke-- = 0;
        while (*val == ' ') val++;

        if      (!strcmp(key, "EnhancedMode7"))       enhanced_mode7   = atoi(val) != 0;
        else if (!strcmp(key, "NoSpriteLimits"))       no_sprite_limits = atoi(val) != 0;
        else if (!strcmp(key, "ExtendedAspectRatio")) {
            extended_aspect = 0;
            int h = extend_y ? 240 : 224;
            if      (strstr(val, "16:9"))  extended_aspect = (h * 16 / 9  - 256) / 2;
            else if (strstr(val, "16:10")) extended_aspect = (h * 16 / 10 - 256) / 2;
            else if (strstr(val, "18:9"))  extended_aspect = (h * 18 / 9  - 256) / 2;
            if (extended_aspect) feat |= 0x401; else feat &= ~0x401;
        }
        else if (!strcmp(key, "ExtendY"))              extend_y = atoi(val) != 0;
        else if (!strcmp(key, "DisplayMode"))          Xbox_D3D_SetDispMode(atoi(val));
        else if (!strcmp(key, "DimFlashes"))           { if (atoi(val)) feat |= 0x10000; }
        else if (!strcmp(key, "DisableLowHealthBeep")) { if (atoi(val)) feat |= 0x40;    }
        else if (!strcmp(key, "SwitchLR"))            { if (atoi(val)) feat |= 0x2;     }
        else if (!strcmp(key, "MirrorToDarkworld"))    { if (atoi(val)) feat |= 0x8;     }
        else if (!strcmp(key, "TurnWhileDashing"))     { if (atoi(val)) feat |= 0x4;     }
        else if (!strcmp(key, "BreakPotsWithSword"))   { if (atoi(val)) feat |= 0x20;    }
        else if (!strcmp(key, "CollectItemsWithSword")){ if (atoi(val)) feat |= 0x10;    }
        else if (!strcmp(key, "SkipIntroOnKeypress"))  { if (atoi(val)) feat |= 0x80;    }
        else if (!strcmp(key, "ShowMaxItemsInYellow")) { if (atoi(val)) feat |= 0x100;   }
        else if (!strcmp(key, "CarryMoreRupees"))      { if (atoi(val)) feat |= 0x800;   }
        else if (!strcmp(key, "MoreActiveBombs"))      { if (atoi(val)) feat |= 0x200;   }
        else if (!strcmp(key, "MiscBugFixes"))         { if (atoi(val)) feat |= 0x1000;  }
        else if (!strcmp(key, "GameChangingBugFixes")) { if (atoi(val)) feat |= 0x4000;  }
        else if (!strcmp(key, "CancelBirdTravel"))     { if (atoi(val)) feat |= 0x2000;  }
    }
    fclose(f);

    Xbox_Cfg_SetEnhancedMode7(enhanced_mode7);
    Xbox_Cfg_SetNoSpriteLimits(no_sprite_limits);
    Xbox_Cfg_SetExtendedAspect(extended_aspect);
    Xbox_Cfg_SetExtendY(extend_y);
    feat &= ~0x401u;
    if (extended_aspect) feat |= 0x401u;
    Xbox_Cfg_SetFeatures(feat);

    char buf[128];
    _snprintf(buf, sizeof(buf), "zelda3: Config loaded OK - mode7=%d wide=%d feat=0x%08X\n",
        enhanced_mode7, extended_aspect, feat);
    OutputDebugStringA(buf);
}
