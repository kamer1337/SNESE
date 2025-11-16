/*
 * ppu.c - Picture Processing Unit implementation
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/ppu.h"
#include "../include/upscaler.h"

void ppu_init(PPU *ppu) {
    int i;
    memset(ppu, 0, sizeof(PPU));
    
    /* Allocate framebuffer */
    ppu->framebuffer = (u32 *)calloc(SCREEN_WIDTH * SCREEN_HEIGHT, sizeof(u32));
    
    /* Allocate layer buffers */
    for (i = 0; i < 5; i++) {
        ppu->layer_buffer[i] = (u8 *)calloc(SCREEN_WIDTH * SCREEN_HEIGHT, 1);
    }
    
    ppu_reset(ppu);
}

void ppu_reset(PPU *ppu) {
    int i;
    
    ppu->vcount = 0;
    ppu->hcount = 0;
    ppu->vblank = false;
    ppu->hblank = false;
    ppu->brightness = 15;
    ppu->forced_blank = true;
    ppu->bg_mode = 0;
    ppu->frame_count = 0;
    
    /* Reset background layers */
    for (i = 0; i < 4; i++) {
        ppu->bg[i].tilemap_addr = 0;
        ppu->bg[i].chr_addr = 0;
        ppu->bg[i].h_scroll = 0;
        ppu->bg[i].v_scroll = 0;
        ppu->bg[i].size = 0;
        ppu->bg[i].enabled = false;
    }
    
    /* Reset CGRAM state */
    ppu->cgram_addr = 0;
    ppu->cgram_latch = false;
    ppu->cgram_buffer = 0;
    
    /* Reset OAM state */
    ppu->oam_addr = 0;
    ppu->oam_buffer = 0;
    
    /* Reset VRAM state */
    ppu->vram_addr = 0;
    ppu->vram_increment = 1;
    
    /* Reset Mode 7 state */
    ppu->m7_matrix_a = 0x0100;  /* Identity matrix (1.0 in fixed point) */
    ppu->m7_matrix_b = 0;
    ppu->m7_matrix_c = 0;
    ppu->m7_matrix_d = 0x0100;  /* Identity matrix (1.0 in fixed point) */
    ppu->m7_center_x = 0;
    ppu->m7_center_y = 0;
    ppu->m7_repeat = 0;
    ppu->m7_h_flip = false;
    ppu->m7_v_flip = false;
    ppu->m7_latch = 0;
    
    ppu->needs_render = false;
    
    /* Clear framebuffer */
    if (ppu->framebuffer) {
        memset(ppu->framebuffer, 0, SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(u32));
    }
}

void ppu_set_memory(PPU *ppu, u8 *vram, u8 *cgram, u8 *oam) {
    ppu->vram = vram;
    ppu->cgram = cgram;
    ppu->oam = oam;
}

void ppu_step_scanline(PPU *ppu) {
    ppu->vcount++;
    
    /* NTSC: 262 scanlines per frame */
    if (ppu->vcount >= SCANLINES_PER_FRAME) {
        ppu->vcount = 0;
        ppu->vblank = false;
        ppu->frame_count++;
        ppu->needs_render = true;
    }
    
    /* VBlank starts at scanline 225 */
    if (ppu->vcount == 225) {
        ppu->vblank = true;
    }
    
    /* Render scanline if in visible area */
    if (ppu->vcount < SCREEN_HEIGHT && !ppu->forced_blank) {
        ppu_render_scanline(ppu);
    }
}

bool ppu_in_vblank(const PPU *ppu) {
    return ppu->vblank;
}

