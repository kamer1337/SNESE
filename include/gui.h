/*
 * gui.h - Minimalistic GUI interface for SNESE
 * 
 * Provides basic GUI functionality including:
 * - ROM selection startup window
 * - Settings panel in rendering window
 */

#ifndef GUI_H
#define GUI_H

#include "types.h"

/* Maximum number of ROM files to display */
#define MAX_ROM_FILES 100
#define MAX_FILENAME_LEN 256
#define ROM_DIR_PATH "roms"

/* ROM file entry */
typedef struct {
    char filename[MAX_FILENAME_LEN];
    char fullpath[MAX_FILENAME_LEN];
} RomEntry;

/* GUI State */
typedef struct {
    bool initialized;
    bool settings_visible;
    
    /* ROM selection */
    RomEntry roms[MAX_ROM_FILES];
    int rom_count;
    int selected_rom;
    
    /* Settings */
    int volume;
    bool vsync;
    int scale_factor;
} GuiState;

/*
 * Initialize GUI system
 */
int gui_init(GuiState *gui);

/*
 * Cleanup GUI system
 */
void gui_cleanup(GuiState *gui);

/*
 * Scan ROM directory and populate ROM list
 */
int gui_scan_roms(GuiState *gui);

/*
 * Show ROM selection window (returns selected ROM path or NULL if cancelled)
 */
char* gui_show_rom_selector(GuiState *gui);

/*
 * Render settings panel overlay
 */
void gui_render_settings(GuiState *gui);

/*
 * Toggle settings panel visibility
 */
void gui_toggle_settings(GuiState *gui);

/*
 * Handle GUI input events
 */
void gui_handle_input(GuiState *gui);

#endif /* GUI_H */
