/*
 * game_maker.h - Built-in Game Maker interface
 * 
 * Provides runtime tools for creating and modifying ROM-based games.
 */

#ifndef GAME_MAKER_H
#define GAME_MAKER_H

#include "types.h"
#include "cartridge.h"
#include "memory.h"
#include "script.h"

/* Game Maker modes */
typedef enum {
    GM_MODE_MAIN_MENU,
    GM_MODE_TILE_EDITOR,
    GM_MODE_SPRITE_EDITOR,
    GM_MODE_TILEMAP_EDITOR,
    GM_MODE_PALETTE_EDITOR,
    GM_MODE_SCRIPT_EDITOR,
    GM_MODE_ROM_INFO,
    GM_MODE_EXIT
} GameMakerMode;

/* Tile editor state */
typedef struct {
    u16 current_tile;        /* Currently selected tile */
    u16 tile_addr;           /* Address in VRAM */
    u8 current_palette;      /* Currently selected palette (0-7) */
    u8 zoom_level;           /* Zoom level for display */
    bool modified;           /* Whether tile has been modified */
} TileEditor;

/* Sprite editor state */
typedef struct {
    u8 current_sprite;       /* Currently selected sprite (0-127) */
    u8 sprite_x;             /* X position */
    u8 sprite_y;             /* Y position */
    u8 sprite_tile;          /* Tile number */
    u8 sprite_palette;       /* Palette (0-7) */
    bool h_flip;             /* Horizontal flip */
    bool v_flip;             /* Vertical flip */
} SpriteEditor;

/* Tilemap editor state */
typedef struct {
    u8 current_layer;        /* Current BG layer (0-3) */
    u16 cursor_x;            /* Cursor X position in tiles */
    u16 cursor_y;            /* Cursor Y position in tiles */
    u16 selected_tile;       /* Currently selected tile to place */
    u8 selected_palette;     /* Palette for tile */
    bool paint_mode;         /* Continuous painting enabled */
} TilemapEditor;

/* Palette editor state */
typedef struct {
    u8 current_palette;      /* Current palette (0-15) */
    u8 current_color;        /* Current color in palette (0-15) */
    u16 color_value;         /* 15-bit RGB color value */
    bool modified;           /* Whether palette has been modified */
} PaletteEditor;

/* Script editor command (different from ScriptCommand in script.h) */
typedef struct {
    char command[256];       /* Command string as typed by user */
} GameMakerScriptCommand;

/* Script editor state */
typedef struct {
    GameMakerScriptCommand commands[256];  /* Script commands */
    u16 num_commands;             /* Number of commands */
    u16 current_command;          /* Current command index */
    char script_buffer[4096];     /* Script text buffer */
} ScriptEditor;

/* Game Maker state */
typedef struct {
    GameMakerMode mode;      /* Current mode */
    Cartridge *cart;         /* Pointer to loaded cartridge */
    Memory *mem;             /* Pointer to memory system */
    
    /* Editor states */
    TileEditor tile_editor;
    SpriteEditor sprite_editor;
    TilemapEditor tilemap_editor;
    PaletteEditor palette_editor;
    ScriptEditor script_editor;
    
    /* Scripting context */
    ScriptContext script_ctx;
    
    /* General state */
    char status_message[128]; /* Status message to display */
    bool running;            /* Whether Game Maker is running */
    bool unsaved_changes;    /* Whether there are unsaved changes */
    char output_filename[256]; /* Filename for saving modified ROM */
} GameMaker;

/* Function declarations */

/*
 * Initialize Game Maker
 */
void gamemaker_init(GameMaker *gm, Cartridge *cart, Memory *mem);

/*
 * Run Game Maker main loop
 */
void gamemaker_run(GameMaker *gm);

/*
 * Cleanup Game Maker resources
 */
void gamemaker_cleanup(GameMaker *gm);

/*
 * Display main menu
 */
void gamemaker_show_menu(GameMaker *gm);

/*
 * Process user input
 */
void gamemaker_process_input(GameMaker *gm, char input);

/*
 * Save modified ROM to file
 */
int gamemaker_save_rom(GameMaker *gm, const char *filename);

/* Tile Editor functions */

/*
 * Enter tile editor mode
 */
void gamemaker_tile_editor(GameMaker *gm);

/*
 * Display tile editor interface
 */
void gamemaker_tile_editor_display(const GameMaker *gm);

/*
 * Edit tile pixel
 */
void gamemaker_tile_edit_pixel(GameMaker *gm, u8 x, u8 y, u8 color);

/*
 * Load tile from VRAM
 */
void gamemaker_tile_load(GameMaker *gm, u16 tile_num);

/*
 * Save tile to VRAM
 */
void gamemaker_tile_save(GameMaker *gm);

/* Sprite Editor functions */

/*
 * Enter sprite editor mode
 */
void gamemaker_sprite_editor(GameMaker *gm);

/*
 * Display sprite editor interface
 */
void gamemaker_sprite_editor_display(const GameMaker *gm);

/*
 * Update sprite properties
 */
void gamemaker_sprite_update(GameMaker *gm);

/* Tilemap Editor functions */

/*
 * Enter tilemap editor mode
 */
void gamemaker_tilemap_editor(GameMaker *gm);

/*
 * Display tilemap editor interface
 */
void gamemaker_tilemap_editor_display(const GameMaker *gm);

/*
 * Place tile at cursor position
 */
void gamemaker_tilemap_place_tile(GameMaker *gm);

/*
 * Move cursor
 */
void gamemaker_tilemap_move_cursor(GameMaker *gm, s16 dx, s16 dy);

/* Palette Editor functions */

/*
 * Enter palette editor mode
 */
void gamemaker_palette_editor(GameMaker *gm);

/*
 * Display palette editor interface
 */
void gamemaker_palette_editor_display(const GameMaker *gm);

/*
 * Modify color value
 */
void gamemaker_palette_set_color(GameMaker *gm, u16 color_value);

/*
 * Export palette to file
 */
int gamemaker_palette_export(const GameMaker *gm, const char *filename);

/*
 * Import palette from file
 */
int gamemaker_palette_import(GameMaker *gm, const char *filename);

/* Script Editor functions */

/*
 * Enter script editor mode
 */
void gamemaker_script_editor(GameMaker *gm);

/*
 * Display script editor interface
 */
void gamemaker_script_editor_display(const GameMaker *gm);

/*
 * Parse and execute script command
 */
int gamemaker_script_execute_command(GameMaker *gm, const char *command);

/*
 * Load script from file
 */
int gamemaker_script_load(GameMaker *gm, const char *filename);

/*
 * Save script to file
 */
int gamemaker_script_save(const GameMaker *gm, const char *filename);

/*
 * Execute entire script
 */
int gamemaker_script_run(GameMaker *gm);

/*
 * Execute script from file
 */
int gamemaker_script_execute_file(GameMaker *gm, const char *filename);

/*
 * Execute script from string
 */
int gamemaker_script_execute_string(GameMaker *gm, const char *script);

/* Utility functions */

/*
 * Display ROM information
 */
void gamemaker_show_rom_info(const GameMaker *gm);

/*
 * Set status message
 */
void gamemaker_set_status(GameMaker *gm, const char *message);

/*
 * Prompt user for confirmation
 */
bool gamemaker_confirm(const char *message);

#endif /* GAME_MAKER_H */