void ppu_write_register(PPU *ppu, u16 address, u8 value) {
    switch (address) {
        case PPU_INIDISP:  /* Display Control */
            ppu->brightness = value & 0x0F;
            ppu->forced_blank = (value & 0x80) != 0;
            break;
            
        case PPU_BGMODE:  /* BG Mode */
            ppu->bg_mode = value & 0x07;
            ppu->bg[0].enabled = true;  /* BG1 typically enabled */
            ppu->bg[1].enabled = (ppu->bg_mode <= 1);  /* BG2 in modes 0-1 */
            ppu->bg[2].enabled = (ppu->bg_mode == 0);  /* BG3 in mode 0 */
            ppu->bg[3].enabled = (ppu->bg_mode == 0);  /* BG4 in mode 0 */
            break;
            
        case PPU_BG1SC:  /* BG1 Tilemap Address */
            ppu->bg[0].tilemap_addr = (value & 0xFC) << 8;
            ppu->bg[0].size = value & 0x03;
            break;
            
        case PPU_BG2SC:  /* BG2 Tilemap Address */
            ppu->bg[1].tilemap_addr = (value & 0xFC) << 8;
            ppu->bg[1].size = value & 0x03;
            break;
            
        case PPU_BG3SC:  /* BG3 Tilemap Address */
            ppu->bg[2].tilemap_addr = (value & 0xFC) << 8;
            ppu->bg[2].size = value & 0x03;
            break;
            
        case PPU_BG4SC:  /* BG4 Tilemap Address */
            ppu->bg[3].tilemap_addr = (value & 0xFC) << 8;
            ppu->bg[3].size = value & 0x03;
            break;
            
        case PPU_BG12NBA:  /* BG1 & BG2 Chr Address */
            ppu->bg[0].chr_addr = (value & 0x0F) << 12;
            ppu->bg[1].chr_addr = (value & 0xF0) << 8;
            break;
            
        case PPU_BG34NBA:  /* BG3 & BG4 Chr Address */
            ppu->bg[2].chr_addr = (value & 0x0F) << 12;
            ppu->bg[3].chr_addr = (value & 0xF0) << 8;
            break;
            
        case PPU_CGADD:  /* CGRAM Address */
            ppu->cgram_addr = value;
            ppu->cgram_latch = false;
            break;
            
        case PPU_CGDATA:  /* CGRAM Data */
            if (!ppu->cgram_latch) {
                ppu->cgram_buffer = value;
                ppu->cgram_latch = true;
            } else {
                if (ppu->cgram && ppu->cgram_addr < CGRAM_SIZE / 2) {
                    ppu->cgram[ppu->cgram_addr * 2] = ppu->cgram_buffer;
                    ppu->cgram[ppu->cgram_addr * 2 + 1] = value & 0x7F;
                }
                ppu->cgram_addr = (ppu->cgram_addr + 1) & 0xFF;
                ppu->cgram_latch = false;
            }
            break;
            
        /* Mode 7 registers */
        case 0x211A:  /* M7SEL - Mode 7 Settings */
            ppu->m7_repeat = value & 0x03;
            ppu->m7_h_flip = (value & 0x01) != 0;
            ppu->m7_v_flip = (value & 0x02) != 0;
            break;
            
        case 0x211B:  /* M7A - Mode 7 Matrix A (low) / (high) */
            if (!ppu->m7_latch) {
                ppu->m7_latch = value;
            } else {
                ppu->m7_matrix_a = (s16)((value << 8) | ppu->m7_latch);
                ppu->m7_latch = 0;
            }
            break;
            
        case 0x211C:  /* M7B - Mode 7 Matrix B (low) / (high) */
            if (!ppu->m7_latch) {
                ppu->m7_latch = value;
            } else {
                ppu->m7_matrix_b = (s16)((value << 8) | ppu->m7_latch);
                ppu->m7_latch = 0;
            }
            break;
            
        case 0x211D:  /* M7C - Mode 7 Matrix C (low) / (high) */
            if (!ppu->m7_latch) {
                ppu->m7_latch = value;
            } else {
                ppu->m7_matrix_c = (s16)((value << 8) | ppu->m7_latch);
                ppu->m7_latch = 0;
            }
            break;
            
        case 0x211E:  /* M7D - Mode 7 Matrix D (low) / (high) */
            if (!ppu->m7_latch) {
                ppu->m7_latch = value;
            } else {
                ppu->m7_matrix_d = (s16)((value << 8) | ppu->m7_latch);
                ppu->m7_latch = 0;
            }
            break;
            
        case 0x211F:  /* M7X - Mode 7 Center X (low) / (high) */
            if (!ppu->m7_latch) {
                ppu->m7_latch = value;
            } else {
                ppu->m7_center_x = (s16)((value << 8) | ppu->m7_latch);
                ppu->m7_latch = 0;
            }
            break;
            
        case 0x2120:  /* M7Y - Mode 7 Center Y (low) / (high) */
            if (!ppu->m7_latch) {
                ppu->m7_latch = value;
            } else {
                ppu->m7_center_y = (s16)((value << 8) | ppu->m7_latch);
                ppu->m7_latch = 0;
            }
            break;
            
        default:
            /* Unhandled register */
            break;
    }
}

