# Phase 5: Built-in Game Maker - Completion Documentation

## Overview

Phase 5 has been successfully completed, implementing a comprehensive built-in Game Maker for SNESE. This tool provides runtime ROM editing capabilities through an interactive command-line interface.

**Completion Date:** November 2025  
**Status:** ✅ Complete

## Implemented Features

### 1. Tile Editor

The Tile Editor allows direct manipulation of tile graphics stored in VRAM.

**Features:**
- Visual tile display (8x8 pixels, 2bpp format)
- ASCII art representation with 4 shades
- Navigate between tiles (next/prev/goto)
- Pixel-by-pixel editing
- Palette selection (0-7)
- Zoom controls (1x-4x)
- View tile data in hexadecimal
- Export tiles to binary files
- Import tiles from binary files

**Usage Example:**
```
1. Enter Tile Editor from main menu
2. Press 'g' to go to a specific tile number
3. Press 'e' to edit a pixel (specify X, Y, color)
4. Press 'c' to change palette
5. Press 's' to save changes to VRAM
6. Press 'x' to export tile data
```

**Technical Details:**
- Tiles are stored in VRAM at address = tile_number * 16
- 2bpp format: 2 bytes per scanline, 16 bytes per tile
- Each pixel is 2 bits (4 colors per tile)
- Bit manipulation for precise pixel editing

### 2. Sprite Editor

The Sprite Editor provides control over Object Attribute Memory (OAM) for sprite management.

**Features:**
- Navigate between 128 sprites
- Modify sprite position (X, Y coordinates)
- Set tile number (0-255)
- Select palette (0-7)
- Toggle horizontal flip
- Toggle vertical flip
- Save to OAM
- List all sprites with properties

**Usage Example:**
```
1. Enter Sprite Editor from main menu
2. Press 'g' to select sprite number
3. Press 'x' or 'y' to set position
4. Press 't' to set tile number
5. Press 'h' or 'v' to toggle flipping
6. Press 's' to save to OAM
7. Press 'l' to list all sprites
```

**OAM Format:**
- 4 bytes per sprite: X, Y, Tile, Attributes
- Attributes byte (VHOOPPPT):
  - V: Vertical flip
  - H: Horizontal flip
  - OO: Priority
  - PPP: Palette (3 bits)
  - T: Tile page

### 3. Tilemap Editor

The Tilemap Editor enables editing of background layer tilemaps.

**Features:**
- WASD cursor navigation
- Place tiles at cursor position
- Select tile and palette
- Switch between BG layers (1-4)
- Paint mode for continuous editing
- View 8x8 tilemap area around cursor
- Real-time position display

**Usage Example:**
```
1. Enter Tilemap Editor from main menu
2. Use WASD to move cursor
3. Press 't' to select tile number
4. Press 'c' to change palette
5. Press 'l' to switch layer (BG1-4)
6. Press 'p' to place tile
7. Press 'm' to enable paint mode
8. Press 'v' to view tilemap area
```

**Tilemap Format:**
- 32x32 tiles per layer
- 2 bytes per tile entry (VHPP CCCC TTTT TTTT)
  - V: Vertical flip
  - H: Horizontal flip
  - PP: Priority
  - CCCC: Palette
  - TTTT TTTT TTTT: Tile number (10 bits)

### 4. Palette Editor

The Palette Editor allows color palette manipulation in CGRAM.

**Features:**
- Navigate colors within palettes (16 colors per palette)
- Navigate between palettes (0-15)
- Edit RGB values individually (0-31 each)
- Set colors via hexadecimal (15-bit BGR555)
- Save to CGRAM (Color RAM)
- View full palette display
- Export palette to file (32 bytes)
- Import palette from file

**Usage Example:**
```
1. Enter Palette Editor from main menu
2. Press 'P' or 'O' to change palette
3. Press 'n' or 'p' to change color
4. Press 'r', 'g', or 'b' to set RGB values
5. Press 'h' to set hex value directly
6. Press 's' to save to CGRAM
7. Press 'v' to view all colors in palette
8. Press 'e' or 'i' to export/import
```

**Color Format (BGR555):**
- 15-bit color value
- 5 bits per channel (0-31)
- Format: 0BBB BBGG GGGR RRRR
- Red: bits 0-4
- Green: bits 5-9
- Blue: bits 10-14

### 5. Script Editor

The Script Editor provides a simple scripting language for batch ROM modifications.

**Features:**
- Command-based scripting language
- Execute commands individually or in batch
- Load/save script files
- Command buffer (256 commands max)
- Syntax help system
- Error reporting

**Supported Commands:**

#### SET Command
```
SET <bank>:<addr> <value>
```
Sets a single byte at the specified ROM address.

Example: `SET 00:8000 FF`

#### FILL Command
```
FILL <bank>:<addr> <value> <length>
```
Fills a range of memory with a value.

Example: `FILL 00:8000 00 100`

#### COPY Command
```
COPY <src_bank>:<src_addr> <dst_bank>:<dst_addr> <length>
```
Copies data from source to destination.

Example: `COPY 00:8000 01:9000 256`

**Script File Format:**
```
# SNESE Game Maker Script
# Comments start with #

SET 00:8000 FF
FILL 00:8100 00 256
COPY 00:8000 01:9000 256
```

**Usage Example:**
```
1. Enter Script Editor from main menu
2. Press 'c' to enter a command
3. Type command (e.g., "SET 00:8000 FF")
4. Press 'e' to execute current command
5. Press 'r' to run all commands
6. Press 's' to save script to file
7. Press 'o' to load script from file
8. Press 'h' for syntax help
```

## Integration with Emulator

The Game Maker integrates seamlessly with the SNES emulator:

