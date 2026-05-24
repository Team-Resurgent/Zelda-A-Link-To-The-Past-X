/*
 * SDL_keycode.h - Xbox/RXDK stub
 *
 * This shim is picked up instead of the real SDL header because
 * src/platform/xbox/ is first in the AdditionalIncludeDirectories.
 * It provides the minimum types needed so config.h compiles cleanly
 * on Xbox without SDL installed.
 */
#ifndef SDL_KEYCODE_H_XBOX_STUB
#define SDL_KEYCODE_H_XBOX_STUB

typedef int SDL_Keycode;
typedef int SDL_Keymod;
typedef int SDL_Scancode;

#define SDLK_SCANCODE_MASK  (1<<30)
#define SDLK_UNKNOWN        0

/* Bare minimum SDLK values so any non-guarded references compile */
#define SDLK_LALT           0
#define SDLK_RALT           0
#define SDLK_LCTRL          0
#define SDLK_RCTRL          0
#define SDLK_LSHIFT         0
#define SDLK_RSHIFT         0

#define KMOD_ALT            0
#define KMOD_CTRL           0
#define KMOD_SHIFT          0
#define KMOD_NONE           0

#endif /* SDL_KEYCODE_H_XBOX_STUB */
