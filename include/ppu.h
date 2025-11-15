/*
 * ppu.h - Picture Processing Unit emulation
 * 
 * Handles graphics rendering, backgrounds, sprites, and video output.
 * (To be fully implemented in Phase 3)
 */

#ifndef PPU_H
#define PPU_H

#include "types.h"

/* Screen dimensions */
#define SCREEN_WIDTH  256
#define SCREEN_HEIGHT 224

/* PPU structure (basic placeholder for Phase 1) */
typedef struct {
    u16 vcount;           /* Vertical counter (scanline) */
    u16 hcount;           /* Horizontal counter */
    bool vblank;          /* In vertical blank */
    bool hblank;          /* In horizontal blank */
    
    u8 brightness;        /* Screen brightness (0-15) */
    bool forced_blank;    /* Forced blank (screen off) */
    
    /* Background settings (simplified for now) */
    u8 bg_mode;           /* Background mode (0-7) */
    
    /* Frame buffer (to be implemented) */
    u32 *framebuffer;     /* RGBA pixel data */
} PPU;

/* Function declarations */

/*
 * Initialize PPU
 */
void ppu_init(PPU *ppu);

/*
 * Reset PPU to power-on state
 */
void ppu_reset(PPU *ppu);

/*
 * Run PPU for one scanline
 */
void ppu_step_scanline(PPU *ppu);

/*
 * Check if in VBlank period
 */
bool ppu_in_vblank(const PPU *ppu);

/*
 * Render current frame (placeholder for Phase 3)
 */
void ppu_render_frame(PPU *ppu);

#endif /* PPU_H */
