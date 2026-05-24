/*==========================================================================
  xbox_menu.cpp - In-game overlay menu for zelda3 Xbox port
  Open/Close: L3 (left thumbstick click)
  Navigate: DPad up/down, A=select, B=back
==========================================================================*/
#include "xbox_platform.h"
extern "C" {
#include "zelda_rtl.h"
#include "variables.h"
#include "features.h"
#include "config.h"
    /* PPU render flag constants */
#define kPpuRenderFlags_NewRenderer    1
#define kPpuRenderFlags_4x4Mode7       2
#define kPpuRenderFlags_Height240      4
#define kPpuRenderFlags_NoSpriteLimits 8
#define kPpuExtraLeftRight             96
/* Use accessors to avoid C/C++ linkage issues with g_config */
    extern "C" int   Xbox_Cfg_GetEnhancedMode7(void);
    extern "C" int   Xbox_Cfg_GetNoSpriteLimits(void);
    extern "C" int   Xbox_Cfg_GetExtendedAspect(void);
    extern "C" int   Xbox_Cfg_GetExtendY(void);
    extern "C" unsigned int Xbox_Cfg_GetFeatures(void);
    extern "C" void  Xbox_Cfg_SetEnhancedMode7(int v);
    extern "C" void  Xbox_Cfg_SetNoSpriteLimits(int v);
    extern "C" void  Xbox_Cfg_SetExtendedAspect(int v);
    extern "C" void  Xbox_Cfg_SetExtendY(int v);
    extern "C" void  Xbox_Cfg_SetFeatures(unsigned int v);
    void Xbox_SetPpuRenderFlags(int flags);
    int  Xbox_GetPpuRenderFlags(void);
    extern "C" void Xbox_Config_Save(void);
    extern "C" void Xbox_Config_Load(void);

    extern "C" void Xbox_Reapply_Config(void);
    extern "C" int Xbox_D3D_GetDispW(void);
    extern "C" int Xbox_D3D_GetDispH(void);
    extern "C" void Xbox_D3D_SetDispMode(int v);
    extern "C" int  Xbox_D3D_GetDispMode(void);
    extern "C" const char* Xbox_GetStateDir(int slot0based);
}

static bool s_exit_requested = false;
extern "C" bool Xbox_Menu_ExitRequested(void) { return s_exit_requested; }
static void Xbox_Menu_RequestExit(void) { s_exit_requested = true; }


/*==========================================================================
  8x8 bitmap font
==========================================================================*/
static const BYTE kFont8x8[][8] = {
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},{0x18,0x18,0x18,0x18,0x18,0x00,0x18,0x00},
  {0x66,0x66,0x00,0x00,0x00,0x00,0x00,0x00},{0x66,0x66,0xFF,0x66,0xFF,0x66,0x66,0x00},
  {0x18,0x3E,0x60,0x3C,0x06,0x7C,0x18,0x00},{0x62,0x66,0x0C,0x18,0x30,0x66,0x46,0x00},
  {0x3C,0x66,0x3C,0x38,0x67,0x66,0x3F,0x00},{0x18,0x18,0x00,0x00,0x00,0x00,0x00,0x00},
  {0x0C,0x18,0x30,0x30,0x30,0x18,0x0C,0x00},{0x30,0x18,0x0C,0x0C,0x0C,0x18,0x30,0x00},
  {0x00,0x66,0x3C,0xFF,0x3C,0x66,0x00,0x00},{0x00,0x18,0x18,0x7E,0x18,0x18,0x00,0x00},
  {0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x30},{0x00,0x00,0x00,0x7E,0x00,0x00,0x00,0x00},
  {0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x00},{0x03,0x06,0x0C,0x18,0x30,0x60,0x40,0x00},
  {0x3C,0x66,0x6E,0x76,0x66,0x66,0x3C,0x00},{0x18,0x38,0x18,0x18,0x18,0x18,0x7E,0x00},
  {0x3C,0x66,0x06,0x0C,0x18,0x30,0x7E,0x00},{0x3C,0x66,0x06,0x1C,0x06,0x66,0x3C,0x00},
  {0x0C,0x1C,0x3C,0x6C,0x7E,0x0C,0x0C,0x00},{0x7E,0x60,0x7C,0x06,0x06,0x66,0x3C,0x00},
  {0x1C,0x30,0x60,0x7C,0x66,0x66,0x3C,0x00},{0x7E,0x06,0x0C,0x18,0x30,0x30,0x30,0x00},
  {0x3C,0x66,0x66,0x3C,0x66,0x66,0x3C,0x00},{0x3C,0x66,0x66,0x3E,0x06,0x0C,0x38,0x00},
  {0x00,0x18,0x18,0x00,0x00,0x18,0x18,0x00},{0x00,0x18,0x18,0x00,0x00,0x18,0x18,0x30},
  {0x0C,0x18,0x30,0x60,0x30,0x18,0x0C,0x00},{0x00,0x00,0x7E,0x00,0x7E,0x00,0x00,0x00},
  {0x30,0x18,0x0C,0x06,0x0C,0x18,0x30,0x00},{0x3C,0x66,0x06,0x0C,0x18,0x00,0x18,0x00},
  {0x3C,0x66,0x6E,0x6E,0x60,0x62,0x3C,0x00},{0x18,0x3C,0x66,0x7E,0x66,0x66,0x66,0x00},
  {0x7C,0x66,0x66,0x7C,0x66,0x66,0x7C,0x00},{0x3C,0x66,0x60,0x60,0x60,0x66,0x3C,0x00},
  {0x78,0x6C,0x66,0x66,0x66,0x6C,0x78,0x00},{0x7E,0x60,0x60,0x7C,0x60,0x60,0x7E,0x00},
  {0x7E,0x60,0x60,0x7C,0x60,0x60,0x60,0x00},{0x3C,0x66,0x60,0x6E,0x66,0x66,0x3C,0x00},
  {0x66,0x66,0x66,0x7E,0x66,0x66,0x66,0x00},{0x3C,0x18,0x18,0x18,0x18,0x18,0x3C,0x00},
  {0x1E,0x0C,0x0C,0x0C,0x0C,0x6C,0x38,0x00},{0x66,0x6C,0x78,0x70,0x78,0x6C,0x66,0x00},
  {0x60,0x60,0x60,0x60,0x60,0x60,0x7E,0x00},{0x63,0x77,0x7F,0x6B,0x63,0x63,0x63,0x00},
  {0x66,0x76,0x7E,0x7E,0x6E,0x66,0x66,0x00},{0x3C,0x66,0x66,0x66,0x66,0x66,0x3C,0x00},
  {0x7C,0x66,0x66,0x7C,0x60,0x60,0x60,0x00},{0x3C,0x66,0x66,0x66,0x6E,0x3C,0x06,0x00},
  {0x7C,0x66,0x66,0x7C,0x6C,0x66,0x66,0x00},{0x3C,0x66,0x60,0x3C,0x06,0x66,0x3C,0x00},
  {0x7E,0x18,0x18,0x18,0x18,0x18,0x18,0x00},{0x66,0x66,0x66,0x66,0x66,0x66,0x3C,0x00},
  {0x66,0x66,0x66,0x66,0x66,0x3C,0x18,0x00},{0x63,0x63,0x63,0x6B,0x7F,0x77,0x63,0x00},
  {0x66,0x66,0x3C,0x18,0x3C,0x66,0x66,0x00},{0x66,0x66,0x66,0x3C,0x18,0x18,0x18,0x00},
  {0x7E,0x06,0x0C,0x18,0x30,0x60,0x7E,0x00},{0x3C,0x30,0x30,0x30,0x30,0x30,0x3C,0x00},
  {0x60,0x30,0x18,0x0C,0x06,0x03,0x01,0x00},{0x3C,0x0C,0x0C,0x0C,0x0C,0x0C,0x3C,0x00},
  {0x18,0x3C,0x66,0x00,0x00,0x00,0x00,0x00},{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF},
};

