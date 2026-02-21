# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This is a **Commodore 64 game/application** called "settlers" being developed in C++. The project is a grid-based, interactive world where you can control multiple NPCs (settlers) with smooth sub-pixel movement across the 40x25 character screen.

## Build & Compilation

**Compiler**: LLVM-MOS (mos-c64-clang)
- Available at: `/home/apohorai/llvm-mos/bin/mos-c64-clang`
- Compiles C++ to 6502 assembly for the Commodore 64

**Build command**:
```bash
/home/apohorai/llvm-mos/bin/mos-c64-clang -O3 -o main.prg main.cc sprite.cc
```

**Output format**: `.prg` file (Commodore 64 executable, typically 8-10KB)

## Architecture & Key Components

### Core Game Loop (`main.cc`)
- `main()`: Infinite loop structure calling VSO-sync, input handling, NPC updates, and rendering
- Fixed 60 FPS sync using VIC-II VSO-sync (`wait_vsync()`)
- Supports up to 10 NPCs (settlers) in array-based management

### NPC System (Settler Structure)
```c
typedef struct {
    uint8_t x, y;           // Tile position (0-39, 0-24)
    uint8_t off_x, off_y;   // Sub-pixel offset (0-7)
    uint8_t old_x, old_y;   // Previous position for cleanup
    uint8_t char_index;     // Character from charset
    uint8_t color;          // C64 color (0-15)
    int8_t dir_x, dir_y;    // Movement direction
    uint8_t steps_remaining;// Animation progress (8 steps per tile)
    uint8_t npc_id;         // ID for temp char lookup
} Settler;
```

Key behaviors:
- Movement is animated over 8 cycles per tile
- Sub-pixel offsets allow smooth animation between tiles
- Each NPC occupies 1-2 temporary character slots for rendering

### Temporary Character Allocator
- **Problem solved**: NPCs need dynamic character space to render with sub-pixel movement across tile boundaries
- **Solution**: Dynamic pool of 128 temporary characters (indices 128-255)
- Uses bitmap tracking (`temp_used[16]`) to efficiently manage allocation
- Each NPC gets 2 temp chars during movement (one for current tile, one for next)
- Functions: `allocate_temp()`, `release_temp()`, `allocate_npc_temps()`, `release_npc_temps()`

### Rendering Engine
- **Character-based grid**: C64 standard 40x25 character display
- **Sub-pixel rendering**: Merges NPC sprite data with background characters using bitwise operations
- `prepare_temp()`: Copies background character to temp slot before overlay
- `draw_settler()`: Composites NPC sprite onto temp characters with X/Y offset handling
- Supports movement in 4 directions (up/down/left/right)

### Input System
- **NPC Selection**: Number keys (1-9, 0) select which settler to control (0=NPC 10)
- **Movement**: Arrow keys move selected NPC in 4 directions
- `handle_input()`: Reads C64 keyboard matrix and initiates movement
- `read_number_key()`: Scans keyboard matrix for number key presses

### Hardware Integration
- **SCREEN_RAM** (0x0400): Text display memory (40x25)
- **COLOR_RAM** (0xD800): Color memory for each character
- **CHARSET_DEST** (0x3000): Custom character set location
- **VIC-II registers** (0xD000-0xD0FF): Screen control registers
- Handles direct memory access for real-time graphics updates

## Asset Files

- **charset_data.h**: Custom character set bitmap (2048 bytes, includes settler sprite)
- **map_data.h**: Static background map data (40x25 = 1000 bytes)
- **sprite.h/.cc**: VIC-II sprite register management (currently minimal usage)

## Current Development Status

- **Branch**: `refactor` - Currently refactoring movement and input system
- **Latest changes**: Dynamic temp allocation system, NPC array management
- **Next steps**: (Based on commit messages) Improving character rendering and movement mechanics

## Code Style Notes

- Heavy use of bit manipulation and pointer arithmetic for C64 memory constraints
- Direct hardware register access via volatile pointers
- All coordinates and calculations fit in 8-bit or 16-bit integers
- Functions include detailed comments explaining VIC-II register usage and memory layout

## Common Development Tasks

### Modify Movement Speed
Change `steps_remaining = 8` in `start_move()` or `update_settler()`. Higher values = slower movement.

### Add/Remove NPCs
Modify `NUM_NPCS` constant in `main.cc` and allocate/deallocate temp chars accordingly.

### Change Character Set
Replace charset_data.h content (2048 bytes) and update `settlers_charset` reference in `init_system()`.

### Debug Information
- Display helper: `drawInt(value, width, x, y, color)` draws integers at screen coordinates
- Currently shows selected NPC number and first two NPCs' coordinates
