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
    
    /* Initialize script editor */
    gm->script_editor.num_commands = 0;
    gm->script_editor.current_command = 0;
    memset(gm->script_editor.script_buffer, 0, sizeof(gm->script_editor.script_buffer));
    
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
    printf("  5. Script Editor     - ROM patching scripts\n");
    printf("  6. ROM Information   - View ROM details\n");
    printf("  7. Save ROM          - Save changes to file\n");
    printf("  8. Exit              - Return to emulator\n");
    printf("\n");
    printf("Select option (1-8): ");
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
                gm->mode = GM_MODE_SCRIPT_EDITOR;
                gamemaker_script_editor(gm);
                break;
            case '6':
                gamemaker_show_rom_info(gm);
                break;
            case '7':
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
            case '8':
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
    char input[64];
    char choice;
    bool editing = true;
    
    printf("\n=== Tile Editor ===\n");
    
    /* Load initial tile */
    gamemaker_tile_load(gm, gm->tile_editor.current_tile);
    
    while (editing) {
        printf("\n");
        gamemaker_tile_editor_display(gm);
        
        printf("\nTile Editor Commands:\n");
        printf("  [n] Next tile    [p] Previous tile\n");
        printf("  [g] Go to tile   [e] Edit pixel\n");
        printf("  [c] Change palette   [z] Zoom in/out\n");
        printf("  [s] Save tile    [x] Export tile data\n");
        printf("  [i] Import tile  [v] View tile hex\n");
        printf("  [q] Return to main menu\n");
        printf("\nCommand: ");
        fflush(stdout);
        
        if (!fgets(input, sizeof(input), stdin)) {
            break;
        }
        
        choice = input[0];
        
        switch (choice) {
            case 'n':
            case 'N':
                gm->tile_editor.current_tile++;
                if (gm->tile_editor.current_tile >= 1024) {
                    gm->tile_editor.current_tile = 0;
                }
                gamemaker_tile_load(gm, gm->tile_editor.current_tile);
                gamemaker_set_status(gm, "Moved to next tile");
                break;
                
            case 'p':
            case 'P':
                if (gm->tile_editor.current_tile > 0) {
                    gm->tile_editor.current_tile--;
                } else {
                    gm->tile_editor.current_tile = 1023;
                }
                gamemaker_tile_load(gm, gm->tile_editor.current_tile);
                gamemaker_set_status(gm, "Moved to previous tile");
                break;
                
            case 'g':
            case 'G': {
                u16 tile_num;
                printf("Enter tile number (0-1023): ");
                if (scanf("%hu", &tile_num) == 1) {
                    getchar(); /* consume newline */
                    if (tile_num < 1024) {
                        gamemaker_tile_load(gm, tile_num);
                        gamemaker_set_status(gm, "Loaded tile");
                    } else {
                        gamemaker_set_status(gm, "Invalid tile number");
                    }
                }
                break;
            }
                
            case 'e':
            case 'E': {
                u8 x, y, color;
                printf("Enter pixel X (0-7): ");
                if (scanf("%hhu", &x) != 1 || x > 7) {
                    gamemaker_set_status(gm, "Invalid X coordinate");
                    getchar();
                    break;
                }
                printf("Enter pixel Y (0-7): ");
                if (scanf("%hhu", &y) != 1 || y > 7) {
                    gamemaker_set_status(gm, "Invalid Y coordinate");
                    getchar();
                    break;
                }
                printf("Enter color (0-3 for 2bpp): ");
                if (scanf("%hhu", &color) != 1 || color > 3) {
                    gamemaker_set_status(gm, "Invalid color");
                    getchar();
                    break;
                }
                getchar(); /* consume newline */
                gamemaker_tile_edit_pixel(gm, x, y, color);
                gamemaker_set_status(gm, "Pixel edited");
                break;
            }
                
            case 'c':
            case 'C': {
                u8 palette;
                printf("Enter palette (0-7): ");
                if (scanf("%hhu", &palette) == 1 && palette < 8) {
                    getchar(); /* consume newline */
                    gm->tile_editor.current_palette = palette;
                    gamemaker_set_status(gm, "Palette changed");
                } else {
                    gamemaker_set_status(gm, "Invalid palette");
                    getchar();
                }
                break;
            }
                
            case 'z':
            case 'Z':
                gm->tile_editor.zoom_level = (gm->tile_editor.zoom_level % 4) + 1;
                snprintf(gm->status_message, sizeof(gm->status_message),
                         "Zoom: %ux", gm->tile_editor.zoom_level);
                break;
                
            case 's':
            case 'S':
                gamemaker_tile_save(gm);
                break;
                
            case 'v':
            case 'V': {
                /* Display tile data in hex */
                u16 addr = gm->tile_editor.tile_addr;
                printf("\nTile data at VRAM $%04X:\n", addr);
                for (int i = 0; i < 16; i += 8) {
                    printf("  %04X: ", addr + i);
                    for (int j = 0; j < 8 && (i + j) < 16; j++) {
                        if (gm->mem && addr + i + j < VRAM_SIZE) {
                            printf("%02X ", gm->mem->vram[addr + i + j]);
                        } else {
                            printf("-- ");
                        }
                    }
                    printf("\n");
                }
                printf("\nPress Enter to continue...");
                getchar();
                break;
            }
                
            case 'x':
            case 'X': {
                char filename[256];
                printf("Enter filename to export: ");
                if (fgets(filename, sizeof(filename), stdin)) {
                    size_t len = strlen(filename);
                    if (len > 0 && filename[len-1] == '\n') {
                        filename[len-1] = '\0';
                    }
                    /* Export tile to file */
                    FILE *f = fopen(filename, "wb");
                    if (f && gm->mem) {
                        fwrite(&gm->mem->vram[gm->tile_editor.tile_addr], 1, 16, f);
                        fclose(f);
                        gamemaker_set_status(gm, "Tile exported");
                    } else {
                        gamemaker_set_status(gm, "Export failed");
                    }
                }
                break;
            }
                
            case 'i':
            case 'I': {
                char filename[256];
                printf("Enter filename to import: ");
                if (fgets(filename, sizeof(filename), stdin)) {
                    size_t len = strlen(filename);
                    if (len > 0 && filename[len-1] == '\n') {
                        filename[len-1] = '\0';
                    }
                    /* Import tile from file */
                    FILE *f = fopen(filename, "rb");
                    if (f && gm->mem) {
                        size_t read = fread(&gm->mem->vram[gm->tile_editor.tile_addr], 1, 16, f);
                        fclose(f);
                        if (read == 16) {
                            gm->tile_editor.modified = true;
                            gm->unsaved_changes = true;
                            gamemaker_set_status(gm, "Tile imported");
                        } else {
                            gamemaker_set_status(gm, "Import failed - invalid size");
                        }
                    } else {
                        gamemaker_set_status(gm, "Import failed - cannot open file");
                    }
                }
                break;
            }
                
            case 'q':
            case 'Q':
                if (gm->tile_editor.modified) {
                    if (gamemaker_confirm("Save changes before exiting?")) {
                        gamemaker_tile_save(gm);
                    }
                }
                editing = false;
                break;
                
            default:
                gamemaker_set_status(gm, "Unknown command");
                break;
        }
    }
    
    gm->mode = GM_MODE_MAIN_MENU;
}

