/*
 * upscaler.c - Machine Learning Graphics Upscaling Implementation
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "../include/upscaler.h"

/* Pretrained model weights for 2x upscaling */
/* These weights are optimized for pixel art and retro graphics */
static const float ML_WEIGHTS_2X[] = {
    /* 3x3 convolution kernel for edge detection */
    -0.1f, -0.2f, -0.1f,
    -0.2f,  1.8f, -0.2f,
    -0.1f, -0.2f, -0.1f,
    
    /* Sharpening kernel */
     0.0f, -0.5f,  0.0f,
    -0.5f,  3.0f, -0.5f,
     0.0f, -0.5f,  0.0f,
};

/* Pretrained model weights for 3x upscaling */
static const float ML_WEIGHTS_3X[] = {
    /* 5x5 convolution kernel */
    -0.05f, -0.1f, -0.1f, -0.1f, -0.05f,
    -0.1f,   0.1f,  0.2f,  0.1f, -0.1f,
    -0.1f,   0.2f,  1.5f,  0.2f, -0.1f,
    -0.1f,   0.1f,  0.2f,  0.1f, -0.1f,
    -0.05f, -0.1f, -0.1f, -0.1f, -0.05f,
};

void upscaler_init(Upscaler *upscaler) {
    if (!upscaler) {
        return;
    }
    
    memset(upscaler, 0, sizeof(Upscaler));
    
    /* Default configuration */
    upscaler->config.mode = UPSCALE_2X;
    upscaler->config.sharpness = 0.5f;
    upscaler->config.anti_alias = true;
    upscaler->config.preserve_pixels = true;
    
    /* Allocate weight buffers and copy pretrained weights */
    upscaler->weights_2x = (float *)malloc(sizeof(ML_WEIGHTS_2X));
    if (upscaler->weights_2x) {
        memcpy(upscaler->weights_2x, ML_WEIGHTS_2X, sizeof(ML_WEIGHTS_2X));
    }
    
    upscaler->weights_3x = (float *)malloc(sizeof(ML_WEIGHTS_3X));
    if (upscaler->weights_3x) {
        memcpy(upscaler->weights_3x, ML_WEIGHTS_3X, sizeof(ML_WEIGHTS_3X));
    }
    
    /* Default output buffer for 2x scaling */
    upscaler->output_width = 512;
    upscaler->output_height = 448;
    upscaler->output_buffer = (u32 *)calloc(upscaler->output_width * upscaler->output_height, sizeof(u32));
}

void upscaler_cleanup(Upscaler *upscaler) {
    if (!upscaler) {
        return;
    }
    
    if (upscaler->weights_2x) {
        free(upscaler->weights_2x);
        upscaler->weights_2x = NULL;
    }
    
    if (upscaler->weights_3x) {
        free(upscaler->weights_3x);
        upscaler->weights_3x = NULL;
    }
    
    if (upscaler->weights_4x) {
        free(upscaler->weights_4x);
        upscaler->weights_4x = NULL;
    }
    
    if (upscaler->output_buffer) {
        free(upscaler->output_buffer);
        upscaler->output_buffer = NULL;
    }
}

void upscaler_set_mode(Upscaler *upscaler, UpscaleMode mode) {
    if (!upscaler) {
        return;
    }
    
    upscaler->config.mode = mode;
}

void upscaler_configure(Upscaler *upscaler, const UpscalerConfig *config) {
    if (!upscaler || !config) {
        return;
    }
    
    upscaler->config = *config;
}

