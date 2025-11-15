/*
 * memory.h - SNES memory management and bus interface
 * 
 * Handles memory mapping, bank switching, and memory operations
 * for the SNES address space (24-bit addressing).
 */

#ifndef MEMORY_H
#define MEMORY_H

#include "types.h"
#include "cartridge.h"

/* Memory regions */
#define WRAM_START     0x7E0000
#define WRAM_END       0x7FFFFF
#define SRAM_START     0x700000
#define SRAM_END       0x7DFFFF

/* Memory structure */
typedef struct {
    u8 wram[WRAM_SIZE];       /* 128KB Work RAM */
    u8 vram[VRAM_SIZE];       /* 64KB Video RAM */
    u8 cgram[CGRAM_SIZE];     /* 512 bytes Color RAM (palette) */
    u8 oam[OAM_SIZE];         /* 544 bytes Object Attribute Memory */
    
    Cartridge *cart;          /* Pointer to loaded cartridge */
    
    /* DMA state */
    bool dma_enabled[8];      /* 8 DMA channels */
    
    /* Memory mapped I/O registers */
    u8 io_registers[0x8000];  /* $2000-$7FFF I/O space */
} Memory;

/* Function declarations */

/*
 * Initialize memory system
 */
void memory_init(Memory *mem);

/*
 * Reset memory to power-on state
 */
void memory_reset(Memory *mem);

/*
 * Set cartridge for memory system
 */
void memory_set_cartridge(Memory *mem, Cartridge *cart);

/*
 * Read byte from 24-bit address
 */
u8 memory_read(Memory *mem, u32 address);

/*
 * Write byte to 24-bit address
 */
void memory_write(Memory *mem, u32 address, u8 value);

/*
 * Read 16-bit word (little-endian)
 */
u16 memory_read16(Memory *mem, u32 address);

/*
 * Write 16-bit word (little-endian)
 */
void memory_write16(Memory *mem, u32 address, u16 value);

/*
 * Read 24-bit long address
 */
u32 memory_read24(Memory *mem, u32 address);

/*
 * DMA transfer
 */
void memory_dma_transfer(Memory *mem, u8 channel);

/*
 * Map bank address to physical address
 * Handles LoROM/HiROM mapping
 */
u32 memory_map_address(const Memory *mem, u8 bank, u16 offset);

/*
 * Dump memory region for debugging
 */
void memory_dump(const Memory *mem, u32 start, u32 length);

#endif /* MEMORY_H */