void gamemaker_tile_editor_display(const GameMaker *gm) {
    int x, y, bit;
    
    printf("\n╔═══════════════════════════════════════════╗\n");
    printf("║          Tile Editor - Tile %04u        ║\n", gm->tile_editor.current_tile);
    printf("╚═══════════════════════════════════════════╝\n");
    
    printf("\nCurrent Tile: %u\n", gm->tile_editor.current_tile);
    printf("Tile Address: $%04X\n", gm->tile_editor.tile_addr);
    printf("Palette: %u\n", gm->tile_editor.current_palette);
    printf("Zoom: %ux\n", gm->tile_editor.zoom_level);
    printf("Modified: %s\n", gm->tile_editor.modified ? "Yes" : "No");
    
    /* Display tile visually (8x8 pixels, 2bpp format) */
    if (gm->mem && gm->tile_editor.tile_addr < VRAM_SIZE - 16) {
        printf("\nTile Visual (8x8, 2bpp):\n");
        printf("  ");
        for (x = 0; x < 8; x++) printf("%d ", x);
        printf("\n");
        
        for (y = 0; y < 8; y++) {
            printf("%d ", y);
            for (x = 0; x < 8; x++) {
                /* Read 2bpp tile data */
                u8 plane0 = gm->mem->vram[gm->tile_editor.tile_addr + y * 2];
                u8 plane1 = gm->mem->vram[gm->tile_editor.tile_addr + y * 2 + 1];
                
                bit = 7 - x;
                u8 color = ((plane0 >> bit) & 1) | (((plane1 >> bit) & 1) << 1);
                
                /* Display using ASCII characters */
                switch (color) {
                    case 0: printf("  "); break;  /* Transparent */
                    case 1: printf("░░"); break;  /* Color 1 */
                    case 2: printf("▒▒"); break;  /* Color 2 */
                    case 3: printf("██"); break;  /* Color 3 */
                }
            }
            printf("\n");
        }
    }
    
    printf("\nStatus: %s\n", gm->status_message);
}