extern LPDIRECT3DDEVICE8 g_pDevice;
struct MenuVert { float x, y, z, rhw; DWORD color; };
#define FVF_MENU (D3DFVF_XYZRHW|D3DFVF_DIFFUSE)

static void DrawRect(float x, float y, float w, float h, DWORD color) {
    MenuVert v[4] = { {x,y,0,1,color},{x + w,y,0,1,color},{x,y + h,0,1,color},{x + w,y + h,0,1,color} };
    g_pDevice->SetVertexShader(FVF_MENU);
    g_pDevice->SetTexture(0, NULL);
    g_pDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, v, sizeof(MenuVert));
}
static void DrawChar(float x, float y, char c, DWORD color, float sc) {
    if (c < 32 || c>127)c = 32;
    const BYTE* g = kFont8x8[(unsigned char)c - 32];
    for (int r = 0;r < 8;r++) { BYTE b = g[r];for (int col = 0;col < 8;col++)if (b & (0x80 >> col))DrawRect(x + col * sc, y + r * sc, sc, sc, color); }
}
static void DrawString(float x, float y, const char* s, DWORD color, float sc) {
    for (;*s;s++, x += 8 * sc) { char c = *s;if (c >= 'a' && c <= 'z')c -= 32;DrawChar(x, y, c, color, sc); }
}

/*==========================================================================
  Toast notification - shown briefly after save/load/cheat
  Matches SMW port pattern exactly.
==========================================================================*/
static char s_toast_msg[64] = "";
static int  s_toast_timer = 0;
#define TOAST_FRAMES 90

static void ShowToast(const char* msg) {
    _snprintf(s_toast_msg, sizeof(s_toast_msg), "%s", msg);
    s_toast_timer = TOAST_FRAMES;
}

extern "C" bool Xbox_Menu_HasToast(void) { return s_toast_timer > 0; }
extern "C" int  Xbox_Menu_GetToastTimer(void) { return s_toast_timer; }
extern "C" void Xbox_Menu_TickToast(void) { if (s_toast_timer > 0) --s_toast_timer; }
extern "C" const char* Xbox_Menu_GetToastMsg(void) { return s_toast_msg; }

/* DrawToast - called from Xbox_Menu_Draw, matches SMW exactly */
static void DrawToastInner(float CX, float CY) {
    if (s_toast_timer <= 0) return;
    --s_toast_timer;
    float alpha = (s_toast_timer < 30) ? (s_toast_timer / 30.0f) : 1.0f;
    DWORD a = (DWORD)(alpha * 255.0f);
    const float tsc = 1.0f;
    const float CW = 8 * tsc, PAD = 4.0f;
    int len = (int)strlen(s_toast_msg);
    float bw = len * CW + PAD * 2 + 4;
    float bh = CW + PAD * 2 + 4;
    float bx = CX - bw * 0.5f;
    float by = CY - bh * 0.5f;
    DWORD ba = (DWORD)(alpha * 0xFF);
    DrawRect(bx - 2, by - 2, bw + 4, bh + 4, (ba << 24) | 0x0000AAFF);
    DrawRect(bx, by, bw, bh, (ba << 24) | 0x00001020);
    float tx = CX - (len * CW * 0.5f);
    float ty = by + PAD;
    DrawString(tx, ty, s_toast_msg, (a << 24) | 0x00FFFFFF, tsc);
}

