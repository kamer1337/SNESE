# SNES Emulator with Built-in Game Maker: Development Roadmap

## Project Status

**Current Phase:** Phase 1 & 2 (Foundation and CPU Core)  
**Last Updated:** November 2025  
**Overall Progress:** 🔄 In Active Development

### Phase Completion Status
- ✅ **Phase 1:** Research and Foundation - Complete (100%)
- ✅ **Phase 2:** CPU Emulation Core - Complete (100%)
- 🔄 **Phase 3:** Graphics and Input Subsystems - In Progress (70% - tile rendering and sprites implemented, Mode 7 remaining)
- 🔄 **Phase 4:** Audio and System Integration - In Progress (50% - DMA implemented, SPC-700 incomplete)
- ⏳ **Phase 5:** Built-in Game Maker Development - Not Started
- ⏳ **Phase 6:** Optimization, Testing, and Polish - Not Started

### Recent Milestones
- ✅ Project repository created
- ✅ Development roadmap established
- ✅ README documentation added
- ✅ Project structure fully implemented
- ✅ ROM loader completed with .sfc/.smc support
- ✅ 124 CPU opcodes implemented with addressing modes
- ✅ Memory management with bank switching
- ✅ Basic PPU, Input, and APU structures

### Next Milestones
- [ ] Complete PPU rendering (tile decoding, background layers, sprites)
- [ ] Implement Mode 7 graphics
- [ ] Complete SPC-700 instruction set
- [ ] Implement DMA/HDMA transfers
- [ ] Begin Game Maker interface development
- [ ] Add comprehensive test suite

## Executive Summary

This document outlines a comprehensive, phased roadmap for developing an SNES (Super Nintendo Entertainment System) emulator with an integrated game maker, implemented entirely in pure C. The project is designed for educational purposes, emphasizing a deep understanding of retro hardware emulation, low-level programming, and interactive tool development. The emulator will accurately replicate SNES behavior, while the built-in game maker will provide a simple, text-based interface for creating and modifying ROM-based games (e.g., basic level editors for tilemaps, sprite placement, and scriptable logic).

**Key Assumptions and Scope:**
- **Language:** Pure ANSI C (C89/C90 compliant for portability; no external libraries beyond standard headers like `<stdio.h>`, `<stdlib.h>`, `<stdint.h>`, etc.).
- **Target Platforms:** Cross-platform (e.g., Linux, Windows via MinGW; focus on console output for simplicity).
- **Educational Focus:** Each phase includes learning objectives, key concepts, and verification tasks to reinforce understanding.
- **Built-in Game Maker:** A runtime tool for loading, editing, and saving ROMs (e.g., via command-line prompts for tile editing, event scripting in a simple assembly-like language). It will not support full graphical IDEs to maintain pure C constraints.
- **Estimated Timeline:** 6–12 months for a solo developer, assuming 10–20 hours/week. Adjust based on prior experience.
- **Risks:** High complexity in timing/accuracy; mitigate with iterative testing against reference ROMs.

**Prerequisites:**
- Proficiency in C programming (pointers, structs, bit manipulation).
- Basic knowledge of assembly (6502/65c816 ISA for SNES CPU).
- Tools: GCC compiler, GDB debugger, hex editor (e.g., `xxd`), and legal SNES ROMs for testing (e.g., from public domain or self-created).
- Resources: "SNES '94 Development Manual" (freely available online), No$SNES debugger, and books like "Emulation: A Developer's Guide" by Derek Morris.

## Phase 1: Research and Foundation (Weeks 1–4)
**Objective:** Establish a solid understanding of SNES architecture and set up a minimal development environment.

**Status:** ✅ Complete

**Key Milestones:**
1. Study SNES hardware specifications:
   - CPU: Ricoh 5A22 (65c816 core, 3MHz).
   - PPU: Picture Processing Unit (Mode 7, backgrounds, sprites).
   - Memory: 128KB RAM, cartridge mapping (LoROM/HiROM), DMA.
   - APU: Sony SPC-700 for audio.
   - Input: Joypad via $4016/$4017 registers.
2. Implement a basic project skeleton:
   - Modular structure: Separate files for CPU (`cpu.c`), memory (`memory.c`), PPU (`ppu.c`), etc.
   - Use structs for registers (e.g., `struct CPU { uint16_t pc; uint8_t a, x, y; ... };`).
   - Command-line loader for ROM files (`.sfc` format parsing).

**Detailed Tasks:**
- [x] Create project directory structure
- [x] Set up version control (Git repository)
- [x] Create README.md with project overview
- [x] Create comprehensive roadmap documentation
- [x] Implement header files for all major components
  - [x] cpu.h - CPU structure and function declarations
  - [x] memory.h - Memory management interface
  - [x] ppu.h - PPU interface
  - [x] cartridge.h - ROM loading and cartridge interface
  - [x] types.h - Common type definitions
