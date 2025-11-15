# SNESE Usage Guide

## Overview

SNESE is a SNES emulator with built-in game maker capabilities, written in pure C for educational purposes.

## Building the Emulator

### Prerequisites
- GCC or compatible C compiler
- Make build system
- Standard C library

### Compilation

```bash
# Standard build
make

# Debug build (with debug symbols and no optimization)
make DEBUG=1

# Clean build
make clean all
```

## Running the Emulator

### Basic Usage

```bash
./snesemu <rom_file.sfc>
```

### Command-Line Options

- `-h, --help` - Display help message
- `-i, --info` - Display ROM information only (don't run emulation)
- `-d, --debug` - Enable debug mode with detailed CPU information

### Examples

```bash
# Display ROM information
./snesemu --info game.sfc

# Run in debug mode
./snesemu --debug game.sfc

# Normal execution
./snesemu game.sfc
```

## Features Implemented

### Phase 1: Foundation ✅
- Project structure and build system
- ROM loading (.sfc/.smc formats)
- ROM header parsing (mapper detection, region, etc.)
- Checksum validation

### Phase 2: CPU Emulation ✅
- Full 65c816 instruction set (126 opcodes)
- All addressing modes
- Emulation and Native modes
- Interrupt handling (NMI, IRQ, BRK, COP)
- CPU debugging features:
  - Instruction disassembler
  - Register state dumps
  - Single-step execution
  - Breakpoint support

### Phase 3: Graphics (In Progress - 70%)
- PPU implementation:
  - Background layer rendering (BG1-BG4)
  - 2bpp tile graphics decoding
  - Sprite rendering (OAM)
  - Palette management (CGRAM)
  - Horizontal/vertical flipping
  - Brightness control
  - PPM file output for debugging
- Mode 7 graphics: **Not yet implemented**

### Phase 4: System Integration (In Progress - 40%)
- DMA transfer system
  - 8 DMA channels
  - CPU->PPU transfers
  - Configurable transfer modes
- Memory management:
  - 128KB Work RAM
  - 64KB Video RAM
  - Bank switching (LoROM/HiROM)
  - Memory-mapped I/O
- Input system:
  - Controller emulation
  - Joypad state management
  - Auto-read mechanism
- APU (basic structure, not fully functional)

## Debug Features

### CPU Debugging

When running with `-d` or `--debug` flag, the emulator will:
- Display the initial CPU state
- Show the first 5 instructions executed
- Print register values after each instruction
- Display the reset vector

### Breakpoints

Breakpoints can be set programmatically in the code:

```c
CPU cpu;
cpu_init(&cpu);

// Add breakpoint at address $00:8000
cpu_add_breakpoint(&cpu, 0x008000);

// Execute until breakpoint
while (!cpu.breakpoint_hit && !cpu.stopped) {
    cpu_step(&cpu);
}
```

### Memory Dumps

Memory regions can be dumped for inspection:

```c
Memory mem;
memory_init(&mem);

// Dump 256 bytes starting at $7E0000
memory_dump(&mem, 0x7E0000, 256);
```

## Output Files

### Frame Output
When emulation completes, the emulator may output:
- `output_frame.ppm` - Screenshot in PPM format (can be viewed with image viewers)

### Audio Output
- `output_audio.wav` - Audio output in WAV format (future implementation)

## ROM Format Support

### Supported Formats
- `.sfc` - SNES ROM files (standard format)
- `.smc` - Super Magicom format (with 512-byte header)

### ROM Header Information
The emulator can display:
- Game title
- Region (NTSC/PAL, country)
- ROM size
- SRAM size
- Mapper type (LoROM/HiROM)
- Cartridge type
- Checksum and validation

## Technical Details

### CPU Emulation
- **Processor**: Ricoh 5A22 (65c816 core)
- **Clock Speed**: ~3.58 MHz (NTSC)
- **Addressing**: 24-bit address space
- **Modes**: 6502 emulation mode and native 16-bit mode

### PPU Emulation
- **Resolution**: 256x224 pixels (NTSC)
- **Colors**: 15-bit RGB (32,768 colors)
- **Palettes**: 256 colors (8 palettes of 32 colors)
- **Backgrounds**: Up to 4 background layers
- **Sprites**: 128 sprites, 8x8 or 16x16 pixels
- **Modes**: Modes 0-1 currently supported

### Memory Map
- `$000000-$3FFFFF` - Cartridge ROM (LoROM)
- `$7E0000-$7FFFFF` - Work RAM (128KB)
- `$2000-$21FF` - PPU registers
- `$4000-$43FF` - Controller and I/O registers

## Limitations (Current Implementation)

- No real-time execution (runs as fast as possible)
- No audio output yet
- Mode 7 graphics not implemented
- SPC-700 audio processor not fully functional
- Game Maker features not yet implemented
- No save state support
- Limited ROM compatibility (simple games only)

## Future Features (Phase 5 & 6)

### Game Maker Mode (Planned)
```bash
./snesemu --maker game.sfc
```

Features will include:
- Tile editor
- Sprite placement
- Level editing
- Event scripting
- ROM patching

### Testing
- Comprehensive test suite
- Test ROM support (blargg's tests)
- Automated regression testing

## Troubleshooting

### "Failed to load ROM file"
- Check that the file exists and path is correct
- Verify file permissions
- Ensure ROM is in .sfc or .smc format

### "Unimplemented opcode"
- Some CPU instructions may not be implemented yet
- Try a different ROM or wait for full implementation

### No visual output
- Check that PPU is properly initialized
- Ensure ROM contains valid graphics data
- Look for `output_frame.ppm` file for debugging

## Contributing

This is an educational project. Contributions should:
- Maintain pure C implementation
- Follow existing code style
- Include documentation
- Pass existing tests

## Resources

- [SNES Development Manual](https://archive.org/)
- [SNESdev Wiki](https://snesdev.mesen.ca/)
- [65c816 Reference](http://www.6502.org/tutorials/65c816opcodes.html)
- [Blargg's Test ROMs](https://github.com/christopherpow/nes-test-roms)

## License

[To be determined]

## Disclaimer

This emulator is for educational purposes only. Users must provide their own legal ROM files. The authors do not condone or support software piracy.