- **Memory Access:** Direct access to VRAM, OAM, and CGRAM
- **ROM Modification:** In-memory patching of cartridge ROM data
- **State Management:** Tracks unsaved changes
- **Save System:** Export modified ROM to new file

## Memory Layout

### VRAM (Video RAM) - 64KB
- Tiles: $0000-$7FFF
- 2bpp tiles: 16 bytes each (512 tiles in 8KB)
- 4bpp tiles: 32 bytes each (256 tiles in 8KB)
- Tilemaps: Usually at $0000-$07FF per layer

### OAM (Object Attribute Memory) - 544 bytes
- Main OAM: 512 bytes (128 sprites × 4 bytes)
- High table: 32 bytes (size and X MSB)

### CGRAM (Color RAM) - 512 bytes
- 256 colors total
- 16 palettes of 16 colors
- 2 bytes per color (BGR555 format)

## File Operations

### Supported File Operations
- **ROM Save:** Export modified ROM data
- **Tile Export/Import:** Binary tile data (16 bytes)
- **Palette Export/Import:** Binary palette data (32 bytes)
- **Script Load/Save:** Text-based script files

### File Formats

**Tile File (.til):**
- Raw binary format
- 16 bytes for 2bpp tile
- 32 bytes for 4bpp tile

**Palette File (.pal):**
- Raw binary format
- 32 bytes (16 colors × 2 bytes)
- BGR555 little-endian format

**Script File (.scr):**
- Plain text format
- One command per line
- Comments start with #

## Usage Workflow

### Creating a Simple ROM Modification

1. **Launch Game Maker:**
   ```bash
   ./snesemu --maker rom.sfc
   ```

2. **Edit Tiles:**
   - Select Tile Editor
   - Navigate to desired tile
   - Edit pixels
   - Save changes

3. **Modify Sprites:**
   - Select Sprite Editor
   - Set sprite positions
   - Assign tiles
   - Save to OAM

4. **Edit Tilemaps:**
   - Select Tilemap Editor
   - Choose layer
   - Place tiles
   - Create level layout

5. **Adjust Colors:**
   - Select Palette Editor
   - Modify colors
   - Save to CGRAM

6. **Apply Scripts:**
   - Select Script Editor
   - Enter batch commands
   - Execute script

7. **Save ROM:**
   - Return to main menu
   - Select "Save ROM"
   - Enter output filename

## Technical Implementation

### Memory Management
- Direct pointers to emulator memory structures
- No data copying for efficiency
- Immediate reflection of changes
- Unsaved changes tracking

### Data Structures
```c
typedef struct {
    TileEditor tile_editor;
    SpriteEditor sprite_editor;
    TilemapEditor tilemap_editor;
    PaletteEditor palette_editor;
    ScriptEditor script_editor;
    Memory *mem;           // Direct memory access
    Cartridge *cart;       // ROM data access
    bool unsaved_changes;  // Modification tracking
} GameMaker;
```

### Command Processing
- String parsing with sscanf
- Address validation
- Bank:offset addressing
- Bounds checking
- Error reporting

## Educational Value

### Learning Objectives Achieved
- Understanding SNES memory architecture
- Binary data manipulation
- Tile-based graphics systems
- Sprite management
- Color palette systems
- ROM structure and patching
- Assembly-like scripting
- File I/O operations

### Skills Developed
- Low-level memory access
- Bit manipulation
- Data format understanding
- Interactive UI design
- Command parsing
- Error handling

## Limitations and Future Enhancements

### Current Limitations
- 2bpp tile editing only (4bpp could be added)
- Text-based interface (no graphical display)
- Manual address calculation
- Basic scripting language
- No undo/redo functionality

### Potential Enhancements
- Graphical tile preview
- 4bpp and 8bpp support
- Visual tilemap editor
- Advanced scripting (variables, loops)
- Undo/redo system
- Copy/paste operations
- Search and replace
- Checksum recalculation
- Hot-reload into emulator

## Testing

### Test Scenarios
1. ✅ Load valid SNES ROM
2. ✅ Edit tile graphics
3. ✅ Modify sprite positions
4. ✅ Place tiles in tilemap
5. ✅ Change color palettes
6. ✅ Execute script commands
7. ✅ Save modified ROM
8. ✅ Import/export data files
9. ✅ Handle invalid inputs
10. ✅ Track unsaved changes

### Validation
- Memory bounds checking
- Address validation
- Value range checking
- File existence verification
- Buffer overflow prevention

## Examples

### Example 1: Simple Tile Edit
```
1. Load ROM
2. Tile Editor → Go to tile 5
3. Edit pixel (2, 3) → color 3
4. Save tile
5. Save ROM
```

### Example 2: Sprite Setup
```
1. Load ROM
2. Sprite Editor → Select sprite 0
3. Set position (128, 112)
4. Set tile 10
5. Set palette 2
6. Save to OAM
7. Save ROM
```

### Example 3: Batch Scripting
```
1. Load ROM
2. Script Editor
3. Enter commands:
   SET 00:8000 FF
   FILL 00:8100 00 256
   COPY 00:8000 01:9000 256
4. Run all commands
5. Save ROM
```

## Conclusion

Phase 5 successfully implements a comprehensive ROM editing toolkit with:
- ✅ 4 specialized editors (Tile, Sprite, Tilemap, Palette)
- ✅ Scripting system with 3 commands
- ✅ File import/export capabilities
- ✅ Direct memory manipulation
- ✅ Change tracking and ROM export
- ✅ Educational documentation

The Game Maker provides a solid foundation for creating and modifying SNES ROMs while teaching important concepts about retro game development and hardware architecture.

**Status:** Phase 5 Complete - Ready for Phase 6 (Optimization, Testing, and Polish)
