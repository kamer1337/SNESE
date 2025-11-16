/*
 * ppu.h - Picture Processing Unit emulation
 * 
 * Handles graphics rendering, backgrounds, sprites, and video output.
 * (To be fully implemented in Phase 3)
 */

#ifndef PPU_H
#define PPU_H

#include "types.h"
#include "upscaler.h"

/* Screen dimensions */
#define SCREEN_WIDTH  256
#define SCREEN_HEIGHT 224
#define SCANLINES_PER_FRAME 262  /* NTSC */

/* PPU Registers */
#define PPU_INIDISP  0x2100  /* Display Control */
#define PPU_OBSEL    0x2101  /* Object Size and Base */
#define PPU_OAMADDL  0x2102  /* OAM Address (low) */
#define PPU_OAMADDH  0x2103  /* OAM Address (high) */
#define PPU_BGMODE   0x2105  /* BG Mode and Character Size */
#define PPU_BG1SC    0x2107  /* BG1 Tilemap Address */
#define PPU_BG2SC    0x2108  /* BG2 Tilemap Address */
#define PPU_BG3SC    0x2109  /* BG3 Tilemap Address */
#define PPU_BG4SC    0x210A  /* BG4 Tilemap Address */
#define PPU_BG12NBA  0x210B  /* BG1 & BG2 Chr Address */
#define PPU_BG34NBA  0x210C  /* BG3 & BG4 Chr Address */
#define PPU_CGADD    0x2121  /* CGRAM Address */
#define PPU_CGDATA   0x2122  /* CGRAM Data */

/* Background layer structure */
typedef struct {
    u16 tilemap_addr;     /* Tilemap base address in VRAM */
    u16 chr_addr;         /* Character data base address */
    u16 h_scroll;         /* Horizontal scroll offset */
    u16 v_scroll;         /* Vertical scroll offset */
    u8 size;              /* Tilemap size (0-3) */
    bool enabled;         /* Layer enabled flag */
    u8 priority[2];       /* Priority levels */
} BGLayer;

/* Sprite/Object structure */
typedef struct {
    s16 x;                /* X position */
    u8 y;                 /* Y position */
    u8 tile;              /* Tile number */
    u8 palette;           /* Palette (0-7) */
    u8 priority;          /* Priority (0-3) */
    bool h_flip;          /* Horizontal flip */
    bool v_flip;          /* Vertical flip */
    bool size;            /* Size (small/large) */
} Sprite;

/* PPU structure */
typedef struct {
    /* Timing */
    u16 vcount;           /* Vertical counter (scanline) */
    u16 hcount;           /* Horizontal counter */
    bool vblank;          /* In vertical blank */
    bool hblank;          /* In horizontal blank */
    
    /* Display control */
    u8 brightness;        /* Screen brightness (0-15) */
    bool forced_blank;    /* Forced blank (screen off) */
    u8 bg_mode;           /* Background mode (0-7) */
    
    /* Background layers */
    BGLayer bg[4];        /* BG1, BG2, BG3, BG4 */
    
    /* Sprites/Objects */
    Sprite oam_sprites[128];  /* 128 sprites in OAM */
    u8 oam_priority[128];     /* Priority table */
    
    /* VRAM and palette */
    u8 *vram;             /* Pointer to VRAM (in Memory structure) */
    u8 *cgram;            /* Pointer to CGRAM (palette) */
    u8 *oam;              /* Pointer to OAM */
    
    /* CGRAM state */
    u16 cgram_addr;       /* CGRAM address pointer */
    bool cgram_latch;     /* CGRAM write latch */
    u8 cgram_buffer;      /* CGRAM buffer for writes */
    
    /* OAM state */
    u16 oam_addr;         /* OAM address pointer */
    u8 oam_buffer;        /* OAM buffer */
    
    /* VRAM state */
    u16 vram_addr;        /* VRAM address pointer */
    u8 vram_increment;    /* VRAM address increment */
    
    /* Mode 7 state */
    s16 m7_matrix_a;      /* Mode 7 matrix parameter A */
    s16 m7_matrix_b;      /* Mode 7 matrix parameter B */
    s16 m7_matrix_c;      /* Mode 7 matrix parameter C */
    s16 m7_matrix_d;      /* Mode 7 matrix parameter D */
    s16 m7_center_x;      /* Mode 7 center X */
    s16 m7_center_y;      /* Mode 7 center Y */
    u8 m7_repeat;         /* Mode 7 repeat mode */
    bool m7_h_flip;       /* Mode 7 horizontal flip */
    bool m7_v_flip;       /* Mode 7 vertical flip */
    u8 m7_latch;          /* Mode 7 write latch */
    
    /* Frame buffer */
    u32 *framebuffer;     /* RGBA pixel data (256x224) */
    u8 *layer_buffer[5];  /* Separate buffers for each layer + sprites */
    
    /* Render flags */
    bool needs_render;    /* Frame needs rendering */
    u32 frame_count;      /* Frame counter */
    
    /* ML Upscaling */
    Upscaler *upscaler;   /* ML upscaling context */
    bool upscaling_enabled; /* Enable ML upscaling */
    u32 *upscaled_buffer; /* Buffer for upscaled output */
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

/*
 * Write to PPU register
 */
void ppu_write_register(PPU *ppu, u16 address, u8 value);

/*
 * Read from PPU register
 */
u8 ppu_read_register(PPU *ppu, u16 address);

/*
 * Set memory pointers (VRAM, CGRAM, OAM)
 */
void ppu_set_memory(PPU *ppu, u8 *vram, u8 *cgram, u8 *oam);

/*
 * Render a single scanline
 */
void ppu_render_scanline(PPU *ppu);

/*
 * Output frame to PPM file
 */
void ppu_output_ppm(const PPU *ppu, const char *filename);

/*
 * Get pixel color from palette
 */
u32 ppu_get_color(const PPU *ppu, u8 palette_index);

/*
 * Render background layer
 */
void ppu_render_background(PPU *ppu, u8 layer);

/*
 * Render sprites/objects
 */
void ppu_render_sprites(PPU *ppu);

/*
 * Render Mode 7 background
 */
void ppu_render_mode7(PPU *ppu);

/*
 * Enable ML upscaling for rendered frames
 */
void ppu_enable_upscaling(PPU *ppu, UpscaleMode mode);

/*
 * Disable ML upscaling
 */
void ppu_disable_upscaling(PPU *ppu);

/*
 * Get upscaled framebuffer (if upscaling is enabled)
 * Returns NULL if upscaling is disabled or not ready
 */
const u32 *ppu_get_upscaled_framebuffer(const PPU *ppu, u16 *width, u16 *height);

#endif /* PPU_H */
