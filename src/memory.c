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
    int i;
    
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
    for (i = 0; i < 8; i++) {
        mem->dma[i].control = 0;
        mem->dma[i].dest_register = 0;
        mem->dma[i].src_addr = 0;
        mem->dma[i].src_bank = 0;
        mem->dma[i].transfer_size = 0;
        mem->dma[i].enabled = false;
        mem->dma[i].hdma_enabled = false;
        mem->dma[i].hdma_table_bank = 0;
        mem->dma[i].hdma_table_addr = 0;
    }
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
    DMAChannel *dma;
    u32 src_addr, dest_addr;
    u16 count;
    u8 direction, increment;
    u8 value;
    
    if (channel >= 8) {
        return;
    }
    
    dma = &mem->dma[channel];
    
    if (!dma->enabled || dma->transfer_size == 0) {
        return;
    }
    
    /* Get transfer direction and addressing mode from control register */
    direction = (dma->control >> 7) & 1;  /* 0=CPU->PPU, 1=PPU->CPU */
    increment = (dma->control >> 3) & 3;  /* 0=increment, 1=fixed, 2=decrement */
    
    (void)direction;  /* Typically CPU->PPU for DMA */
    
    /* Build source address */
    src_addr = (dma->src_bank << 16) | dma->src_addr;
    
    /* Destination is a PPU register (B-bus) in $2100-$21FF range */
    dest_addr = 0x2100 + dma->dest_register;
    
    /* Perform transfer */
    for (count = 0; count < dma->transfer_size; count++) {
        /* Read from source */
        value = memory_read(mem, src_addr);
        
        /* Write to destination */
        if (dest_addr >= 0x2100 && dest_addr < 0x2200) {
            /* PPU register write */
            mem->io_registers[dest_addr - 0x2000] = value;
            
            /* Handle specific PPU register writes */
            if (dest_addr == 0x2118 || dest_addr == 0x2119) {
                /* VRAM write */
                /* Would need to properly handle VRAM address and increment */
            }
        }
        
        /* Update source address based on increment mode */
        switch (increment) {
            case 0:  /* Increment */
                src_addr++;
                break;
            case 1:  /* Fixed */
                /* Don't change address */
                break;
            case 2:  /* Decrement */
                src_addr--;
                break;
        }
    }
    
    /* Clear transfer size to indicate completion */
    dma->transfer_size = 0;
    dma->enabled = false;
    
    /* Note: Real SNES takes 8 cycles per byte + overhead */
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

void memory_dma_setup(Memory *mem, u8 channel, u8 control, u8 dest_reg,
                      u32 src_addr, u16 size) {
    if (channel >= 8) {
        return;
    }
    
    mem->dma[channel].control = control;
    mem->dma[channel].dest_register = dest_reg;
    mem->dma[channel].src_bank = (src_addr >> 16) & 0xFF;
    mem->dma[channel].src_addr = src_addr & 0xFFFF;
    mem->dma[channel].transfer_size = size;
    mem->dma[channel].enabled = true;
}

void memory_dma_trigger(Memory *mem, u8 channels_mask) {
    int i;
    
    /* Trigger all enabled channels */
    for (i = 0; i < 8; i++) {
        if (channels_mask & (1 << i)) {
            memory_dma_transfer(mem, i);
        }
    }
}

void memory_hdma_init(Memory *mem, u8 channel) {
    DMAChannel *dma;
    
    if (channel >= 8) {
        return;
    }
    
    dma = &mem->dma[channel];
    
    if (!dma->hdma_enabled) {
        return;
    }
    
    /* Initialize HDMA table address and indirect address */
    /* This prepares the channel for scanline-by-scanline transfers */
    dma->transfer_size = 0;  /* Will be set per scanline */
}

void memory_hdma_run(Memory *mem) {
    int i;
    DMAChannel *dma;
    u8 line_count;
    u32 table_addr;
    u8 dest_reg;
    u8 data;
    
    /* Process each HDMA channel */
    for (i = 0; i < 8; i++) {
        dma = &mem->dma[i];
        
        if (!dma->hdma_enabled) {
            continue;
        }
        
        /* Get table address */
        table_addr = ((u32)dma->hdma_table_bank << 16) | dma->hdma_table_addr;
        
        /* Read line count from table */
        line_count = memory_read(mem, table_addr);
        
        if (line_count == 0) {
            /* End of table or repeat mode */
            continue;
        }
        
        dest_reg = dma->dest_register;
        
        /* HDMA transfer modes */
        switch (dma->control & 0x07) {
            case 0:  /* 1 register write */
                table_addr++;
                data = memory_read(mem, table_addr);
                memory_write(mem, 0x2100 + dest_reg, data);
                dma->hdma_table_addr++;
                break;
                
            case 1:  /* 2 registers write */
                table_addr++;
                data = memory_read(mem, table_addr);
                memory_write(mem, 0x2100 + dest_reg, data);
                table_addr++;
                data = memory_read(mem, table_addr);
                memory_write(mem, 0x2101 + dest_reg, data);
                dma->hdma_table_addr += 2;
                break;
                
            case 2:  /* 1 register write twice */
                table_addr++;
                data = memory_read(mem, table_addr);
                memory_write(mem, 0x2100 + dest_reg, data);
                table_addr++;
                data = memory_read(mem, table_addr);
                memory_write(mem, 0x2100 + dest_reg, data);
                dma->hdma_table_addr += 2;
                break;
                
            case 3:  /* 2 registers write twice */
            case 4:  /* 4 registers write */
                /* More complex modes - simplified implementation */
                table_addr++;
                data = memory_read(mem, table_addr);
                memory_write(mem, 0x2100 + dest_reg, data);
                dma->hdma_table_addr++;
                break;
                
            default:
                /* Other modes */
                dma->hdma_table_addr++;
                break;
        }
        
        /* Decrement line counter if not infinite repeat */
        if (line_count < 0x80) {
            line_count--;
            if (line_count == 0) {
                /* Move to next entry in table */
                dma->hdma_table_addr++;
            }
        }
    }
}