void gamemaker_tile_edit_pixel(GameMaker *gm, u8 x, u8 y, u8 color) {
    if (!gm->mem || x > 7 || y > 7 || color > 3) {
        return;
    }
    
    u16 addr = gm->tile_editor.tile_addr + y * 2;
    
    if (addr + 1 >= VRAM_SIZE) {
        return;
    }
    
    u8 bit_mask = 1 << (7 - x);
    
    /* Set or clear bit in plane 0 */
    if (color & 1) {
        gm->mem->vram[addr] |= bit_mask;
    } else {
        gm->mem->vram[addr] &= ~bit_mask;
    }
    
    /* Set or clear bit in plane 1 */
    if (color & 2) {
        gm->mem->vram[addr + 1] |= bit_mask;
    } else {
        gm->mem->vram[addr + 1] &= ~bit_mask;
    }
    
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
    char input[64];
    char choice;
    bool editing = true;
    
    printf("\n=== Sprite Editor ===\n");
    
    while (editing) {
        printf("\n");
        gamemaker_sprite_editor_display(gm);
        
        printf("\nSprite Editor Commands:\n");
        printf("  [n] Next sprite    [p] Previous sprite\n");
        printf("  [g] Go to sprite   [x] Set X position\n");
        printf("  [y] Set Y position [t] Set tile number\n");
        printf("  [c] Change palette [h] Toggle H-flip\n");
        printf("  [v] Toggle V-flip  [s] Save changes\n");
        printf("  [l] List all sprites  [q] Return to main menu\n");
        printf("\nCommand: ");
        fflush(stdout);
        
        if (!fgets(input, sizeof(input), stdin)) {
            break;
        }
        
        choice = input[0];
        
        switch (choice) {
            case 'n':
            case 'N':
                gm->sprite_editor.current_sprite = (gm->sprite_editor.current_sprite + 1) % 128;
                gamemaker_set_status(gm, "Moved to next sprite");
                break;
                
            case 'p':
            case 'P':
                gm->sprite_editor.current_sprite = (gm->sprite_editor.current_sprite + 127) % 128;
                gamemaker_set_status(gm, "Moved to previous sprite");
                break;
                
            case 'g':
            case 'G': {
                u8 sprite_num;
                printf("Enter sprite number (0-127): ");
                if (scanf("%hhu", &sprite_num) == 1 && sprite_num < 128) {
                    getchar(); /* consume newline */
                    gm->sprite_editor.current_sprite = sprite_num;
                    gamemaker_set_status(gm, "Selected sprite");
                } else {
                    gamemaker_set_status(gm, "Invalid sprite number");
                    getchar();
                }
                break;
            }
                
            case 'x':
            case 'X': {
                u8 x_pos;
                printf("Enter X position (0-255): ");
                if (scanf("%hhu", &x_pos) == 1) {
                    getchar();
                    gm->sprite_editor.sprite_x = x_pos;
                    gamemaker_sprite_update(gm);
                } else {
                    gamemaker_set_status(gm, "Invalid X position");
                    getchar();
                }
                break;
            }
                
            case 'y':
            case 'Y': {
                u8 y_pos;
                printf("Enter Y position (0-255): ");
                if (scanf("%hhu", &y_pos) == 1) {
                    getchar();
                    gm->sprite_editor.sprite_y = y_pos;
                    gamemaker_sprite_update(gm);
                } else {
                    gamemaker_set_status(gm, "Invalid Y position");
                    getchar();
                }
                break;
            }
                
            case 't':
            case 'T': {
                u8 tile_num;
                printf("Enter tile number (0-255): ");
                if (scanf("%hhu", &tile_num) == 1) {
                    getchar();
                    gm->sprite_editor.sprite_tile = tile_num;
                    gamemaker_sprite_update(gm);
                } else {
                    gamemaker_set_status(gm, "Invalid tile number");
                    getchar();
                }
                break;
            }
                
            case 'c':
            case 'C': {
                u8 palette;
                printf("Enter palette (0-7): ");
                if (scanf("%hhu", &palette) == 1 && palette < 8) {
                    getchar();
                    gm->sprite_editor.sprite_palette = palette;
                    gamemaker_sprite_update(gm);
                } else {
                    gamemaker_set_status(gm, "Invalid palette");
                    getchar();
                }
                break;
            }
                
            case 'h':
            case 'H':
                gm->sprite_editor.h_flip = !gm->sprite_editor.h_flip;
                gamemaker_sprite_update(gm);
                snprintf(gm->status_message, sizeof(gm->status_message),
                         "H-flip: %s", gm->sprite_editor.h_flip ? "On" : "Off");
                break;
                
            case 'v':
            case 'V':
                gm->sprite_editor.v_flip = !gm->sprite_editor.v_flip;
                gamemaker_sprite_update(gm);
                snprintf(gm->status_message, sizeof(gm->status_message),
                         "V-flip: %s", gm->sprite_editor.v_flip ? "On" : "Off");
                break;
                
            case 's':
            case 'S':
                if (gm->mem) {
                    /* Write sprite data to OAM */
                    u8 idx = gm->sprite_editor.current_sprite;
                    u16 oam_offset = idx * 4;
                    
                    if (oam_offset + 3 < OAM_SIZE) {
                        gm->mem->oam[oam_offset] = gm->sprite_editor.sprite_x;
                        gm->mem->oam[oam_offset + 1] = gm->sprite_editor.sprite_y;
                        gm->mem->oam[oam_offset + 2] = gm->sprite_editor.sprite_tile;
                        
                        /* Attribute byte: VHOOPPPT */
                        u8 attr = (gm->sprite_editor.v_flip ? 0x80 : 0) |
                                  (gm->sprite_editor.h_flip ? 0x40 : 0) |
                                  ((gm->sprite_editor.sprite_palette & 7) << 1);
                        gm->mem->oam[oam_offset + 3] = attr;
                        
                        gm->unsaved_changes = true;
                        gamemaker_set_status(gm, "Sprite saved to OAM");
                    }
                }
                break;
                
            case 'l':
            case 'L':
                printf("\n=== All Sprites ===\n");
                printf("Idx  X    Y    Tile Pal  Flip\n");
                printf("---  ---  ---  ---- ---  ----\n");
                if (gm->mem) {
                    for (int i = 0; i < 128; i++) {
                        u16 oam_offset = i * 4;
                        if (oam_offset + 3 < OAM_SIZE) {
                            u8 x = gm->mem->oam[oam_offset];
                            u8 y = gm->mem->oam[oam_offset + 1];
                            u8 tile = gm->mem->oam[oam_offset + 2];
                            u8 attr = gm->mem->oam[oam_offset + 3];
                            u8 pal = (attr >> 1) & 7;
                            bool hf = (attr & 0x40) != 0;
                            bool vf = (attr & 0x80) != 0;
                            
                            printf("%3d  %3u  %3u  %4u  %u   %c%c\n",
                                   i, x, y, tile, pal,
                                   hf ? 'H' : '-',
                                   vf ? 'V' : '-');
                            
                            if ((i + 1) % 20 == 0 && i < 127) {
                                printf("Press Enter for more...");
                                getchar();
                            }
                        }
                    }
                }
                printf("\nPress Enter to continue...");
                getchar();
                break;
                
            case 'q':
            case 'Q':
                if (gm->unsaved_changes) {
                    if (gamemaker_confirm("Save sprite changes?")) {
                        /* Already saved to OAM, just confirm */
                        gamemaker_set_status(gm, "Changes saved");
                    }
                }
                editing = false;
                break;
                
            default:
                gamemaker_set_status(gm, "Unknown command");
                break;
        }
    }
    
    gm->mode = GM_MODE_MAIN_MENU;
}

void gamemaker_sprite_editor_display(const GameMaker *gm) {
    printf("\n╔═══════════════════════════════════════════╗\n");
    printf("║       Sprite Editor - Sprite %03u         ║\n", gm->sprite_editor.current_sprite);
    printf("╚═══════════════════════════════════════════╝\n");
    
    printf("\nSprite: %u\n", gm->sprite_editor.current_sprite);
    printf("Position: (%u, %u)\n", gm->sprite_editor.sprite_x, gm->sprite_editor.sprite_y);
    printf("Tile: %u\n", gm->sprite_editor.sprite_tile);
    printf("Palette: %u\n", gm->sprite_editor.sprite_palette);
    printf("H-Flip: %s, V-Flip: %s\n",
           gm->sprite_editor.h_flip ? "Yes" : "No",
           gm->sprite_editor.v_flip ? "Yes" : "No");
    
    /* Display current OAM data for this sprite */
    if (gm->mem) {
        u16 oam_offset = gm->sprite_editor.current_sprite * 4;
        if (oam_offset + 3 < OAM_SIZE) {
            printf("\nCurrent OAM Data:\n");
            printf("  X: %u, Y: %u\n", 
                   gm->mem->oam[oam_offset],
                   gm->mem->oam[oam_offset + 1]);
            printf("  Tile: %u\n", gm->mem->oam[oam_offset + 2]);
            printf("  Attr: $%02X\n", gm->mem->oam[oam_offset + 3]);
        }
    }
    
    printf("\nStatus: %s\n", gm->status_message);
}

void gamemaker_sprite_update(GameMaker *gm) {
    gm->unsaved_changes = true;
    gamemaker_set_status(gm, "Sprite updated");
}

/* Tilemap Editor implementation */

