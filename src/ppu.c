/*
 * ppu.c - Picture Processing Unit implementation
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/ppu.h"

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
            
        default:
            /* Unhandled register */
            break;
    }
}

u8 ppu_read_register(PPU *ppu, u16 address) {
    (void)address;  /* Unused for now */
    (void)ppu;
    return 0;  /* Placeholder */
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
    
    /* Render enabled background layers (back to front) */
    /* In mode 0: all 4 layers, mode 1: 2-3 layers, mode 7: 1 layer */
    for (i = 3; i >= 0; i--) {
        if (ppu->bg[i].enabled) {
            ppu_render_background(ppu, i);
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