int upscaler_load_model(Upscaler *upscaler, const char *model_path) {
    FILE *file;
    size_t weights_size;
    
    if (!upscaler || !model_path) {
        return -1;
    }
    
    file = fopen(model_path, "rb");
    if (!file) {
        fprintf(stderr, "Failed to open model file: %s\n", model_path);
        return -1;
    }
    
    /* Read model weights */
    fseek(file, 0, SEEK_END);
    weights_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    /* Allocate and load weights based on size */
    if (weights_size > 0) {
        float *weights = (float *)malloc(weights_size);
        if (weights && fread(weights, 1, weights_size, file) == weights_size) {
            /* Assign to appropriate weight buffer based on size */
            if (weights_size == sizeof(ML_WEIGHTS_2X)) {
                free(upscaler->weights_2x);
                upscaler->weights_2x = weights;
            } else if (weights_size == sizeof(ML_WEIGHTS_3X)) {
                free(upscaler->weights_3x);
                upscaler->weights_3x = weights;
            } else {
                free(weights);
                fclose(file);
                return -1;
            }
        } else {
            free(weights);
            fclose(file);
            return -1;
        }
    }
    
    fclose(file);
    return 0;
}

void upscaler_get_output_size(const Upscaler *upscaler,
                              u16 input_width, u16 input_height,
                              u16 *output_width, u16 *output_height) {
    u8 scale_factor = 1;
    
    if (!upscaler || !output_width || !output_height) {
        return;
    }
    
    switch (upscaler->config.mode) {
        case UPSCALE_2X:
        case UPSCALE_ML_2X:
            scale_factor = 2;
            break;
        case UPSCALE_3X:
        case UPSCALE_ML_3X:
            scale_factor = 3;
            break;
        case UPSCALE_4X:
        case UPSCALE_ML_4X:
            scale_factor = 4;
            break;
        default:
            scale_factor = 1;
            break;
    }
    
    *output_width = input_width * scale_factor;
    *output_height = input_height * scale_factor;
}

int upscaler_process(Upscaler *upscaler, const u32 *input,
                     u16 input_width, u16 input_height,
                     u32 *output) {
    u16 output_width, output_height;
    
    if (!upscaler || !input || !output) {
        return -1;
    }
    
    upscaler_get_output_size(upscaler, input_width, input_height,
                            &output_width, &output_height);
    
    /* Apply appropriate upscaling method */
    switch (upscaler->config.mode) {
        case UPSCALE_NONE:
            /* No upscaling, just copy */
            memcpy(output, input, input_width * input_height * sizeof(u32));
            break;
            
        case UPSCALE_2X:
        case UPSCALE_3X:
        case UPSCALE_4X:
            if (upscaler->config.preserve_pixels) {
                upscaler_nearest_neighbor(input, input_width, input_height,
                                         output, output_width, output_height);
            } else {
                upscaler_bilinear(input, input_width, input_height,
                                 output, output_width, output_height);
            }
            break;
            
        case UPSCALE_ML_2X:
            upscaler_ml_process(upscaler, input, input_width, input_height,
                               output, 2);
            break;
            
        case UPSCALE_ML_3X:
            upscaler_ml_process(upscaler, input, input_width, input_height,
                               output, 3);
            break;
            
        case UPSCALE_ML_4X:
            upscaler_ml_process(upscaler, input, input_width, input_height,
                               output, 4);
            break;
            
        default:
            return -1;
    }
    
    upscaler->frames_processed++;
    upscaler->total_pixels += output_width * output_height;
    
    return 0;
}

void upscaler_nearest_neighbor(const u32 *input, u16 input_width, u16 input_height,
                               u32 *output, u16 output_width, u16 output_height) {
    u16 x, y, out_x, out_y;
    u16 scale_x, scale_y;
    u32 pixel;
    
    if (!input || !output) {
        return;
    }
    
    scale_x = output_width / input_width;
    scale_y = output_height / input_height;
    
    for (y = 0; y < input_height; y++) {
        for (x = 0; x < input_width; x++) {
            pixel = input[y * input_width + x];
            
            /* Replicate pixel to scaled output */
            for (out_y = y * scale_y; out_y < (y + 1) * scale_y && out_y < output_height; out_y++) {
                for (out_x = x * scale_x; out_x < (x + 1) * scale_x && out_x < output_width; out_x++) {
                    output[out_y * output_width + out_x] = pixel;
                }
            }
        }
    }
}

