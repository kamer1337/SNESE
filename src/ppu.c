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
    int x;
    u32 *line;
    
    if (!ppu->framebuffer || ppu->vcount >= SCREEN_HEIGHT) {
        return;
    }
    
    line = &ppu->framebuffer[ppu->vcount * SCREEN_WIDTH];
    
    /* Simple rendering - fill with background color for now */
    for (x = 0; x < SCREEN_WIDTH; x++) {
        line[x] = ppu_get_color(ppu, 0);  /* Palette entry 0 is backdrop */
    }
    
    /* TODO: Render background layers and sprites */
    /* This would involve:
     * 1. Rendering each enabled BG layer based on mode
     * 2. Rendering sprites
     * 3. Applying priorities
     * 4. Applying brightness
     */
}

void ppu_render_background(PPU *ppu, u8 layer) {
    /* Placeholder for background rendering */
    (void)ppu;
    (void)layer;
    
    /* TODO: Implement tile fetching and rendering
     * 1. Calculate tile position based on scroll
     * 2. Fetch tile from tilemap in VRAM
     * 3. Fetch tile graphics from character data
     * 4. Apply palette
     * 5. Draw to layer buffer
     */
}

void ppu_render_sprites(PPU *ppu) {
    /* Placeholder for sprite rendering */
    (void)ppu;
    
    /* TODO: Implement sprite rendering
     * 1. Parse OAM data into sprite structures
     * 2. For each sprite:
     *    - Check if on current scanline
     *    - Fetch sprite graphics
     *    - Apply palette
     *    - Apply flipping
     *    - Draw to sprite layer buffer
     */
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
