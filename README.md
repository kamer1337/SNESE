# SNESE - SNES Emulator with Built-in Game Maker

A Super Nintendo Entertainment System (SNES) emulator with an integrated game maker, implemented entirely in pure C for educational purposes.

## Overview

SNESE is an educational project aimed at developing a cycle-accurate SNES emulator while providing tools for creating and modifying SNES games. The project emphasizes understanding retro hardware emulation, low-level programming, and interactive tool development.

### Key Features (Planned)

- **Pure C Implementation**: Written in ANSI C (C89/C90) for maximum portability
- **Cycle-Accurate Emulation**: Accurate reproduction of SNES hardware behavior
- **Built-in Game Maker**: Runtime tools for creating and modifying ROM-based games
- **Educational Focus**: Designed to teach emulation concepts and low-level programming
- **Cross-Platform**: Targets Linux, Windows (via MinGW), and other platforms

## Architecture

The emulator will emulate the following SNES components:

- **CPU**: Ricoh 5A22 (65c816 core, 3MHz)
- **PPU**: Picture Processing Unit (backgrounds, sprites, Mode 7)
- **APU**: Sony SPC-700 for audio processing
- **Memory**: 128KB RAM with cartridge mapping (LoROM/HiROM)
- **Input**: Joypad controller support

## Development Roadmap

This project follows a phased development approach. See [roadmap.md](roadmap.md) for detailed information:

1. **Phase 1**: Research and Foundation (Weeks 1-4)
2. **Phase 2**: CPU Emulation Core (Weeks 5-12)
3. **Phase 3**: Graphics and Input Subsystems (Weeks 13-20)
4. **Phase 4**: Audio and System Integration (Weeks 21-28)
5. **Phase 5**: Built-in Game Maker Development (Weeks 29-36)
6. **Phase 6**: Optimization, Testing, and Polish (Weeks 37-44)

## Current Status

🚧 **In Active Development** - Phases 3 and 4 Complete!

- ✅ Phase 1: Foundation and project structure - **Complete (100%)**
- ✅ Phase 2: CPU emulation core - **Complete (100%)**
  - 126 opcodes implemented including all addressing modes
  - Breakpoint debugging support
  - Full 65c816 instruction set
- ✅ Phase 3: Graphics and input - **Complete (100%)**
  - Tile rendering and sprite support
  - Mode 7 affine transformation graphics
  - Input system with controller emulation
- ✅ Phase 4: Audio and system integration - **Complete (100%)**
  - SPC-700 instruction set (50+ opcodes)
  - BRR audio sample decoding
  - DMA and HDMA transfer systems
- 🔄 Phase 5: Game Maker - **In Progress (10%)**
  - Basic interface and menu system
  - Editor placeholders created
- ⏳ Phase 6: Optimization and testing - **Upcoming**

## Prerequisites

- C compiler (GCC or Clang recommended)
- Make build system
- Basic knowledge of:
  - C programming (pointers, structs, bit manipulation)
  - Assembly language (6502/65c816 ISA)
  - SNES architecture

## Building

```bash
# Clone the repository
git clone https://github.com/kamer1337/SNESE.git
cd SNESE

# Build the project
make

# Run the emulator
./snesemu rom.sfc
```

## Usage

### Running the Emulator

```bash
./snesemu <rom_file.sfc>
```

### Game Maker Mode (Phase 5 - In Development)

```bash
./snesemu --maker <rom_file.sfc>
```

Access the built-in Game Maker for ROM editing:
- Tile Editor: Edit tile graphics
- Sprite Editor: Modify sprite properties
- Tilemap Editor: Edit background layers
- Palette Editor: Adjust color palettes
- ROM Export: Save modified ROMs

## Project Structure

```
SNESE/
├── src/              # Source files
│   ├── cpu.c         # CPU emulation
│   ├── memory.c      # Memory management
│   ├── ppu.c         # Graphics processing
│   ├── apu.c         # Audio processing
│   ├── input.c       # Controller input
│   └── main.c        # Main entry point
├── include/          # Header files
├── tests/            # Test ROMs and unit tests
├── docs/             # Additional documentation
├── Makefile          # Build configuration
├── README.md         # This file
└── roadmap.md        # Detailed development roadmap
```

## Testing

Legal SNES ROMs are required for testing. You can use:
- Public domain ROMs
- Homebrew ROMs
- Test ROMs (e.g., blargg's test suite)

```bash
# Run tests (future)
make test
```

## Resources

- [SNES Development Manual](https://archive.org/)
- [SNESdev Wiki](https://snesdev.mesen.ca/)
- [65c816 Reference](http://www.6502.org/tutorials/65c816opcodes.html)
- [Blargg's Test ROMs](https://github.com/christopherpow/nes-test-roms)
- [EmuDev Community](https://www.reddit.com/r/EmuDev/)

## Educational Goals

This project is designed to teach:
- Hardware emulation techniques
- CPU instruction set implementation
- Memory management and banking
- Graphics rendering pipelines
- Audio synthesis
- Low-level C programming
- Bit manipulation and binary formats

## Contributing

This is primarily an educational project, but suggestions and improvements are welcome. Please ensure any contributions maintain the pure C implementation philosophy.

## License

[To be determined]

## Acknowledgments

- SNES development community
- SNESdev wiki contributors
- Blargg for test ROMs
- The EmuDev community

## Disclaimer

This project is for educational purposes only. Users must provide their own legal ROM files. The authors do not condone or support software piracy.
