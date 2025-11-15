# SNESE Testing Guide

## Current Implementation Status

SNESE is currently in **Phase 1 & 2** implementation, with basic ROM loading and initial CPU emulation.

## What's Implemented

### Phase 1 - Foundation
- ✅ ROM loading (.sfc/.smc files)
- ✅ ROM header parsing
- ✅ Cartridge detection (LoROM/HiROM)
- ✅ Memory system initialization
- ✅ Build system (Makefile)

### Phase 2 - CPU Core (Partial)
- ✅ CPU structure and registers (A, X, Y, SP, PC, D, DBR, PBR, P, E)
- ✅ Status flags (N, V, M, X, D, I, Z, C, E)
- ✅ Interrupt handling structure (NMI, IRQ)
- ✅ Stack operations (push/pull 8-bit and 16-bit)
- ✅ Basic instruction set:
  - Flag instructions: CLC, SEC, CLI, SEI, CLD, SED, CLV, REP, SEP
  - Load immediate: LDA, LDX, LDY
  - Transfer: TAX, TAY, TXA, TYA
  - Stack: PHA, PLA, PHP, PLP
  - Increment/Decrement: INX, INY, DEX, DEY
  - Branches: BCC, BCS, BEQ, BNE, BMI, BPL, BRA
  - Jumps: JMP (absolute and long)
  - Misc: NOP

## Testing the Emulator

### Building

```bash
# Clean build
make clean

# Build release version
make

# Build debug version
make DEBUG=1

# Show build information
make info
```

### Running

```bash
# Display help
./snesemu --help

# Display ROM information only
./snesemu --info <rom_file.sfc>

# Run emulator (with debug output)
./snesemu --debug <rom_file.sfc>

# Run emulator (normal mode)
./snesemu <rom_file.sfc>
```

### Expected Behavior

#### ROM Information Display
When loading a ROM, SNESE will:
1. Parse the ROM header
2. Display ROM information:
   - Title
   - Mapper type (LoROM/HiROM)
   - ROM size
   - SRAM size and battery status
   - Region code
   - Version
   - Checksum validation

#### CPU Initialization
The emulator will:
1. Initialize CPU to power-on state
2. Set emulation mode
3. Load reset vector from ROM
4. Display initial CPU state

#### Debug Mode
With `--debug` flag, the emulator will:
1. Show initial CPU state (registers and flags)
2. Display reset vector address
3. Execute first 5 instructions
4. Show CPU state after execution
5. Display disassembly of executed instructions

### Test ROM Requirements

To test SNESE, you need legal SNES ROM files:
- Public domain ROMs
- Homebrew ROMs
- Self-created test ROMs

**Note:** SNESE does not include any ROM files. Users must provide their own legal ROMs.

### Example Output

```
╔═══════════════════════════════════════════════════════╗
║       SNESE - SNES Emulator & Game Maker             ║
║              Educational Project v0.1                ║
╚═══════════════════════════════════════════════════════╝

Loading ROM: test.sfc

=== ROM Information ===
File: test.sfc
Title: Test ROM
Mapper: LoROM
ROM Size: 512 KB (524288 bytes)
SRAM: None
Region: North America (0x01)
Version: 1.0
Checksum: 0x1234 (Complement: 0xEDCB)
Checksum: VALID
=======================

Initializing emulator...
CPU reset: PC=$8000
Emulator initialized

Reset vector: $8000

First instructions at reset vector:
  $00:8000: SEI
  $00:8001: CLC
  $00:8002: LDA #$00
  $00:8004: TAX
  $00:8005: TAY

CPU state after 5 instructions:
CPU State:
  PC: $00:8006
  A: $0000  X: $0000  Y: $0000
  SP: $01FF  D: $0000
  DBR: $00  P: $34 [--MX-IZ-]
  E: 1 (Emulation mode)
  Cycles: 10

=== Emulation Complete ===
This is Phase 1 implementation - basic ROM loading and CPU initialization
Full emulation loop will be implemented in Phase 2 and beyond
```

## Known Limitations

### Current Phase (1 & 2)
- Only basic CPU instructions implemented
- No memory-addressed load/store operations yet
- No arithmetic operations (ADC, SBC) yet
- No logic operations (AND, OR, EOR) yet
- No subroutine calls (JSR, RTS, RTI) yet
- Graphics (PPU) not implemented
- Audio (APU) not implemented
- Input not implemented

### Next Steps
- Implement remaining CPU instructions
- Add all addressing modes
- Implement JSR/RTS for subroutines
- Add arithmetic and logic operations
- Begin graphics subsystem (Phase 3)

## Creating Test ROMs

For educational purposes, you can create simple test ROMs using 65c816 assemblers:

### Example Test ROM (Assembly)
```assembly
; Simple test ROM - resets and does basic operations
.MEMORYMAP
DEFAULTSLOT 0
SLOTSIZE $8000
SLOTS 1
.ENDME

.ROMBANKSIZE $8000
.ROMBANKS 1

.BANK 0
.ORG $8000

reset:
    SEI             ; Disable interrupts
    CLC             ; Clear carry
    LDA #$00        ; Load 0 into A
    TAX             ; Transfer A to X
    TAY             ; Transfer A to Y
    
loop:
    INX             ; Increment X
    BRA loop        ; Branch to loop

.ORG $FFFC
.DW reset          ; Reset vector
```

Assemble with WLA-DX or similar 65c816 assembler.

## Troubleshooting

### Build Issues
- Ensure GCC is installed: `gcc --version`
- Ensure Make is installed: `make --version`
- For missing dependencies, install build tools: `sudo apt-get install build-essential`

### ROM Loading Issues
- Verify ROM file exists and is readable
- Check file size (must be at least 32KB)
- Ensure ROM format is .sfc or .smc
- SMC headers (512 bytes) are automatically detected and skipped

### Execution Issues
- Most ROMs will hit unimplemented opcodes quickly
- This is expected in Phase 1/2 implementation
- Debug mode shows which opcodes are being executed
- Full game compatibility requires Phase 3+ implementation

## Future Testing

As more phases are completed:
- **Phase 3:** Test ROM rendering and graphics output
- **Phase 4:** Test audio output
- **Phase 5:** Test game maker functionality
- **Phase 6:** Run comprehensive test suites (blargg's tests)

## Contributing Test Cases

If you create test ROMs or find issues, please document:
1. ROM details (size, type, expected behavior)
2. Actual behavior observed
3. Expected vs. actual output
4. CPU state at time of issue

## Resources

- [65c816 Instruction Reference](http://www.6502.org/tutorials/65c816opcodes.html)
- [SNES Development Manual](https://archive.org/)
- [SNESdev Wiki](https://snesdev.mesen.ca/)
- [WLA-DX Assembler](https://github.com/vhelin/wla-dx)
