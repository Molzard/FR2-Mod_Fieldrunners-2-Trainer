# FR2 Mod - Fieldrunners 2 Trainer

A modding tool for Fieldrunners 2 that injects a custom DLL to provide in-game overlay with various cheats.

## Features

- **In-Game Overlay** - OpenGL-based menu rendered directly on top of the game
- **Add Main Menu Coins** - Add coins to the main menu wallet
- **Add In-Game Coins** - Add money during gameplay
- **Freeze Player Health** - Lock player health to max value
- **Damage Multiplier** - Custom multipliers (OFF, 2x, 5x, 10x, 100x) via native damage hook
- **Wave Controls** - Skip Wave and Instant Victory trigger via `WaveManager`
- **Auto Upgrade** - Instantly max-upgrades (Level 3) newly placed or existing towers for free
- **Illegal Grid Placement** - Bypass tile checks and pathfinding to build anywhere on the map
- **Bullet Hell 360°** - Radial Danmaku barrage damaging all active creeps on the field
- **Real-time Game State** - Displays current wave, kills, coins, and HP
- **Expandable Menu** - Parent-child menu hierarchy with expand/collapse
- **Persona 5 Themed UI** - Stylized red-black overlay design

## Project Structure

```text
FR2_Mod/
├── src/
│   ├── Core/
│   │   └── Logger/
│   │       ├── Logger.h
│   │       └── Logger.cpp
│   ├── SDK/
│   │   ├── GameState.h
│   │   └── GameState.cpp
│   ├── Features/
│   │   ├── Coin.h / Coin.cpp
│   │   ├── Health.h / Health.cpp
│   │   ├── Damage.h / Damage.cpp
│   │   ├── Wave.h / Wave.cpp
│   │   ├── AutoUpgrade.h / AutoUpgrade.cpp
│   │   ├── IllegalPlacement.h / IllegalPlacement.cpp
│   │   └── BulletHell.h / BulletHell.cpp
│   ├── Memory/
│   │   ├── MemoryManager.h
│   │   └── MemoryManager.cpp
│   └── UI/
│       ├── MenuManager.h
│       └── MenuManager.cpp
├── hooks.asm
├── dllmain.cpp
├── FR2_Mod.vcxproj
└── main.py
```

## How It Works

### DLL Injection
The Python injector (`main.py` or `FR2_Injector.exe`) finds the game process and injects `FR2_Mod.dll` using `CreateRemoteThread` + `LoadLibraryA`.

### Hooking
The DLL hooks into NVIDIA's OpenGL driver (`nvoglv32.dll`) via MinHook, intercepting `DrvSwapLayerBuffers` to render the overlay every frame.

### Overlay Rendering
The menu is rendered using OpenGL immediate mode (`glBegin`/`glEnd`) with bitmap fonts created via `wglUseFontBitmaps`.

### Memory Manipulation
Game values are modified directly through pointers resolved at runtime using the module base address of `Fieldrunners2.dll`.

## Requirements

### For Building the DLL
- Visual Studio 2022 (with C++ Desktop Development)
- Windows SDK 10.0+
- MinHook library (included as NuGet package or static lib)
- Build as **Win32 (32-bit)** - the game is 32-bit!

### For Running the Injector
- Python 3.12+ **32-bit** (for the Python version)
- OR use the pre-built `FR2_Injector.exe` (no Python needed)

### For Playing
- Fieldrunners 2 (any version, tested on Steam release)
- Windows 10/11 64-bit (32-bit game runs via WoW64)
- NVIDIA GPU (the hook targets `nvoglv32.dll`)

## Usage

### Method 1: Pre-built EXE (Recommended)
1. Copy `FR2_Injector.exe` and `FR2_Mod.dll` to the game folder
2. Run `FR2_Injector.exe` as **Administrator**
3. Launch the game
4. Click **"INJECT DLL"** when the loading screen appears
5. The overlay menu will appear in-game

### Method 2: Python Script
```batch
C:\Python312-32\python.exe main.py
```

## Menu Controls

| Key | Action |
|:----|:-------|
| I   | Navigate Up |
| K   | Navigate Down |
| J   | Select / Execute |
| L   | Hide / Show Menu |

## Building from Source

### DLL (FR2_Mod.dll)
1. Open FR2_Mod.vcxproj in Visual Studio 2022
2. Set configuration to Debug | Win32
3. Build solution (Ctrl+Shift+B)
4. Output: Debug\FR2_Mod.dll

### Injector (FR2_Injector.exe)
```batch
# Using Python 32-bit
pip install pyinstaller
pyinstaller --onefile --windowed --name="FR2_Injector" main.py
```

## Notes

* The DLL must be 32-bit (x86) to match the game architecture
* Run everything as Administrator for proper memory access
* The game uses OpenGL (not DirectX), hence the NVIDIA driver hook
* This tool is for educational purposes only

## License

This project is for personal use. Feel free to modify and learn from it.

## Credits
* MinHook by TsudaKageyu
* Fieldrunners 2 by Subatomic Studios