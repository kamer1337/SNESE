/*
 * ppu.c - Picture Processing Unit implementation
 */

#include <string.h>
#include "../include/ppu.h"

void ppu_init(PPU *ppu) {
    memset(ppu, 0, sizeof(PPU));
    ppu_reset(ppu);
}

void ppu_reset(PPU *ppu) {
    ppu->vcount = 0;
    ppu->hcount = 0;
    ppu->vblank = false;
    ppu->hblank = false;
    ppu->brightness = 15;
    ppu->forced_blank = true;
    ppu->bg_mode = 0;
    ppu->framebuffer = NULL;
}

void ppu_step_scanline(PPU *ppu) {
    ppu->vcount++;
    
    /* NTSC: 262 scanlines per frame */
    if (ppu->vcount >= 262) {
        ppu->vcount = 0;
        ppu->vblank = false;
    }
    
    /* VBlank starts at scanline 225 */
    if (ppu->vcount == 225) {
        ppu->vblank = true;
    }
}

bool ppu_in_vblank(const PPU *ppu) {
    return ppu->vblank;
}

void ppu_render_frame(PPU *ppu) {
    /* Placeholder - to be implemented in Phase 3 */
    (void)ppu;
}
