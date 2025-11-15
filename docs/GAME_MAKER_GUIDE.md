# SNESE Game Maker User Guide

## Overview

The SNESE Game Maker is a built-in ROM editing tool that allows you to modify SNES games at runtime. It provides editors for tiles, sprites, palettes, tilemaps, and a scripting language for advanced ROM modifications.

## Launching Game Maker

To launch the Game Maker mode:

```bash
./snesemu --maker rom_file.sfc
```

## Main Menu

The Game Maker main menu provides access to all editing tools:

1. **Tile Editor** - Edit tile graphics pixel by pixel
2. **Sprite Editor** - Modify sprite properties (position, tile, palette, flips)
3. **Tilemap Editor** - Edit background layer tilemaps
4. **Palette Editor** - Edit color palettes with RGB controls
5. **Script Editor** - Execute ROM modification scripts
6. **ROM Information** - View detailed ROM information
7. **Save ROM** - Export modified ROM to a new file
8. **Exit** - Return to emulator or exit

## Tile Editor

The Tile Editor allows you to edit individual tiles pixel by pixel.

### Commands

- `l <num>` - Load tile number (0-1023)
- `s` - Save tile to ROM
- `v <addr>` - View tile at VRAM address (hex)
- `p <pal>` - Change palette (0-7)
- `e <x> <y> <color>` - Edit pixel at (x,y) with color index
- `d` - Display tile data in hex
- `b` - Return to main menu

### Example Session

```
l 100        # Load tile 100
p 2          # Use palette 2
e 0 0 3      # Set pixel at (0,0) to color 3
e 1 0 3      # Set pixel at (1,0) to color 3
d            # Display hex data
s            # Save changes
```

### Tile Format

SNES tiles are 8x8 pixels stored in planar format:
- 2bpp (4 colors): 16 bytes per tile
- 4bpp (16 colors): 32 bytes per tile
- 8bpp (256 colors): 64 bytes per tile

Currently the editor supports 2bpp format.

## Sprite Editor

The Sprite Editor lets you modify Object Attribute Memory (OAM) entries.

### Commands

- `n <num>` - Select sprite number (0-127)
- `x <pos>` - Set X position (0-255)
- `y <pos>` - Set Y position (0-255)
- `t <tile>` - Set tile number
- `p <pal>` - Set palette (0-7)
- `h` - Toggle horizontal flip
- `v` - Toggle vertical flip
- `s` - Save sprite to OAM
- `b` - Return to main menu

### Example Session

```
n 0          # Select sprite 0
x 64         # Position at X=64
y 96         # Position at Y=96
t 10         # Use tile 10
p 3          # Use palette 3
s            # Save changes
```

## Palette Editor

The Palette Editor provides RGB color editing capabilities.

### Commands

- `p <pal>` - Select palette (0-15)
- `c <color>` - Select color in palette (0-15)
- `r <val>` - Set red component (0-31)
- `g <val>` - Set green component (0-31)
- `b <val>` - Set blue component (0-31)
- `s <hex>` - Set full 15-bit color value
- `d` - Display current palette
- `w` - Write to CGRAM
- `x` - Return to main menu

### Example Session

```
p 0          # Select palette 0
c 1          # Select color 1
r 31         # Max red
g 0          # No green
b 0          # No blue
w            # Write to CGRAM
d            # Display palette
```

### Color Format

SNES uses 15-bit RGB color format:
- 5 bits each for Red, Green, and Blue
- Range: 0-31 for each component
- Format: `0bbbbbgggggrrrrr` (little-endian)

## Tilemap Editor

The Tilemap Editor allows you to modify background layer tilemaps.

### Commands

- `l <layer>` - Select BG layer (0-3)
- `t <tile>` - Select tile to place
- `p <palette>` - Select palette (0-7)
- `m <x> <y>` - Move cursor to position
- `s` - Place selected tile at cursor
- `c` - Toggle paint mode
- `v` - View tilemap at cursor area
- `b` - Return to main menu

### Example Session

```
l 0          # Select BG1
t 42         # Select tile 42
p 2          # Use palette 2
m 10 8       # Move cursor to (10, 8)
s            # Place tile
```

### Tilemap Format

SNES tilemaps are 32x32 tiles with 2-byte entries:
- 10 bits: Tile number (0-1023)
- 3 bits: Palette number (0-7)
- 2 bits: Flip flags (horizontal/vertical)
- 1 bit: Priority

## Script Editor

The Script Editor allows you to execute ROM modification scripts.

