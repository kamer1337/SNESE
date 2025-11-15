/*
 * game_maker.c - Built-in Game Maker implementation
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/game_maker.h"
#include "../include/cartridge.h"

void gamemaker_init(GameMaker *gm, Cartridge *cart, Memory *mem) {
    memset(gm, 0, sizeof(GameMaker));
    
    gm->cart = cart;
    gm->mem = mem;
    gm->mode = GM_MODE_MAIN_MENU;
    gm->running = true;
    gm->unsaved_changes = false;
    
    /* Initialize tile editor */
    gm->tile_editor.current_tile = 0;
    gm->tile_editor.tile_addr = 0;
    gm->tile_editor.current_palette = 0;
    gm->tile_editor.zoom_level = 1;
    gm->tile_editor.modified = false;
    
    /* Initialize sprite editor */
    gm->sprite_editor.current_sprite = 0;
    gm->sprite_editor.sprite_x = 128;
    gm->sprite_editor.sprite_y = 112;
    gm->sprite_editor.sprite_tile = 0;
    gm->sprite_editor.sprite_palette = 0;
    gm->sprite_editor.h_flip = false;
    gm->sprite_editor.v_flip = false;
    
    /* Initialize tilemap editor */
    gm->tilemap_editor.current_layer = 0;
    gm->tilemap_editor.cursor_x = 0;
    gm->tilemap_editor.cursor_y = 0;
    gm->tilemap_editor.selected_tile = 0;
    gm->tilemap_editor.selected_palette = 0;
    gm->tilemap_editor.paint_mode = false;
    
    /* Initialize palette editor */
    gm->palette_editor.current_palette = 0;
    gm->palette_editor.current_color = 0;
    gm->palette_editor.color_value = 0;
    gm->palette_editor.modified = false;
    
    gamemaker_set_status(gm, "Game Maker initialized");
}

void gamemaker_cleanup(GameMaker *gm) {
    if (gm->unsaved_changes) {
        printf("\nWarning: Unsaved changes will be lost.\n");
    }
}

void gamemaker_show_menu(GameMaker *gm) {
    printf("\n");
    printf("╔═══════════════════════════════════════════════════════╗\n");
    printf("║       SNESE - Built-in Game Maker                    ║\n");
    printf("║              Educational ROM Editor                  ║\n");
    printf("╚═══════════════════════════════════════════════════════╝\n");
    printf("\n");
    
    printf("Current ROM: %s\n", gm->cart ? gm->cart->filename : "None");
    printf("Status: %s\n", gm->status_message);
    printf("Unsaved changes: %s\n\n", gm->unsaved_changes ? "Yes" : "No");
    
    printf("Main Menu:\n");
    printf("  1. Tile Editor       - Edit tile graphics\n");
    printf("  2. Sprite Editor     - Edit sprite properties\n");
    printf("  3. Tilemap Editor    - Edit background tilemaps\n");
    printf("  4. Palette Editor    - Edit color palettes\n");
    printf("  5. ROM Information   - View ROM details\n");
    printf("  6. Save ROM          - Save changes to file\n");
    printf("  7. Exit              - Return to emulator\n");
    printf("\n");
    printf("Select option (1-7): ");
    fflush(stdout);
}

void gamemaker_process_input(GameMaker *gm, char input) {
    if (gm->mode == GM_MODE_MAIN_MENU) {
        switch (input) {
            case '1':
                gm->mode = GM_MODE_TILE_EDITOR;
                gamemaker_tile_editor(gm);
                break;
            case '2':
                gm->mode = GM_MODE_SPRITE_EDITOR;
                gamemaker_sprite_editor(gm);
                break;
            case '3':
                gm->mode = GM_MODE_TILEMAP_EDITOR;
                gamemaker_tilemap_editor(gm);
                break;
            case '4':
                gm->mode = GM_MODE_PALETTE_EDITOR;
                gamemaker_palette_editor(gm);
                break;
            case '5':
                gamemaker_show_rom_info(gm);
                break;
            case '6':
                {
                    char filename[256];
                    printf("Enter output filename: ");
                    if (fgets(filename, sizeof(filename), stdin)) {
                        /* Remove newline */
                        size_t len = strlen(filename);
                        if (len > 0 && filename[len-1] == '\n') {
                            filename[len-1] = '\0';
                        }
                        gamemaker_save_rom(gm, filename);
                    }
                }
                break;
            case '7':
                if (gm->unsaved_changes) {
                    if (gamemaker_confirm("You have unsaved changes. Exit anyway?")) {
                        gm->mode = GM_MODE_EXIT;
                        gm->running = false;
                    }
                } else {
                    gm->mode = GM_MODE_EXIT;
                    gm->running = false;
                }
                break;
            default:
                gamemaker_set_status(gm, "Invalid option");
                break;
        }
    }
}