void gamemaker_tilemap_editor(GameMaker *gm) {
    char input[64];
    char choice;
    bool editing = true;
    
    printf("\n=== Tilemap Editor ===\n");
    
    while (editing) {
        printf("\n");
        gamemaker_tilemap_editor_display(gm);
        
        printf("\nTilemap Editor Commands:\n");
        printf("  [w/a/s/d] Move cursor  [p] Place tile\n");
        printf("  [t] Select tile        [c] Change palette\n");
        printf("  [l] Change layer (BG1-4)  [m] Toggle paint mode\n");
        printf("  [v] View tilemap area  [e] Export tilemap\n");
        printf("  [i] Import tilemap     [q] Return to main menu\n");
        printf("\nCommand: ");
        fflush(stdout);
        
        if (!fgets(input, sizeof(input), stdin)) {
            break;
        }
        
        choice = input[0];
        
        switch (choice) {
            case 'w':
            case 'W':
                if (gm->tilemap_editor.cursor_y > 0) {
                    gm->tilemap_editor.cursor_y--;
                }
                gamemaker_set_status(gm, "Cursor moved up");
                break;
                
            case 's':
            case 'S':
                if (gm->tilemap_editor.cursor_y < 31) {
                    gm->tilemap_editor.cursor_y++;
                }
                gamemaker_set_status(gm, "Cursor moved down");
                break;
                
            case 'a':
            case 'A':
                if (gm->tilemap_editor.cursor_x > 0) {
                    gm->tilemap_editor.cursor_x--;
                }
                gamemaker_set_status(gm, "Cursor moved left");
                break;
                
            case 'd':
            case 'D':
                if (gm->tilemap_editor.cursor_x < 31) {
                    gm->tilemap_editor.cursor_x++;
                }
                gamemaker_set_status(gm, "Cursor moved right");
                break;
                
            case 'p':
            case 'P':
                gamemaker_tilemap_place_tile(gm);
                break;
                
            case 't':
            case 'T': {
                u16 tile_num;
                printf("Enter tile number (0-1023): ");
                if (scanf("%hu", &tile_num) == 1 && tile_num < 1024) {
                    getchar();
                    gm->tilemap_editor.selected_tile = tile_num;
                    gamemaker_set_status(gm, "Tile selected");
                } else {
                    gamemaker_set_status(gm, "Invalid tile number");
                    getchar();
                }
                break;
            }
                
            case 'c':
            case 'C': {
                u8 palette;
                printf("Enter palette (0-7): ");
                if (scanf("%hhu", &palette) == 1 && palette < 8) {
                    getchar();
                    gm->tilemap_editor.selected_palette = palette;
                    gamemaker_set_status(gm, "Palette changed");
                } else {
                    gamemaker_set_status(gm, "Invalid palette");
                    getchar();
                }
                break;
            }
                
            case 'l':
            case 'L': {
                u8 layer;
                printf("Enter layer (1-4 for BG1-4): ");
                if (scanf("%hhu", &layer) == 1 && layer >= 1 && layer <= 4) {
                    getchar();
                    gm->tilemap_editor.current_layer = layer - 1;
                    snprintf(gm->status_message, sizeof(gm->status_message),
                             "Layer: BG%u", layer);
                } else {
                    gamemaker_set_status(gm, "Invalid layer");
                    getchar();
                }
                break;
            }
                
            case 'm':
            case 'M':
                gm->tilemap_editor.paint_mode = !gm->tilemap_editor.paint_mode;
                snprintf(gm->status_message, sizeof(gm->status_message),
                         "Paint mode: %s", gm->tilemap_editor.paint_mode ? "On" : "Off");
                break;
                
            case 'v':
            case 'V': {
                /* Display a portion of the tilemap around cursor */
                printf("\n=== Tilemap View (8x8 around cursor) ===\n");
                if (gm->mem) {
                    /* Calculate tilemap address for current layer */
                    /* Tilemap is 32x32 tiles, 2 bytes per entry */
                    u16 base_addr = gm->tilemap_editor.current_layer * 0x800;
                    
                    printf("   ");
                    for (int x = 0; x < 8; x++) {
                        int cx = (gm->tilemap_editor.cursor_x - 4 + x) & 0x1F;
                        printf("%02X ", cx);
                    }
                    printf("\n");
                    
                    for (int y = 0; y < 8; y++) {
                        int cy = (gm->tilemap_editor.cursor_y - 4 + y) & 0x1F;
                        printf("%02X ", cy);
                        
                        for (int x = 0; x < 8; x++) {
                            int cx = (gm->tilemap_editor.cursor_x - 4 + x) & 0x1F;
                            u16 offset = base_addr + (cy * 32 + cx) * 2;
                            
                            if (offset + 1 < VRAM_SIZE) {
                                u16 tile_data = gm->mem->vram[offset] | 
                                               (gm->mem->vram[offset + 1] << 8);
                                printf("%03X ", tile_data & 0x3FF);
                            } else {
                                printf("--- ");
                            }
                        }
                        printf("\n");
                    }
                }
                printf("\nPress Enter to continue...");
                getchar();
                break;
            }
                
            case 'q':
            case 'Q':
                editing = false;
                break;
                
            default:
                gamemaker_set_status(gm, "Unknown command");
                break;
        }
    }
    
    gm->mode = GM_MODE_MAIN_MENU;
}

void gamemaker_tilemap_editor_display(const GameMaker *gm) {
    printf("\n╔═══════════════════════════════════════════╗\n");
    printf("║       Tilemap Editor - BG%u               ║\n", gm->tilemap_editor.current_layer + 1);
    printf("╚═══════════════════════════════════════════╝\n");
    
    printf("\nLayer: BG%u\n", gm->tilemap_editor.current_layer + 1);
    printf("Cursor: (%u, %u)\n", gm->tilemap_editor.cursor_x, gm->tilemap_editor.cursor_y);
    printf("Selected Tile: %u\n", gm->tilemap_editor.selected_tile);
    printf("Palette: %u\n", gm->tilemap_editor.selected_palette);
    printf("Paint Mode: %s\n", gm->tilemap_editor.paint_mode ? "On" : "Off");
    
    /* Show current tile at cursor position */
    if (gm->mem) {
        u16 base_addr = gm->tilemap_editor.current_layer * 0x800;
        u16 offset = base_addr + (gm->tilemap_editor.cursor_y * 32 + 
                                  gm->tilemap_editor.cursor_x) * 2;
        
        if (offset + 1 < VRAM_SIZE) {
            u16 tile_data = gm->mem->vram[offset] | (gm->mem->vram[offset + 1] << 8);
            u16 tile_num = tile_data & 0x3FF;
            u8 palette = (tile_data >> 10) & 7;
            bool hflip = (tile_data & 0x4000) != 0;
            bool vflip = (tile_data & 0x8000) != 0;
            
            printf("\nCurrent Position Data:\n");
            printf("  Tile: %u, Palette: %u\n", tile_num, palette);
            printf("  Flip: %c%c\n", hflip ? 'H' : '-', vflip ? 'V' : '-');
        }
    }
    
    printf("\nStatus: %s\n", gm->status_message);
}