u8 ppu_read_register(PPU *ppu, u16 address) {
    if (!ppu) {
        return 0;
    }
    
    /* Most PPU registers are write-only, but some status registers are readable */
    switch (address) {
        case 0x2137:  /* SLHV - Software Latch for H/V Counter */
            /* Latch H/V counters (no return value, write-only) */
            return 0;
            
        case 0x2138:  /* OAMDATAREAD - OAM Data Read */
            /* Read from OAM at current address */
            if (ppu->oam && ppu->oam_addr < 544) {  /* 512 bytes main OAM + 32 bytes high table */
                u8 value = ppu->oam[ppu->oam_addr];
                ppu->oam_addr = (ppu->oam_addr + 1) & 0x1FF;
                return value;
            }
            return 0;
            
        case 0x2139:  /* VMDATALREAD - VRAM Data Read (low byte) */
            /* Read low byte of VRAM data */
            if (ppu->vram && ppu->vram_addr * 2 + 1 < VRAM_SIZE) {
                return ppu->vram[ppu->vram_addr * 2];
            }
            return 0;
            
        case 0x213A:  /* VMDATAHREAD - VRAM Data Read (high byte) */
            /* Read high byte of VRAM data */
            if (ppu->vram && ppu->vram_addr * 2 + 1 < VRAM_SIZE) {
                u8 value = ppu->vram[ppu->vram_addr * 2 + 1];
                /* Increment VRAM address after high byte read */
                ppu->vram_addr += ppu->vram_increment;
                return value;
            }
            return 0;
            
        case 0x213B:  /* CGDATAREAD - CGRAM Data Read */
            /* Read from CGRAM at current address */
            if (ppu->cgram && ppu->cgram_addr * 2 + 1 < CGRAM_SIZE) {
                u8 value;
                if (!ppu->cgram_latch) {
                    /* Read low byte */
                    value = ppu->cgram[ppu->cgram_addr * 2];
                    ppu->cgram_latch = true;
                } else {
                    /* Read high byte */
                    value = ppu->cgram[ppu->cgram_addr * 2 + 1];
                    ppu->cgram_latch = false;
                    ppu->cgram_addr = (ppu->cgram_addr + 1) & 0xFF;
                }
                return value;
            }
            return 0;
            
        case 0x213C:  /* OPHCT - Horizontal Scanline Location (low) */
            /* Return low 8 bits of horizontal counter */
            return ppu->hcount & 0xFF;
            
        case 0x213D:  /* OPVCT - Vertical Scanline Location (low) */
            /* Return low 8 bits of vertical counter */
            return ppu->vcount & 0xFF;
            
        case 0x213E:  /* STAT77 - PPU Status Flag and Version */
            /* Bit 7: Time over flag, Bit 6: Range over flag */
            /* Bits 4-0: PPU version (typically 1) */
            return 0x01;  /* PPU version 1, no time/range over */
            
        case 0x213F:  /* STAT78 - PPU Status Flag and Version */
            /* Bit 7: V-blank flag, Bit 6: H-blank flag */
            /* Bit 5: PAL/NTSC flag (0=NTSC, 1=PAL) */
            /* Bits 4-0: PPU version (typically 2) */
            {
                u8 status = 0x02;  /* PPU version 2, NTSC */
                if (ppu->vblank) {
                    status |= 0x80;  /* Set V-blank flag */
                }
                if (ppu->hblank) {
                    status |= 0x40;  /* Set H-blank flag */
                }
                return status;
            }
            
        default:
            /* Most PPU registers are write-only */
            return 0;
    }
}

