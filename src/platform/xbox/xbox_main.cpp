/*==========================================================================
  xbox_main.cpp  -  Zelda3 Original Xbox entry point (RXDK / XDK native)
  All saves go to UDATA via XCreateSaveGame (dashboard managed).
==========================================================================*/
#include "xbox_platform.h"
#include "xbox_menu.h"

extern "C" void XboxZeldaInit(void);
extern "C" void XboxZeldaFrame(uint16_t buttons);
extern "C" void XboxZeldaFrameLogicOnly(uint16_t buttons);
extern "C" void Xbox_Audio_PushSilence(void);
extern "C" void Xbox_Audio_Resume(void);
extern "C" void Xbox_Menu_DrawToast(void);
extern "C" bool Xbox_Menu_HasToast(void);
extern "C" bool Xbox_Menu_ExitRequested(void);

extern "C" long __cdecl _ftol(double);
extern "C" long __cdecl _ftol2_sse(double f) { return _ftol(f); }

/* -----------------------------------------------------------------------
   Drive mounting
----------------------------------------------------------------------- */
typedef struct _XBOX_STRING {
    USHORT Length;
    USHORT MaximumLength;
    PCHAR  Buffer;
} XBOX_STRING;

static void BuildXboxString(XBOX_STRING& s, const char* z) {
    USHORT L = (USHORT)strlen(z);
    s.Length = L;
    s.MaximumLength = L + 1;
    s.Buffer = (PCHAR)z;
}

extern "C" LONG __stdcall IoCreateSymbolicLink(XBOX_STRING* linkName, XBOX_STRING* targetName);
extern "C" LONG __stdcall IoDeleteSymbolicLink(XBOX_STRING* linkName);

static void MountDrive(const char* driveLetter, const char* devicePath)
{
    char dosPath[32];
    _snprintf(dosPath, sizeof(dosPath), "\\??\\%s", driveLetter);
    XBOX_STRING link, target;
    BuildXboxString(link, dosPath);
    BuildXboxString(target, devicePath);
    IoDeleteSymbolicLink(&link);
    IoCreateSymbolicLink(&link, &target);
}

/* -----------------------------------------------------------------------
   Xbox_EnsureSaveDir
   Tries T:\saves\zelda3 (TDATA) first, falls back to E:\saves\zelda3
   Matches SMW port pattern exactly.
----------------------------------------------------------------------- */
extern "C" void Xbox_EnsureSaveDir(void)
{
    /* Mount the HDD - required before any file I/O */
    MountDrive("E:", "\\Device\\Harddisk0\\Partition1");
    /* All saves now go to UDATA via Xbox_ResolveSavePaths - no TDATA dirs needed */
    OutputDebugStringA("zelda3: HDD mounted\n");
}

/* -----------------------------------------------------------------------
   Timing
----------------------------------------------------------------------- */
static LARGE_INTEGER s_freq;
static LARGE_INTEGER s_last;

static void Timing_Init(void)
{
    QueryPerformanceFrequency(&s_freq);
    QueryPerformanceCounter(&s_last);
}

/* -----------------------------------------------------------------------
   Entry point
----------------------------------------------------------------------- */
void __cdecl main(void)
{
    /* Probe for assets */
    /* Assets always in D:\Media\ - D: is always the launch directory on Xbox */
    OutputDebugStringA("zelda3: assets at D:\\Media\\\n");

    Xbox_EnsureSaveDir();

    if (FAILED(Xbox_D3D_Init())) {
        OutputDebugStringA("zelda3: D3D8 init FAILED\n");
        for (;;) Sleep(1000);
    }

    if (FAILED(Xbox_Audio_Init()))
        OutputDebugStringA("zelda3: Audio init failed (silent)\n");

    Xbox_Input_Init();
    XboxZeldaInit();
    Timing_Init();

    bool running = true;

    while (running) {
        Xbox_Menu_Update();
        if (Xbox_Menu_ExitRequested()) { running = false; break; }

        Xbox_Input_Poll();

        if (Xbox_Menu_IsOpen()) {
            extern void Xbox_D3D_DrawMenuOverlay(void);
            Xbox_D3D_DrawMenuOverlay();
            Xbox_Audio_PushSilence();
        }
        else if (Xbox_Menu_IsPaused()) {
            extern void Xbox_D3D_DrawDimOverlay(void);
            Xbox_D3D_DrawDimOverlay();
            Xbox_Audio_PushSilence();
            Sleep(16);
        }
        else if (Xbox_Menu_IsTurbo()) {
            Xbox_Audio_Resume();
            uint16_t joy = Xbox_Input_GetJoypad();
            XboxZeldaFrameLogicOnly(joy);
            XboxZeldaFrameLogicOnly(joy);
            XboxZeldaFrameLogicOnly(joy);
            XboxZeldaFrame(joy);
        }
        else {
            Xbox_Audio_Resume();
            XboxZeldaFrame(Xbox_Input_GetJoypad());
        }
    }

    Xbox_Input_Shutdown();
    Xbox_Audio_Shutdown();
    Xbox_D3D_Shutdown();

    LD_LAUNCH_DASHBOARD LaunchData;
    ZeroMemory(&LaunchData, sizeof(LaunchData));
    LaunchData.dwReason = XLD_LAUNCH_DASHBOARD_MAIN_MENU;
    XLaunchNewImage(NULL, (LAUNCH_DATA*)&LaunchData);
}