void gamemaker_tilemap_place_tile(GameMaker *gm) {
    if (!gm->mem) {
        return;
    }
    
    /* Calculate address in VRAM */
    u16 base_addr = gm->tilemap_editor.current_layer * 0x800;
    u16 offset = base_addr + (gm->tilemap_editor.cursor_y * 32 + 
                              gm->tilemap_editor.cursor_x) * 2;
    
    if (offset + 1 >= VRAM_SIZE) {
        gamemaker_set_status(gm, "Invalid tilemap address");
        return;
    }
    
    /* Build tile data word: VHPP CCCC TTTT TTTT */
    /* V = vertical flip, H = horizontal flip */
    /* PP = priority, CCCC = palette */
    /* TTTT TTTT TTTT = tile number (10 bits) */
    u16 tile_data = gm->tilemap_editor.selected_tile & 0x3FF;
    tile_data |= (gm->tilemap_editor.selected_palette & 7) << 10;
    
    /* Write to VRAM */
    gm->mem->vram[offset] = tile_data & 0xFF;
    gm->mem->vram[offset + 1] = (tile_data >> 8) & 0xFF;
    
    gm->unsaved_changes = true;
    gamemaker_set_status(gm, "Tile placed");
    
    /* Auto-advance cursor in paint mode */
    if (gm->tilemap_editor.paint_mode) {
        gm->tilemap_editor.cursor_x = (gm->tilemap_editor.cursor_x + 1) & 0x1F;
    }
}

void gamemaker_tilemap_move_cursor(GameMaker *gm, s16 dx, s16 dy) {
    gm->tilemap_editor.cursor_x += dx;
    gm->tilemap_editor.cursor_y += dy;
}

/* Palette Editor implementation */

