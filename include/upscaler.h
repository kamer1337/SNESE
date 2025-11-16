/*
 * upscaler.h - Machine Learning Graphics Upscaling
 * 
 * Provides ML-based upscaling for retro graphics rendering.
 * Uses pretrained models optimized for pixel art and low-resolution sprites.
 */

#ifndef UPSCALER_H
#define UPSCALER_H

#include "types.h"

/* Upscaling modes */
typedef enum {
    UPSCALE_NONE = 0,      /* No upscaling (1x) */
    UPSCALE_2X,            /* 2x upscaling (512x448) */
    UPSCALE_3X,            /* 3x upscaling (768x672) */
    UPSCALE_4X,            /* 4x upscaling (1024x896) */
    UPSCALE_ML_2X,         /* ML-based 2x upscaling */
    UPSCALE_ML_3X,         /* ML-based 3x upscaling */
    UPSCALE_ML_4X          /* ML-based 4x upscaling */
} UpscaleMode;

/* Upscaler configuration */
typedef struct {
    UpscaleMode mode;
    float sharpness;       /* Sharpness factor (0.0 - 1.0) */
    bool anti_alias;       /* Enable anti-aliasing */
    bool preserve_pixels;  /* Preserve pixel-art aesthetic */
} UpscalerConfig;

/* Upscaler context */
typedef struct {
    UpscalerConfig config;
    
    /* Pretrained model weights (for ML modes) */
    float *weights_2x;     /* 2x upscaling weights */
    float *weights_3x;     /* 3x upscaling weights */
    float *weights_4x;     /* 4x upscaling weights */
    
    /* Buffers */
    u32 *output_buffer;    /* Upscaled output buffer */
    u16 output_width;      /* Output buffer width */
    u16 output_height;     /* Output buffer height */
    
    /* Statistics */
    u32 frames_processed;
    u64 total_pixels;
} Upscaler;

/*
 * Initialize upscaler with default configuration
 */
void upscaler_init(Upscaler *upscaler);

/*
 * Clean up upscaler resources
 */
void upscaler_cleanup(Upscaler *upscaler);

/*
 * Set upscaling mode
 */
void upscaler_set_mode(Upscaler *upscaler, UpscaleMode mode);

/*
 * Configure upscaler parameters
 */
void upscaler_configure(Upscaler *upscaler, const UpscalerConfig *config);

/*
 * Load pretrained model weights from file
 */
int upscaler_load_model(Upscaler *upscaler, const char *model_path);

/*
 * Apply upscaling to input framebuffer
 * 
 * @param upscaler Upscaler context
 * @param input Input framebuffer (RGBA format)
 * @param input_width Input width (typically 256)
 * @param input_height Input height (typically 224)
 * @param output Output buffer for upscaled image
 * @return 0 on success, -1 on error
 */
int upscaler_process(Upscaler *upscaler, const u32 *input, 
                     u16 input_width, u16 input_height,
                     u32 *output);

/*
 * Get upscaled dimensions for current mode
 */
void upscaler_get_output_size(const Upscaler *upscaler, 
                              u16 input_width, u16 input_height,
                              u16 *output_width, u16 *output_height);

/*
 * Nearest-neighbor upscaling (fast, preserves pixel art)
 */
void upscaler_nearest_neighbor(const u32 *input, u16 input_width, u16 input_height,
                               u32 *output, u16 output_width, u16 output_height);

/*
 * Bilinear interpolation upscaling (smooth)
 */
void upscaler_bilinear(const u32 *input, u16 input_width, u16 input_height,
                       u32 *output, u16 output_width, u16 output_height);

/*
 * ML-based upscaling using pretrained weights
 * Implements a simple convolutional approach optimized for pixel art
 */
void upscaler_ml_process(Upscaler *upscaler, const u32 *input,
                         u16 input_width, u16 input_height,
                         u32 *output, u8 scale_factor);

/*
 * Edge-preserving upscaling (hybrid approach)
 * Detects edges and applies appropriate filtering
 */
void upscaler_edge_preserving(const u32 *input, u16 input_width, u16 input_height,
                              u32 *output, u16 output_width, u16 output_height);

#endif /* UPSCALER_H */