u32 ppu_get_color(const PPU *ppu, u8 palette_index) {
    u16 color15;
    u8 r, g, b;
    
    if (!ppu->cgram) {
        return 0xFF000000;  /* Black */
    }
    
    /* Read 15-bit color from CGRAM (2 bytes per color) */
    color15 = ppu->cgram[palette_index * 2] | 
              ((ppu->cgram[palette_index * 2 + 1] & 0x7F) << 8);
    
    /* Convert 5-bit RGB to 8-bit RGB */
    r = (color15 & 0x1F) << 3;
    g = ((color15 >> 5) & 0x1F) << 3;
    b = ((color15 >> 10) & 0x1F) << 3;
    
    /* Return RGBA (0xAABBGGRR format for little-endian) */
    return 0xFF000000 | (b << 16) | (g << 8) | r;
}

void ppu_render_scanline(PPU *ppu) {
    int x, i;
    u32 *line;
    
    if (!ppu->framebuffer || ppu->vcount >= SCREEN_HEIGHT) {
        return;
    }
    
    line = &ppu->framebuffer[ppu->vcount * SCREEN_WIDTH];
    
    /* Fill with background color (palette entry 0) */
    for (x = 0; x < SCREEN_WIDTH; x++) {
        line[x] = ppu_get_color(ppu, 0);
    }
    
    /* Check for Mode 7 rendering */
    if (ppu->bg_mode == 7) {
        ppu_render_mode7(ppu);
    } else {
        /* Render enabled background layers (back to front) */
        /* In mode 0: all 4 layers, mode 1: 2-3 layers */
        for (i = 3; i >= 0; i--) {
            if (ppu->bg[i].enabled) {
                ppu_render_background(ppu, i);
            }
        }
    }
    
    /* Render sprites on top of backgrounds */
    ppu_render_sprites(ppu);
    
    /* Apply brightness adjustment */
    if (ppu->brightness < 15) {
        for (x = 0; x < SCREEN_WIDTH; x++) {
            u32 pixel = line[x];
            u8 r = ((pixel & 0xFF) * ppu->brightness) / 15;
            u8 g = (((pixel >> 8) & 0xFF) * ppu->brightness) / 15;
            u8 b = (((pixel >> 16) & 0xFF) * ppu->brightness) / 15;
            line[x] = 0xFF000000 | (b << 16) | (g << 8) | r;
        }
    }
}

void ppu_render_background(PPU *ppu, u8 layer) {
    int x, y;
    u16 tile_x, tile_y;
    u16 tilemap_addr, tile_num, tile_addr;
    u16 tile_attr;
    u8 pixel_data[8];
    u8 palette;
    bool h_flip, v_flip;
    u16 priority;
    
    if (layer >= 4 || !ppu->bg[layer].enabled || !ppu->vram) {
        return;
    }
    
    /* Get current scanline */
    y = ppu->vcount;
    if (y >= SCREEN_HEIGHT) {
        return;
    }
    
    /* Calculate tile row based on scroll offset */
    tile_y = (y + ppu->bg[layer].v_scroll) / 8;
    
    /* Render each tile in the scanline */
    for (x = 0; x < SCREEN_WIDTH; x += 8) {
        int pixel_x, pixel_y;
        u8 bit0, bit1, color_index;
        
        /* Calculate tile column based on scroll offset */
        tile_x = (x + ppu->bg[layer].h_scroll) / 8;
        
        /* Get tilemap entry address */
        /* Tilemap format: 2 bytes per tile (tile number + attributes) */
        tilemap_addr = ppu->bg[layer].tilemap_addr + ((tile_y * 32 + tile_x) * 2);
        
        /* Clamp to VRAM size */
        if (tilemap_addr >= VRAM_SIZE - 1) {
            continue;
        }
        
        /* Read tile number and attributes */
        tile_num = ppu->vram[tilemap_addr] | ((ppu->vram[tilemap_addr + 1] & 0x03) << 8);
        tile_attr = ppu->vram[tilemap_addr + 1];
        
        /* Extract attributes */
        palette = (tile_attr >> 2) & 0x07;  /* Palette number (0-7) */
        priority = (tile_attr >> 5) & 0x01; /* Priority */
        h_flip = (tile_attr & 0x40) != 0;   /* Horizontal flip */
        v_flip = (tile_attr & 0x80) != 0;   /* Vertical flip */
        
        (void)priority;  /* Unused for now */
        
        /* Calculate tile graphics address */
        /* Each tile is 8x8 pixels, 2 bits per pixel (4 colors per tile) */
        /* In 2bpp mode: 16 bytes per tile (2 bitplanes) */
        tile_addr = ppu->bg[layer].chr_addr + (tile_num * 16);
        
        if (tile_addr >= VRAM_SIZE - 15) {
            continue;
        }
        
        /* Get the pixel row within this tile */
        pixel_y = (y + ppu->bg[layer].v_scroll) % 8;
        if (v_flip) {
            pixel_y = 7 - pixel_y;
        }
        
        /* Read the two bitplanes for this row */
        /* Bitplane 0 at offset 0, bitplane 1 at offset 1 (interleaved) */
        bit0 = ppu->vram[tile_addr + (pixel_y * 2)];
        bit1 = ppu->vram[tile_addr + (pixel_y * 2) + 1];
        
        /* Decode 8 pixels */
        for (pixel_x = 0; pixel_x < 8; pixel_x++) {
            int shift = h_flip ? pixel_x : (7 - pixel_x);
            
            /* Combine bits from both planes */
            color_index = ((bit0 >> shift) & 1) | (((bit1 >> shift) & 1) << 1);
            
            pixel_data[pixel_x] = color_index;
        }
        
        /* Draw pixels to framebuffer */
        for (pixel_x = 0; pixel_x < 8 && (x + pixel_x) < SCREEN_WIDTH; pixel_x++) {
            u8 color_idx = pixel_data[pixel_x];
            
            /* Skip transparent pixels (color 0) */
            if (color_idx == 0) {
                continue;
            }
            
            /* Calculate final palette index */
            u8 final_palette = (palette * 4) + color_idx;
            
            /* Draw pixel */
            ppu->framebuffer[y * SCREEN_WIDTH + x + pixel_x] = ppu_get_color(ppu, final_palette);
        }
    }
}