extern "C" void Xbox_Menu_DrawToast(void) { /* stub - drawing happens inside Xbox_Menu_Draw */ }

static void DrawMenu(float cx, float cy, const char* title, const char** items, int count, int sel, float sc) {
    float CW = 8 * sc, PAD = 8.0f, ROW = 8 * sc + 4.0f;
    int maxLen = (int)strlen(title);
    for (int i = 0; i < count; i++) {
        int l = (int)strlen(items[i]) + 2;
        if (l > maxLen) maxLen = l;
    }
    float mw = maxLen * CW + PAD * 2 + 4;
    float mh = PAD * 2 + (count + 1) * ROW + 4;
    float mx = cx - mw / 2, my = cy - mh / 2;
    DrawRect(mx - 2, my - 2, mw + 4, mh + 4, 0xFF00AAFF);
    DrawRect(mx, my, mw, mh, 0xE0001020);
    /* Centred title */
    float titleX = mx + (mw - (float)strlen(title) * CW) * 0.5f;
    DrawString(titleX, my + PAD, title, 0xFFFFFF00, sc);
    float y = my + PAD + ROW;
    for (int i = 0; i < count; i++) {
        DWORD col = (i == sel) ? 0xFF00FF00 : 0xFFCCCCCC;
        int itemLen = (int)strlen(items[i]);
        float itemX = mx + (mw - (itemLen + 2) * CW) * 0.5f;
        if (i == sel) DrawString(itemX, y, ">", col, sc);
        DrawString(itemX + CW * 2, y, items[i], col, sc);
        y += ROW;
    }
}

/*==========================================================================
  Pages
==========================================================================*/
enum MenuPage {
    PAGE_MAIN = 0, PAGE_CHEATS, PAGE_CONSUMABLES, PAGE_EQUIPMENT,
    PAGE_ITEMS, PAGE_BOTTLES, PAGE_GODMODE,
    PAGE_SETTINGS_GFX, PAGE_SETTINGS_GAME,
    PAGE_SAVE, PAGE_LOAD,
    PAGE_COUNT
};

/*==========================================================================
  State
==========================================================================*/
static bool s_open = false;
static bool s_turbo = false;
static bool s_paused = false;
static bool s_justClosed = false;
static bool s_godHealth = false;
static bool s_godMagic = false;
static bool s_godBombs = false;
static bool s_godArrows = false;
static bool s_godRupees = false;
static MenuPage s_page = PAGE_MAIN;
static int  s_sel[PAGE_COUNT] = { 0 };
static int  s_pending_dispmode = -1; /* -1 = no pending change */

/*==========================================================================
  Menu item arrays
==========================================================================*/
static const char* kMain[] = {
    "CHEATS","SETTINGS",
    "SAVE STATE","LOAD STATE",
    "TURBO MODE","PAUSE GAME",
    "RESET GAME","EXIT GAME","CLOSE"
};
static const int kMainN = 9;

static const char* kCheats[] = {
    "CONSUMABLES","EQUIPMENT","ITEMS","BOTTLES","GOD MODE","BACK"
};
static const int kCheatsN = 6;

static const char* kConsumables[] = {
    "FILL HEALTH","FILL MAGIC","FILL RUPEES (+100)",
    "FILL BOMBS","UPGRADE BOMBS",
    "FILL ARROWS","UPGRADE ARROWS",
    "MAX HEARTS (20)","BACK"
};
static const int kConsumablesN = 9;

static const char* kEquipment[] = {
    "SWORD LV1 (FIGHTER)","SWORD LV2 (MASTER)",
    "SWORD LV3 (TEMPERED)","SWORD LV4 (GOLDEN)",
    "SHIELD LV1","SHIELD LV2 (RED)","SHIELD LV3 (MIRROR)",
    "ARMOR LV1 (BLUE MAIL)","ARMOR LV2 (RED MAIL)",
    "POWER GLOVES","TITAN MITTS",
    "PEGASUS BOOTS","FLIPPERS","MOON PEARL",
    "BACK"
};
static const int kEquipmentN = 15;

static const char* kItems[] = {
    "BOW + ARROWS","BOW + SILVER ARROWS",
    "BLUE BOOMERANG","RED BOOMERANG",
    "HOOKSHOT",
    "MUSHROOM","MAGIC POWDER",
    "FIRE ROD","ICE ROD",
    "BOMBOS MEDALLION","ETHER MEDALLION","QUAKE MEDALLION",
    "LAMP (TORCH)","HAMMER",
    "SHOVEL","FLUTE (OCARINA)",
    "BUG NET","BOOK OF MUDORA",
    "CANE OF SOMARIA","CANE OF BYRNA",
    "MAGIC CAPE","MAGIC MIRROR",
    "BACK"
};
static const int kItemsN = 23;

static const char* kBottles[] = {
    "4 EMPTY BOTTLES","4x RED POTION",
    "4x GREEN POTION","4x BLUE POTION",
    "4x FAIRIES","REMOVE ALL BOTTLES","BACK"
};
static const int kBottlesN = 7;

static const char* kSlots[] = {
    "SLOT 1","SLOT 2","SLOT 3","SLOT 4","SLOT 5",
    "SLOT 6","SLOT 7","SLOT 8","SLOT 9","SLOT 10"
};

