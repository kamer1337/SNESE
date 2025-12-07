# SNESE Beginner's Tutorial

## Introduction

Welcome to SNESE! This tutorial will guide you through the basics of using the SNES emulator and its built-in Game Maker to create and modify SNES games.

## Table of Contents

1. [Getting Started](#getting-started)
2. [Loading Your First ROM](#loading-your-first-rom)
3. [Understanding SNES Graphics](#understanding-snes-graphics)
4. [Game Maker Basics](#game-maker-basics)
5. [Editing Tiles](#editing-tiles)
6. [Working with Sprites](#working-with-sprites)
7. [Modifying Palettes](#modifying-palettes)
8. [Creating Tilemaps](#creating-tilemaps)
9. [Using Scripts](#using-scripts)
10. [Practical Examples](#practical-examples)

## Getting Started

### Prerequisites

Before you begin, make sure you have:
- A C compiler (GCC recommended)
- Make build system
- Legal SNES ROM files (.sfc or .smc format)
- Basic understanding of hexadecimal numbers
- Terminal/command line experience

### Building SNESE

```bash
# Clone the repository
git clone https://github.com/kamer1337/SNESE.git
cd SNESE

# Build the emulator
make

# Verify the build
./snesemu --help
```

Expected output:
```
SNESE - SNES Emulator with Built-in Game Maker
Usage: ./snesemu [options] <rom_file>
...
```

## Loading Your First ROM

### Method 1: Interactive ROM Selection

1. Place your ROM files in the `roms/` directory:
```bash
mkdir -p roms
cp your_game.sfc roms/
```

2. Launch the emulator:
```bash
./snesemu
```

3. Select a ROM from the list using number keys and press Enter.

### Method 2: Direct Loading

```bash
./snesemu roms/your_game.sfc
```

### Method 3: ROM Information Only

To view ROM details without running it:
```bash
./snesemu --info roms/your_game.sfc
```

You'll see information like:
- Game title
- ROM size
- Mapper type (LoROM/HiROM)
- Region
- Checksum

## Understanding SNES Graphics

### Key Concepts

**Tiles**
- Basic building block: 8x8 pixels
- Stored in VRAM (Video RAM)
- Can be 2bpp, 4bpp, or 8bpp (bits per pixel)
- We focus on 2bpp (4 colors per tile)

**Sprites (Objects)**
- Movable graphics
- Made from tiles
- Can be 8x8, 16x16, 32x32, or 64x64 pixels
- SNES supports 128 sprites

**Palettes**
- Define available colors
- 16 palettes × 16 colors = 256 colors total
- Each color is 15-bit (BGR555 format)
- Range: 0-31 for each color component (R, G, B)

**Tilemaps**
- Arrange tiles to create backgrounds
- 4 background layers (BG1-BG4)
- Each layer can scroll independently

## Game Maker Basics

### Launching Game Maker

```bash
./snesemu --maker roms/your_game.sfc
```

You'll see the main menu:
```
=== SNESE Game Maker ===
1. Tile Editor
2. Sprite Editor
3. Tilemap Editor
4. Palette Editor
5. Script Editor
6. Save ROM
0. Exit
```

### Navigation Tips

- Use number keys to select options
- Press 'q' or '0' to go back/exit
- Changes are made in memory until you save
- Always save your work before exiting!

## Editing Tiles

### Exercise 1: View a Tile

1. Select **1. Tile Editor** from main menu
2. Enter tile address (start with 0)
3. View the 8x8 pixel representation

```
Tile at 0x0000 (Palette 0):
. . # #  . . # #
. # . .  . # . .
# . . .  # . . .
# . . .  # . . .
. # . .  . # . .
. . # #  . . # #
. . . .  . . . .
. . . .  . . . .
```

### Exercise 2: Edit a Tile

1. View a tile (as above)
2. Press 'e' to enter edit mode
3. Use commands:
   - `x y c` - Set pixel at (x,y) to color c (0-3)
   - Example: `0 0 1` sets top-left pixel to color 1
   - `x y 0` - Clear a pixel
4. Press 'q' to exit edit mode
5. Press 's' to save changes

### Exercise 3: Change Tile Palette

1. View a tile
2. Press 'p' to change palette
3. Enter palette number (0-7)
4. See how the tile looks with different colors

### Understanding 2bpp Format

Each tile uses 2 bits per pixel:
- **Color 0 (00)**: Transparent or background
- **Color 1 (01)**: First color from palette
- **Color 2 (10)**: Second color from palette
- **Color 3 (11)**: Third color from palette

## Working with Sprites

### Exercise 4: View Sprite Properties

1. Select **2. Sprite Editor** from main menu
2. Enter sprite number (0-127)
3. View sprite attributes:

```
Sprite #0:
  X Position: 100
  Y Position: 50
  Tile Number: 16
  Palette: 2
  Horizontal Flip: No
  Vertical Flip: No
  Priority: 1
```

### Exercise 5: Move a Sprite

1. View a sprite (as above)
2. Press 'x' to change X position
3. Enter new X value (0-255)
4. Press 'y' to change Y position
5. Enter new Y value (0-223)
6. Save changes with 's'

### Exercise 6: Change Sprite Appearance

```
Commands in Sprite Editor:
x - Change X position
y - Change Y position
t - Change tile number
p - Change palette
h - Toggle horizontal flip
v - Toggle vertical flip
s - Save changes
q - Quit
```

## Modifying Palettes

### Exercise 7: View a Palette

1. Select **4. Palette Editor** from main menu
2. Enter palette number (0-15)
3. View all 16 colors in the palette:

```
Palette 0:
 0: #0000 (  0,  0,  0) ████ Black
 1: #7FFF ( 31, 31, 31) ████ White
 2: #001F ( 31,  0,  0) ████ Red
 3: #03E0 (  0, 31,  0) ████ Green
 ...
```

### Exercise 8: Edit a Color

1. View a palette
2. Press 'e' to edit mode
3. Enter color index (0-15)
4. Choose method:
   - **RGB**: Enter R, G, B values (0-31 each)
   - **HEX**: Enter BGR555 hex value
5. Save with 's'

### Color Format: BGR555

SNES uses 15-bit colors in BGR555 format:
- 5 bits for Blue (0-31)
- 5 bits for Green (0-31)
- 5 bits for Red (0-31)

Hex format: `0bbbbbgggggrrrrr`

Examples:
- Pure Red: `0x001F` (R=31, G=0, B=0)
- Pure Green: `0x03E0` (R=0, G=31, B=0)
- Pure Blue: `0x7C00` (R=0, G=0, B=31)
- White: `0x7FFF` (R=31, G=31, B=31)
- Black: `0x0000` (R=0, G=0, B=0)

### Exercise 9: Create a Custom Palette

Let's create a palette for a forest scene:

```
Palette 5 (Forest):
 0: #0000 - Black (transparent)
 1: #0210 - Dark brown (tree trunks)
 2: #02E0 - Dark green (leaves)
 3: #07E0 - Bright green (highlights)
 4: #0015 - Light brown (ground)
 5: #4210 - Medium blue (sky)
 6: #7C14 - Light blue (sky highlights)
 7: #7FFF - White (clouds)
```

Commands:
```
e 1    → Enter RGB → 16 0 0   (Dark brown)
e 2    → Enter RGB → 0 23 0   (Dark green)
e 3    → Enter RGB → 0 31 0   (Bright green)
...
s      → Save palette
```

## Creating Tilemaps

### Exercise 10: Navigate the Tilemap

1. Select **3. Tilemap Editor** from main menu
2. Select layer (1-4)
3. Navigate with WASD keys:
   - W: Move up
   - A: Move left
   - S: Move down
   - D: Move right

### Exercise 11: Place Tiles

```
Current position: (10, 5)
Current tile: 0

Commands:
t - Change tile number
p - Paint mode (continuous placement)
w/a/s/d - Navigate
l - Change layer
v - View area
q - Quit
```

To create a simple pattern:
1. Press 't' and enter tile number (e.g., 10)
2. Press 'p' to enter paint mode
3. Use WASD to move and paint tiles
4. Press 'p' again to exit paint mode
5. Press 's' to save

## Using Scripts

### Exercise 12: Your First Script

Scripts allow batch modifications to the ROM.

1. Select **5. Script Editor** from main menu
2. Create a script file `my_script.txt`:

```
# My First Script
# Set bytes at specific addresses

SET 0x8000 0x42
SET 0x8001 0xFF
FILL 0x9000 0x9010 0xAA
```

3. Load and execute:
   - Press 'l' to load
   - Enter filename: `my_script.txt`
   - Press 'x' to execute

### Script Commands

**SET** - Write a single byte
```
SET <address> <value>
Example: SET 0x8000 0x42
```

**FILL** - Fill a range with a value
```
FILL <start> <end> <value>
Example: FILL 0x1000 0x1100 0xFF
```

**COPY** - Copy data to an address
```
COPY <address> <byte1> <byte2> ...
Example: COPY 0x2000 0x01 0x02 0x03 0x04
```

**Comments**
```
# This is a comment
; This is also a comment
```

**Labels** (for future GOTO support)
```
:LABEL_NAME
```

### Exercise 13: Batch Tile Modification

Create `clear_tiles.txt`:
```
# Clear first 16 tiles
FILL 0x0000 0x0200 0x00

# Set tile 0 to a solid block
COPY 0x0000 0xFF 0xFF 0xFF 0xFF 0xFF 0xFF 0xFF 0xFF
COPY 0x0008 0xFF 0xFF 0xFF 0xFF 0xFF 0xFF 0xFF 0xFF
```

This script:
- Clears the first 16 tiles (16 × 16 bytes = 256 bytes)
- Creates a solid white tile at position 0

## Practical Examples

### Example 1: Change Character Sprite Color

**Goal**: Change Mario's color from red to blue

1. Find sprite palette (usually palette 0-2 for main character)
2. Open Palette Editor
3. Locate red color (e.g., color index 2)
4. Change to blue:
   - Press 'e', select color 2
   - Enter RGB: 0, 0, 31 (pure blue)
5. Save ROM

### Example 2: Create Custom Level Background

**Goal**: Create a brick wall pattern

1. **Design Brick Tile** (Tile Editor):
   ```
   Tile pattern for brick (tile 100):
   # # # # # # # #
   # . . . . . . #
   # . . . . . . #
   # # # # # # # #
   # . . . . . . #
   # . . . . . . #
   # # # # # # # #
   ```

2. **Apply Palette** (Palette Editor):
   - Palette 3: Brown/orange for bricks
   - Color 1: #0010 (dark red/brown)
   - Color 2: #001F (bright red)
   - Color 3: #0019 (orange)

3. **Fill Tilemap** (Tilemap Editor):
   - Select BG1 layer
   - Use tile 100
   - Paint across the screen

4. **Save** your modified ROM

### Example 3: Add a Custom Logo

**Goal**: Add "SNESE" text using tiles

1. **Create Letter Tiles**:
   - Tile 200: 'S'
   - Tile 201: 'N'
   - Tile 202: 'E'
   - Tile 203: 'S'
   - Tile 204: 'E'

2. **Arrange in Tilemap**:
   ```
   Position (0,0): Tile 200 (S)
   Position (1,0): Tile 201 (N)
   Position (2,0): Tile 202 (E)
   Position (3,0): Tile 203 (S)
   Position (4,0): Tile 204 (E)
   ```

3. **Use Script for Efficiency**:
   ```
   # Place SNESE text at tilemap position 0,0
   SET 0x4000 0xC8    # Tile 200 at (0,0)
   SET 0x4001 0xC9    # Tile 201 at (1,0)
   SET 0x4002 0xCA    # Tile 202 at (2,0)
   SET 0x4003 0xCB    # Tile 203 at (3,0)
   SET 0x4004 0xCC    # Tile 204 at (4,0)
   ```

## Common Mistakes and Solutions

### Mistake 1: Changes Don't Appear

**Problem**: You made changes but don't see them in the game.

**Solutions**:
- Did you save the ROM? (Press 's' in editors)
- Did you save the ROM file? (Option 6 in main menu)
- Are you editing the correct address?
- Is the game loading from the saved ROM?

### Mistake 2: Colors Look Wrong

**Problem**: Palette changes show incorrect colors.

**Solutions**:
- Remember: SNES uses BGR555, not RGB
- Values range from 0-31, not 0-255
- Hex format is BGR, not RGB
- Try decimal RGB input first

### Mistake 3: Tile Changes Not Visible

**Problem**: Modified tiles don't show up.

**Solutions**:
- Check if the tile is actually used by the game
- Verify you're editing 2bpp format, not 4bpp/8bpp
- Make sure you're looking at the right VRAM address
- Try changing a tile you can see on screen first

### Mistake 4: Script Errors

**Problem**: Script doesn't execute properly.

**Solutions**:
- Check syntax: `SET 0x1000 0x42` (hex with 0x prefix)
- Ensure addresses are within ROM bounds
- Check for typos in commands
- Use comments to document complex scripts

## Tips and Tricks

### Tip 1: Find Used Tiles

To find which tiles a game uses:
1. View the game in tilemap editor
2. Navigate around to see tile numbers
3. Note the ranges used
4. Edit only those ranges

### Tip 2: Backup Your ROM

Always keep the original:
```bash
cp original.sfc original.sfc.backup
```

The emulator also creates internal backups you can restore.

### Tip 3: Use Scripts for Repetitive Tasks

Instead of manually setting 100 bytes:
```
# Bad: Manual
SET 0x1000 0xFF
SET 0x1001 0xFF
SET 0x1002 0xFF
...

# Good: Script
FILL 0x1000 0x1064 0xFF
```

### Tip 4: Test Incrementally

After each change:
1. Save the ROM
2. Run the game
3. Verify the change worked
4. Move to next change

Don't make 50 changes then test!

### Tip 5: Use Hexadecimal Calculator

Keep a hex calculator handy:
- Windows: Calculator app (Programmer mode)
- Linux: `bc` command or `gnome-calculator`
- Online: speedcrunch or similar

## Next Steps

Now that you've completed the tutorial:

1. **Practice**: Try modifying a simple game
2. **Explore**: Read the Game Maker Guide for advanced features
3. **Experiment**: Create your own tile patterns and palettes
4. **Share**: Show your modifications to the community
5. **Learn**: Study SNES architecture for deeper understanding

## Additional Resources

- [GAME_MAKER_GUIDE.md](GAME_MAKER_GUIDE.md) - Complete reference
- [PHASE_5_COMPLETION.md](PHASE_5_COMPLETION.md) - Technical details
- [SNESdev Wiki](https://snesdev.mesen.ca/) - SNES development docs
- [Reddit r/EmuDev](https://www.reddit.com/r/EmuDev/) - Community support

## Glossary

- **2bpp**: 2 bits per pixel (4 colors)
- **BGR555**: 15-bit color format (5 bits each for B, G, R)
- **OAM**: Object Attribute Memory (sprite data)
- **Tile**: 8x8 pixel graphic unit
- **VRAM**: Video RAM (graphics memory)
- **Palette**: Set of colors available for graphics
- **Tilemap**: Arrangement of tiles forming backgrounds
- **Sprite**: Movable object made from tiles

## Need Help?

If you're stuck:
1. Check this tutorial again
2. Read the comprehensive Game Maker Guide
3. Review the examples
4. Check GitHub issues
5. Ask the community

## Conclusion

Congratulations on completing the SNESE tutorial! You now have the skills to:
- Load and view ROM information
- Edit tiles and create custom graphics
- Modify sprite positions and attributes
- Change color palettes
- Create tilemaps and backgrounds
- Use scripts for batch modifications

Keep practicing and experimenting. The best way to learn is by doing!

Happy ROM hacking!

---

**Last Updated**: December 2025  
**Version**: 1.0