void ppu_render_sprites(PPU *ppu) {
    int sprite_idx, y;
    u8 sprite_y, sprite_tile, sprite_attr;
    u16 sprite_x;
    u8 palette, priority;
    bool h_flip, v_flip;
    int sprite_size_x = 8, sprite_size_y = 8;
    
    if (!ppu->oam || !ppu->vram) {
        return;
    }
    
    /* Current scanline */
    y = ppu->vcount;
    if (y >= SCREEN_HEIGHT) {
        return;
    }
    
    /* Render sprites (128 sprites in OAM) */
    /* OAM format: 4 bytes per sprite + 2 bits in high table */
    for (sprite_idx = 0; sprite_idx < 128; sprite_idx++) {
        int oam_offset = sprite_idx * 4;
        int pixel_y, pixel_x;
        u16 tile_addr;
        u8 bit0, bit1, color_index;
        
        /* Read sprite attributes from OAM */
        sprite_x = ppu->oam[oam_offset] | ((ppu->oam[512 + (sprite_idx >> 2)] >> ((sprite_idx & 3) * 2)) & 1) << 8;
        sprite_y = ppu->oam[oam_offset + 1];
        sprite_tile = ppu->oam[oam_offset + 2];
        sprite_attr = ppu->oam[oam_offset + 3];
        
        /* Extract attributes */
        palette = ((sprite_attr >> 1) & 0x07) + 8;  /* Sprite palettes 8-15 */
        priority = (sprite_attr >> 4) & 0x03;
        h_flip = (sprite_attr & 0x40) != 0;
        v_flip = (sprite_attr & 0x80) != 0;
        
        (void)priority;  /* Unused for now */
        
        /* Check if sprite is on this scanline */
        if (y < sprite_y || y >= (sprite_y + sprite_size_y)) {
            continue;
        }
        
        /* Calculate row within sprite */
        pixel_y = y - sprite_y;
        if (v_flip) {
            pixel_y = sprite_size_y - 1 - pixel_y;
        }
        
        /* Calculate tile graphics address */
        /* Sprites use 4bpp (16 colors), 32 bytes per 8x8 tile */
        tile_addr = (sprite_tile * 32);
        
        if (tile_addr >= VRAM_SIZE - 31) {
            continue;
        }
        
        /* Read 4 bitplanes for this row */
        bit0 = ppu->vram[tile_addr + (pixel_y * 2)];
        bit1 = ppu->vram[tile_addr + (pixel_y * 2) + 1];
        /* For 4bpp, would need planes 2 and 3 as well */
        
        /* Draw sprite pixels */
        for (pixel_x = 0; pixel_x < sprite_size_x; pixel_x++) {
            int screen_x = sprite_x + pixel_x;
            int shift;
            
            if (screen_x < 0 || screen_x >= SCREEN_WIDTH) {
                continue;
            }
            
            shift = h_flip ? pixel_x : (7 - pixel_x);
            
            /* Get color index (2bpp for now, simplified) */
            color_index = ((bit0 >> shift) & 1) | (((bit1 >> shift) & 1) << 1);
            
            /* Skip transparent pixels */
            if (color_index == 0) {
                continue;
            }
            
            /* Calculate final palette index */
            u8 final_palette = (palette * 4) + color_index;
            
            /* Draw pixel (sprites have priority over backgrounds) */
            ppu->framebuffer[y * SCREEN_WIDTH + screen_x] = ppu_get_color(ppu, final_palette);
        }
    }
}