void gamemaker_run(GameMaker *gm) {
    char input[16];
    
    printf("\n=== Entering Game Maker Mode ===\n");
    
    while (gm->running) {
        if (gm->mode == GM_MODE_MAIN_MENU) {
            gamemaker_show_menu(gm);
            
            if (fgets(input, sizeof(input), stdin)) {
                gamemaker_process_input(gm, input[0]);
            }
        }
    }
    
    printf("\n=== Exiting Game Maker Mode ===\n");
}

int gamemaker_save_rom(GameMaker *gm, const char *filename) {
    FILE *file;
    
    if (!gm->cart || !gm->cart->rom_data) {
        gamemaker_set_status(gm, "Error: No ROM loaded");
        return ERROR;
    }
    
    file = fopen(filename, "wb");
    if (!file) {
        gamemaker_set_status(gm, "Error: Cannot create output file");
        return ERROR;
    }
    
    /* Write ROM data */
    fwrite(gm->cart->rom_data, 1, gm->cart->rom_size, file);
    fclose(file);
    
    gm->unsaved_changes = false;
    snprintf(gm->status_message, sizeof(gm->status_message),
             "ROM saved to: %s", filename);
    
    printf("\n%s\n", gm->status_message);
    printf("Press Enter to continue...");
    fflush(stdout);
    getchar();
    
    return SUCCESS;
}

/* Tile Editor implementation */

void gamemaker_tile_editor(GameMaker *gm) {
    printf("\n=== Tile Editor ===\n");
    printf("(Placeholder - Full implementation in Phase 5)\n\n");
    
    gamemaker_tile_editor_display(gm);
    
    printf("\nTile Editor features:\n");
    printf("  - View and edit individual tiles\n");
    printf("  - Pixel-by-pixel editing\n");
    printf("  - Palette selection\n");
    printf("  - Zoom controls\n");
    printf("  - Export/Import tiles\n");
    printf("\nPress Enter to return to main menu...");
    fflush(stdout);
    getchar();
    
    gm->mode = GM_MODE_MAIN_MENU;
}

void gamemaker_tile_editor_display(const GameMaker *gm) {
    printf("\nCurrent Tile: %u\n", gm->tile_editor.current_tile);
    printf("Tile Address: $%04X\n", gm->tile_editor.tile_addr);
    printf("Palette: %u\n", gm->tile_editor.current_palette);
    printf("Zoom: %ux\n", gm->tile_editor.zoom_level);
    printf("Modified: %s\n", gm->tile_editor.modified ? "Yes" : "No");
}

void gamemaker_tile_edit_pixel(GameMaker *gm, u8 x, u8 y, u8 color) {
    (void)x;
    (void)y;
    (void)color;
    gm->tile_editor.modified = true;
    gm->unsaved_changes = true;
}

void gamemaker_tile_load(GameMaker *gm, u16 tile_num) {
    gm->tile_editor.current_tile = tile_num;
    gm->tile_editor.tile_addr = tile_num * 16;  /* 16 bytes per tile (2bpp) */
    gm->tile_editor.modified = false;
}

void gamemaker_tile_save(GameMaker *gm) {
    /* Save tile data back to VRAM */
    gm->tile_editor.modified = false;
    gamemaker_set_status(gm, "Tile saved");
}

/* Sprite Editor implementation */

void gamemaker_sprite_editor(GameMaker *gm) {
    printf("\n=== Sprite Editor ===\n");
    printf("(Placeholder - Full implementation in Phase 5)\n\n");
    
    gamemaker_sprite_editor_display(gm);
    
    printf("\nSprite Editor features:\n");
    printf("  - Select and modify sprites\n");
    printf("  - Position adjustment\n");
    printf("  - Tile assignment\n");
    printf("  - Palette selection\n");
    printf("  - Flip controls\n");
    printf("\nPress Enter to return to main menu...");
    fflush(stdout);
    getchar();
    
    gm->mode = GM_MODE_MAIN_MENU;
}

void gamemaker_sprite_editor_display(const GameMaker *gm) {
    printf("\nSprite: %u\n", gm->sprite_editor.current_sprite);
    printf("Position: (%u, %u)\n", gm->sprite_editor.sprite_x, gm->sprite_editor.sprite_y);
    printf("Tile: %u\n", gm->sprite_editor.sprite_tile);
    printf("Palette: %u\n", gm->sprite_editor.sprite_palette);
    printf("H-Flip: %s, V-Flip: %s\n",
           gm->sprite_editor.h_flip ? "Yes" : "No",
           gm->sprite_editor.v_flip ? "Yes" : "No");
}

