# SNESE Game Maker - User Guide

## Table of Contents

1. [Introduction](#introduction)
2. [Getting Started](#getting-started)
3. [Main Menu](#main-menu)
4. [Tile Editor](#tile-editor)
5. [Sprite Editor](#sprite-editor)
6. [Tilemap Editor](#tilemap-editor)
7. [Palette Editor](#palette-editor)
8. [Script Editor](#script-editor)
9. [Saving Your Work](#saving-your-work)
10. [Tips and Tricks](#tips-and-tricks)
11. [Troubleshooting](#troubleshooting)

## Introduction

The SNESE Game Maker is an integrated ROM editing tool that allows you to modify SNES game assets including:
- Graphics tiles
- Sprite positions and properties
- Background tilemaps
- Color palettes
- ROM data via scripting

This tool is designed for educational purposes and to help you understand how SNES games are structured.

## Getting Started

### Prerequisites
- A valid SNES ROM file (.sfc or .smc format)
- Basic understanding of hexadecimal numbers
- Familiarity with tile-based graphics

### Launching Game Maker

To enter Game Maker mode:
```bash
./snesemu --maker your_rom.sfc
```

You will see the main menu with several editing options.

## Main Menu

The main menu provides access to all Game Maker features:

```
Main Menu:
  1. Tile Editor       - Edit tile graphics
  2. Sprite Editor     - Edit sprite properties
  3. Tilemap Editor    - Edit background tilemaps
  4. Palette Editor    - Edit color palettes
  5. Script Editor     - ROM patching scripts
  6. ROM Information   - View ROM details
  7. Save ROM          - Save changes to file
  8. Exit              - Return to emulator
```

### Navigation
- Enter a number (1-8) to select an option
- Press Enter to confirm

## Tile Editor

The Tile Editor lets you view and modify individual tile graphics.

### What are Tiles?
Tiles are 8x8 pixel graphics used to build game visuals. The SNES supports:
- **2bpp tiles**: 4 colors per tile (16 bytes)
- **4bpp tiles**: 16 colors per tile (32 bytes)

The current implementation focuses on 2bpp tiles.

### Commands

**Navigation:**
- `n` - Next tile
- `p` - Previous tile
- `g` - Go to specific tile number (0-1023)

**Editing:**
- `e` - Edit pixel (specify X: 0-7, Y: 0-7, Color: 0-3)
- `c` - Change palette (0-7)
- `z` - Change zoom level (1x-4x)

**Data Operations:**
- `v` - View tile data in hexadecimal
- `s` - Save tile to VRAM
- `x` - Export tile to file
- `i` - Import tile from file

**Exit:**
- `q` - Return to main menu

### Visual Display

Tiles are displayed using ASCII art:
- `  ` - Transparent (color 0)
- `░░` - Color 1
- `▒▒` - Color 2
- `██` - Color 3

### Example Workflow

1. Launch Tile Editor
2. Press `g` and enter `42` to go to tile 42
3. Press `e` to edit a pixel
   - Enter X: `3`
   - Enter Y: `4`
   - Enter color: `2`
4. Press `s` to save
5. Press `q` to exit

## Sprite Editor

The Sprite Editor manages sprite objects (OAM entries).

### What are Sprites?
Sprites are movable graphics objects. The SNES supports:
- 128 sprites maximum
- Position control (X, Y)
- Tile assignment
- Palette selection
- Flipping (horizontal/vertical)

### Commands

**Navigation:**
- `n` - Next sprite
- `p` - Previous sprite
- `g` - Go to specific sprite (0-127)

**Properties:**
- `x` - Set X position (0-255)
- `y` - Set Y position (0-255)
- `t` - Set tile number (0-255)
- `c` - Change palette (0-7)

**Transformations:**
- `h` - Toggle horizontal flip
- `v` - Toggle vertical flip

**Operations:**
- `s` - Save sprite to OAM
- `l` - List all sprites
- `q` - Return to main menu

### Example Workflow

1. Launch Sprite Editor
2. Press `g` and enter `0` to edit sprite 0
3. Press `x` and enter `100` for X position
4. Press `y` and enter `80` for Y position
5. Press `t` and enter `10` for tile number
6. Press `c` and enter `1` for palette
7. Press `h` to flip horizontally
8. Press `s` to save to OAM
9. Press `q` to exit

## Tilemap Editor

The Tilemap Editor allows you to place tiles to create backgrounds.

### What are Tilemaps?
Tilemaps define which tiles appear at each position on screen:
- 4 layers (BG1, BG2, BG3, BG4)
- 32×32 tiles per layer
- Each entry specifies tile number, palette, and flip

### Commands

**Navigation:**
- `w` - Move cursor up
- `s` - Move cursor down
- `a` - Move cursor left
- `d` - Move cursor right

**Editing:**
- `p` - Place tile at cursor
- `t` - Select tile to place (0-1023)
- `c` - Change palette (0-7)
- `l` - Change layer (1-4 for BG1-4)

**Modes:**
- `m` - Toggle paint mode (auto-advance cursor)

**View:**
- `v` - View 8×8 tilemap area around cursor
- `q` - Return to main menu

### Example Workflow

1. Launch Tilemap Editor
2. Press `l` and enter `1` to select BG1
3. Press `t` and enter `5` to select tile 5
4. Press `c` and enter `0` to select palette 0
5. Use `w`, `a`, `s`, `d` to move cursor
6. Press `p` to place tile
7. Press `m` to enable paint mode
8. Use `d` repeatedly to paint a row
9. Press `q` to exit

## Palette Editor

The Palette Editor lets you modify color palettes.

### What are Palettes?
The SNES uses palettes to define colors:
- 16 palettes total (0-15)
- 16 colors per palette
- 15-bit BGR555 format (5 bits per channel)
- Values: 0-31 per channel

### Commands

**Navigation:**
- `n` - Next color in palette
- `p` - Previous color in palette
- `P` - Next palette
- `O` - Previous palette

**Editing:**
- `r` - Set red value (0-31)
- `g` - Set green value (0-31)
- `b` - Set blue value (0-31)
- `h` - Set hexadecimal value (0-7FFF)

**Operations:**
- `s` - Save color to CGRAM
- `v` - View all colors in current palette
- `e` - Export palette to file
- `i` - Import palette from file
- `q` - Return to main menu

### Color Format

BGR555 format: `0BBB BBGG GGGR RRRR`
- Red: bits 0-4
- Green: bits 5-9
- Blue: bits 10-14

Example: `$7FFF` = White (31, 31, 31)

### Example Workflow

1. Launch Palette Editor
2. Press `P` to select palette (default 0)
3. Press `n` to navigate to color
4. Press `r` and enter `31` for maximum red
5. Press `g` and enter `15` for mid green
6. Press `b` and enter `0` for no blue
7. Press `s` to save color
8. Press `v` to view entire palette
9. Press `q` to exit

## Script Editor

The Script Editor provides batch ROM modification capabilities.

### What is Scripting?
Scripts are sequences of commands that modify ROM data:
- SET: Change single byte
- FILL: Fill range with value
- COPY: Copy data between addresses

### Commands

**Script Management:**
- `c` - Enter new command
- `l` - List all commands
- `d` - Delete command by index
- `x` - Clear all commands

**Execution:**
- `e` - Execute current command
- `r` - Run all commands

**File Operations:**
- `s` - Save script to file
- `o` - Load script from file

**Help:**
- `h` - Show syntax help
- `q` - Return to main menu

### Script Syntax

#### SET Command
```
SET <bank>:<addr> <value>
```
Set a single byte at ROM address.

Example: `SET 00:8000 FF`
- Sets byte at bank $00, offset $8000 to $FF

#### FILL Command
```
FILL <bank>:<addr> <value> <length>
```
Fill a range with a value.

Example: `FILL 00:8100 00 100`
- Fills 256 bytes starting at $00:8100 with $00

#### COPY Command
```
COPY <src_bank>:<src_addr> <dst_bank>:<dst_addr> <length>
```
Copy data between addresses.

Example: `COPY 00:8000 01:9000 100`
- Copies 256 bytes from $00:8000 to $01:9000

### Script File Format

Save scripts as text files:
```
# This is a comment
# Comments start with #

SET 00:8000 FF
FILL 00:8100 00 100
COPY 00:8000 01:9000 256
```

### Example Workflow

1. Launch Script Editor
2. Press `c` to enter command
3. Type: `SET 00:8000 FF`
4. Press `c` again for another command
5. Type: `FILL 00:8100 00 256`
6. Press `r` to run all commands
7. Press `s` to save script
8. Enter filename: `mods.scr`
9. Press `q` to exit

## Saving Your Work

### Save ROM
To save all modifications:
1. Return to main menu
2. Select option 7 (Save ROM)
3. Enter output filename (e.g., `modified.sfc`)

The saved ROM includes:
- Modified tiles in ROM
- Changed sprites in ROM
- Updated tilemaps
- Modified palettes
- All script changes

### Export Individual Assets

**Tiles:**
- Use Tile Editor → Export (`x`)
- Saves 16 bytes to file
- Load with Import (`i`)

**Palettes:**
- Use Palette Editor → Export (`e`)
- Saves 32 bytes to file
- Load with Import (`i`)

**Scripts:**
- Use Script Editor → Save (`s`)
- Saves text file
- Load with Load (`o`)

## Tips and Tricks

### General Tips
1. **Save Often**: ROM modifications are not saved until you explicitly save
2. **Use Scripts**: For repetitive tasks, scripts are much faster
3. **Backup Original**: Always keep a copy of the original ROM
4. **Track Changes**: The status bar shows if you have unsaved changes

### Tile Editing
- Start with simple shapes before complex graphics
- Use different zoom levels to see details
- Export tiles you like for reuse
- Remember color 0 is usually transparent

### Sprite Management
- List all sprites (`l`) to see what's already in use
- Standard positions: X=0-255, Y=0-239 (on screen)
- Use negative Y values for sprites above screen
- Priority controls which sprites appear in front

### Tilemap Design
- Plan your layout before placing tiles
- Use paint mode for faster horizontal rows
- Different layers can create parallax effects
- Layer 1 usually has highest priority

### Color Selection
- View full palette to maintain consistency
- RGB values 0-31 can be subtle differences
- Export palettes to reuse in other projects
- Test colors in actual game to see results

### Scripting
- Write scripts in external editor for complex operations
- Use comments to document what each command does
- Test commands individually before running all
- Scripts can automate tedious modifications

## Troubleshooting

### Common Issues

**Problem: Changes don't appear**
- Solution: Make sure you saved (press `s`)
- Solution: Check if you're editing the right tile/sprite number
- Solution: Verify you saved the ROM (option 7)

**Problem: Invalid address error**
- Solution: Check address is within ROM size
- Solution: Verify bank:offset format (hex values)
- Solution: Ensure destination has enough space

**Problem: Colors look wrong**
- Solution: Remember BGR format, not RGB
- Solution: Check you saved to CGRAM (press `s`)
- Solution: Verify palette number is correct

**Problem: Sprites not visible**
- Solution: Check Y position (must be < 240 for visibility)
- Solution: Verify tile number exists in VRAM
- Solution: Ensure sprite is within 128 sprite limit

**Problem: Script command fails**
- Solution: Check syntax with help (`h`)
- Solution: Verify addresses are in hex
- Solution: Ensure ROM has space for operation

### Getting Help

1. Use the `h` command in Script Editor for syntax
2. Check the status message after operations
3. Consult PHASE_5_COMPLETION.md for technical details
4. Refer to SNES development documentation

## Keyboard Reference

### Quick Reference Card

**Tile Editor:**
```
n/p    - Next/Previous tile
g      - Go to tile
e      - Edit pixel
c      - Change palette
z      - Zoom
v      - View hex
s      - Save
x/i    - Export/Import
q      - Quit
```

**Sprite Editor:**
```
n/p    - Next/Previous sprite
g      - Go to sprite
x/y    - Position
t      - Tile number
c      - Palette
h/v    - Flip H/V
s      - Save
l      - List all
q      - Quit
```

**Tilemap Editor:**
```
w/a/s/d - Move cursor
p       - Place tile
t       - Select tile
c       - Palette
l       - Layer
m       - Paint mode
v       - View area
q       - Quit
```

**Palette Editor:**
```
n/p    - Next/Prev color
P/O    - Next/Prev palette
r/g/b  - Set RGB
h      - Set hex
s      - Save
v      - View all
e/i    - Export/Import
q      - Quit
```

**Script Editor:**
```
c      - Enter command
e      - Execute current
r      - Run all
l      - List
d      - Delete
x      - Clear all
s/o    - Save/Load
h      - Help
q      - Quit
```

## Examples

### Example 1: Create a Simple Pattern

**Objective:** Create a checkerboard pattern in tiles

1. Enter Tile Editor
2. Go to tile 1
3. Edit pixels to create a solid square (all color 3)
4. Save tile
5. Go to tile 2
6. Leave empty (all color 0)
7. Enter Tilemap Editor
8. Select tile 1, place at (0,0)
9. Select tile 2, place at (1,0)
10. Repeat alternating pattern
11. Save ROM

### Example 2: Position Sprites for a Menu

**Objective:** Create a sprite-based menu cursor

1. Enter Sprite Editor
2. Select sprite 0
3. Set position (16, 80)
4. Set tile to arrow tile number
5. Set palette 7 (for highlighting)
6. Save sprite
7. Repeat for multiple menu items (Y+16 each)
8. Save ROM

### Example 3: Batch Color Change

**Objective:** Change all colors in a palette

1. Enter Palette Editor
2. Select palette 0
3. Export current palette (backup)
4. Edit each color (0-15)
5. Save after each color
6. View full palette to verify
7. Save ROM

### Example 4: ROM Data Modification

**Objective:** Change title screen data

1. Enter Script Editor
2. Add command: `SET 00:FFC0 48` (Change title byte)
3. Add command: `SET 00:FFC1 45`
4. Add command: `SET 00:FFC2 4C`
5. Add command: `SET 00:FFC3 4C`
6. Add command: `SET 00:FFC4 4F`
7. Run all commands
8. Save ROM

## Conclusion

The SNESE Game Maker provides powerful tools for ROM editing and modification. With practice, you can create custom graphics, layouts, and modifications to SNES ROMs.

Remember:
- Experiment and learn
- Save backups frequently
- Test changes in the emulator
- Share your creations

Happy ROM hacking!