void ppu_render_frame(PPU *ppu) {
    /* Full frame rendering - called once per frame */
    if (!ppu->needs_render) {
        return;
    }
    
    /* Rendering is done scanline-by-scanline in ppu_render_scanline() */
    /* This function can be used for post-processing or output */
    
    ppu->needs_render = false;
}

void ppu_output_ppm(const PPU *ppu, const char *filename) {
    FILE *f;
    int x, y;
    u32 pixel;
    u8 r, g, b;
    
    if (!ppu->framebuffer) {
        fprintf(stderr, "Error: No framebuffer to output\n");
        return;
    }
    
    f = fopen(filename, "wb");
    if (!f) {
        fprintf(stderr, "Error: Cannot create PPM file '%s'\n", filename);
        return;
    }
    
    /* Write PPM header */
    fprintf(f, "P6\n%d %d\n255\n", SCREEN_WIDTH, SCREEN_HEIGHT);
    
    /* Write pixel data */
    for (y = 0; y < SCREEN_HEIGHT; y++) {
        for (x = 0; x < SCREEN_WIDTH; x++) {
            pixel = ppu->framebuffer[y * SCREEN_WIDTH + x];
            r = pixel & 0xFF;
            g = (pixel >> 8) & 0xFF;
            b = (pixel >> 16) & 0xFF;
            fputc(r, f);
            fputc(g, f);
            fputc(b, f);
        }
    }
    
    fclose(f);
    printf("Frame output to %s\n", filename);
}

void ppu_render_mode7(PPU *ppu) {
    int x, y;
    s32 screen_x, screen_y;
    s32 world_x, world_y;
    s32 a, b, c, d;
    s32 center_x, center_y;
    u8 tile_data, color_index;
    u16 tile_addr;
    int tile_x, tile_y, pixel_x, pixel_y;
    
    if (!ppu->vram || ppu->bg_mode != 7) {
        return;
    }
    
    /* Get current scanline */
    y = ppu->vcount;
    if (y >= SCREEN_HEIGHT) {
        return;
    }
    
    /* Get transformation matrix parameters (8.8 fixed point) */
    a = ppu->m7_matrix_a;
    b = ppu->m7_matrix_b;
    c = ppu->m7_matrix_c;
    d = ppu->m7_matrix_d;
    center_x = ppu->m7_center_x;
    center_y = ppu->m7_center_y;
    
    /* Render each pixel on this scanline */
    for (x = 0; x < SCREEN_WIDTH; x++) {
        /* Convert screen coordinates to centered coordinates */
        screen_x = x - 128;
        screen_y = y - 112;
        
        /* Apply affine transformation: 
         * world_x = a * screen_x + b * screen_y + center_x
         * world_y = c * screen_x + d * screen_y + center_y
         */
        world_x = ((a * screen_x) >> 8) + ((b * screen_y) >> 8) + center_x;
        world_y = ((c * screen_x) >> 8) + ((d * screen_y) >> 8) + center_y;
        
        /* Handle repeat modes */
        switch (ppu->m7_repeat & 0x03) {
            case 0:  /* Repeat */
                world_x &= 0x3FF;  /* Wrap to 1024x1024 */
                world_y &= 0x3FF;
                break;
            case 1:  /* No repeat - transparent outside */
                if (world_x < 0 || world_x >= 1024 || world_y < 0 || world_y >= 1024) {
                    continue;  /* Skip this pixel */
                }
                break;
            case 2:  /* Repeat with tile 0 outside */
            case 3:
                if (world_x < 0 || world_x >= 1024 || world_y < 0 || world_y >= 1024) {
                    world_x = 0;
                    world_y = 0;
                }
                break;
        }
        
        /* Convert world coordinates to tile coordinates */
        tile_x = world_x >> 3;  /* Divide by 8 */
        tile_y = world_y >> 3;
        pixel_x = world_x & 7;  /* Modulo 8 */
        pixel_y = world_y & 7;
        
        /* Mode 7 uses a 128x128 tile map at the start of VRAM */
        /* Each tilemap entry is 1 byte (tile number) */
        tile_addr = (tile_y * 128 + tile_x) & 0x3FFF;
        
        u8 tile_num = ppu->vram[tile_addr];
        
        /* Mode 7 tiles are 8x8 pixels, 8bpp (1 byte per pixel) */
        /* Tile data starts at $0000 in VRAM, 64 bytes per tile */
        tile_addr = (tile_num * 64) + (pixel_y * 8) + pixel_x;
        
        tile_data = ppu->vram[tile_addr];
        color_index = tile_data;
        
        /* Skip transparent pixels (color 0) */
        if (color_index == 0) {
            continue;
        }
        
        /* Draw pixel to framebuffer */
        ppu->framebuffer[y * SCREEN_WIDTH + x] = ppu_get_color(ppu, color_index);
    }
}

