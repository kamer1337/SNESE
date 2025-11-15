# SNESE Development Notes

## Implementation Progress

### Completed (Phase 1 & 2 Initial)

#### Project Structure
- Modular C source code organization
- Header files for all major components
- Clean separation of concerns
- Build system with Makefile

#### Cartridge/ROM Management
- ROM file loading (.sfc/.smc formats)
- SMC header detection and skipping
- ROM header parsing (title, mapper, size, region, etc.)
- Mapper type detection (LoROM/HiROM)
- Checksum validation
- SRAM allocation for battery-backed saves

#### Memory System
- Work RAM (128KB)
- VRAM (64KB) - structure only
- Color RAM (512 bytes) - structure only
- OAM (544 bytes) - structure only
- Memory mapping basics
- Read/write operations for 8-bit, 16-bit, and 24-bit values
- Bank/offset address translation

#### CPU Emulation (65c816)
- Complete register set (A, X, Y, SP, PC, D, DBR, PBR, P, E)
- Status flags (N, V, M, X, D, I, Z, C)
- Emulation mode flag
- Stack operations (8-bit and 16-bit)
- Interrupt handling structure (NMI, IRQ)
- Cycle counting
- 40+ implemented opcodes including:
  - All flag manipulation instructions
  - Load immediate (LDA, LDX, LDY)
  - Transfer instructions (TAX, TAY, TXA, TYA)
  - Stack operations (PHA, PLA, PHP, PLP)
  - Increment/Decrement (INX, INY, DEX, DEY)
  - All branch instructions
  - Jump instructions (JMP absolute and long)
- Instruction disassembler for debugging

#### Build System
- Makefile with multiple targets
- Debug and release configurations
- Clean compilation with no warnings
- ANSI C99 compliant code

### In Progress (Phase 2)

#### CPU Instructions
- Need to implement:
  - Memory-addressed load/store (absolute, indexed, indirect, etc.)
  - Arithmetic operations (ADC, SBC, INC, DEC)
  - Logic operations (AND, ORA, EOR, BIT)
  - Shift/Rotate (ASL, LSR, ROL, ROR)
  - Subroutine operations (JSR, RTS, RTI, RTL)
  - Comparison operations (CMP, CPX, CPY)
  - Block move (MVN, MVP)
  - Miscellaneous (XCE, XBA, etc.)

#### Addressing Modes
- Implemented: Immediate
- Need to implement:
  - Absolute
  - Absolute indexed (X, Y)
  - Direct page
  - Direct page indexed
  - Indirect
  - Indexed indirect
  - Indirect indexed
  - Stack relative
  - Long addressing modes

### Not Started (Future Phases)

#### Phase 3 - Graphics (PPU)
- Background layers (BG1-BG4)
- Sprite rendering (OAM)
- Tile decoding
- Palette management
- Mode 7 support
- Framebuffer rendering
- VBlank/HBlank timing

#### Phase 4 - Audio (APU)
- SPC-700 CPU emulation
- DSP emulation
- 8-channel ADPCM synthesis
- Audio output

#### Phase 5 - Game Maker
- ROM editor interface
- Tile editor
- Sprite placement
- Script injection
- Hot-reload functionality

#### Phase 6 - Optimization
- Performance profiling
- Cycle accuracy improvements
- Test suite integration
- Documentation completion

## Technical Decisions

### Language & Standards
- **ANSI C99**: Chosen for portability and educational value
- **No external libraries**: Only standard C library (stdio, stdlib, string, stdint)
- **Pure C implementation**: No assembly or platform-specific code

### Architecture Choices
- **Modular design**: Separate files for each subsystem
- **Global memory system**: Accessible by CPU for read/write operations
- **Struct-based components**: Clean encapsulation of state
- **Function-based API**: Clear interfaces between modules

### Memory Management
- **Static allocation**: VRAM, WRAM, etc. are statically sized
- **Dynamic ROM loading**: ROM data allocated based on file size
- **Bank/offset addressing**: Full 24-bit address space support
- **Simplified mapping**: Basic LoROM/HiROM support, to be expanded