### Commands

- `f <file>` - Execute script from file
- `e <cmd>` - Execute single command
- `h` - Show scripting help
- `b` - Return to main menu

### Scripting Language

The scripting language provides commands for ROM manipulation:

#### SET Command
Set a byte at a ROM address:
```
SET address value
```
Example: `SET 10000 FF`

#### SET16 Command
Set a 16-bit word (little-endian):
```
SET16 address value
```
Example: `SET16 7FFC 8000`

#### FILL Command
Fill a memory region with a value:
```
FILL address size value
```
Example: `FILL 20000 100 00`

#### COPY Command
Copy data from one location to another:
```
COPY source destination size
```
Example: `COPY 10000 20000 200`

#### CHECKSUM Command
Recalculate and update ROM checksum:
```
CHECKSUM
```

#### Comments
Lines starting with `;` or `#` are comments:
```
; This is a comment
# This is also a comment
```

### Example Script

```
; Modify ROM header
SET 7FC0 54    ; T
SET 7FC1 45    ; E
SET 7FC2 53    ; S
SET 7FC3 54    ; T

; Fill area with pattern
FILL 10000 1000 AA

; Update checksum
CHECKSUM
```

### Running Scripts

From command line:
1. Select option 5 (Script Editor)
2. Type `f script.txt` to run a script file
3. Or type `e SET 1000 FF` to run a single command

## Saving Changes

To save your modifications:

1. Select option 7 from main menu
2. Enter output filename (e.g., `modified.sfc`)
3. ROM will be saved with updated checksum

**Important**: Always create backups of original ROM files before editing!

## Best Practices

1. **Backup First** - Always work on copies of ROMs
2. **Test Changes** - Save and test modifications frequently
3. **Use Scripts** - For complex changes, use scripts for repeatability
4. **Document Changes** - Keep notes on what you modify
5. **Checksum Updates** - Always run CHECKSUM after modifications

## Technical Details

### Memory Layout

- ROM data is directly editable
- VRAM, CGRAM, and OAM are in-memory representations
- Changes to VRAM/CGRAM/OAM need to be saved back to ROM
- ROM addresses are raw file offsets, not SNES addresses

### Limitations

- Tile editor currently supports 2bpp format only
- Tilemap editor uses simplified 32x32 addressing
- No real-time preview of changes
- Limited to editing existing ROM structure

### Future Enhancements

- Support for 4bpp and 8bpp tile formats
- Visual tile/sprite preview
- Undo/redo functionality
- Import/export of graphics
- Hot reload with emulator integration

## Troubleshooting

**Problem**: Changes don't appear in saved ROM
**Solution**: Make sure to save after editing and run CHECKSUM

**Problem**: Can't find tiles/sprites
**Solution**: Use ROM information viewer to understand ROM layout

**Problem**: Script errors
**Solution**: Check syntax - all numbers are hexadecimal

**Problem**: Corrupted ROM after editing
**Solution**: Restore from backup and verify address ranges

## Examples

### Example 1: Change Palette Colors

```
# Select Game Maker
# Choose Palette Editor (4)
p 0          # Select palette 0
c 0          # Background color
s 0000       # Set to black
w            # Write to CGRAM
```

### Example 2: Reposition Sprite

```
# Choose Sprite Editor (2)
n 5          # Select sprite 5
x 120        # Move to center
y 112        # Move to middle
s            # Save
```

### Example 3: Bulk ROM Patching

Create a file `patch.txt`:
```
; Apply custom patch
SET 1000 4C    ; JMP
SET 1001 00    ; to
SET 1002 90    ; $9000
CHECKSUM
```

Run it:
```
# Choose Script Editor (5)
f patch.txt
```

## Reference

### Memory Ranges

- ROM: 0x000000 - (rom_size)
- VRAM: 64KB (0x0000-0xFFFF)
- CGRAM: 512 bytes (0x000-0x1FF)
- OAM: 544 bytes (0x000-0x21F)

### Color Values

Common SNES colors (15-bit format):
- Black: 0x0000
- White: 0x7FFF
- Red: 0x001F
- Green: 0x03E0
- Blue: 0x7C00

### Useful ROM Offsets

(These vary by ROM, check header):
- LoROM header: 0x7FB0-0x7FDF
- HiROM header: 0xFFB0-0xFFDF
- Title: Header + 0x00 (21 bytes)
- Checksum: Header + 0x1E (2 bytes)

---

For more information, see the main SNESE documentation and roadmap.