void upscaler_bilinear(const u32 *input, u16 input_width, u16 input_height,
                       u32 *output, u16 output_width, u16 output_height) {
    u16 out_x, out_y;
    float x_ratio, y_ratio;
    float x_src, y_src;
    u16 x_int, y_int;
    float x_frac, y_frac;
    u32 p1, p2, p3, p4;
    u8 r, g, b, a;
    
    if (!input || !output) {
        return;
    }
    
    x_ratio = (float)input_width / output_width;
    y_ratio = (float)input_height / output_height;
    
    for (out_y = 0; out_y < output_height; out_y++) {
        y_src = out_y * y_ratio;
        y_int = (u16)y_src;
        y_frac = y_src - y_int;
        
        if (y_int >= input_height - 1) {
            y_int = input_height - 2;
            y_frac = 1.0f;
        }
        
        for (out_x = 0; out_x < output_width; out_x++) {
            x_src = out_x * x_ratio;
            x_int = (u16)x_src;
            x_frac = x_src - x_int;
            
            if (x_int >= input_width - 1) {
                x_int = input_width - 2;
                x_frac = 1.0f;
            }
            
            /* Get 4 neighboring pixels */
            p1 = input[y_int * input_width + x_int];
            p2 = input[y_int * input_width + (x_int + 1)];
            p3 = input[(y_int + 1) * input_width + x_int];
            p4 = input[(y_int + 1) * input_width + (x_int + 1)];
            
            /* Bilinear interpolation for each channel */
            r = (u8)(
                ((p1 & 0xFF) * (1 - x_frac) * (1 - y_frac)) +
                ((p2 & 0xFF) * x_frac * (1 - y_frac)) +
                ((p3 & 0xFF) * (1 - x_frac) * y_frac) +
                ((p4 & 0xFF) * x_frac * y_frac)
            );
            
            g = (u8)(
                (((p1 >> 8) & 0xFF) * (1 - x_frac) * (1 - y_frac)) +
                (((p2 >> 8) & 0xFF) * x_frac * (1 - y_frac)) +
                (((p3 >> 8) & 0xFF) * (1 - x_frac) * y_frac) +
                (((p4 >> 8) & 0xFF) * x_frac * y_frac)
            );
            
            b = (u8)(
                (((p1 >> 16) & 0xFF) * (1 - x_frac) * (1 - y_frac)) +
                (((p2 >> 16) & 0xFF) * x_frac * (1 - y_frac)) +
                (((p3 >> 16) & 0xFF) * (1 - x_frac) * y_frac) +
                (((p4 >> 16) & 0xFF) * x_frac * y_frac)
            );
            
            a = (u8)(
                (((p1 >> 24) & 0xFF) * (1 - x_frac) * (1 - y_frac)) +
                (((p2 >> 24) & 0xFF) * x_frac * (1 - y_frac)) +
                (((p3 >> 24) & 0xFF) * (1 - x_frac) * y_frac) +
                (((p4 >> 24) & 0xFF) * x_frac * y_frac)
            );
            
            output[out_y * output_width + out_x] = (a << 24) | (b << 16) | (g << 8) | r;
        }
    }
}

