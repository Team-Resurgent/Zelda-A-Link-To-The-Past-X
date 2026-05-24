/*==========================================================================
  xbox_input.cpp  -  XDK gamepad input for zelda3 Xbox port

  zelda3 NMI_ReadJoypads() reverses all 16 bits, so we use post-reversal
  bit positions from kJoypadH/kJoypadL in zelda_rtl.h:
    bit0=B  bit1=Y  bit2=Sel  bit3=Start
    bit4=Up bit5=Dn bit6=Lt   bit7=Rt
    bit8=A  bit9=X  bit10=L   bit11=R

  Keyboard -> SNES -> Xbox mapping (matching zelda3 SDL gamepad defaults):
    Keyboard Z (SNES B, sword)   -> Xbox A button
    Keyboard X (SNES A, run)     -> Xbox B button
    Keyboard A (SNES Y, item1)   -> Xbox X button
    Keyboard S (SNES X, item2)   -> Xbox Y button
    Keyboard C (SNES L)          -> Xbox Left Trigger
    Keyboard V (SNES R)          -> Xbox Right Trigger
    Keyboard Enter (SNES Start)  -> Xbox Start
    Keyboard RShift (SNES Select)-> Xbox Back
    Arrow keys (directions)      -> Xbox DPad
==========================================================================*/

#include "xbox_platform.h"

/* Post-reversal bit positions (matching kJoypadH/kJoypadL) */
#define JOY_B       0x0001  /* sword/action  (kbd: Z) */
#define JOY_Y       0x0002  /* item 1        (kbd: A) */
#define JOY_SELECT  0x0004  /* select/map    (kbd: RShift) */
#define JOY_START   0x0008  /* pause/start   (kbd: Enter) */
#define JOY_UP      0x0010
#define JOY_DOWN    0x0020
#define JOY_LEFT    0x0040
#define JOY_RIGHT   0x0080
#define JOY_A       0x0100  /* run/dash      (kbd: X) */
#define JOY_X       0x0200  /* item 2        (kbd: S) */
#define JOY_L       0x0400  /* L shoulder    (kbd: C) */
#define JOY_R       0x0800  /* R shoulder    (kbd: V) */

#define THRESH 32
#define NUM_PORTS 4

static HANDLE s_hPads[NUM_PORTS];
static uint16_t s_joypad;
static bool s_suppress = false;

void Xbox_Input_Init(void)
{
    XDEVICE_PREALLOC_TYPE prealloc[] = { { XDEVICE_TYPE_GAMEPAD, 4 } };
    XInitDevices(1, prealloc);
    Sleep(50);

    DWORD ins, rem;
    XGetDeviceChanges(XDEVICE_TYPE_GAMEPAD, &ins, &rem);
    for (DWORD i = 0; i < NUM_PORTS; i++)
        if (ins & (1 << i))
            s_hPads[i] = XInputOpen(XDEVICE_TYPE_GAMEPAD, i, XDEVICE_NO_SLOT, NULL);
}

void Xbox_Input_Poll(void)
{
    DWORD ins, rem;
    if (XGetDeviceChanges(XDEVICE_TYPE_GAMEPAD, &ins, &rem)) {
        for (DWORD i = 0; i < NUM_PORTS; i++) {
            if (rem & (1 << i)) { XInputClose(s_hPads[i]); s_hPads[i] = NULL; }
            if (ins & (1 << i)) s_hPads[i] = XInputOpen(XDEVICE_TYPE_GAMEPAD, i, XDEVICE_NO_SLOT, NULL);
        }
    }

    uint16_t joy = 0;
    for (DWORD i = 0; i < NUM_PORTS; i++) {
        if (!s_hPads[i]) continue;
        XINPUT_STATE st;
        ZeroMemory(&st, sizeof(st));
        if (XInputGetState(s_hPads[i], &st) != ERROR_SUCCESS) continue;

        WORD  b = st.Gamepad.wButtons;
        BYTE* ab = st.Gamepad.bAnalogButtons;

        /* DPad - movement */
        if (b & XINPUT_GAMEPAD_DPAD_UP)    joy |= JOY_UP;
        if (b & XINPUT_GAMEPAD_DPAD_DOWN)  joy |= JOY_DOWN;
        if (b & XINPUT_GAMEPAD_DPAD_LEFT)  joy |= JOY_LEFT;
        if (b & XINPUT_GAMEPAD_DPAD_RIGHT) joy |= JOY_RIGHT;

        /* Start / Select */
        if (b & XINPUT_GAMEPAD_START)      joy |= JOY_START;
        if (b & XINPUT_GAMEPAD_BACK)       joy |= JOY_SELECT;

        /* Face buttons (analog) - matching keyboard layout:
           A=sword(Z), B=run(X), X=item1(A), Y=item2(S) */
        if (ab[XINPUT_GAMEPAD_A] > THRESH) joy |= JOY_B;   /* sword */
        if (ab[XINPUT_GAMEPAD_B] > THRESH) joy |= JOY_A;   /* run */
        if (ab[XINPUT_GAMEPAD_X] > THRESH) joy |= JOY_Y;   /* item 1 */
        if (ab[XINPUT_GAMEPAD_Y] > THRESH) joy |= JOY_X;   /* item 2 */

        /* Shoulders - item rotation */
        if (ab[XINPUT_GAMEPAD_LEFT_TRIGGER] > THRESH) joy |= JOY_L;
        if (ab[XINPUT_GAMEPAD_RIGHT_TRIGGER] > THRESH) joy |= JOY_R;

        /* If suppressing, zero input until controller fully released */
        if (s_suppress) {
            if (joy == 0) s_suppress = false;
            else joy = 0;
        }

        break;
    }
    s_joypad = joy;
}

void Xbox_Input_SuppressUntilRelease(void) { s_suppress = true; }

uint16_t Xbox_Input_GetJoypad(void) { return s_joypad; }

XINPUT_STATE Xbox_Input_GetRawState(void)
{
    XINPUT_STATE st;
    ZeroMemory(&st, sizeof(st));
    for (int i = 0; i < NUM_PORTS; i++) {
        if (s_hPads[i] && XInputGetState(s_hPads[i], &st) == ERROR_SUCCESS)
            break;
    }
    return st;
}

void Xbox_Input_Shutdown(void)
{
    for (int i = 0; i < NUM_PORTS; i++)
        if (s_hPads[i]) { XInputClose(s_hPads[i]); s_hPads[i] = NULL; }
}