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
    
    /* Initialize script context */
    script_init(&gm->script_ctx, cart, mem);
    
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
    printf("  5. Script Editor     - Run ROM modification scripts\n");
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
    if (!gm->cart || !gm->cart->rom_data) {
        gamemaker_set_status(gm, "Error: No ROM loaded");
        return ERROR;
    }
    
    /* Update ROM checksum before saving */
    cartridge_update_checksum(gm->cart);
    
    /* Save ROM using cartridge function */
    if (cartridge_save_rom(gm->cart, filename) != SUCCESS) {
        gamemaker_set_status(gm, "Error: Cannot save ROM file");
        return ERROR;
    }
    
    gm->unsaved_changes = false;
    snprintf(gm->status_message, sizeof(gm->status_message),
             "ROM saved to: %s", filename);
    
    printf("\n%s\n", gm->status_message);
    printf("Checksum updated: 0x%04X\n", gm->cart->header.checksum);
    printf("Press Enter to continue...");
    fflush(stdout);
    getchar();
    
    return SUCCESS;
}

/* Tile Editor implementation */

void gamemaker_tile_editor(GameMaker *gm) {
    char input[256];
    bool editing = true;
    
    printf("\n=== Tile Editor ===\n");
    printf("Edit tile graphics in ROM/VRAM\n\n");
    
    while (editing) {
        gamemaker_tile_editor_display(gm);
        
        printf("\nCommands:\n");
        printf("  l <num>  - Load tile number\n");
        printf("  s        - Save tile to ROM\n");
        printf("  v <addr> - View tile at VRAM address\n");
        printf("  p <pal>  - Change palette (0-7)\n");
        printf("  e <x> <y> <color> - Edit pixel at (x,y) with color\n");
        printf("  d        - Display tile data (hex)\n");
        printf("  b        - Return to main menu\n");
        printf("\nCommand: ");
        fflush(stdout);
        
        if (!fgets(input, sizeof(input), stdin)) {
            break;
        }
        
        /* Parse command */
        char cmd = input[0];
        switch (cmd) {
            case 'l': {
                u16 tile_num;
                if (sscanf(input + 1, "%hu", &tile_num) == 1) {
                    gamemaker_tile_load(gm, tile_num);
                    printf("Loaded tile %u\n", tile_num);
                } else {
                    printf("Usage: l <tile_number>\n");
                }
                break;
            }
            case 's':
                gamemaker_tile_save(gm);
                printf("Tile saved to ROM\n");
                break;
            case 'v': {
                u16 addr;
                if (sscanf(input + 1, "%hx", &addr) == 1) {
                    gm->tile_editor.tile_addr = addr;
                    printf("Viewing VRAM address $%04X\n", addr);
                } else {
                    printf("Usage: v <hex_address>\n");
                }
                break;
            }
            case 'p': {
                u8 pal;
                if (sscanf(input + 1, "%hhu", &pal) == 1 && pal < 8) {
                    gm->tile_editor.current_palette = pal;
                    printf("Palette set to %u\n", pal);
                } else {
                    printf("Usage: p <palette_0-7>\n");
                }
                break;
            }
            case 'e': {
                u8 x, y, color;
                if (sscanf(input + 1, "%hhu %hhu %hhu", &x, &y, &color) == 3) {
                    if (x < 8 && y < 8) {
                        gamemaker_tile_edit_pixel(gm, x, y, color);
                        printf("Pixel (%u,%u) set to color %u\n", x, y, color);
                    } else {
                        printf("Error: Coordinates must be 0-7\n");
                    }
                } else {
                    printf("Usage: e <x> <y> <color>\n");
                }
                break;
            }
            case 'd': {
                /* Display tile data */
                printf("\nTile data at address $%04X:\n", gm->tile_editor.tile_addr);
                if (gm->mem && gm->tile_editor.tile_addr + 16 <= VRAM_SIZE) {
                    for (u16 i = 0; i < 16; i++) {
                        if (i % 8 == 0) printf("  ");
                        printf("%02X ", gm->mem->vram[gm->tile_editor.tile_addr + i]);
                        if (i % 8 == 7) printf("\n");
                    }
                } else {
                    printf("  (Invalid address or no VRAM)\n");
                }
                break;
            }
            case 'b':
                editing = false;
                break;
            default:
                printf("Unknown command\n");
                break;
        }
        
        printf("\n");
    }
    
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
    /* SNES tiles are stored in 2bpp, 4bpp, or 8bpp format
     * For simplicity, we'll handle 2bpp (4 colors) here
     * Each tile is 8x8 pixels, stored as bitplanes
     */
    u16 addr = gm->tile_editor.tile_addr;
    
    if (!gm->mem || addr + 16 > VRAM_SIZE) {
        return;
    }
    
    /* Calculate bit position */
    u8 row_offset = y;
    u8 bit_pos = 7 - x;
    
    /* For 2bpp: 2 bytes per row, low bit plane first */
    u8 *low_plane = &gm->mem->vram[addr + row_offset * 2];
    u8 *high_plane = &gm->mem->vram[addr + row_offset * 2 + 1];
    
    /* Set pixel bits */
    if (color & 0x01) {
        *low_plane |= (1 << bit_pos);
    } else {
        *low_plane &= ~(1 << bit_pos);
    }
    
    if (color & 0x02) {
        *high_plane |= (1 << bit_pos);
    } else {
        *high_plane &= ~(1 << bit_pos);
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
    /* Save tile data from VRAM back to ROM
     * This requires mapping VRAM address to ROM address
     * For simplicity, we'll copy VRAM tile data to ROM at appropriate offset
     */
    if (!gm->tile_editor.modified) {
        gamemaker_set_status(gm, "No changes to save");
        return;
    }
    
    if (!gm->mem || !gm->cart || !gm->cart->rom_data) {
        gamemaker_set_status(gm, "Error: No ROM or VRAM available");
        return;
    }
    
    /* For now, we'll save the tile data to a known ROM offset
     * In a full implementation, this would map to the proper CHR data location
     */
    u16 vram_addr = gm->tile_editor.tile_addr;
    u32 rom_offset = 0x10000;  /* Example: character data starts at bank 2 */
    
    if (rom_offset + 16 < gm->cart->rom_size && vram_addr + 16 <= VRAM_SIZE) {
        /* Copy 16 bytes (one 2bpp tile) from VRAM to ROM */
        for (u16 i = 0; i < 16; i++) {
            cartridge_write_rom(gm->cart, rom_offset + vram_addr + i, 
                               gm->mem->vram[vram_addr + i]);
        }
        
        gm->tile_editor.modified = false;
        gm->unsaved_changes = true;
        gamemaker_set_status(gm, "Tile saved to ROM");
    } else {
        gamemaker_set_status(gm, "Error: Invalid address range");
    }
}

/* Sprite Editor implementation */

void gamemaker_sprite_editor(GameMaker *gm) {
    char input[256];
    bool editing = true;
    
    printf("\n=== Sprite Editor ===\n");
    printf("Edit sprite properties in OAM\n\n");
    
    while (editing) {
        gamemaker_sprite_editor_display(gm);
        
        printf("\nCommands:\n");
        printf("  n <num>  - Select sprite number (0-127)\n");
        printf("  x <pos>  - Set X position\n");
        printf("  y <pos>  - Set Y position\n");
        printf("  t <tile> - Set tile number\n");
        printf("  p <pal>  - Set palette (0-7)\n");
        printf("  h        - Toggle horizontal flip\n");
        printf("  v        - Toggle vertical flip\n");
        printf("  s        - Save sprite to OAM\n");
        printf("  b        - Return to main menu\n");
        printf("\nCommand: ");
        fflush(stdout);
        
        if (!fgets(input, sizeof(input), stdin)) {
            break;
        }
        
        char cmd = input[0];
        switch (cmd) {
            case 'n': {
                u8 sprite_num;
                if (sscanf(input + 1, "%hhu", &sprite_num) == 1 && sprite_num < 128) {
                    gm->sprite_editor.current_sprite = sprite_num;
                    /* Load sprite data from OAM if available */
                    if (gm->mem) {
                        u16 oam_offset = sprite_num * 4;
                        if (oam_offset + 3 < OAM_SIZE) {
                            gm->sprite_editor.sprite_x = gm->mem->oam[oam_offset];
                            gm->sprite_editor.sprite_y = gm->mem->oam[oam_offset + 1];
                            gm->sprite_editor.sprite_tile = gm->mem->oam[oam_offset + 2];
                            u8 attrib = gm->mem->oam[oam_offset + 3];
                            gm->sprite_editor.sprite_palette = (attrib >> 1) & 0x07;
                            gm->sprite_editor.h_flip = (attrib & 0x40) != 0;
                            gm->sprite_editor.v_flip = (attrib & 0x80) != 0;
                        }
                    }
                    printf("Selected sprite %u\n", sprite_num);
                } else {
                    printf("Usage: n <sprite_0-127>\n");
                }
                break;
            }
            case 'x': {
                u8 pos;
                if (sscanf(input + 1, "%hhu", &pos) == 1) {
                    gm->sprite_editor.sprite_x = pos;
                    printf("X position set to %u\n", pos);
                } else {
                    printf("Usage: x <position>\n");
                }
                break;
            }
            case 'y': {
                u8 pos;
                if (sscanf(input + 1, "%hhu", &pos) == 1) {
                    gm->sprite_editor.sprite_y = pos;
                    printf("Y position set to %u\n", pos);
                } else {
                    printf("Usage: y <position>\n");
                }
                break;
            }
            case 't': {
                u8 tile;
                if (sscanf(input + 1, "%hhu", &tile) == 1) {
                    gm->sprite_editor.sprite_tile = tile;
                    printf("Tile set to %u\n", tile);
                } else {
                    printf("Usage: t <tile_number>\n");
                }
                break;
            }
            case 'p': {
                u8 pal;
                if (sscanf(input + 1, "%hhu", &pal) == 1 && pal < 8) {
                    gm->sprite_editor.sprite_palette = pal;
                    printf("Palette set to %u\n", pal);
                } else {
                    printf("Usage: p <palette_0-7>\n");
                }
                break;
            }
            case 'h':
                gm->sprite_editor.h_flip = !gm->sprite_editor.h_flip;
                printf("Horizontal flip: %s\n", gm->sprite_editor.h_flip ? "On" : "Off");
                break;
            case 'v':
                gm->sprite_editor.v_flip = !gm->sprite_editor.v_flip;
                printf("Vertical flip: %s\n", gm->sprite_editor.v_flip ? "On" : "Off");
                break;
            case 's':
                gamemaker_sprite_update(gm);
                printf("Sprite saved to OAM\n");
                break;
            case 'b':
                editing = false;
                break;
            default:
                printf("Unknown command\n");
                break;
        }
        
        printf("\n");
    }
    
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
    if (!gm->mem) {
        gamemaker_set_status(gm, "Error: No OAM available");
        return;
    }
    
    /* Calculate OAM offset for current sprite (4 bytes per sprite) */
    u16 oam_offset = gm->sprite_editor.current_sprite * 4;
    
    if (oam_offset + 3 >= OAM_SIZE) {
        gamemaker_set_status(gm, "Error: Invalid sprite number");
        return;
    }
    
    /* Write sprite data to OAM */
    gm->mem->oam[oam_offset] = gm->sprite_editor.sprite_x;
    gm->mem->oam[oam_offset + 1] = gm->sprite_editor.sprite_y;
    gm->mem->oam[oam_offset + 2] = gm->sprite_editor.sprite_tile;
    
    /* Build attribute byte */
    u8 attrib = 0;
    attrib |= (gm->sprite_editor.sprite_palette & 0x07) << 1;
    if (gm->sprite_editor.h_flip) attrib |= 0x40;
    if (gm->sprite_editor.v_flip) attrib |= 0x80;
    gm->mem->oam[oam_offset + 3] = attrib;
    
    gm->unsaved_changes = true;
    gamemaker_set_status(gm, "Sprite updated in OAM");
}

/* Tilemap Editor implementation */

void gamemaker_tilemap_editor(GameMaker *gm) {
    char input[256];
    bool editing = true;
    
    printf("\n=== Tilemap Editor ===\n");
    printf("Edit background layer tilemaps\n\n");
    
    while (editing) {
        gamemaker_tilemap_editor_display(gm);
        
        printf("\nCommands:\n");
        printf("  l <layer>      - Select BG layer (0-3)\n");
        printf("  t <tile>       - Select tile to place\n");
        printf("  p <palette>    - Select palette (0-7)\n");
        printf("  m <x> <y>      - Move cursor to position\n");
        printf("  s              - Place selected tile at cursor\n");
        printf("  c              - Toggle paint mode\n");
        printf("  v              - View tilemap at cursor area\n");
        printf("  b              - Return to main menu\n");
        printf("\nCommand: ");
        fflush(stdout);
        
        if (!fgets(input, sizeof(input), stdin)) {
            break;
        }
        
        char cmd = input[0];
        switch (cmd) {
            case 'l': {
                u8 layer;
                if (sscanf(input + 1, "%hhu", &layer) == 1 && layer < 4) {
                    gm->tilemap_editor.current_layer = layer;
                    printf("Selected BG%u\n", layer + 1);
                } else {
                    printf("Usage: l <layer_0-3>\n");
                }
                break;
            }
            case 't': {
                u16 tile;
                if (sscanf(input + 1, "%hu", &tile) == 1) {
                    gm->tilemap_editor.selected_tile = tile;
                    printf("Selected tile %u\n", tile);
                } else {
                    printf("Usage: t <tile_number>\n");
                }
                break;
            }
            case 'p': {
                u8 pal;
                if (sscanf(input + 1, "%hhu", &pal) == 1 && pal < 8) {
                    gm->tilemap_editor.selected_palette = pal;
                    printf("Palette set to %u\n", pal);
                } else {
                    printf("Usage: p <palette_0-7>\n");
                }
                break;
            }
            case 'm': {
                u16 x, y;
                if (sscanf(input + 1, "%hu %hu", &x, &y) == 2) {
                    gm->tilemap_editor.cursor_x = x;
                    gm->tilemap_editor.cursor_y = y;
                    printf("Cursor moved to (%u, %u)\n", x, y);
                } else {
                    printf("Usage: m <x> <y>\n");
                }
                break;
            }
            case 's':
                gamemaker_tilemap_place_tile(gm);
                printf("Tile placed at (%u, %u)\n", 
                       gm->tilemap_editor.cursor_x, gm->tilemap_editor.cursor_y);
                break;
            case 'c':
                gm->tilemap_editor.paint_mode = !gm->tilemap_editor.paint_mode;
                printf("Paint mode: %s\n", gm->tilemap_editor.paint_mode ? "On" : "Off");
                break;
            case 'v': {
                /* Display tilemap area around cursor */
                printf("\nTilemap at cursor area:\n");
                if (gm->mem) {
                    /* Display 5x5 grid around cursor */
                    u16 base_x = (gm->tilemap_editor.cursor_x > 2) ? 
                                 gm->tilemap_editor.cursor_x - 2 : 0;
                    u16 base_y = (gm->tilemap_editor.cursor_y > 2) ? 
                                 gm->tilemap_editor.cursor_y - 2 : 0;
                    
                    for (u16 y = 0; y < 5; y++) {
                        printf("  ");
                        for (u16 x = 0; x < 5; x++) {
                            u16 tile_x = base_x + x;
                            u16 tile_y = base_y + y;
                            
                            /* Calculate VRAM address (simplified) */
                            u16 vram_addr = tile_y * 32 + tile_x;
                            
                            if (vram_addr * 2 + 1 < VRAM_SIZE) {
                                u16 tile_data = gm->mem->vram[vram_addr * 2] |
                                               (gm->mem->vram[vram_addr * 2 + 1] << 8);
                                u16 tile_num = tile_data & 0x3FF;
                                
                                if (tile_x == gm->tilemap_editor.cursor_x &&
                                    tile_y == gm->tilemap_editor.cursor_y) {
                                    printf("[%03X]", tile_num);
                                } else {
                                    printf(" %03X ", tile_num);
                                }
                            } else {
                                printf(" --- ");
                            }
                        }
                        printf("\n");
                    }
                }
                break;
            }
            case 'b':
                editing = false;
                break;
            default:
                printf("Unknown command\n");
                break;
        }
        
        printf("\n");
    }
    
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
    if (!gm->mem) {
        gamemaker_set_status(gm, "Error: No VRAM available");
        return;
    }
    
    /* Calculate VRAM address for tilemap
     * SNES tilemap format: 32x32 tiles per screen
     * Each tile entry is 2 bytes with format:
     * vhopppcc cccccccc
     * v = vertical flip, h = horizontal flip
     * o = priority, ppp = palette, cccccccccc = tile number
     */
    u16 tile_x = gm->tilemap_editor.cursor_x;
    u16 tile_y = gm->tilemap_editor.cursor_y;
    
    /* Simple 32x32 tilemap addressing */
    u16 vram_offset = tile_y * 32 + tile_x;
    u16 vram_addr = vram_offset * 2;
    
    if (vram_addr + 1 >= VRAM_SIZE) {
        gamemaker_set_status(gm, "Error: Cursor position out of range");
        return;
    }
    
    /* Build tile entry */
    u16 tile_entry = gm->tilemap_editor.selected_tile & 0x3FF;
    tile_entry |= (gm->tilemap_editor.selected_palette & 0x07) << 10;
    
    /* Write to VRAM */
    gm->mem->vram[vram_addr] = tile_entry & 0xFF;
    gm->mem->vram[vram_addr + 1] = (tile_entry >> 8) & 0xFF;
    
    gm->unsaved_changes = true;
    gamemaker_set_status(gm, "Tile placed");
}

void gamemaker_tilemap_move_cursor(GameMaker *gm, s16 dx, s16 dy) {
    gm->tilemap_editor.cursor_x += dx;
    gm->tilemap_editor.cursor_y += dy;
}

/* Palette Editor implementation */

void gamemaker_palette_editor(GameMaker *gm) {
    char input[256];
    bool editing = true;
    
    printf("\n=== Palette Editor ===\n");
    printf("Edit color palettes (15-bit RGB)\n\n");
    
    while (editing) {
        gamemaker_palette_editor_display(gm);
        
        printf("\nCommands:\n");
        printf("  p <pal>       - Select palette (0-15)\n");
        printf("  c <color>     - Select color in palette (0-15)\n");
        printf("  r <val>       - Set red component (0-31)\n");
        printf("  g <val>       - Set green component (0-31)\n");
        printf("  b <val>       - Set blue component (0-31)\n");
        printf("  s <hex>       - Set full 15-bit color value (e.g., 7FFF)\n");
        printf("  d             - Display current palette\n");
        printf("  w             - Write to CGRAM\n");
        printf("  x             - Return to main menu\n");
        printf("\nCommand: ");
        fflush(stdout);
        
        if (!fgets(input, sizeof(input), stdin)) {
            break;
        }
        
        char cmd = input[0];
        switch (cmd) {
            case 'p': {
                u8 pal;
                if (sscanf(input + 1, "%hhu", &pal) == 1 && pal < 16) {
                    gm->palette_editor.current_palette = pal;
                    /* Load color 0 of this palette */
                    gm->palette_editor.current_color = 0;
                    if (gm->mem) {
                        u16 cgram_offset = (pal * 16) * 2;
                        if (cgram_offset + 1 < CGRAM_SIZE) {
                            gm->palette_editor.color_value = 
                                gm->mem->cgram[cgram_offset] |
                                (gm->mem->cgram[cgram_offset + 1] << 8);
                        }
                    }
                    printf("Selected palette %u\n", pal);
                } else {
                    printf("Usage: p <palette_0-15>\n");
                }
                break;
            }
            case 'c': {
                u8 color;
                if (sscanf(input + 1, "%hhu", &color) == 1 && color < 16) {
                    gm->palette_editor.current_color = color;
                    /* Load this color value */
                    if (gm->mem) {
                        u16 cgram_offset = (gm->palette_editor.current_palette * 16 + color) * 2;
                        if (cgram_offset + 1 < CGRAM_SIZE) {
                            gm->palette_editor.color_value = 
                                gm->mem->cgram[cgram_offset] |
                                (gm->mem->cgram[cgram_offset + 1] << 8);
                        }
                    }
                    printf("Selected color %u\n", color);
                } else {
                    printf("Usage: c <color_0-15>\n");
                }
                break;
            }
            case 'r': {
                u8 val;
                if (sscanf(input + 1, "%hhu", &val) == 1 && val < 32) {
                    gm->palette_editor.color_value = 
                        (gm->palette_editor.color_value & 0x7FE0) | (val & 0x1F);
                    printf("Red set to %u\n", val);
                } else {
                    printf("Usage: r <value_0-31>\n");
                }
                break;
            }
            case 'g': {
                u8 val;
                if (sscanf(input + 1, "%hhu", &val) == 1 && val < 32) {
                    gm->palette_editor.color_value = 
                        (gm->palette_editor.color_value & 0x7C1F) | ((val & 0x1F) << 5);
                    printf("Green set to %u\n", val);
                } else {
                    printf("Usage: g <value_0-31>\n");
                }
                break;
            }
            case 'b': {
                u8 val;
                if (sscanf(input + 1, "%hhu", &val) == 1 && val < 32) {
                    gm->palette_editor.color_value = 
                        (gm->palette_editor.color_value & 0x03FF) | ((val & 0x1F) << 10);
                    printf("Blue set to %u\n", val);
                } else {
                    printf("Usage: b <value_0-31>\n");
                }
                break;
            }
            case 's': {
                u16 val;
                if (sscanf(input + 1, "%hx", &val) == 1) {
                    gamemaker_palette_set_color(gm, val);
                    printf("Color set to $%04X\n", val & 0x7FFF);
                } else {
                    printf("Usage: s <hex_value>\n");
                }
                break;
            }
            case 'd': {
                /* Display current palette */
                printf("\nPalette %u colors:\n", gm->palette_editor.current_palette);
                if (gm->mem) {
                    for (u8 i = 0; i < 16; i++) {
                        u16 cgram_offset = (gm->palette_editor.current_palette * 16 + i) * 2;
                        if (cgram_offset + 1 < CGRAM_SIZE) {
                            u16 color = gm->mem->cgram[cgram_offset] |
                                       (gm->mem->cgram[cgram_offset + 1] << 8);
                            u8 r = color & 0x1F;
                            u8 g = (color >> 5) & 0x1F;
                            u8 b = (color >> 10) & 0x1F;
                            printf("  %2u: $%04X  RGB(%2u,%2u,%2u)\n", i, color, r, g, b);
                        }
                    }
                }
                break;
            }
            case 'w':
                /* Write current color to CGRAM */
                if (gm->mem) {
                    u16 cgram_offset = (gm->palette_editor.current_palette * 16 + 
                                       gm->palette_editor.current_color) * 2;
                    if (cgram_offset + 1 < CGRAM_SIZE) {
                        gm->mem->cgram[cgram_offset] = gm->palette_editor.color_value & 0xFF;
                        gm->mem->cgram[cgram_offset + 1] = (gm->palette_editor.color_value >> 8) & 0xFF;
                        gm->palette_editor.modified = true;
                        gm->unsaved_changes = true;
                        printf("Color written to CGRAM\n");
                    }
                }
                break;
            case 'x':
                editing = false;
                break;
            default:
                printf("Unknown command\n");
                break;
        }
        
        printf("\n");
    }
    
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

/* Script Editor implementation */

void gamemaker_script_editor(GameMaker *gm) {
    char input[256];
    bool editing = true;
    
    printf("\n=== Script Editor ===\n");
    printf("Execute ROM modification scripts\n\n");
    
    while (editing) {
        printf("\nScript Editor:\n");
        printf("  f <file>  - Execute script from file\n");
        printf("  e <cmd>   - Execute single command\n");
        printf("  h         - Show scripting help\n");
        printf("  b         - Return to main menu\n");
        printf("\nCommand: ");
        fflush(stdout);
        
        if (!fgets(input, sizeof(input), stdin)) {
            break;
        }
        
        char cmd = input[0];
        switch (cmd) {
            case 'f': {
                char filename[256];
                if (sscanf(input + 1, " %255s", filename) == 1) {
                    printf("Executing script: %s\n", filename);
                    if (gamemaker_script_execute_file(gm, filename) == SUCCESS) {
                        printf("Script executed successfully\n");
                        gm->unsaved_changes = true;
                    } else {
                        const char *error = script_get_error(&gm->script_ctx);
                        printf("Script error: %s\n", error ? error : "Unknown error");
                    }
                } else {
                    printf("Usage: f <filename>\n");
                }
                break;
            }
            case 'e': {
                /* Execute single command */
                char *script_cmd = input + 1;
                while (*script_cmd == ' ') script_cmd++;
                
                if (*script_cmd != '\0' && *script_cmd != '\n') {
                    if (gamemaker_script_execute_string(gm, script_cmd) == SUCCESS) {
                        printf("Command executed\n");
                        gm->unsaved_changes = true;
                    } else {
                        const char *error = script_get_error(&gm->script_ctx);
                        printf("Error: %s\n", error ? error : "Unknown error");
                    }
                } else {
                    printf("Usage: e <command>\n");
                }
                break;
            }
            case 'h':
                script_print_help();
                break;
            case 'b':
                editing = false;
                break;
            default:
                printf("Unknown command\n");
                break;
        }
        
        printf("\n");
    }
    
    gm->mode = GM_MODE_MAIN_MENU;
}

int gamemaker_script_execute_file(GameMaker *gm, const char *filename) {
    return script_execute_file(&gm->script_ctx, filename);
}

int gamemaker_script_execute_string(GameMaker *gm, const char *script) {
    return script_execute_string(&gm->script_ctx, script);
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
