/*
 * main.c - SNESE main entry point
 * 
 * SNES Emulator with Built-in Game Maker
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/types.h"
#include "../include/cartridge.h"
#include "../include/memory.h"
#include "../include/cpu.h"
#include "../include/ppu.h"
#include "../include/input.h"
#include "../include/apu.h"
#include "../include/game_maker.h"
#include "../include/gui.h"

/* Global system components */
Memory g_memory;
CPU g_cpu;
PPU g_ppu;
InputSystem g_input;
APU g_apu;
Cartridge g_cartridge;
GuiState g_gui;

static void print_usage(const char *program_name) {
    printf("SNESE - SNES Emulator with Built-in Game Maker\n");
    printf("Usage: %s [options] [rom_file.sfc]\n\n", program_name);
    printf("Options:\n");
    printf("  -h, --help       Show this help message\n");
    printf("  -i, --info       Display ROM information only\n");
    printf("  -d, --debug      Enable debug mode\n");
    printf("  -g, --gui        Show ROM selection GUI (default if no ROM specified)\n");
    printf("  --maker          Launch game maker mode\n");
    printf("\n");
    printf("If no ROM file is specified, the ROM selection GUI will be shown.\n");
    printf("\n");
}

static void print_banner(void) {
    printf("\n");
    printf("╔═══════════════════════════════════════════════════════╗\n");
    printf("║       SNESE - SNES Emulator & Game Maker             ║\n");
    printf("║              Educational Project v0.1                ║\n");
    printf("╚═══════════════════════════════════════════════════════╝\n");
    printf("\n");
}

