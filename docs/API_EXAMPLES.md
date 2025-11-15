# SNESE API Examples

This document provides code examples for using SNESE's internal APIs.

## Table of Contents
- [CPU Emulation](#cpu-emulation)
- [Memory Management](#memory-management)
- [PPU Operations](#ppu-operations)
- [DMA Transfers](#dma-transfers)
- [Input Handling](#input-handling)

## CPU Emulation

### Basic CPU Initialization and Execution

```c
#include "cpu.h"
#include "memory.h"

// Global memory system (required by CPU)
Memory g_memory;

void example_cpu_basic() {
    CPU cpu;
    
    // Initialize CPU
    cpu_init(&cpu);
    
    // CPU is now in emulation mode with PC at reset vector
    printf("Initial PC: $%04X\n", cpu.pc);
    
    // Execute one instruction
    u32 cycles = cpu_step(&cpu);
    printf("Executed in %u cycles\n", cycles);
    
    // Print CPU state
    cpu_print_state(&cpu);
}
```

### Single-Step Debugging

```c
void example_cpu_debug() {
    CPU cpu;
    char disasm[64];
    int i;
    
    cpu_init(&cpu);
    
    // Execute 10 instructions with disassembly
    for (i = 0; i < 10; i++) {
        // Disassemble current instruction
        cpu_disassemble(&cpu, disasm, sizeof(disasm));
        printf("$%02X:%04X: %s\n", cpu.pbr, cpu.pc, disasm);
        
        // Execute
        cpu_step(&cpu);
        
        // Check if stopped
        if (cpu.stopped) {
            printf("CPU stopped\n");
            break;
        }
    }
}
```

### Using Breakpoints

```c
void example_cpu_breakpoints() {
    CPU cpu;
    
    cpu_init(&cpu);
    
    // Add breakpoint at address $00:8000
    if (cpu_add_breakpoint(&cpu, 0x008000)) {
        printf("Breakpoint set at $00:8000\n");
    }
    
    // Add another breakpoint
    cpu_add_breakpoint(&cpu, 0x008100);
    
    // Execute until breakpoint
    while (!cpu.breakpoint_hit && !cpu.stopped) {
        cpu_step(&cpu);
    }
    
    if (cpu.breakpoint_hit) {
        printf("Hit breakpoint at $%02X:%04X\n", cpu.pbr, cpu.pc);
        cpu_print_state(&cpu);
    }
    
    // Clear all breakpoints
    cpu_clear_breakpoints(&cpu);
}
```

### Triggering Interrupts

```c
void example_cpu_interrupts() {
    CPU cpu;
    
    cpu_init(&cpu);
    
    // Trigger NMI (Non-Maskable Interrupt)
    cpu_nmi(&cpu);
    
    // Next cpu_step() will handle the NMI
    cpu_step(&cpu);
    
    // Trigger IRQ (if IRQ is not disabled)
    cpu_irq(&cpu);
    cpu_step(&cpu);
}
```

## Memory Management

### Basic Memory Operations

```c
#include "memory.h"
#include "cartridge.h"

void example_memory_basic() {
    Memory mem;
    Cartridge cart;
    
    // Initialize memory
    memory_init(&mem);
    
    // Load cartridge
    if (cartridge_load(&cart, "game.sfc") == SUCCESS) {
        memory_set_cartridge(&mem, &cart);
    }
    
    // Write to Work RAM
    memory_write(&mem, 0x7E0000, 0x42);
    
    // Read from Work RAM
    u8 value = memory_read(&mem, 0x7E0000);
    printf("Value: $%02X\n", value);
    
    // Write 16-bit value
    memory_write16(&mem, 0x7E0100, 0x1234);
    
    // Read 16-bit value
    u16 value16 = memory_read16(&mem, 0x7E0100);
    printf("Value: $%04X\n", value16);
    
    // Cleanup
    cartridge_unload(&cart);
}
```

### Memory Dumps

```c
void example_memory_dump() {
    Memory mem;
    
    memory_init(&mem);
    
    // Dump 256 bytes from Work RAM
    memory_dump(&mem, 0x7E0000, 256);
    
    // Dump ROM header area
    memory_dump(&mem, 0x00FFB0, 80);
}
```

## PPU Operations

### Initializing PPU

```c
#include "ppu.h"

void example_ppu_init() {
    PPU ppu;
    Memory mem;
    
    memory_init(&mem);
    ppu_init(&ppu);
    
    // Connect PPU to memory
    ppu_set_memory(&ppu, mem.vram, mem.cgram, mem.oam);
    
    // Configure display
    ppu.brightness = 15;  // Full brightness
    ppu.forced_blank = false;  // Enable display
    ppu.bg_mode = 0;  // Mode 0 (4 layers, 4 colors each)
    
    // Enable background layers
    ppu.bg[0].enabled = true;
    ppu.bg[1].enabled = true;
}
```

### Setting Up Background Layers

```c
void example_ppu_backgrounds() {
    PPU ppu;
    
    ppu_init(&ppu);
    
    // Configure BG1
    ppu.bg[0].tilemap_addr = 0x0000;  // Tilemap at VRAM $0000
    ppu.bg[0].chr_addr = 0x1000;      // Tile graphics at $1000
    ppu.bg[0].h_scroll = 0;
    ppu.bg[0].v_scroll = 0;
    ppu.bg[0].enabled = true;
    
    // Configure BG2 with scrolling
    ppu.bg[1].tilemap_addr = 0x0400;
    ppu.bg[1].chr_addr = 0x2000;
    ppu.bg[1].h_scroll = 16;  // Scroll 16 pixels right
    ppu.bg[1].v_scroll = 8;   // Scroll 8 pixels down
    ppu.bg[1].enabled = true;
}
```

### Rendering Frames

```c
void example_ppu_render() {
    PPU ppu;
    int frame;
    
    ppu_init(&ppu);
    
    // Render 60 frames (1 second at 60 FPS)
    for (frame = 0; frame < 60; frame++) {
        int scanline;
        
        // Render each scanline
        for (scanline = 0; scanline < SCANLINES_PER_FRAME; scanline++) {
            ppu_step_scanline(&ppu);
        }
        
        // Output this frame
        if (frame == 0) {
            ppu_output_ppm(&ppu, "frame_000.ppm");
        }
    }
}
```

## DMA Transfers

### Basic DMA Transfer

```c
void example_dma_basic() {
    Memory mem;
    
    memory_init(&mem);
    
    // Setup DMA channel 0 to transfer data to VRAM
    // Source: $7E0000 (Work RAM)
    // Destination: PPU register $2118 (VRAM Data)
    // Size: 1024 bytes
    memory_dma_setup(&mem, 
        0,           // Channel 0
        0x01,        // Control: CPU->PPU, increment
        0x18,        // Dest register: $2118 (VRAM)
        0x7E0000,    // Source address
        1024         // Size
    );
    
    // Trigger the transfer
    memory_dma_trigger(&mem, 0x01);  // Bit 0 = channel 0
}
```

### Multiple DMA Channels

```c
void example_dma_multiple() {
    Memory mem;
    
    memory_init(&mem);
    
    // Setup multiple DMA channels
    
    // Channel 0: Transfer tile data to VRAM
    memory_dma_setup(&mem, 0, 0x01, 0x18, 0x008000, 2048);
    
    // Channel 1: Transfer palette to CGRAM
    memory_dma_setup(&mem, 1, 0x00, 0x22, 0x00A000, 512);
    
    // Channel 2: Transfer sprite data to OAM
    memory_dma_setup(&mem, 2, 0x00, 0x04, 0x00B000, 544);
    
    // Trigger all three channels (bits 0, 1, 2)
    memory_dma_trigger(&mem, 0x07);
}
```

## Input Handling

### Basic Controller Input

```c
#include "input.h"

void example_input_basic() {
    InputSystem input;
    
    input_init(&input);
    
    // Simulate button press
    input_set_button(&input, 0, BUTTON_A, true);
    input_set_button(&input, 0, BUTTON_START, true);
    
    // Check if buttons are pressed
    if (input_is_pressed(&input, 0, BUTTON_A)) {
        printf("A button pressed\n");
    }
    
    // Perform auto-read (done during VBlank)
    input_auto_read(&input);
    
    // Release buttons
    input_set_button(&input, 0, BUTTON_A, false);
    input_set_button(&input, 0, BUTTON_START, false);
}
```

### Edge Detection

```c
void example_input_edges() {
    InputSystem input;
    
    input_init(&input);
    
    // First frame: no button pressed
    input_auto_read(&input);
    
    // Second frame: button pressed
    input_set_button(&input, 0, BUTTON_A, true);
    input_auto_read(&input);
    
    // Check for button press edge (just pressed)
    if (input_is_pressed_edge(&input, 0, BUTTON_A)) {
        printf("A button just pressed!\n");
    }
}
```

### Multiple Controllers

```c
void example_input_multiplayer() {
    InputSystem input;
    
    input_init(&input);
    
    // Player 1 input
    input_set_button(&input, 0, BUTTON_A, true);
    input_set_button(&input, 0, BUTTON_UP, true);
    
    // Player 2 input
    input_set_button(&input, 1, BUTTON_B, true);
    input_set_button(&input, 1, BUTTON_DOWN, true);
    
    // Check both controllers
    if (input_is_pressed(&input, 0, BUTTON_A)) {
        printf("Player 1: A pressed\n");
    }
    
    if (input_is_pressed(&input, 1, BUTTON_B)) {
        printf("Player 2: B pressed\n");
    }
}
```

## Complete Example: Mini Emulation Loop

```c
#include <stdio.h>
#include "types.h"
#include "cpu.h"
#include "memory.h"
#include "ppu.h"
#include "input.h"
#include "cartridge.h"

// Global memory (required by CPU)
Memory g_memory;
CPU g_cpu;
PPU g_ppu;
InputSystem g_input;

void run_one_frame() {
    u32 frame_cycles = 89342;  // NTSC: ~89342 cycles per frame
    u32 cycles_executed = 0;
    
    while (cycles_executed < frame_cycles && !g_cpu.stopped) {
        // Execute CPU
        u32 cpu_cycles = cpu_step(&g_cpu);
        cycles_executed += cpu_cycles;
        
        // Execute PPU (proportional to CPU cycles)
        if (cycles_executed % 1364 == 0) {
            ppu_step_scanline(&g_ppu);
            
            // Trigger NMI on VBlank
            if (g_ppu.vcount == 225) {
                cpu_nmi(&g_cpu);
                input_auto_read(&g_input);
            }
        }
    }
}

int main(int argc, char *argv[]) {
    Cartridge cart;
    int frame;
    
    if (argc < 2) {
        printf("Usage: %s <rom_file>\n", argv[0]);
        return 1;
    }
    
    // Load ROM
    if (cartridge_load(&cart, argv[1]) != SUCCESS) {
        printf("Failed to load ROM\n");
        return 1;
    }
    
    // Initialize systems
    memory_init(&g_memory);
    memory_set_cartridge(&g_memory, &cart);
    cpu_init(&g_cpu);
    ppu_init(&g_ppu);
    input_init(&g_input);
    
    ppu_set_memory(&g_ppu, g_memory.vram, g_memory.cgram, g_memory.oam);
    
    // Run for 60 frames
    for (frame = 0; frame < 60; frame++) {
        run_one_frame();
        
        // Output first frame
        if (frame == 0) {
            ppu_render_frame(&g_ppu);
            ppu_output_ppm(&g_ppu, "output.ppm");
        }
    }
    
    printf("Emulation complete!\n");
    printf("Total cycles: %llu\n", (unsigned long long)g_cpu.cycles);
    
    // Cleanup
    cartridge_unload(&cart);
    
    return 0;
}
```

## Notes

- All examples assume proper error checking in production code
- Global `g_memory` is required because CPU functions access it directly
- Remember to initialize all subsystems before use
- Call cleanup functions (like `cartridge_unload`) when done
- Cycle counts are approximate and may vary based on instruction timing