/*==========================================================================
  Cheat actions - directly write to g_ram using addresses from variables.h
  PatchCommand only handles 'w','W','k','o','l','E' - all others must be
  written directly.
==========================================================================*/
extern "C" uint8 g_ram[];

/* Helper: write a byte directly and sync emulator state */
static void RamPoke(int addr, uint8 val) {
    g_ram[addr] = val;
}
static void RamPoke16(int addr, uint16_t val) {
    g_ram[addr] = (uint8)(val & 0xFF);
    g_ram[addr + 1] = (uint8)(val >> 8);
}

/* Bomb/arrow upgrade tables - match exactly what sprite_main.c uses */
static const uint8 kBombUpgradeMax[8] = { 0x10, 0x15, 0x20, 0x25, 0x30, 0x35, 0x40, 0x50 };
static const uint8 kArrowUpgradeMax[8] = { 0x30, 0x35, 0x40, 0x45, 0x50, 0x55, 0x60, 0x70 };

static void DoConsumable(int s) {
    switch (s) {
    case 0: /* Fill health - restore to capacity, same as drinking red potion */
        RamPoke(0xF36D, g_ram[0xF36C]); /* link_health_current = link_health_capacity */
        break;
    case 1: /* Fill magic - 128 is the hard cap (hud.c line 375-376) */
        RamPoke(0xF36E, 0x80); /* link_magic_power = 128 */
        break;
    case 2: /* Fill rupees +100 */
    {
        uint16_t r = *(uint16_t*)(g_ram + 0xF360) + 100; if (r > 9999) r = 9999;
        RamPoke16(0xF360, r); RamPoke16(0xF362, r);
    }
    break;
    case 3: /* Fill bombs - fill to current upgrade level max */
    {
        uint8 maxb = kMaxBombsForLevel[g_ram[0xF370]];
        RamPoke(0xF375, maxb); /* link_bomb_filler */
        RamPoke(0xF343, maxb); /* link_item_bombs  */
    }
    break;
    case 4: /* Upgrade bombs - mirrors sprite_main.c case 8 exactly.
               Only updates upgrade level and filler (capacity).
               Does NOT change current bomb count - player keeps what they have. */
    {
        int i = g_ram[0xF370]; /* link_bomb_upgrades */
        if (i < 7) {
            i++;
            RamPoke(0xF370, (uint8)i);           /* link_bomb_upgrades++ */
            RamPoke(0xF375, kBombUpgradeMax[i]); /* link_bomb_filler = new max */
        }
    }
    break;
    case 5: /* Fill arrows - fill to current upgrade level max */
    {
        uint8 maxa = kMaxArrowsForLevel[g_ram[0xF371]];
        RamPoke(0xF376, maxa); /* link_arrow_filler */
        RamPoke(0xF377, maxa); /* link_num_arrows   */
    }
    break;
    case 6: /* Upgrade arrows - mirrors sprite_main.c case 12 exactly.
               Only updates upgrade level and filler (capacity).
               Does NOT change current arrow count - player keeps what they have. */
    {
        int i = g_ram[0xF371]; /* link_arrow_upgrades */
        if (i < 7) {
            i++;
            RamPoke(0xF371, (uint8)i);            /* link_arrow_upgrades++ */
            RamPoke(0xF376, kArrowUpgradeMax[i]); /* link_arrow_filler = new max */
        }
    }
    break;
    case 7: /* Max hearts (0xA0 = 20 hearts, each heart = 8 units) */
        RamPoke(0xF36C, 0xA0); RamPoke(0xF36D, 0xA0);
        break;
    case 8: return; /* BACK */
    }
}

static void DoEquipment(int s) {
    switch (s) {
    case 0:  RamPoke(0xF359, 1); break; /* Sword Lv1 - Fighter */
    case 1:  RamPoke(0xF359, 2); break; /* Sword Lv2 - Master */
    case 2:  RamPoke(0xF359, 3); break; /* Sword Lv3 - Tempered */
    case 3:  RamPoke(0xF359, 4); break; /* Sword Lv4 - Golden */
    case 4:  RamPoke(0xF35A, 1); break; /* Shield Lv1 */
    case 5:  RamPoke(0xF35A, 2); break; /* Shield Lv2 - Red */
    case 6:  RamPoke(0xF35A, 3); break; /* Shield Lv3 - Mirror */
    case 7:  RamPoke(0xF35B, 1); break; /* Armor Lv1 - Blue Mail */
    case 8:  RamPoke(0xF35B, 2); break; /* Armor Lv2 - Red Mail */
    case 9:  RamPoke(0xF354, 1); break; /* Power Gloves */
    case 10: RamPoke(0xF354, 2); break; /* Titan Mitts */
    case 11: RamPoke(0xF355, 1); break; /* Pegasus Boots */
    case 12: RamPoke(0xF356, 1); break; /* Flippers */
    case 13: RamPoke(0xF357, 1); break; /* Moon Pearl */
    case 14: return; /* BACK */
    }
}