int main(int argc, char *argv[]) {
    int i;
    char *rom_filename = NULL;
    bool info_only = false;
    bool debug_mode = false;
    bool maker_mode = false;
    bool show_gui = false;
    
    print_banner();
    
    /* Parse command line arguments */
    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "-i") == 0 || strcmp(argv[i], "--info") == 0) {
            info_only = true;
        } else if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--debug") == 0) {
            debug_mode = true;
        } else if (strcmp(argv[i], "-g") == 0 || strcmp(argv[i], "--gui") == 0) {
            show_gui = true;
        } else if (strcmp(argv[i], "--maker") == 0) {
            maker_mode = true;
        } else if (argv[i][0] != '-') {
            rom_filename = argv[i];
        }
    }
    
    /* Initialize GUI */
    if (gui_init(&g_gui) != SUCCESS) {
        fprintf(stderr, "Failed to initialize GUI\n");
        return 1;
    }
    
    /* Show GUI if no ROM file was provided or --gui flag is set */
    if (!rom_filename || show_gui) {
        printf("Scanning for ROM files...\n");
        rom_filename = gui_show_rom_selector(&g_gui);
        
        if (!rom_filename) {
            printf("No ROM selected. Exiting.\n");
            gui_cleanup(&g_gui);
            return 0;
        }
    }
    
    /* Check if ROM file was provided */
    if (!rom_filename) {
        fprintf(stderr, "Error: No ROM file specified\n\n");
        print_usage(argv[0]);
        return 1;
    }
    
    /* Load ROM cartridge */
    printf("Loading ROM: %s\n", rom_filename);
    if (cartridge_load(&g_cartridge, rom_filename) != SUCCESS) {
        fprintf(stderr, "Failed to load ROM file\n");
        gui_cleanup(&g_gui);
        return 1;
    }
    
    /* Display ROM information */
    cartridge_print_info(&g_cartridge);
    
    /* If info-only mode, exit here */
    if (info_only) {
        printf("Info-only mode: exiting\n");
        cartridge_unload(&g_cartridge);
        gui_cleanup(&g_gui);
        return 0;
    }
    
    /* If maker mode, enter Game Maker */
    if (maker_mode) {
        GameMaker game_maker;
        
        /* Initialize memory for Game Maker */
        memory_init(&g_memory);
        memory_set_cartridge(&g_memory, &g_cartridge);
        
        /* Enter Game Maker */
        gamemaker_init(&game_maker, &g_cartridge, &g_memory);
        gamemaker_run(&game_maker);
        gamemaker_cleanup(&game_maker);
        
        cartridge_unload(&g_cartridge);
        gui_cleanup(&g_gui);
        return 0;
    }
    
    /* Initialize emulator components */
    printf("Initializing emulator...\n");
    memory_init(&g_memory);
    memory_set_cartridge(&g_memory, &g_cartridge);
    
    cpu_init(&g_cpu);
    ppu_init(&g_ppu);
    input_init(&g_input);
    apu_init(&g_apu);
    
    /* Connect PPU to memory */
    ppu_set_memory(&g_ppu, g_memory.vram, g_memory.cgram, g_memory.oam);
    
    printf("Emulator initialized\n");
    printf("  CPU: 65c816 @ ~3.58 MHz\n");
    printf("  PPU: Graphics subsystem ready\n");
    printf("  APU: SPC-700 + DSP audio ready\n");
    printf("  Input: Controller emulation ready\n");
    
    /* Display settings */
    printf("\nSettings:\n");
    printf("  Volume: %d%%\n", g_gui.volume);
    printf("  VSync: %s\n", g_gui.vsync ? "On" : "Off");
    printf("  Scale Factor: %dx\n", g_gui.scale_factor);
    printf("\n");
    
    /* Print initial CPU state */
    if (debug_mode) {
        cpu_print_state(&g_cpu);
        printf("\n");
    }
    
    /* Display reset vector information */
    printf("Reset vector: $%04X\n", g_cpu.pc);
    
    /* Read and display first few instructions */
    if (debug_mode) {
        char disasm_buffer[64];
        printf("\nFirst instructions at reset vector:\n");
        
        for (i = 0; i < 5; i++) {
            cpu_disassemble(&g_cpu, disasm_buffer, sizeof(disasm_buffer));
            printf("  $%02X:%04X: %s\n", g_cpu.pbr, g_cpu.pc, disasm_buffer);
            
            /* Step one instruction */
            cpu_step(&g_cpu);
            
            if (g_cpu.stopped) {
                printf("  CPU stopped (unimplemented opcode)\n");
                break;
            }
        }
        
        printf("\nCPU state after %d instructions:\n", i);
        cpu_print_state(&g_cpu);
    } else {
        /* Run emulation loop for a short test */
        printf("\n=== Running Emulation Test ===\n");
        printf("Executing 1 frame of emulation...\n\n");
        
        /* Run one frame worth of cycles */
        /* NTSC: ~89342 cycles per frame at 3.58 MHz / 60 Hz */
        u32 frame_cycles = 89342;
        u32 cycles_executed = 0;
        
        while (cycles_executed < frame_cycles && !g_cpu.stopped) {
            /* Execute CPU */
            u32 cpu_cycles = cpu_step(&g_cpu);
            cycles_executed += cpu_cycles;
            
            /* Execute PPU (proportional to CPU cycles) */
            /* PPU runs at same speed, roughly 1 scanline per 1364 cycles */
            if (cycles_executed % 1364 == 0) {
                ppu_step_scanline(&g_ppu);
                
                /* Trigger NMI on VBlank */
                if (g_ppu.vcount == 225) {
                    cpu_nmi(&g_cpu);
                    input_auto_read(&g_input);
                }
            }
            
            /* Execute APU (runs slower, ~1.024 MHz) */
            /* Approximately 1 APU cycle per 3.5 CPU cycles */
            apu_run(&g_apu, cpu_cycles / 3);
        }
        
        printf("Frame emulation complete:\n");
        printf("  CPU cycles: %u\n", cycles_executed);
        printf("  PPU scanline: %u\n", g_ppu.vcount);
        printf("  APU cycles: %lu\n", (unsigned long)g_apu.cpu.cycles);
        
        /* Render and output frame */
        if (g_ppu.framebuffer) {
            ppu_render_frame(&g_ppu);
            ppu_output_ppm(&g_ppu, "output_frame.ppm");
        }
        
        /* Output audio if any was generated */
        if (g_apu.buffer_pos > 0) {
            apu_output_wav(&g_apu, "output_audio.wav");
        }
    }
    
    printf("\n=== Emulation Complete ===\n");
    printf("This is Phase 1 implementation - basic ROM loading and CPU initialization\n");
    printf("Full emulation loop will be implemented in Phase 2 and beyond\n\n");
    
    /* Cleanup */
    cartridge_unload(&g_cartridge);
    gui_cleanup(&g_gui);
    
    return 0;
}