### CPU Implementation
- **Switch-based dispatch**: Opcode execution via switch statement
- **Flag-based register sizing**: M and X flags determine 8-bit vs 16-bit mode
- **Cycle accuracy**: Each instruction tracks cycles used
- **Interrupt support**: NMI and IRQ with vector handling

## Code Quality

### Compilation Standards
- Compiles with `-Wall -Wextra -pedantic`
- Zero warnings in release build
- C99 standard compliance verified

### Code Style
- Descriptive variable names
- Inline comments for complex logic
- Function documentation in headers
- Consistent formatting

### Error Handling
- File I/O error checking
- Memory allocation verification
- ROM validation (size, checksum)
- Graceful handling of invalid input

## Performance Considerations

### Current Performance
- No optimization yet (educational focus)
- Switch-based dispatch is readable but not fastest
- No JIT compilation
- No caching

### Future Optimizations
- Function pointer tables for opcode dispatch
- Inline hot paths
- Memory access optimization
- JIT compilation stubs
- Cycle scheduling improvements

## Testing Strategy

### Unit Testing (Future)
- CPU instruction tests
- Memory mapping tests
- Addressing mode tests
- Flag behavior tests

### Integration Testing (Future)
- blargg's CPU test ROMs
- Known game behavior tests
- Homebrew ROM tests

### Current Testing
- Manual ROM loading tests
- Visual inspection of disassembly
- CPU state verification
- Checksum validation

## Known Issues & Limitations

### Current Limitations
1. **Limited instruction set**: Only ~40 of 256 opcodes implemented
2. **No addressing modes**: Only immediate mode for loads
3. **No arithmetic**: ADC/SBC not implemented
4. **No subroutines**: JSR/RTS not working
5. **No graphics**: PPU is stub only
6. **No audio**: APU not started
7. **No input**: Controller not implemented

### Design Limitations
1. **Single-threaded**: No parallel execution
2. **No real-time audio**: Will output WAV files instead
3. **Console output**: No GUI, text-based interface
4. **Limited debugging**: Basic disassembly only

### Platform Limitations
1. **ANSI C only**: No platform-specific optimizations
2. **No GPU acceleration**: Pure CPU rendering
3. **No SDL/OpenGL**: Framebuffer to files or ASCII art

## Development Workflow

### Building
```bash
make clean all        # Clean build
make DEBUG=1          # Debug build with symbols
make info             # Show build configuration
```

### Testing
```bash
./snesemu --help      # Show usage
./snesemu -i rom.sfc  # ROM information only
./snesemu -d rom.sfc  # Debug mode with CPU trace
```

### Code Changes
1. Edit source files in `src/` or headers in `include/`
2. Run `make clean all` to rebuild
3. Test with sample ROMs
4. Verify no warnings with `-Wall -Wextra -pedantic`
5. Commit changes with descriptive messages

## Resources Used

### Documentation
- SNES Development Manual (1994)
- 65c816 Programming Manual
- SNESdev Wiki
- No$SNS documentation

### Test ROMs
- Homebrew test ROMs
- blargg's test suite (planned)
- Custom test programs

### Tools
- GCC compiler
- GDB debugger
- Hex editors (xxd, hexdump)
- Git for version control

## Next Steps

### Immediate (Phase 2 Completion)
1. Implement memory-addressed load/store operations
2. Add arithmetic instructions (ADC, SBC)
3. Add logic operations (AND, OR, EOR)
4. Implement JSR/RTS for subroutines
5. Add comparison operations
6. Complete all addressing modes
7. Add instruction tests

### Short-term (Phase 3 Start)
1. Begin PPU implementation
2. Basic tile rendering
3. Background layer support
4. Simple framebuffer output
5. VBlank timing

### Long-term (Phases 4-6)
1. Audio subsystem
2. Game maker tools
3. Optimization and polish
4. Comprehensive testing
5. Documentation completion
6. Release v1.0

## Notes for Contributors

### Code Contributions
- Follow existing code style
- Add comments for complex logic
- Update documentation
- Test thoroughly before committing
- Keep changes minimal and focused

### Documentation Contributions
- Keep docs up-to-date with code
- Add examples where helpful
- Explain educational concepts
- Reference source material

### Testing Contributions
- Report bugs with details
- Create test ROMs
- Verify fixes
- Document test procedures