void gamemaker_sprite_update(GameMaker *gm) {
    gm->unsaved_changes = true;
    gamemaker_set_status(gm, "Sprite updated");
}

/* Tilemap Editor implementation */

void gamemaker_tilemap_editor(GameMaker *gm) {
    printf("\n=== Tilemap Editor ===\n");
    printf("(Placeholder - Full implementation in Phase 5)\n\n");
    
    gamemaker_tilemap_editor_display(gm);
    
    printf("\nTilemap Editor features:\n");
    printf("  - Edit background layers\n");
    printf("  - Place and remove tiles\n");
    printf("  - Layer selection\n");
    printf("  - Paint mode\n");
    printf("  - Tile picker\n");
    printf("\nPress Enter to return to main menu...");
    fflush(stdout);
    getchar();
    
    gm->mode = GM_MODE_MAIN_MENU;
}

void gamemaker_tilemap_editor_display(const GameMaker *gm) {
    printf("\nLayer: BG%u\n", gm->tilemap_editor.current_layer + 1);
    printf("Cursor: (%u, %u)\n", gm->tilemap_editor.cursor_x, gm->tilemap_editor.cursor_y);
    printf("Selected Tile: %u\n", gm->tilemap_editor.selected_tile);
    printf("Palette: %u\n", gm->tilemap_editor.selected_palette);
    printf("Paint Mode: %s\n", gm->tilemap_editor.paint_mode ? "On" : "Off");
}

void gamemaker_tilemap_place_tile(GameMaker *gm) {
    gm->unsaved_changes = true;
    gamemaker_set_status(gm, "Tile placed");
}

void gamemaker_tilemap_move_cursor(GameMaker *gm, s16 dx, s16 dy) {
    gm->tilemap_editor.cursor_x += dx;
    gm->tilemap_editor.cursor_y += dy;
}

/* Palette Editor implementation */

void gamemaker_palette_editor(GameMaker *gm) {
    printf("\n=== Palette Editor ===\n");
    printf("(Placeholder - Full implementation in Phase 5)\n\n");
    
    gamemaker_palette_editor_display(gm);
    
    printf("\nPalette Editor features:\n");
    printf("  - Edit color palettes\n");
    printf("  - RGB value adjustment\n");
    printf("  - Visual color picker\n");
    printf("  - Import/Export palettes\n");
    printf("  - Preview colors\n");
    printf("\nPress Enter to return to main menu...");
    fflush(stdout);
    getchar();
    
    gm->mode = GM_MODE_MAIN_MENU;
}

void gamemaker_palette_editor_display(const GameMaker *gm) {
    u8 r, g, b;
    
    printf("\nPalette: %u\n", gm->palette_editor.current_palette);
    printf("Color: %u\n", gm->palette_editor.current_color);
    
    /* Extract RGB from 15-bit value */
    r = (gm->palette_editor.color_value & 0x1F);
    g = ((gm->palette_editor.color_value >> 5) & 0x1F);
    b = ((gm->palette_editor.color_value >> 10) & 0x1F);
    
    printf("RGB: (%u, %u, %u)\n", r, g, b);
    printf("Modified: %s\n", gm->palette_editor.modified ? "Yes" : "No");
}

void gamemaker_palette_set_color(GameMaker *gm, u16 color_value) {
    gm->palette_editor.color_value = color_value & 0x7FFF;
    gm->palette_editor.modified = true;
    gm->unsaved_changes = true;
}

int gamemaker_palette_export(const GameMaker *gm, const char *filename) {
    (void)gm;
    (void)filename;
    /* TODO: Export palette data */
    return SUCCESS;
}

int gamemaker_palette_import(GameMaker *gm, const char *filename) {
    (void)gm;
    (void)filename;
    /* TODO: Import palette data */
    return SUCCESS;
}

/* Utility functions */

void gamemaker_show_rom_info(const GameMaker *gm) {
    printf("\n=== ROM Information ===\n\n");
    
    if (gm->cart) {
        cartridge_print_info(gm->cart);
    } else {
        printf("No ROM loaded.\n");
    }
    
    printf("\nPress Enter to continue...");
    fflush(stdout);
    getchar();
}

void gamemaker_set_status(GameMaker *gm, const char *message) {
    strncpy(gm->status_message, message, sizeof(gm->status_message) - 1);
    gm->status_message[sizeof(gm->status_message) - 1] = '\0';
}

bool gamemaker_confirm(const char *message) {
    char input[16];
    
    printf("\n%s (y/n): ", message);
    fflush(stdout);
    
    if (fgets(input, sizeof(input), stdin)) {
        return (input[0] == 'y' || input[0] == 'Y');
    }
    
    return false;
}