static void DoItem(int s) {
    switch (s) {
        /* Bow: 2=wood+arrows, 4=silver+arrows. Also give 30 arrows. */
    case 0:  RamPoke(0xF340, 2); RamPoke(0xF376, 30); RamPoke(0xF377, 30); break; /* Bow + Arrows */
    case 1:  RamPoke(0xF340, 4); RamPoke(0xF376, 30); RamPoke(0xF377, 30); break; /* Bow + Silver Arrows */
    case 2:  RamPoke(0xF341, 1); break; /* Blue Boomerang */
    case 3:  RamPoke(0xF341, 2); break; /* Red Boomerang */
    case 4:  RamPoke(0xF342, 1); break; /* Hookshot */
    case 5:  RamPoke(0xF344, 1); break; /* Mushroom */
    case 6:  RamPoke(0xF344, 2); break; /* Magic Powder */
    case 7:  RamPoke(0xF345, 1); break; /* Fire Rod */
    case 8:  RamPoke(0xF346, 1); break; /* Ice Rod */
    case 9:  RamPoke(0xF347, 1); break; /* Bombos Medallion */
    case 10: RamPoke(0xF348, 1); break; /* Ether Medallion */
    case 11: RamPoke(0xF349, 1); break; /* Quake Medallion */
    case 12: RamPoke(0xF34A, 1); break; /* Lamp */
    case 13: RamPoke(0xF34B, 1); break; /* Hammer */
        /* Flute: 1=shovel, 2=flute/ocarina */
    case 14: RamPoke(0xF34C, 1); break; /* Shovel */
    case 15: RamPoke(0xF34C, 2); break; /* Flute (Ocarina) */
    case 16: RamPoke(0xF34D, 1); break; /* Bug Net */
    case 17: RamPoke(0xF34E, 1); break; /* Book of Mudora */
    case 18: RamPoke(0xF350, 1); break; /* Cane of Somaria */
    case 19: RamPoke(0xF351, 1); break; /* Cane of Byrna */
    case 20: RamPoke(0xF352, 1); break; /* Magic Cape */
        /* Mirror: 1=map, 2=magic mirror */
    case 21: RamPoke(0xF353, 2); break; /* Magic Mirror */
    case 22: return; /* BACK */
    }
}

static void DoBottle(int s) {
    /* Bottles 0-3 at 0xF35C-0xF35F: 0=empty,1=mushroom,2=empty_bottle,3=red,4=green,5=blue,6=fairy */
    switch (s) {
    case 0: /* 4 Empty Bottles */
        RamPoke(0xF35C, 2); RamPoke(0xF35D, 2); RamPoke(0xF35E, 2); RamPoke(0xF35F, 2);
        break;
    case 1: /* 4x Red Potion */
        RamPoke(0xF35C, 3); RamPoke(0xF35D, 3); RamPoke(0xF35E, 3); RamPoke(0xF35F, 3);
        break;
    case 2: /* 4x Green Potion */
        RamPoke(0xF35C, 4); RamPoke(0xF35D, 4); RamPoke(0xF35E, 4); RamPoke(0xF35F, 4);
        break;
    case 3: /* 4x Blue Potion */
        RamPoke(0xF35C, 5); RamPoke(0xF35D, 5); RamPoke(0xF35E, 5); RamPoke(0xF35F, 5);
        break;
    case 4: /* 4x Fairies */
        RamPoke(0xF35C, 6); RamPoke(0xF35D, 6); RamPoke(0xF35E, 6); RamPoke(0xF35F, 6);
        break;
    case 5: /* Remove all bottles */
        RamPoke(0xF35C, 0); RamPoke(0xF35D, 0); RamPoke(0xF35E, 0); RamPoke(0xF35F, 0);
        break;
    case 6: return; /* BACK */
    }
}

/*==========================================================================
  Input
==========================================================================*/
extern XINPUT_STATE Xbox_Input_GetRawState(void);
static bool BtnPressed(DWORD cur, DWORD prev, DWORD mask) { return (cur & mask) && !(prev & mask); }

/*==========================================================================
  Public API
==========================================================================*/
extern "C" {
    bool Xbox_Menu_IsOpen(void) { return s_open; }
    bool Xbox_Menu_IsTurbo(void) { return s_turbo; }
    bool Xbox_Menu_IsPaused(void) { return s_paused; }
    bool Xbox_Menu_JustClosed(void) { bool v = s_justClosed;s_justClosed = false;return v; }
    bool Xbox_Menu_IsGodHealth(void) { return s_godHealth; }
    bool Xbox_Menu_IsGodMagic(void) { return s_godMagic; }
    bool Xbox_Menu_IsGodBombs(void) { return s_godBombs; }
    bool Xbox_Menu_IsGodArrows(void) { return s_godArrows; }
    bool Xbox_Menu_IsGodRupees(void) { return s_godRupees; }
}