void gamemaker_palette_editor(GameMaker *gm) {
    char input[64];
    char choice;
    bool editing = true;
    
    printf("\n=== Palette Editor ===\n");
    
    while (editing) {
        printf("\n");
        gamemaker_palette_editor_display(gm);
        
        printf("\nPalette Editor Commands:\n");
        printf("  [n] Next color     [p] Previous color\n");
        printf("  [P] Next palette   [O] Previous palette\n");
        printf("  [r] Set red (0-31)     [g] Set green (0-31)\n");
        printf("  [b] Set blue (0-31)    [h] Set hex value\n");
        printf("  [s] Save color     [v] View full palette\n");
        printf("  [e] Export palette [i] Import palette\n");
        printf("  [q] Return to main menu\n");
        printf("\nCommand: ");
        fflush(stdout);
        
        if (!fgets(input, sizeof(input), stdin)) {
            break;
        }
        
        choice = input[0];
        
        switch (choice) {
            case 'n':
            case 'N':
                gm->palette_editor.current_color = (gm->palette_editor.current_color + 1) & 0xF;
                /* Load color from CGRAM */
                if (gm->mem) {
                    u16 addr = (gm->palette_editor.current_palette * 16 + 
                               gm->palette_editor.current_color) * 2;
                    if (addr + 1 < CGRAM_SIZE) {
                        gm->palette_editor.color_value = 
                            gm->mem->cgram[addr] | (gm->mem->cgram[addr + 1] << 8);
                    }
                }
                gamemaker_set_status(gm, "Moved to next color");
                break;
                
            case 'p':
            case 'P':
                if (choice == 'p') {
                    gm->palette_editor.current_color = (gm->palette_editor.current_color + 15) & 0xF;
                    /* Load color from CGRAM */
                    if (gm->mem) {
                        u16 addr = (gm->palette_editor.current_palette * 16 + 
                                   gm->palette_editor.current_color) * 2;
                        if (addr + 1 < CGRAM_SIZE) {
                            gm->palette_editor.color_value = 
                                gm->mem->cgram[addr] | (gm->mem->cgram[addr + 1] << 8);
                        }
                    }
                    gamemaker_set_status(gm, "Moved to previous color");
                } else {
                    /* 'P' - Next palette */
                    gm->palette_editor.current_palette = (gm->palette_editor.current_palette + 1) & 0xF;
                    /* Load color from CGRAM */
                    if (gm->mem) {
                        u16 addr = (gm->palette_editor.current_palette * 16 + 
                                   gm->palette_editor.current_color) * 2;
                        if (addr + 1 < CGRAM_SIZE) {
                            gm->palette_editor.color_value = 
                                gm->mem->cgram[addr] | (gm->mem->cgram[addr + 1] << 8);
                        }
                    }
                    gamemaker_set_status(gm, "Moved to next palette");
                }
                break;
                
            case 'O':
                gm->palette_editor.current_palette = (gm->palette_editor.current_palette + 15) & 0xF;
                /* Load color from CGRAM */
                if (gm->mem) {
                    u16 addr = (gm->palette_editor.current_palette * 16 + 
                               gm->palette_editor.current_color) * 2;
                    if (addr + 1 < CGRAM_SIZE) {
                        gm->palette_editor.color_value = 
                            gm->mem->cgram[addr] | (gm->mem->cgram[addr + 1] << 8);
                    }
                }
                gamemaker_set_status(gm, "Moved to previous palette");
                break;
                
            case 'r':
            case 'R': {
                u8 red;
                printf("Enter red value (0-31): ");
                if (scanf("%hhu", &red) == 1 && red <= 31) {
                    getchar();
                    gm->palette_editor.color_value = 
                        (gm->palette_editor.color_value & 0x7FE0) | red;
                    gm->palette_editor.modified = true;
                    gamemaker_set_status(gm, "Red value set");
                } else {
                    gamemaker_set_status(gm, "Invalid red value");
                    getchar();
                }
                break;
            }
                
            case 'g':
            case 'G':
                if (choice == 'g') {
                    u8 green;
                    printf("Enter green value (0-31): ");
                    if (scanf("%hhu", &green) == 1 && green <= 31) {
                        getchar();
                        gm->palette_editor.color_value = 
                            (gm->palette_editor.color_value & 0x7C1F) | (green << 5);
                        gm->palette_editor.modified = true;
                        gamemaker_set_status(gm, "Green value set");
                    } else {
                        gamemaker_set_status(gm, "Invalid green value");
                        getchar();
                    }
                }
                break;
                
            case 'b':
            case 'B': {
                u8 blue;
                printf("Enter blue value (0-31): ");
                if (scanf("%hhu", &blue) == 1 && blue <= 31) {
                    getchar();
                    gm->palette_editor.color_value = 
                        (gm->palette_editor.color_value & 0x03FF) | (blue << 10);
                    gm->palette_editor.modified = true;
                    gamemaker_set_status(gm, "Blue value set");
                } else {
                    gamemaker_set_status(gm, "Invalid blue value");
                    getchar();
                }
                break;
            }
                
            case 'h':
            case 'H': {
                u16 hex_value;
                printf("Enter 15-bit hex value (0-7FFF): ");
                if (scanf("%hx", &hex_value) == 1 && hex_value <= 0x7FFF) {
                    getchar();
                    gm->palette_editor.color_value = hex_value;
                    gm->palette_editor.modified = true;
                    gamemaker_set_status(gm, "Hex value set");
                } else {
                    gamemaker_set_status(gm, "Invalid hex value");
                    getchar();
                }
                break;
            }
                
            case 's':
            case 'S':
                if (gm->mem) {
                    u16 addr = (gm->palette_editor.current_palette * 16 + 
                               gm->palette_editor.current_color) * 2;
                    if (addr + 1 < CGRAM_SIZE) {
                        gm->mem->cgram[addr] = gm->palette_editor.color_value & 0xFF;
                        gm->mem->cgram[addr + 1] = (gm->palette_editor.color_value >> 8) & 0xFF;
                        gm->palette_editor.modified = false;
                        gm->unsaved_changes = true;
                        gamemaker_set_status(gm, "Color saved to CGRAM");
                    }
                }
                break;
                
            case 'v':
            case 'V': {
                /* Display entire palette */
                printf("\n=== Palette %u (All Colors) ===\n", gm->palette_editor.current_palette);
                printf("Idx  RGB Value  R   G   B\n");
                printf("---  ---------  --  --  --\n");
                if (gm->mem) {
                    for (int i = 0; i < 16; i++) {
                        u16 addr = (gm->palette_editor.current_palette * 16 + i) * 2;
                        if (addr + 1 < CGRAM_SIZE) {
                            u16 color = gm->mem->cgram[addr] | (gm->mem->cgram[addr + 1] << 8);
                            u8 r = color & 0x1F;
                            u8 g = (color >> 5) & 0x1F;
                            u8 b = (color >> 10) & 0x1F;
                            printf("%2d   $%04X      %2u  %2u  %2u\n", i, color, r, g, b);
                        }
                    }
                }
                printf("\nPress Enter to continue...");
                getchar();
                break;
            }
                
            case 'e':
            case 'E': {
                char filename[256];
                printf("Enter filename to export palette: ");
                if (fgets(filename, sizeof(filename), stdin)) {
                    size_t len = strlen(filename);
                    if (len > 0 && filename[len-1] == '\n') {
                        filename[len-1] = '\0';
                    }
                    if (gamemaker_palette_export(gm, filename) == SUCCESS) {
                        gamemaker_set_status(gm, "Palette exported");
                    } else {
                        gamemaker_set_status(gm, "Export failed");
                    }
                }
                break;
            }
                
            case 'i':
            case 'I': {
                char filename[256];
                printf("Enter filename to import palette: ");
                if (fgets(filename, sizeof(filename), stdin)) {
                    size_t len = strlen(filename);
                    if (len > 0 && filename[len-1] == '\n') {
                        filename[len-1] = '\0';
                    }
                    if (gamemaker_palette_import(gm, filename) == SUCCESS) {
                        /* Reload current color */
                        if (gm->mem) {
                            u16 addr = (gm->palette_editor.current_palette * 16 + 
                                       gm->palette_editor.current_color) * 2;
                            if (addr + 1 < CGRAM_SIZE) {
                                gm->palette_editor.color_value = 
                                    gm->mem->cgram[addr] | (gm->mem->cgram[addr + 1] << 8);
                            }
                        }
                        gamemaker_set_status(gm, "Palette imported");
                    } else {
                        gamemaker_set_status(gm, "Import failed");
                    }
                }
                break;
            }
                
            case 'q':
            case 'Q':
                if (gm->palette_editor.modified) {
                    if (gamemaker_confirm("Save current color before exiting?")) {
                        /* Save code already implemented in 's' case */
                        if (gm->mem) {
                            u16 addr = (gm->palette_editor.current_palette * 16 + 
                                       gm->palette_editor.current_color) * 2;
                            if (addr + 1 < CGRAM_SIZE) {
                                gm->mem->cgram[addr] = gm->palette_editor.color_value & 0xFF;
                                gm->mem->cgram[addr + 1] = (gm->palette_editor.color_value >> 8) & 0xFF;
                                gm->unsaved_changes = true;
                            }
                        }
                    }
                }
                editing = false;
                break;
                
            default:
                gamemaker_set_status(gm, "Unknown command");
                break;
        }
    }
    
    gm->mode = GM_MODE_MAIN_MENU;
}

void gamemaker_palette_editor_display(const GameMaker *gm) {
    u8 r, g, b;
    
    printf("\n╔═══════════════════════════════════════════╗\n");
    printf("║       Palette Editor - Palette %2u         ║\n", gm->palette_editor.current_palette);
    printf("╚═══════════════════════════════════════════╝\n");
    
    printf("\nPalette: %u\n", gm->palette_editor.current_palette);
    printf("Color: %u\n", gm->palette_editor.current_color);
    
    /* Extract RGB from 15-bit value (BGR555 format) */
    r = (gm->palette_editor.color_value & 0x1F);
    g = ((gm->palette_editor.color_value >> 5) & 0x1F);
    b = ((gm->palette_editor.color_value >> 10) & 0x1F);
    
    printf("RGB: (%u, %u, %u)\n", r, g, b);
    printf("Hex: $%04X\n", gm->palette_editor.color_value);
    printf("Modified: %s\n", gm->palette_editor.modified ? "Yes" : "No");
    
    /* Visual representation */
    printf("\nColor Preview (approximate):\n");
    printf("  ");
    for (int i = 0; i < 10; i++) {
        printf("█");
    }
    printf("  [RGB: %2u,%2u,%2u]\n", r, g, b);
    
    printf("\nStatus: %s\n", gm->status_message);
}

void gamemaker_palette_set_color(GameMaker *gm, u16 color_value) {
    gm->palette_editor.color_value = color_value & 0x7FFF;
    gm->palette_editor.modified = true;
    gm->unsaved_changes = true;
}