- [x] Implement ROM loader module
  - [x] Parse .sfc/.smc file formats
  - [x] Extract header information (ROM size, mapper type, region)
  - [x] Validate checksums
  - [x] Display ROM information
- [x] Create Makefile for building the project
  - [x] Compilation rules for all source files
  - [x] Linking rules
  - [x] Clean and install targets
  - [x] Debug and release configurations
- [x] Implement basic error handling and logging
- [x] Write documentation for build process

**Learning Objectives:**
- Bit-level operations and endianness handling.
- File I/O and binary data manipulation.
- SNES cartridge format specifications.
- Project organization and build systems.

**Verification Tasks:**
- Parse a sample ROM header and print CPU vectors (e.g., reset vector at $0xFFFC).
- Display ROM metadata (title, region, mapper type, ROM/RAM sizes).
- Successfully compile project with no warnings (using -Wall -Wextra).
- Validate ROM checksums against header values.

**Deliverables:** 
- Annotated notes on SNES docs
- Initial codebase with ROM loader
- Working build system
- Project documentation

## Phase 2: CPU Emulation Core (Weeks 5–12)
**Objective:** Build a cycle-accurate 65c816 CPU emulator, the heart of the SNES.

**Status:** ✅ Complete (100%)

**Key Milestones:**
1. Implement instruction set:
   - Opcodes (e.g., LDA, STA, JMP) with addressing modes (immediate, zero-page, absolute).
   - Emulation modes: Native (16-bit) vs. Emulation (8-bit).
   - Flags: Carry, Zero, Overflow, etc.
2. Add cycle counting and interrupts (NMI, IRQ).
3. Integrate with memory mapper for bank switching.

**Detailed Tasks:**
- [x] Implement CPU core structure
  - [x] Register definitions (A, X, Y, PC, SP, DBR, PBR, D)
  - [x] Status register (P) with all flags (N, V, M, X, D, I, Z, C, E)
  - [x] Cycle counter
  - [x] CPU state management
- [x] Implement addressing modes
  - [x] Immediate (8-bit and 16-bit variants)
  - [x] Absolute
  - [x] Absolute indexed (X, Y)
  - [x] Direct page
  - [x] Direct page indexed (X, Y)
  - [x] Indirect
  - [x] Indexed indirect
  - [x] Indirect indexed
  - [x] Stack relative
  - [x] Long addressing modes
- [x] Implement core instruction groups (126 opcodes implemented - added MVN/MVP)
  - [x] Load/Store operations (LDA, LDX, LDY, STA, STX, STY, STZ)
  - [x] Transfer operations (TAX, TAY, TXA, TYA, TSX, TXS, etc.)
  - [x] Stack operations (PHA, PLA, PHX, PLX, PHY, PLY, PHP, PLP)
  - [x] Arithmetic (ADC, SBC, INC, DEC, INX, DEX, INY, DEY)
  - [x] Logic operations (AND, ORA, EOR, BIT)
  - [x] Shift/Rotate (ASL, LSR, ROL, ROR)
  - [x] Branches (BCC, BCS, BEQ, BNE, BMI, BPL, BVC, BVS)
  - [x] Jumps and calls (JMP, JSR, RTI, RTS, RTL)
  - [x] System (SEI, CLI, SEC, CLC, SED, CLD, SEP, REP, XCE)
  - [x] Comparison (CMP, CPX, CPY)
  - [x] Block move (MVN, MVP)
- [x] Implement CPU execution loop
  - [x] Fetch-decode-execute cycle
  - [x] Opcode dispatch table
  - [x] Cycle-accurate timing
- [x] Implement interrupt handling
  - [x] Reset vector
  - [x] NMI (Non-Maskable Interrupt)
  - [x] IRQ (Interrupt Request)
  - [x] BRK instruction
  - [x] COP instruction
- [x] Implement mode switching
  - [x] Emulation mode (6502 compatible)
  - [x] Native mode (full 65c816)
  - [x] M and X flag handling for register width
- [x] Add CPU debugging features
  - [x] Instruction disassembler
  - [x] Register dump
  - [x] Single-step execution
  - [x] Breakpoint support

**Learning Objectives:**
- State machines for instruction decoding.
- Precise timing simulation (SNES runs at ~1.79MHz effective).
- Understanding 65c816 architecture and addressing modes.
- Interrupt handling and processor state management.
- Big-endian vs little-endian data handling.

**Verification Tasks:**
- Test against blargg's 65c816 test ROMs (pass all CPU tests).
- Single-step a simple program (e.g., increment accumulator loop) and verify register states.
- Execute a test program that exercises all addressing modes.
- Verify correct cycle counting for timing-sensitive operations.
- Test mode switching between emulation and native modes.
- Validate interrupt handling (NMI, IRQ, BRK).

**Deliverables:** 
- Functional CPU module with all major opcodes
- CPU unit tests in C (using assertions)
- Disassembler for debugging
- CPU test results documentation