/* ML Upscaling implementation */

void ppu_enable_upscaling(PPU *ppu, UpscaleMode mode) {
    u16 output_width, output_height;
    
    if (!ppu) {
        return;
    }
    
    /* Initialize upscaler if not already initialized */
    if (!ppu->upscaler) {
        ppu->upscaler = (Upscaler *)malloc(sizeof(Upscaler));
        if (!ppu->upscaler) {
            fprintf(stderr, "Failed to allocate upscaler\n");
            return;
        }
        upscaler_init(ppu->upscaler);
    }
    
    /* Set upscaling mode */
    upscaler_set_mode(ppu->upscaler, mode);
    
    /* Allocate upscaled buffer */
    upscaler_get_output_size(ppu->upscaler, SCREEN_WIDTH, SCREEN_HEIGHT,
                            &output_width, &output_height);
    
    if (ppu->upscaled_buffer) {
        free(ppu->upscaled_buffer);
    }
    
    ppu->upscaled_buffer = (u32 *)calloc(output_width * output_height, sizeof(u32));
    if (!ppu->upscaled_buffer) {
        fprintf(stderr, "Failed to allocate upscaled buffer\n");
        ppu->upscaling_enabled = false;
        return;
    }
    
    ppu->upscaling_enabled = true;
    
    printf("ML Upscaling enabled: %dx%d -> %dx%d\n", 
           SCREEN_WIDTH, SCREEN_HEIGHT, output_width, output_height);
}

void ppu_disable_upscaling(PPU *ppu) {
    if (!ppu) {
        return;
    }
    
    ppu->upscaling_enabled = false;
    
    if (ppu->upscaled_buffer) {
        free(ppu->upscaled_buffer);
        ppu->upscaled_buffer = NULL;
    }
    
    if (ppu->upscaler) {
        upscaler_cleanup(ppu->upscaler);
        free(ppu->upscaler);
        ppu->upscaler = NULL;
    }
}

const u32 *ppu_get_upscaled_framebuffer(const PPU *ppu, u16 *width, u16 *height) {
    if (!ppu || !ppu->upscaling_enabled || !ppu->upscaled_buffer || !ppu->upscaler) {
        return NULL;
    }
    
    /* Apply upscaling to current framebuffer */
    if (ppu->framebuffer) {
        upscaler_process(ppu->upscaler, ppu->framebuffer,
                        SCREEN_WIDTH, SCREEN_HEIGHT,
                        ppu->upscaled_buffer);
        
        /* Get output dimensions */
        if (width && height) {
            upscaler_get_output_size(ppu->upscaler, SCREEN_WIDTH, SCREEN_HEIGHT,
                                    width, height);
        }
        
        return ppu->upscaled_buffer;
    }
    
    return NULL;
}
