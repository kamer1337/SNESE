/*
 * upscaler_demo.c - Demonstration of ML upscaling functionality
 * 
 * This standalone program demonstrates the ML upscaling feature
 * without requiring the full emulator to link.
 */

#include <stdio.h>
#include <stdlib.h>
#include "../include/upscaler.h"

/* Create a simple test pattern */
void create_test_pattern(u32 *buffer, u16 width, u16 height) {
    u16 x, y;
    u32 color;
    
    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
            /* Create a checkerboard pattern with colored squares */
            if ((x / 8 + y / 8) % 2 == 0) {
                /* Red square */
                color = 0xFF0000FF;
            } else {
                /* Blue square */
                color = 0xFFFF0000;
            }
            
            /* Add some gradient */
            color = (color & 0xFF000000) | 
                   ((((color >> 16) & 0xFF) * x / width) << 16) |
                   ((((color >> 8) & 0xFF) * y / height) << 8) |
                   (color & 0xFF);
            
            buffer[y * width + x] = color;
        }
    }
}

/* Save buffer as PPM image */
void save_ppm(const char *filename, const u32 *buffer, u16 width, u16 height) {
    FILE *file;
    u16 x, y;
    u32 pixel;
    u8 r, g, b;
    
    file = fopen(filename, "wb");
    if (!file) {
        fprintf(stderr, "Failed to open %s\n", filename);
        return;
    }
    
    fprintf(file, "P6\n%d %d\n255\n", width, height);
    
    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
            pixel = buffer[y * width + x];
            r = pixel & 0xFF;
            g = (pixel >> 8) & 0xFF;
            b = (pixel >> 16) & 0xFF;
            fputc(r, file);
            fputc(g, file);
            fputc(b, file);
        }
    }
    
    fclose(file);
    printf("Saved: %s (%dx%d)\n", filename, width, height);
}

int main(void) {
    Upscaler upscaler;
    u32 *input_buffer;
    u32 *output_buffer;
    u16 input_width = 256;
    u16 input_height = 224;
    u16 output_width, output_height;
    UpscaleMode modes[] = {UPSCALE_2X, UPSCALE_ML_2X, UPSCALE_3X, UPSCALE_ML_3X};
    const char *mode_names[] = {"2x_nearest", "2x_ml", "3x_nearest", "3x_ml"};
    int i;
    char filename[256];
    
    printf("=== ML Graphics Upscaling Demo ===\n\n");
    
    /* Create input test pattern */
    input_buffer = (u32 *)malloc(input_width * input_height * sizeof(u32));
    if (!input_buffer) {
        fprintf(stderr, "Failed to allocate input buffer\n");
        return 1;
    }
    
    create_test_pattern(input_buffer, input_width, input_height);
    save_ppm("test_input.ppm", input_buffer, input_width, input_height);
    
    /* Initialize upscaler */
    upscaler_init(&upscaler);
    printf("Upscaler initialized\n\n");
    
    /* Test different upscaling modes */
    for (i = 0; i < 4; i++) {
        printf("Testing %s upscaling...\n", mode_names[i]);
        
        /* Set mode */
        upscaler_set_mode(&upscaler, modes[i]);
        upscaler_get_output_size(&upscaler, input_width, input_height,
                                &output_width, &output_height);
        
        printf("  Input:  %dx%d\n", input_width, input_height);
        printf("  Output: %dx%d\n", output_width, output_height);
        
        /* Allocate output buffer */
        output_buffer = (u32 *)malloc(output_width * output_height * sizeof(u32));
        if (!output_buffer) {
            fprintf(stderr, "Failed to allocate output buffer\n");
            continue;
        }
        
        /* Process */
        if (upscaler_process(&upscaler, input_buffer, input_width, input_height,
                            output_buffer) == 0) {
            snprintf(filename, sizeof(filename), "test_output_%s.ppm", mode_names[i]);
            save_ppm(filename, output_buffer, output_width, output_height);
            printf("  ✓ Success\n\n");
        } else {
            printf("  ✗ Failed\n\n");
        }
        
        free(output_buffer);
    }
    
    /* Cleanup */
    upscaler_cleanup(&upscaler);
    free(input_buffer);
    
    printf("Statistics:\n");
    printf("  Frames processed: %u\n", upscaler.frames_processed);
    printf("  Total pixels: %llu\n", (unsigned long long)upscaler.total_pixels);
    
    printf("\nDemo complete! Check the generated PPM files.\n");
    printf("To view: convert test_output_2x_ml.ppm test_output_2x_ml.png\n");
    
    return 0;
}