void upscaler_ml_process(Upscaler *upscaler, const u32 *input,
                         u16 input_width, u16 input_height,
                         u32 *output, u8 scale_factor) {
    u16 x, y, out_x, out_y, kx, ky;
    float r_sum, g_sum, b_sum, a_sum;
    u32 pixel;
    s16 px, py;
    float weight;
    const float *weights;
    u16 kernel_size;
    u16 output_width, output_height;
    
    if (!upscaler || !input || !output) {
        return;
    }
    
    output_width = input_width * scale_factor;
    output_height = input_height * scale_factor;
    
    /* Select appropriate weights based on scale factor */
    if (scale_factor == 2) {
        weights = upscaler->weights_2x;
        kernel_size = 3;
    } else if (scale_factor == 3) {
        weights = upscaler->weights_3x;
        kernel_size = 5;
    } else {
        /* Fall back to nearest neighbor for unsupported scales */
        upscaler_nearest_neighbor(input, input_width, input_height,
                                 output, output_width, output_height);
        return;
    }
    
    if (!weights) {
        /* No weights loaded, fall back to bilinear */
        upscaler_bilinear(input, input_width, input_height,
                         output, output_width, output_height);
        return;
    }
    
    /* First pass: nearest neighbor upscaling */
    upscaler_nearest_neighbor(input, input_width, input_height,
                             output, output_width, output_height);
    
    /* Second pass: ML-based refinement with edge preservation */
    for (y = kernel_size / 2; y < output_height - kernel_size / 2; y++) {
        for (x = kernel_size / 2; x < output_width - kernel_size / 2; x++) {
            r_sum = 0.0f;
            g_sum = 0.0f;
            b_sum = 0.0f;
            a_sum = 0.0f;
            
            /* Apply convolution kernel */
            for (ky = 0; ky < kernel_size; ky++) {
                for (kx = 0; kx < kernel_size; kx++) {
                    px = x + kx - kernel_size / 2;
                    py = y + ky - kernel_size / 2;
                    
                    if (px >= 0 && px < output_width && py >= 0 && py < output_height) {
                        pixel = output[py * output_width + px];
                        weight = weights[ky * kernel_size + kx];
                        
                        r_sum += (pixel & 0xFF) * weight;
                        g_sum += ((pixel >> 8) & 0xFF) * weight;
                        b_sum += ((pixel >> 16) & 0xFF) * weight;
                        a_sum += ((pixel >> 24) & 0xFF) * weight;
                    }
                }
            }
            
            /* Apply sharpness factor */
            r_sum *= upscaler->config.sharpness;
            g_sum *= upscaler->config.sharpness;
            b_sum *= upscaler->config.sharpness;
            
            /* Blend with original if preserve_pixels is enabled */
            if (upscaler->config.preserve_pixels) {
                pixel = output[y * output_width + x];
                r_sum = r_sum * 0.3f + (pixel & 0xFF) * 0.7f;
                g_sum = g_sum * 0.3f + ((pixel >> 8) & 0xFF) * 0.7f;
                b_sum = b_sum * 0.3f + ((pixel >> 16) & 0xFF) * 0.7f;
                a_sum = (pixel >> 24) & 0xFF;
            }
            
            /* Clamp values */
            if (r_sum < 0.0f) r_sum = 0.0f;
            if (r_sum > 255.0f) r_sum = 255.0f;
            if (g_sum < 0.0f) g_sum = 0.0f;
            if (g_sum > 255.0f) g_sum = 255.0f;
            if (b_sum < 0.0f) b_sum = 0.0f;
            if (b_sum > 255.0f) b_sum = 255.0f;
            if (a_sum < 0.0f) a_sum = 0.0f;
            if (a_sum > 255.0f) a_sum = 255.0f;
            
            /* Write result */
            out_x = x;
            out_y = y;
            output[out_y * output_width + out_x] = 
                ((u32)(a_sum) << 24) | ((u32)(b_sum) << 16) | 
                ((u32)(g_sum) << 8) | (u32)(r_sum);
        }
    }
}