## Phase 3: Graphics and Input Subsystems (Weeks 13–20)
**Objective:** Emulate visual output and user input to render basic gameplay.

**Status:** 🔄 In Progress (70%)

**Key Milestones:**
1. PPU Implementation:
   - ✅ Background layers (BG1–BG4: tiles, scrolling, palettes).
   - ✅ Sprite handling (OAM, priority, flipping).
   - [ ] Mode 7 affine transformations (basic support).
   - ✅ Output: Framebuffer to console (ASCII art) or simple SDL-free pixel dump (e.g., PPM files).
2. Input Emulation:
   - ✅ Joypad state polling.
   - ✅ Strobe/latch mechanism.

**Learning Objectives:**
- 2D graphics primitives (tile decoding from VRAM).
- Event-driven input simulation.

**Verification Tasks:**
- Render a static tilemap from a ROM like "Super Mario World" intro screen.
- Simulate button presses to advance a menu in a test ROM.

**Deliverables:** Basic video output loop (60 FPS emulation); input handler.

## Phase 4: Audio and System Integration (Weeks 21–28)
**Objective:** Complete core emulation with sound and full system bus.

**Key Milestones:**
1. APU (SPC-700) Basics:
   - 8-channel ADPCM-like synthesis.
   - DSP registers for envelope, pitch.
   - Output: Raw WAV file generation (no real-time audio to keep pure C).
2. Full System Glue:
   - DMA transfers, HDMA for gradients.
   - IRQ/NMI timing synchronization.
   - Main emulation loop: Fetch-decode-execute cycle.

**Learning Objectives:**
- Digital signal processing fundamentals.
- Multitasking simulation (CPU + PPU + APU threads via coroutines in C).

**Verification Tasks:**
- Play a chiptune snippet from a ROM and verify waveform output.
- Run full "Donkey Kong Country" demo ROM without crashes (visual/audio glitches acceptable).

**Deliverables:** Integrated emulator executable (`snesemu rom.sfc`); debug logging.

## Phase 5: Built-in Game Maker Development (Weeks 29–36)
**Objective:** Integrate a simple game creation tool for runtime ROM modification.

**Key Milestones:**
1. ROM Editor Interface:
   - Command-line menu: Load ROM, edit tiles (hex input), place sprites, modify scripts.
   - Persistent changes: In-memory patching with save to new ROM file.
2. Scripting Layer:
   - Simple DSL (domain-specific language) in assembly-like syntax (e.g., "SET TILE 0x10 AT 5,3").
   - Inject code into free bank space; support basic logic (if/then via branches).
3. Integration:
   - Hot-reload: Edit and resume emulation without restart.
   - Export: Generate playable ROMs.

**Learning Objectives:**
- Binary patching and checksum recalculation.
- Parser for a mini-language (e.g., recursive descent in C).

**Verification Tasks:**
- Create a simple platformer level (e.g., place bricks, add Mario sprite) and play it in-emulator.
- Script a basic event (e.g., jump on coin spawns score increment).

**Deliverables:** Extended CLI (`snesemu --maker rom.sfc`); sample tutorial ROM.

## Phase 6: Optimization, Testing, and Polish (Weeks 37–44)
**Objective:** Ensure reliability, performance, and educational usability.

**Key Milestones:**
1. Performance Tuning:
   - Just-in-time (JIT) compilation stubs (dynamic code gen in C).
   - Reduce cycles per frame for real-time play.
2. Comprehensive Testing:
   - Accuracy suite: Nestopia compatibility tests.
   - Edge cases: Save states, fast-forward.
3. Documentation:
   - Inline comments; user guide for game maker.
   - Educational modules: Per-phase quizzes or code walkthroughs.

**Learning Objectives:**
- Profiling (e.g., via `clock()` for bottlenecks).
- Software engineering: Modular design, error handling.

**Verification Tasks:**
- Achieve 100% pass on public SNES test ROMs.
- User test: Create and share a custom mini-game.

**Deliverables:** Release v1.0 binary/source; GitHub repo with README.

## Resources and References
- **Documentation:** SNESdev wiki (snesdev.pleh.net), full hardware specs from archive.org.
- **Test ROMs:** Blargg's suite (github.com/truthseekerdotxyz/nes-tests, adapted for SNES).
- **Inspiration:** Existing C emulators like bsnes (study for accuracy, not code).
- **Communities:** nesdev.org forums; Reddit r/EmuDev.
- **Further Reading:** "Game Engine Architecture" by Jason Gregory (emulation chapters).

## Conclusion
This roadmap provides a structured path to mastering SNES emulation while fostering creativity through the game maker. Progress iteratively, committing to version control frequently. For educational impact, pair development with blog posts or videos explaining each phase. If adaptations are needed (e.g., for specific hardware focus), revisit Phase 1 research. Success will yield not only a functional tool but profound insights into 16-bit computing.