int gamemaker_palette_export(const GameMaker *gm, const char *filename) {
    FILE *file;
    
    if (!gm->mem || !filename) {
        return ERROR;
    }
    
    file = fopen(filename, "wb");
    if (!file) {
        return ERROR;
    }
    
    /* Export current palette (16 colors, 32 bytes) */
    u16 base_addr = gm->palette_editor.current_palette * 32;
    
    if (base_addr + 32 <= CGRAM_SIZE) {
        fwrite(&gm->mem->cgram[base_addr], 1, 32, file);
    }
    
    fclose(file);
    return SUCCESS;
}

int gamemaker_palette_import(GameMaker *gm, const char *filename) {
    FILE *file;
    size_t bytes_read;
    
    if (!gm->mem || !filename) {
        return ERROR;
    }
    
    file = fopen(filename, "rb");
    if (!file) {
        return ERROR;
    }
    
    /* Import palette (16 colors, 32 bytes) */
    u16 base_addr = gm->palette_editor.current_palette * 32;
    
    if (base_addr + 32 <= CGRAM_SIZE) {
        bytes_read = fread(&gm->mem->cgram[base_addr], 1, 32, file);
        fclose(file);
        
        if (bytes_read == 32) {
            gm->unsaved_changes = true;
            return SUCCESS;
        }
    } else {
        fclose(file);
    }
    
    return ERROR;
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

/* Script Editor implementation */

void gamemaker_script_editor(GameMaker *gm) {
    char input[256];
    char choice;
    bool editing = true;
    
    printf("\n=== Script Editor ===\n");
    printf("Simple scripting language for ROM patching\n");
    
    while (editing) {
        printf("\n");
        gamemaker_script_editor_display(gm);
        
        printf("\nScript Editor Commands:\n");
        printf("  [c] Enter command       [e] Execute current command\n");
        printf("  [r] Run all commands    [l] List all commands\n");
        printf("  [d] Delete command      [x] Clear all commands\n");
        printf("  [s] Save script         [o] Load script\n");
        printf("  [h] Help (show syntax)  [q] Return to main menu\n");
        printf("\nCommand: ");
        fflush(stdout);
        
        if (!fgets(input, sizeof(input), stdin)) {
            break;
        }
        
        choice = input[0];
        
        switch (choice) {
            case 'c':
            case 'C': {
                printf("\nEnter script command:\n");
                printf("  SET <bank>:<addr> <value>    - Set byte at address\n");
                printf("  FILL <bank>:<addr> <value> <len> - Fill range\n");
                printf("  COPY <src_bank>:<src> <dst_bank>:<dst> <len> - Copy data\n");
                printf("\nCommand: ");
                if (fgets(input, sizeof(input), stdin)) {
                    if (gm->script_editor.num_commands < 256) {
                        /* Store command for later execution */
                        size_t len = strlen(input);
                        if (len > 0 && input[len-1] == '\n') {
                            input[len-1] = '\0';
                        }
                        strncpy(gm->script_editor.commands[gm->script_editor.num_commands].command,
                               input, 63);
                        gm->script_editor.num_commands++;
                        gamemaker_set_status(gm, "Command added");
                    } else {
                        gamemaker_set_status(gm, "Command buffer full");
                    }
                }
                break;
            }
                
            case 'e':
            case 'E': {
                if (gm->script_editor.current_command < gm->script_editor.num_commands) {
                    const char *cmd = gm->script_editor.commands[gm->script_editor.current_command].command;
                    if (gamemaker_script_execute_command(gm, cmd) == SUCCESS) {
                        gamemaker_set_status(gm, "Command executed");
                    } else {
                        gamemaker_set_status(gm, "Command failed");
                    }
                } else {
                    gamemaker_set_status(gm, "No command selected");
                }
                break;
            }
                
            case 'r':
            case 'R':
                if (gamemaker_script_run(gm) == SUCCESS) {
                    snprintf(gm->status_message, sizeof(gm->status_message),
                             "Executed %u commands", gm->script_editor.num_commands);
                } else {
                    gamemaker_set_status(gm, "Script execution failed");
                }
                break;
                
            case 'l':
            case 'L':
                printf("\n=== Script Commands ===\n");
                for (u16 i = 0; i < gm->script_editor.num_commands; i++) {
                    printf("%3u: %s\n", i, gm->script_editor.commands[i].command);
                }
                if (gm->script_editor.num_commands == 0) {
                    printf("(No commands)\n");
                }
                printf("\nPress Enter to continue...");
                getchar();
                break;
                
            case 'd':
            case 'D': {
                u16 idx;
                printf("Enter command index to delete: ");
                if (scanf("%hu", &idx) == 1) {
                    getchar();
                    if (idx < gm->script_editor.num_commands) {
                        /* Shift commands down */
                        for (u16 i = idx; i < gm->script_editor.num_commands - 1; i++) {
                            gm->script_editor.commands[i] = gm->script_editor.commands[i + 1];
                        }
                        gm->script_editor.num_commands--;
                        gamemaker_set_status(gm, "Command deleted");
                    } else {
                        gamemaker_set_status(gm, "Invalid index");
                    }
                } else {
                    getchar();
                }
                break;
            }
                
            case 'x':
            case 'X':
                if (gamemaker_confirm("Clear all commands?")) {
                    gm->script_editor.num_commands = 0;
                    gm->script_editor.current_command = 0;
                    gamemaker_set_status(gm, "Commands cleared");
                }
                break;
                
            case 's':
            case 'S': {
                char filename[256];
                printf("Enter filename to save script: ");
                if (fgets(filename, sizeof(filename), stdin)) {
                    size_t len = strlen(filename);
                    if (len > 0 && filename[len-1] == '\n') {
                        filename[len-1] = '\0';
                    }
                    if (gamemaker_script_save(gm, filename) == SUCCESS) {
                        gamemaker_set_status(gm, "Script saved");
                    } else {
                        gamemaker_set_status(gm, "Save failed");
                    }
                }
                break;
            }
                
            case 'o':
            case 'O': {
                char filename[256];
                printf("Enter filename to load script: ");
                if (fgets(filename, sizeof(filename), stdin)) {
                    size_t len = strlen(filename);
                    if (len > 0 && filename[len-1] == '\n') {
                        filename[len-1] = '\0';
                    }
                    if (gamemaker_script_load(gm, filename) == SUCCESS) {
                        gamemaker_set_status(gm, "Script loaded");
                    } else {
                        gamemaker_set_status(gm, "Load failed");
                    }
                }
                break;
            }
                
            case 'h':
            case 'H':
                printf("\n=== Script Syntax Help ===\n");
                printf("\nCommands:\n");
                printf("  SET <bank>:<addr> <value>\n");
                printf("    - Set byte at ROM address\n");
                printf("    - Example: SET 00:8000 FF\n");
                printf("\n  FILL <bank>:<addr> <value> <length>\n");
                printf("    - Fill range with value\n");
                printf("    - Example: FILL 00:8000 00 100\n");
                printf("\n  COPY <src_bank>:<src_addr> <dst_bank>:<dst_addr> <length>\n");
                printf("    - Copy data between addresses\n");
                printf("    - Example: COPY 00:8000 01:9000 256\n");
                printf("\nAddresses are in hexadecimal (bank:offset format)\n");
                printf("Values and lengths can be decimal or hex (prefix with 0x)\n");
                printf("\nPress Enter to continue...");
                getchar();
                break;
                
            case 'q':
            case 'Q':
                editing = false;
                break;
                
            default:
                gamemaker_set_status(gm, "Unknown command");
                break;
        }
    }
    
    gm->mode = GM_MODE_MAIN_MENU;
}

void gamemaker_script_editor_display(const GameMaker *gm) {
    printf("\n╔═══════════════════════════════════════════╗\n");
    printf("║          Script Editor                    ║\n");
    printf("╚═══════════════════════════════════════════╝\n");
    
    printf("\nCommands: %u/256\n", gm->script_editor.num_commands);
    printf("Current: %u\n", gm->script_editor.current_command);
    
    if (gm->script_editor.num_commands > 0) {
        printf("\nRecent commands:\n");
        u16 start = gm->script_editor.num_commands > 5 ? 
                    gm->script_editor.num_commands - 5 : 0;
        for (u16 i = start; i < gm->script_editor.num_commands; i++) {
            printf("  %3u: %s\n", i, gm->script_editor.commands[i].command);
        }
    }
    
    printf("\nStatus: %s\n", gm->status_message);
}

int gamemaker_script_execute_command(GameMaker *gm, const char *command) {
    char cmd[16];
    u8 bank, dst_bank, src_bank;
    u16 addr, dst_addr, src_addr;
    u8 value;
    u16 length;
    
    if (!gm->cart || !command) {
        return ERROR;
    }
    
    /* Parse command */
    if (sscanf(command, "%15s", cmd) != 1) {
        return ERROR;
    }
    
    /* SET command: SET bank:addr value */
    if (strcmp(cmd, "SET") == 0) {
        if (sscanf(command, "SET %hhx:%hx %hhx", &bank, &addr, &value) == 3) {
            u32 rom_addr = (bank * 0x10000) + addr;
            if (rom_addr < gm->cart->rom_size) {
                gm->cart->rom_data[rom_addr] = value;
                gm->unsaved_changes = true;
                return SUCCESS;
            }
        }
    }
    /* FILL command: FILL bank:addr value length */
    else if (strcmp(cmd, "FILL") == 0) {
        if (sscanf(command, "FILL %hhx:%hx %hhx %hu", &bank, &addr, &value, &length) == 4) {
            u32 rom_addr = (bank * 0x10000) + addr;
            if (rom_addr + length <= gm->cart->rom_size) {
                memset(&gm->cart->rom_data[rom_addr], value, length);
                gm->unsaved_changes = true;
                return SUCCESS;
            }
        }
    }
    /* COPY command: COPY src_bank:src_addr dst_bank:dst_addr length */
    else if (strcmp(cmd, "COPY") == 0) {
        if (sscanf(command, "COPY %hhx:%hx %hhx:%hx %hu", 
                  &src_bank, &src_addr, &dst_bank, &dst_addr, &length) == 5) {
            u32 src_rom = (src_bank * 0x10000) + src_addr;
            u32 dst_rom = (dst_bank * 0x10000) + dst_addr;
            if (src_rom + length <= gm->cart->rom_size && 
                dst_rom + length <= gm->cart->rom_size) {
                memmove(&gm->cart->rom_data[dst_rom], &gm->cart->rom_data[src_rom], length);
                gm->unsaved_changes = true;
                return SUCCESS;
            }
        }
    }
    
    return ERROR;
}

int gamemaker_script_load(GameMaker *gm, const char *filename) {
    FILE *file;
    char line[256];
    
    if (!filename) {
        return ERROR;
    }
    
    file = fopen(filename, "r");
    if (!file) {
        return ERROR;
    }
    
    /* Clear existing commands */
    gm->script_editor.num_commands = 0;
    
    /* Read commands from file */
    while (fgets(line, sizeof(line), file) && gm->script_editor.num_commands < 256) {
        size_t len = strlen(line);
        if (len > 0 && line[len-1] == '\n') {
            line[len-1] = '\0';
        }
        
        /* Skip empty lines and comments */
        if (len == 0 || line[0] == '#' || line[0] == '\n') {
            continue;
        }
        
        strncpy(gm->script_editor.commands[gm->script_editor.num_commands].command,
               line, 63);
        gm->script_editor.num_commands++;
    }
    
    fclose(file);
    return SUCCESS;
}

int gamemaker_script_save(const GameMaker *gm, const char *filename) {
    FILE *file;
    
    if (!filename) {
        return ERROR;
    }
    
    file = fopen(filename, "w");
    if (!file) {
        return ERROR;
    }
    
    /* Write header comment */
    fprintf(file, "# SNESE Game Maker Script\n");
    fprintf(file, "# Auto-generated script file\n\n");
    
    /* Write all commands */
    for (u16 i = 0; i < gm->script_editor.num_commands; i++) {
        fprintf(file, "%s\n", gm->script_editor.commands[i].command);
    }
    
    fclose(file);
    return SUCCESS;
}

int gamemaker_script_run(GameMaker *gm) {
    int errors = 0;
    
    for (u16 i = 0; i < gm->script_editor.num_commands; i++) {
        gm->script_editor.current_command = i;
        if (gamemaker_script_execute_command(gm, gm->script_editor.commands[i].command) != SUCCESS) {
            errors++;
            printf("Error executing command %u: %s\n", i, gm->script_editor.commands[i].command);
        }
    }
    
    return errors == 0 ? SUCCESS : ERROR;
}
