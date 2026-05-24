# Zelda 3 – A Link to the Past X
### Original Xbox Port

A native Original Xbox port of [zelda3](https://github.com/snesrev/zelda3), the open-source reimplementation of The Legend of Zelda: A Link to the Past.

Runs natively on Original Xbox hardware using Direct3D 8, DirectSound and XInput. No emulator required.

---

## Features

### Core Game
- Full game playable from start to end
- 60fps gameplay
- 720p output (falls back to 480p if not supported by your TV)
- 4:3 and 16:9 widescreen support

### Xbox-Specific Features
- **In-game menu** — Press **L3** (left stick click) to open
- **Save states** — 10 save slots, fully integrated with the Xbox Dashboard memory manager. Delete individual save states directly from the dashboard.
- **Dashboard-managed saves** — SRAM and all save states appear in the Xbox Dashboard under Memory → Hard Drive. Users can copy saves to a memory unit or delete them from there.
- **Turbo mode** — Runs the game at 4x speed
- **Pause with dim overlay**
- **Cheats menu** — God mode (health, magic, bombs, arrows, rupees), fill/upgrade items, equipment and bottle management
- **Settings menu** — Toggle all enhanced features in-game

### Enhanced Features (toggleable in Settings)
- Dim flashes
- No low health beep
- Mirror to Dark World
- Turn while dashing
- Break pots with sword (Master Sword or better required)
- Collect items with sword
- Skip intro
- Show max items in yellow
- Carry 9999 rupees
- 4 active bombs
- Misc bug fixes
- Game-changing bug fixes
- Cancel bird travel
- Switch L/R item slots (secondary item slot system)

---

## Requirements

- Original Xbox console (any revision)
- Hard drive (internal or replacement)
- [RXDK](https://github.com/Team-Resurgent/RXDK) — Xbox Development Kit for MSVC
- Visual Studio 2019 or newer
- A **US region** copy of the Zelda 3 ROM (`zelda3.sfc`)
  - SHA256: `66871d66be19ad2c34c927d6b14cd8eb6fc3181965b6e517cb361f7316009cfb`
- Python 3.x (for asset extraction)

---

## Building

### Step 1 — Install Python and dependencies

1. Download and install [Python 3](https://www.python.org/downloads/) — check **"Add to PATH"** during install
2. Open a command prompt and run:
```
python -m pip install --upgrade pip pillow pyyaml
```

### Step 2 — Extract game assets

1. Place your US ROM named `zelda3.sfc` in the root of the repository
2. Double-click `extract_assets.bat` in the root directory
3. This creates `zelda3_assets.dat` in the same folder — keep this file

### Step 3 — Install RXDK

1. Download [RXDK](https://github.com/Team-Resurgent/RXDK) and follow its installation instructions
2. Ensure the `RXDK_LIBS` environment variable is set (the RXDK installer does this automatically)

### Step 4 — Open and build the project

1. Open `src/zelda3_xbox.vcxproj` in Visual Studio 2019 or newer
2. Select **Release | Win32** configuration
3. Place `zelda3_assets.dat` into the `src/Media/` folder
4. Press **F7** (Build Solution)

### Step 5 — Build output

After a successful build the post-build script automatically:
- Patches the XBE subsystem flag
- Builds the title image and save image into `.xbx` format
- Assembles the build folder
- Packs an XISO

```
src/Release/
    zelda3_xbox.xbe          Raw XBE (intermediate)
    Build/
        zelda3_xbox/
            default.xbe      ← Copy this to your Xbox
            Media/
                zelda3_assets.dat
    XISO/
        zelda3_xbox.iso      ← Burn or mount this
```

The build summary printed at the end shows:
```
Title:    Zelda ALTTP X
Title ID: 0x54520002
TDATA:    E:\TDATA\54520002
UDATA:    E:\UDATA\54520002
Build:    ...\Release\Build\zelda3_xbox
ISO:      ...\Release\XISO\zelda3_xbox.iso
```

---

## Installation

### Option A — Copy files to Xbox hard drive (FTP)
1. Connect to your Xbox via FTP
2. Copy the entire `Build/zelda3_xbox/` folder to your Xbox (e.g. `E:\Games\ZeldaALTTPX\`)
3. Launch `default.xbe` from your dashboard

### Option B — Burn/mount ISO
1. Use the generated `XISO/zelda3_xbox.iso`
2. Burn to DVD-R or mount via a disc emulator

---

## File Layout on Xbox

```
E:\Games\ZeldaALTTPX\
    default.xbe
    Media\
        zelda3_assets.dat
```

The game will always look for assets in `D:\Media\` — on Xbox, `D:` always points to the launch directory regardless of which dashboard you use.

---

## Save Data

All saves are stored in **UDATA** — the Xbox Dashboard's user data region. This means saves appear in the dashboard memory manager and can be managed (copied to memory unit, deleted) without launching the game.

| Dashboard entry | Contents |
|---|---|
| `Zelda ALTTP - Game Save` | SRAM (main game progress) + config |
| `Zelda ALTTP - State 1` | Save state slot 1 |
| `Zelda ALTTP - State 2` | Save state slot 2 |
| ... | ... |
| `Zelda ALTTP - State 10` | Save state slot 10 |

Save state folders are only created when you first save to that slot — unused slots create no folders.

To identify folders via FTP, each save container includes an `info.txt` file with the save name and directory path.

---

## Controls

| Xbox Button | SNES Button | Action |
|---|---|---|
| A | B | Sword / action |
| B | A | Run / dash |
| X | Y | Item slot 1 |
| Y | X | Item slot 2 (with Switch L/R enabled) |
| Left Trigger | L | Item rotate left / L item slot |
| Right Trigger | R | Item rotate right / R item slot |
| D-Pad | D-Pad | Movement |
| Start | Start | Pause / map |
| Back | Select | Map / select |
| L3 (left stick click) | — | Open Xbox menu |

### Xbox Menu (L3)

| Menu item | Description |
|---|---|
| CHEATS | Consumables, equipment, items, god mode |
| SETTINGS | Graphics and gameplay feature toggles |
| SAVE STATE | Save to one of 10 slots |
| LOAD STATE | Load from a slot (shows error if slot is empty) |
| TURBO MODE | Toggle 4x speed |
| PAUSE GAME | Pause with dim overlay |
| RESET GAME | Reset to title screen |
| EXIT GAME | Exit to Xbox dashboard |
| CLOSE | Close menu and resume |

---

## Project Structure

```
src/
    platform/
        xbox/               ← All Xbox-specific code (never modify base game files)
            xbox_main.cpp       Entry point, game loop, drive mounting
            xbox_d3d.cpp        Direct3D 8 renderer
            xbox_audio.cpp      DirectSound audio thread
            xbox_input.cpp      XInput controller handling
            xbox_menu.cpp       In-game menu, cheats, settings, save/load
            xbox_menu.h         Menu declarations
            xbox_savegame.cpp   UDATA save path resolution
            xbox_platform.h     Master Xbox header
            xbox_fixes.h        Compiler fixes
            config_xbox.c       Config/INI save and load
            alloca_probe.cpp    XDK linker stubs
            zelda_rtl_xbox.c    Game loop, asset loading, god mode
            xdk_crt_shim.h      CRT shims + fopen redirector
            SDL.h               SDL stub (not used, satisfies includes)
            SDL_keycode.h       SDL keycode stub
            stdbool.h           C99 bool for MSVC
            stdint.h            C99 fixed-width integers for MSVC
            intrin.h            Intrinsics stub
    Media/                  ← Place zelda3_assets.dat here before building
        Copy Assets Here.txt
    zelda3_xbox.vcxproj     Visual Studio project file
    zelda3_xbox.vcxproj.filters  Solution Explorer filters
```

The base game source files (`ancilla.c`, `dungeon.c`, `player.c`, etc.) are **never modified**. All Xbox-specific code lives entirely in `platform/xbox/`.

---

## License

The zelda3 reimplementation is licensed under the MIT license. See `LICENSE.txt` for details.

The Xbox port code in `src/platform/xbox/` follows the same license.