/*==========================================================================
  Update
==========================================================================*/
extern "C" void Xbox_Menu_Update(void)
{
    XINPUT_STATE st = Xbox_Input_GetRawState();
    DWORD cur = st.Gamepad.wButtons;
    BYTE* ab = st.Gamepad.bAnalogButtons;

    static DWORD prev = 0;

    /* L3 toggles menu */
    if (BtnPressed(cur, prev, XINPUT_GAMEPAD_LEFT_THUMB)) {
        s_open = !s_open;
        if (s_open) { s_page = PAGE_MAIN; for (int i = 0;i < PAGE_COUNT;i++) s_sel[i] = 0; s_pending_dispmode = -1; }
    }

    if (!s_open) { prev = cur; return; }

    /* Count for current page */
    int count = 0;
    switch (s_page) {
    case PAGE_MAIN:          count = kMainN;      break;
    case PAGE_CHEATS:        count = kCheatsN;    break;
    case PAGE_CONSUMABLES:   count = kConsumablesN;break;
    case PAGE_EQUIPMENT:     count = kEquipmentN; break;
    case PAGE_ITEMS:         count = kItemsN;     break;
    case PAGE_BOTTLES:       count = kBottlesN;   break;
    case PAGE_GODMODE:       count = 6;           break;
    case PAGE_SETTINGS_GFX:  count = 4;           break;
    case PAGE_SETTINGS_GAME: count = 15;          break;
    case PAGE_SAVE:
    case PAGE_LOAD:          count = 10;          break;
    default: break;
    }

    int* sel = &s_sel[(int)s_page];

    if (BtnPressed(cur, prev, XINPUT_GAMEPAD_DPAD_UP))   *sel = (*sel - 1 + count) % count;
    if (BtnPressed(cur, prev, XINPUT_GAMEPAD_DPAD_DOWN))  *sel = (*sel + 1) % count;

    /* B = back */
    bool bBtn = ab[XINPUT_GAMEPAD_B] > 128;
    bool bPrev = (prev & 0x10000) != 0;
    if (bBtn && !bPrev) {
        switch (s_page) {
        case PAGE_MAIN:
            if (s_pending_dispmode >= 0 && s_pending_dispmode != Xbox_D3D_GetDispMode()) {
                Xbox_D3D_SetDispMode(s_pending_dispmode);
                Xbox_Cfg_SetExtendedAspect(s_pending_dispmode == 2 ? 71 : 0);
                Xbox_Reapply_Config();
                Xbox_Config_Save();
            }
            s_pending_dispmode = -1;
            s_open = false; s_justClosed = true; Xbox_Input_SuppressUntilRelease(); break;
        case PAGE_CHEATS:
        case PAGE_SETTINGS_GFX:
        case PAGE_SAVE:
        case PAGE_LOAD:         s_page = PAGE_MAIN; break;
        case PAGE_SETTINGS_GAME:s_page = PAGE_SETTINGS_GFX; break;
        default:                s_page = PAGE_CHEATS; break;
        }
    }

    /* A = select */
    bool aBtn = ab[XINPUT_GAMEPAD_A] > 128;
    bool aPrev = (prev & 0x20000) != 0;
    if (aBtn && !aPrev) {
        bool close = false, goBack = false;
        switch (s_page) {

        case PAGE_MAIN:
            switch (*sel) {
            case 0: s_page = PAGE_CHEATS; break;
            case 1: s_page = PAGE_SETTINGS_GFX; break;
            case 2: s_page = PAGE_SAVE; break;
            case 3: s_page = PAGE_LOAD; break;
            case 4: s_turbo = !s_turbo; close = true; break;
            case 5: s_paused = !s_paused; close = true; break;
            case 6: ZeldaReset(true); ZeldaReadSram(); Xbox_Config_Load(); Xbox_Reapply_Config(); close = true; break;
            case 7: Xbox_Menu_RequestExit(); close = true; break;
            case 8: close = true; break;
            }
            break;

        case PAGE_CHEATS:
            switch (*sel) {
            case 0: s_page = PAGE_CONSUMABLES; break;
            case 1: s_page = PAGE_EQUIPMENT; break;
            case 2: s_page = PAGE_ITEMS; break;
            case 3: s_page = PAGE_BOTTLES; break;
            case 4: s_page = PAGE_GODMODE; break;
            case 5: goBack = true; break;
            }
            break;

        case PAGE_CONSUMABLES:
            DoConsumable(*sel);
            if (*sel == kConsumablesN - 1) { goBack = true; }
            else { ShowToast(kConsumables[*sel]); close = true; }
            break;

        case PAGE_EQUIPMENT:
            DoEquipment(*sel);
            if (*sel == kEquipmentN - 1) { goBack = true; }
            else { ShowToast(kEquipment[*sel]); close = true; }
            break;

        case PAGE_ITEMS:
            DoItem(*sel);
            if (*sel == kItemsN - 1) { goBack = true; }
            else { ShowToast(kItems[*sel]); close = true; }
            break;

        case PAGE_BOTTLES:
            DoBottle(*sel);
            if (*sel == kBottlesN - 1) { goBack = true; }
            else { ShowToast(kBottles[*sel]); close = true; }
            break;

        case PAGE_GODMODE:
            switch (*sel) {
            case 0: s_godHealth = !s_godHealth; break;
            case 1: s_godMagic = !s_godMagic; break;
            case 2: s_godBombs = !s_godBombs; break;
            case 3: s_godArrows = !s_godArrows; break;
            case 4: s_godRupees = !s_godRupees; break;
            case 5: goBack = true; break;
            }
            break;

        case PAGE_SETTINGS_GFX: {
            switch (*sel) {
            case 0: {
                int cur = s_pending_dispmode >= 0 ? s_pending_dispmode : Xbox_D3D_GetDispMode();
                s_pending_dispmode = (cur + 1) % 3;
                break;
            }
            case 1: Xbox_Cfg_SetNoSpriteLimits(!Xbox_Cfg_GetNoSpriteLimits());
                Xbox_SetPpuRenderFlags(Xbox_GetPpuRenderFlags() ^ kPpuRenderFlags_NoSpriteLimits);
                Xbox_Config_Save(); break;
            case 2: s_page = PAGE_SETTINGS_GAME; break;
            case 3: goBack = true; break;
            }
            break;
        }

        case PAGE_SETTINGS_GAME: {
            unsigned int f = Xbox_Cfg_GetFeatures();
            switch (*sel) {
            case 0:  f ^= kFeatures0_DimFlashes; break;
            case 1:  f ^= kFeatures0_DisableLowHealthBeep; break;
            case 2:  f ^= kFeatures0_MirrorToDarkworld; break;
            case 3:  f ^= kFeatures0_TurnWhileDashing; break;
            case 4:  f ^= kFeatures0_BreakPotsWithSword; break;
            case 5:  f ^= kFeatures0_CollectItemsWithSword; break;
            case 6:  f ^= kFeatures0_SkipIntroOnKeypress; break;
            case 7:  f ^= kFeatures0_CarryMoreRupees; break;
            case 8:  f ^= kFeatures0_MoreActiveBombs; break;
            case 9:  f ^= kFeatures0_ShowMaxItemsInYellow; break;
            case 10: f ^= kFeatures0_MiscBugFixes; break;
            case 11: f ^= kFeatures0_GameChangingBugFixes; break;
            case 12: f ^= kFeatures0_CancelBirdTravel; break;
            case 13: f ^= kFeatures0_SwitchLR; break;
            case 14: goBack = true; break;
            }
            if (!goBack) { Xbox_Cfg_SetFeatures(f); Xbox_Config_Save(); }
            break;
        }

        case PAGE_SAVE: {
            char _dbg[64]; _snprintf(_dbg, sizeof(_dbg), "zelda3: Saving state slot %d\n", *sel + 1);
            OutputDebugStringA(_dbg);
            SaveLoadSlot(kSaveLoad_Save, *sel + 1);
            _snprintf(_dbg, sizeof(_dbg), "zelda3: State slot %d saved OK\n", *sel + 1);
            OutputDebugStringA(_dbg);
            char _t[32]; _snprintf(_t, sizeof(_t), "STATE SAVED - SLOT %d", *sel + 1);
            ShowToast(_t); close = true; break;
        }
        case PAGE_LOAD: {
            /* Check state exists before loading - Xbox_GetStateDir returns NULL if not found */
            const char* stateDir = Xbox_GetStateDir(*sel);
            if (stateDir) {
                char _dbg[64]; _snprintf(_dbg, sizeof(_dbg), "zelda3: Loading state slot %d\n", *sel + 1);
                OutputDebugStringA(_dbg);
                SaveLoadSlot(kSaveLoad_Load, *sel + 1);
                _snprintf(_dbg, sizeof(_dbg), "zelda3: State slot %d load OK\n", *sel + 1);
                OutputDebugStringA(_dbg);
                char _t[32]; _snprintf(_t, sizeof(_t), "STATE LOADED - SLOT %d", *sel + 1);
                ShowToast(_t);
            }
            else {
                OutputDebugStringA("zelda3: State slot not found - nothing loaded\n");
                ShowToast("NO SAVE IN THIS SLOT");
            }
            close = true; break;
        }

        default: break;
        }

        if (goBack) {
            switch (s_page) {
            case PAGE_SETTINGS_GAME: s_page = PAGE_SETTINGS_GFX; break;
            case PAGE_CHEATS:
            case PAGE_SETTINGS_GFX:
            case PAGE_SAVE:
            case PAGE_LOAD:          s_page = PAGE_MAIN; break;
            default:                 s_page = PAGE_CHEATS; break;
            }
        }
        if (close) {
            /* Apply pending display mode change on close */
            if (s_pending_dispmode >= 0 && s_pending_dispmode != Xbox_D3D_GetDispMode()) {
                Xbox_D3D_SetDispMode(s_pending_dispmode);
                Xbox_Cfg_SetExtendedAspect(s_pending_dispmode == 2 ? 71 : 0);
                Xbox_Reapply_Config();
                Xbox_Config_Save();
            }
            s_pending_dispmode = -1;
            s_open = false; s_justClosed = true; Xbox_Input_SuppressUntilRelease();
        }
    }

    prev = cur | (bBtn ? 0x10000 : 0) | (aBtn ? 0x20000 : 0);
}

