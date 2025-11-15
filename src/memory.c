/*
 * memory.c - SNES memory management implementation
 */

#include <stdio.h>
#include <string.h>
#include "../include/memory.h"

void memory_init(Memory *mem) {
    memset(mem, 0, sizeof(Memory));
    memory_reset(mem);
}

void memory_reset(Memory *mem) {
    /* Clear Work RAM */
    memset(mem->wram, 0, WRAM_SIZE);
    
    /* Clear VRAM */
    memset(mem->vram, 0, VRAM_SIZE);
    
    /* Clear Color RAM */
    memset(mem->cgram, 0, CGRAM_SIZE);
    
    /* Clear OAM */
    memset(mem->oam, 0, OAM_SIZE);
    
    /* Clear I/O registers */
    memset(mem->io_registers, 0, sizeof(mem->io_registers));
    
    /* Reset DMA channels */
    memset(mem->dma_enabled, 0, sizeof(mem->dma_enabled));
}

void memory_set_cartridge(Memory *mem, Cartridge *cart) {
    mem->cart = cart;
}

u8 memory_read(Memory *mem, u32 address) {
    u8 bank = (address >> 16) & 0xFF;
    u16 offset = address & 0xFFFF;
    
    /* Work RAM banks ($7E-$7F) */
    if (bank >= 0x7E && bank <= 0x7F) {
        u32 wram_addr = ((bank - 0x7E) << 16) | offset;
        return mem->wram[wram_addr];
    }
    
    /* Mirror of low WRAM in banks $00-$3F and $80-$BF */
    if (((bank <= 0x3F) || (bank >= 0x80 && bank <= 0xBF)) &&
        (offset <= 0x1FFF)) {
        return mem->wram[offset];
    }
    
    /* I/O registers ($2000-$5FFF in banks $00-$3F and $80-$BF) */
    if (((bank <= 0x3F) || (bank >= 0x80 && bank <= 0xBF)) &&
        (offset >= 0x2000 && offset <= 0x5FFF)) {
        return mem->io_registers[offset - 0x2000];
    }
    
    /* ROM access - delegate to cartridge */
    if (mem->cart) {
        /* Simplified ROM mapping - to be expanded based on mapper type */
        if (bank <= 0x7D) {
            if (offset >= 0x8000) {
                /* LoROM: banks $00-$7D, offset $8000-$FFFF */
                u32 rom_addr = (bank << 15) | (offset & 0x7FFF);
                if (rom_addr < mem->cart->rom_size) {
                    return mem->cart->rom_data[rom_addr];
                }
            }
        }
    }
    
    /* Open bus - return 0xFF for unmapped regions */
    return 0xFF;
}

void memory_write(Memory *mem, u32 address, u8 value) {
    u8 bank = (address >> 16) & 0xFF;
    u16 offset = address & 0xFFFF;
    
    /* Work RAM banks ($7E-$7F) */
    if (bank >= 0x7E && bank <= 0x7F) {
        u32 wram_addr = ((bank - 0x7E) << 16) | offset;
        mem->wram[wram_addr] = value;
        return;
    }
    
    /* Mirror of low WRAM in banks $00-$3F and $80-$BF */
    if (((bank <= 0x3F) || (bank >= 0x80 && bank <= 0xBF)) &&
        (offset <= 0x1FFF)) {
        mem->wram[offset] = value;
        return;
    }
    
    /* I/O registers ($2000-$5FFF in banks $00-$3F and $80-$BF) */
    if (((bank <= 0x3F) || (bank >= 0x80 && bank <= 0xBF)) &&
        (offset >= 0x2000 && offset <= 0x5FFF)) {
        mem->io_registers[offset - 0x2000] = value;
        return;
    }
    
    /* SRAM write - delegate to cartridge if applicable */
    if (mem->cart && mem->cart->has_sram) {
        /* Simplified SRAM mapping - to be expanded */
        cartridge_write(mem->cart, offset, value);
    }
}

u16 memory_read16(Memory *mem, u32 address) {
    u8 low = memory_read(mem, address);
    u8 high = memory_read(mem, address + 1);
    return low | (high << 8);
}

void memory_write16(Memory *mem, u32 address, u16 value) {
    memory_write(mem, address, value & 0xFF);
    memory_write(mem, address + 1, (value >> 8) & 0xFF);
}

u32 memory_read24(Memory *mem, u32 address) {
    u8 low = memory_read(mem, address);
    u8 mid = memory_read(mem, address + 1);
    u8 high = memory_read(mem, address + 2);
    return low | (mid << 8) | (high << 16);
}

void memory_dma_transfer(Memory *mem, u8 channel) {
    /* DMA implementation for Phase 4 */
    /* DMA transfers data between memory regions */
    /* Channels 0-7 available */
    
    if (channel >= 8) {
        return;
    }
    
    /* Enable the DMA channel */
    mem->dma_enabled[channel] = true;
    
    /* TODO: Full DMA implementation would:
     * 1. Read DMA control registers for the channel
     * 2. Determine source and destination addresses
     * 3. Determine transfer size and direction
     * 4. Perform the actual memory copy
     * 5. Update DMA channel registers
     * 6. Take appropriate number of CPU cycles
     */
    
    /* For now, this is a placeholder that marks the channel as active */
}

u32 memory_map_address(const Memory *mem, u8 bank, u16 offset) {
    /* Simplified address mapping - to be expanded based on cartridge mapper */
    (void)mem;  /* Unused for now */
    return (bank << 16) | offset;
}

void memory_dump(const Memory *mem, u32 start, u32 length) {
    u32 i;
    printf("\nMemory dump at $%06X:\n", start);
    
    for (i = 0; i < length; i++) {
        if (i % 16 == 0) {
            printf("%06X: ", start + i);
        }
        
        printf("%02X ", memory_read((Memory *)mem, start + i));
        
        if (i % 16 == 15) {
            printf("\n");
        }
    }
    
    if (length % 16 != 0) {
        printf("\n");
    }
    printf("\n");
}