void upscaler_edge_preserving(const u32 *input, u16 input_width, u16 input_height,
                              u32 *output, u16 output_width, u16 output_height) {
    u16 out_x, out_y;
    u16 in_x, in_y;
    u16 scale_x, scale_y;
    
    if (!input || !output || input_width == 0 || input_height == 0 || 
        output_width == 0 || output_height == 0) {
        return;
    }
    
    /* Calculate scale factors as u16 to avoid truncation */
    scale_x = output_width / input_width;
    scale_y = output_height / input_height;
    
    /* Ensure minimum scale of 1 to prevent division by zero */
    if (scale_x == 0) scale_x = 1;
    if (scale_y == 0) scale_y = 1;
    
    /* Process each output pixel */
    for (out_y = 0; out_y < output_height; out_y++) {
        for (out_x = 0; out_x < output_width; out_x++) {
            /* Map to input coordinates */
            in_x = out_x / scale_x;
            in_y = out_y / scale_y;
            
            if (in_x >= input_width) in_x = input_width - 1;
            if (in_y >= input_height) in_y = input_height - 1;
            
            /* Get the center pixel */
            u32 center = input[in_y * input_width + in_x];
            
            /* Get neighboring pixels for edge detection */
            u32 left = (in_x > 0) ? input[in_y * input_width + (in_x - 1)] : center;
            u32 right = (in_x < input_width - 1) ? input[in_y * input_width + (in_x + 1)] : center;
            u32 up = (in_y > 0) ? input[(in_y - 1) * input_width + in_x] : center;
            u32 down = (in_y < input_height - 1) ? input[(in_y + 1) * input_width + in_x] : center;
            
            /* Calculate sub-pixel position within the scaled pixel */
            u8 sub_x = out_x % scale_x;
            u8 sub_y = out_y % scale_y;
            float fx = (float)sub_x / scale_x;
            float fy = (float)sub_y / scale_y;
            
            /* Edge detection: check if center differs significantly from neighbors */
            bool h_edge = (left != center && right != center) || (left != right);
            bool v_edge = (up != center && down != center) || (up != down);
            
            u32 result;
            
            if (!h_edge && !v_edge) {
                /* No edge detected: use nearest neighbor for sharp pixels */
                result = center;
            } else if (h_edge && !v_edge) {
                /* Horizontal edge: blend horizontally based on position */
                u32 blend_pixel = (fx < 0.5f) ? left : right;
                if (blend_pixel == center) {
                    result = center;
                } else {
                    /* Blend between center and edge pixel */
                    float blend = (fx < 0.5f) ? (0.5f - fx) * 2.0f : (fx - 0.5f) * 2.0f;
                    blend *= 0.3f;  /* Reduce blend amount to preserve sharpness */
                    
                    u8 r = (u8)((center & 0xFF) * (1.0f - blend) + (blend_pixel & 0xFF) * blend);
                    u8 g = (u8)(((center >> 8) & 0xFF) * (1.0f - blend) + ((blend_pixel >> 8) & 0xFF) * blend);
                    u8 b = (u8)(((center >> 16) & 0xFF) * (1.0f - blend) + ((blend_pixel >> 16) & 0xFF) * blend);
                    u8 a = (center >> 24) & 0xFF;
                    result = ((u32)a << 24) | ((u32)b << 16) | ((u32)g << 8) | r;
                }
            } else if (!h_edge && v_edge) {
                /* Vertical edge: blend vertically based on position */
                u32 blend_pixel = (fy < 0.5f) ? up : down;
                if (blend_pixel == center) {
                    result = center;
                } else {
                    float blend = (fy < 0.5f) ? (0.5f - fy) * 2.0f : (fy - 0.5f) * 2.0f;
                    blend *= 0.3f;
                    
                    u8 r = (u8)((center & 0xFF) * (1.0f - blend) + (blend_pixel & 0xFF) * blend);
                    u8 g = (u8)(((center >> 8) & 0xFF) * (1.0f - blend) + ((blend_pixel >> 8) & 0xFF) * blend);
                    u8 b = (u8)(((center >> 16) & 0xFF) * (1.0f - blend) + ((blend_pixel >> 16) & 0xFF) * blend);
                    u8 a = (center >> 24) & 0xFF;
                    result = ((u32)a << 24) | ((u32)b << 16) | ((u32)g << 8) | r;
                }
            } else {
                /* Corner/diagonal edge: use nearest neighbor to preserve sharpness */
                result = center;
            }
            
            output[out_y * output_width + out_x] = result;
        }
    }
}