/*==========================================================================
  Draw
==========================================================================*/
extern "C" void Xbox_Menu_Draw(void)
{
    if (!g_pDevice) return;

    const float SC = 1.0f;
    const float CX = (float)Xbox_D3D_GetDispW() / 2.0f;
    const float CY = (float)Xbox_D3D_GetDispH() / 2.0f;

    if (!s_open) {
        DrawToastInner(CX, CY);
        return;
    }

    switch (s_page) {

    case PAGE_MAIN:
        DrawMenu(CX, CY, "ZELDA A LINK TO THE PAST", (const char**)kMain, kMainN, s_sel[PAGE_MAIN], SC);
        break;

    case PAGE_CHEATS:
        DrawMenu(CX, CY, "CHEATS", (const char**)kCheats, kCheatsN, s_sel[PAGE_CHEATS], SC);
        break;

    case PAGE_CONSUMABLES:
        DrawMenu(CX, CY, "CONSUMABLES", (const char**)kConsumables, kConsumablesN, s_sel[PAGE_CONSUMABLES], SC);
        break;

    case PAGE_EQUIPMENT:
        DrawMenu(CX, CY, "EQUIPMENT", (const char**)kEquipment, kEquipmentN, s_sel[PAGE_EQUIPMENT], SC);
        break;

    case PAGE_ITEMS:
        DrawMenu(CX, CY, "ITEMS", (const char**)kItems, kItemsN, s_sel[PAGE_ITEMS], SC);
        break;

    case PAGE_BOTTLES:
        DrawMenu(CX, CY, "BOTTLES", (const char**)kBottles, kBottlesN, s_sel[PAGE_BOTTLES], SC);
        break;

    case PAGE_GODMODE: {
        static char gm0[32], gm1[32], gm2[32], gm3[32], gm4[32];
        _snprintf(gm0, sizeof(gm0), "NO DAMAGE      [%s]", s_godHealth ? "ON " : "OFF");
        _snprintf(gm1, sizeof(gm1), "NO MAGIC USE   [%s]", s_godMagic ? "ON " : "OFF");
        _snprintf(gm2, sizeof(gm2), "INF BOMBS      [%s]", s_godBombs ? "ON " : "OFF");
        _snprintf(gm3, sizeof(gm3), "INF ARROWS     [%s]", s_godArrows ? "ON " : "OFF");
        _snprintf(gm4, sizeof(gm4), "INF RUPEES     [%s]", s_godRupees ? "ON " : "OFF");
        static const char* gmItems[6];
        gmItems[0] = gm0;gmItems[1] = gm1;gmItems[2] = gm2;
        gmItems[3] = gm3;gmItems[4] = gm4;gmItems[5] = "BACK";
        DrawMenu(CX, CY, "GOD MODE", (const char**)gmItems, 6, s_sel[PAGE_GODMODE], SC);
        break;
    }

    case PAGE_SETTINGS_GFX: {
        static char sg0[32], sg1[32];
        {
            static const char* modes[] = { "4:3","FILL","16:9" };
            int show = s_pending_dispmode >= 0 ? s_pending_dispmode : Xbox_D3D_GetDispMode();
            _snprintf(sg0, sizeof(sg0), "DISPLAY MODE     [%s]", modes[show]);
        }
        _snprintf(sg1, sizeof(sg1), "NO SPRITE LIMITS [%s]", Xbox_Cfg_GetNoSpriteLimits() ? "ON " : "OFF");
        static const char* sgItems[4];
        sgItems[0] = sg0;sgItems[1] = sg1;
        sgItems[2] = "GAMEPLAY OPTIONS >";sgItems[3] = "BACK";
        DrawMenu(CX, CY, "SETTINGS - GRAPHICS", (const char**)sgItems, 4, s_sel[PAGE_SETTINGS_GFX], SC);
        break;
    }

    case PAGE_SETTINGS_GAME: {
        unsigned int f = Xbox_Cfg_GetFeatures();
        static char sp0[32], sp1[32], sp2[32], sp3[32], sp4[32], sp5[32], sp6[32];
        static char sp7[32], sp8[32], sp9[32], sp10[32], sp11[32], sp12[32];
        _snprintf(sp0, sizeof(sp0), "DIM FLASHES      [%s]", (f & kFeatures0_DimFlashes) ? "ON " : "OFF");
        _snprintf(sp1, sizeof(sp1), "NO HEALTH BEEP   [%s]", (f & kFeatures0_DisableLowHealthBeep) ? "ON " : "OFF");
        _snprintf(sp2, sizeof(sp2), "MIRROR DARKWORLD [%s]", (f & kFeatures0_MirrorToDarkworld) ? "ON " : "OFF");
        _snprintf(sp3, sizeof(sp3), "TURN WHILE DASH  [%s]", (f & kFeatures0_TurnWhileDashing) ? "ON " : "OFF");
        _snprintf(sp4, sizeof(sp4), "BREAK POTS/SWORD [%s]", (f & kFeatures0_BreakPotsWithSword) ? "ON " : "OFF");
        _snprintf(sp5, sizeof(sp5), "COLLECT W/SWORD  [%s]", (f & kFeatures0_CollectItemsWithSword) ? "ON " : "OFF");
        _snprintf(sp6, sizeof(sp6), "SKIP INTRO       [%s]", (f & kFeatures0_SkipIntroOnKeypress) ? "ON " : "OFF");
        _snprintf(sp7, sizeof(sp7), "CARRY 9999 RUPEES[%s]", (f & kFeatures0_CarryMoreRupees) ? "ON " : "OFF");
        _snprintf(sp8, sizeof(sp8), "4 ACTIVE BOMBS   [%s]", (f & kFeatures0_MoreActiveBombs) ? "ON " : "OFF");
        _snprintf(sp9, sizeof(sp9), "SHOW MAX YELLOW  [%s]", (f & kFeatures0_ShowMaxItemsInYellow) ? "ON " : "OFF");
        _snprintf(sp10, sizeof(sp10), "MISC BUG FIXES   [%s]", (f & kFeatures0_MiscBugFixes) ? "ON " : "OFF");
        _snprintf(sp11, sizeof(sp11), "GAME BUG FIXES   [%s]", (f & kFeatures0_GameChangingBugFixes) ? "ON " : "OFF");
        _snprintf(sp12, sizeof(sp12), "CANCEL BIRD TRVL [%s]", (f & kFeatures0_CancelBirdTravel) ? "ON " : "OFF");
        char sp13[32];
        _snprintf(sp13, sizeof(sp13), "SWITCH LR ITEMS  [%s]", (f & kFeatures0_SwitchLR) ? "ON " : "OFF");
        static const char* spItems[15];
        spItems[0] = sp0;spItems[1] = sp1;spItems[2] = sp2;spItems[3] = sp3;
        spItems[4] = sp4;spItems[5] = sp5;spItems[6] = sp6;spItems[7] = sp7;
        spItems[8] = sp8;spItems[9] = sp9;spItems[10] = sp10;spItems[11] = sp11;
        spItems[12] = sp12;spItems[13] = sp13;spItems[14] = "BACK";
        DrawMenu(CX, CY, "SETTINGS - GAMEPLAY", (const char**)spItems, 15, s_sel[PAGE_SETTINGS_GAME], SC);
        break;
    }

    case PAGE_SAVE:
        DrawMenu(CX, CY, "SAVE STATE", (const char**)kSlots, 10, s_sel[PAGE_SAVE], SC);
        break;

    case PAGE_LOAD:
        DrawMenu(CX, CY, "LOAD STATE", (const char**)kSlots, 10, s_sel[PAGE_LOAD], SC);
        break;

    default: break;
    }
}