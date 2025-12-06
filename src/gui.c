/*
 * gui.c - Minimalistic GUI implementation
 * 
 * Simple console-based GUI for ROM selection and settings
 * Compatible with GCC 15.2+ on Windows 11 x64 and Linux
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "../include/gui.h"

/* Platform-specific directory handling */
#ifdef _WIN32
    #include <windows.h>
    #include <direct.h>
    #define mkdir(path, mode) _mkdir(path)
    #define stat _stat
    #define S_ISREG(m) (((m) & _S_IFMT) == _S_IFREG)
#else
    #include <dirent.h>
#endif

int gui_init(GuiState *gui) {
    memset(gui, 0, sizeof(GuiState));
    gui->initialized = true;
    gui->settings_visible = false;
    gui->selected_rom = 0;
    gui->volume = 80;
    gui->vsync = true;
    gui->scale_factor = 2;
    return SUCCESS;
}

void gui_cleanup(GuiState *gui) {
    gui->initialized = false;
}

static int is_rom_file(const char *filename) {
    size_t len = strlen(filename);
    if (len < 4) return 0;
    
    const char *ext = filename + len - 4;
    return (strcmp(ext, ".sfc") == 0 || 
            strcmp(ext, ".SFC") == 0 ||
            strcmp(ext, ".smc") == 0 ||
            strcmp(ext, ".SMC") == 0);
}

int gui_scan_roms(GuiState *gui) {
    struct stat st;
    char fullpath[MAX_FILENAME_LEN];
    
    gui->rom_count = 0;
    
#ifdef _WIN32
    /* Windows implementation using FindFirstFile/FindNextFile */
    WIN32_FIND_DATAA findData;
    HANDLE hFind;
    char searchPath[MAX_FILENAME_LEN];
    
    snprintf(searchPath, sizeof(searchPath), "%s\\*", ROM_DIR_PATH);
    
    hFind = FindFirstFileA(searchPath, &findData);
    if (hFind == INVALID_HANDLE_VALUE) {
        DWORD error = GetLastError();
        if (error == ERROR_PATH_NOT_FOUND || error == ERROR_FILE_NOT_FOUND) {
            printf("Warning: Could not open '%s' directory. Creating it...\n", ROM_DIR_PATH);
            mkdir(ROM_DIR_PATH);
        }
        return SUCCESS;
    }
    
    do {
        /* Skip . and .. */
        if (strcmp(findData.cFileName, ".") == 0 || strcmp(findData.cFileName, "..") == 0) {
            continue;
        }
        
        /* Skip directories */
        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            continue;
        }
        
        /* Check if it's a ROM file */
        if (is_rom_file(findData.cFileName) && gui->rom_count < MAX_ROM_FILES) {
            snprintf(fullpath, sizeof(fullpath), "%s/%s", ROM_DIR_PATH, findData.cFileName);
            strncpy(gui->roms[gui->rom_count].filename, findData.cFileName, MAX_FILENAME_LEN - 1);
            gui->roms[gui->rom_count].filename[MAX_FILENAME_LEN - 1] = '\0';
            strncpy(gui->roms[gui->rom_count].fullpath, fullpath, MAX_FILENAME_LEN - 1);
            gui->roms[gui->rom_count].fullpath[MAX_FILENAME_LEN - 1] = '\0';
            gui->rom_count++;
        }
    } while (FindNextFileA(hFind, &findData) != 0 && gui->rom_count < MAX_ROM_FILES);
    
    FindClose(hFind);
#else
    /* POSIX implementation using dirent */
    DIR *dir;
    struct dirent *entry;
    
    dir = opendir(ROM_DIR_PATH);
    if (!dir) {
        printf("Warning: Could not open '%s' directory. Creating it...\n", ROM_DIR_PATH);
        mkdir(ROM_DIR_PATH, 0755);
        return SUCCESS;
    }
    
    while ((entry = readdir(dir)) != NULL && gui->rom_count < MAX_ROM_FILES) {
        /* Skip . and .. */
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        /* Check if it's a ROM file */
        if (is_rom_file(entry->d_name)) {
            /* Verify it's a regular file using stat */
            snprintf(fullpath, sizeof(fullpath), "%s/%s", ROM_DIR_PATH, entry->d_name);
            if (stat(fullpath, &st) == 0 && S_ISREG(st.st_mode)) {
                strncpy(gui->roms[gui->rom_count].filename, entry->d_name, MAX_FILENAME_LEN - 1);
                gui->roms[gui->rom_count].filename[MAX_FILENAME_LEN - 1] = '\0';
                strncpy(gui->roms[gui->rom_count].fullpath, fullpath, MAX_FILENAME_LEN - 1);
                gui->roms[gui->rom_count].fullpath[MAX_FILENAME_LEN - 1] = '\0';
                gui->rom_count++;
            }
        }
    }
    
    closedir(dir);
#endif
    
    return SUCCESS;
}

static void print_box_top(void) {
    printf("╔════════════════════════════════════════════════════════════════════╗\n");
}

static void print_box_bottom(void) {
    printf("╚════════════════════════════════════════════════════════════════════╝\n");
}

static void print_box_line(const char *text) {
    printf("║ %-66s ║\n", text);
}

char* gui_show_rom_selector(GuiState *gui) {
    int choice;
    char input[256];
    static char selected_path[MAX_FILENAME_LEN];
    
    /* Scan for ROMs */
    gui_scan_roms(gui);
    
    /* Display ROM selection window */
    printf("\n");
    print_box_top();
    print_box_line("SNESE - ROM Selection");
    print_box_line("");
    
    if (gui->rom_count == 0) {
        print_box_line("No ROM files found in 'roms/' directory.");
        print_box_line("");
        print_box_line("Please place .sfc or .smc ROM files in the 'roms' directory.");
        print_box_bottom();
        return NULL;
    }
    
    print_box_line("Available ROMs:");
    print_box_line("");
    
    for (int i = 0; i < gui->rom_count; i++) {
        char line[256];
        snprintf(line, sizeof(line), "  [%2d] %s", i + 1, gui->roms[i].filename);
        print_box_line(line);
    }
    
    print_box_line("");
    print_box_line("  [0] Exit");
    print_box_bottom();
    
    printf("\nSelect ROM number: ");
    fflush(stdout);
    
    if (fgets(input, sizeof(input), stdin)) {
        choice = atoi(input);
        
        if (choice == 0) {
            return NULL;
        }
        
        if (choice > 0 && choice <= gui->rom_count) {
            gui->selected_rom = choice - 1;
            strncpy(selected_path, gui->roms[gui->selected_rom].fullpath, MAX_FILENAME_LEN - 1);
            return selected_path;
        }
    }
    
    return NULL;
}

void gui_render_settings(GuiState *gui) {
    if (!gui->settings_visible) {
        return;
    }
    
    printf("\n");
    print_box_top();
    print_box_line("Settings");
    print_box_line("");
    
    char line[256];
    snprintf(line, sizeof(line), "  Volume: %d%%", gui->volume);
    print_box_line(line);
    
    snprintf(line, sizeof(line), "  VSync: %s", gui->vsync ? "On" : "Off");
    print_box_line(line);
    
    snprintf(line, sizeof(line), "  Scale Factor: %dx", gui->scale_factor);
    print_box_line(line);
    
    print_box_line("");
    print_box_line("Press [S] to save and close settings");
    print_box_bottom();
    printf("\n");
}

void gui_toggle_settings(GuiState *gui) {
    gui->settings_visible = !gui->settings_visible;
    
    if (gui->settings_visible) {
        gui_render_settings(gui);
    }
}

void gui_handle_input(GuiState *gui) {
    char input[256];
    
    if (!gui->settings_visible) {
        return;
    }
    
    printf("Settings command ([v]olume, v[s]ync, s[c]ale, [q]uit): ");
    fflush(stdout);
    
    if (fgets(input, sizeof(input), stdin)) {
        char cmd = input[0];
        
        switch (cmd) {
            case 'v':
            case 'V':
                printf("Enter volume (0-100): ");
                fflush(stdout);
                if (fgets(input, sizeof(input), stdin)) {
                    int vol = atoi(input);
                    if (vol >= 0 && vol <= 100) {
                        gui->volume = vol;
                        printf("Volume set to %d%%\n", gui->volume);
                    }
                }
                break;
                
            case 's':
            case 'S':
                gui->vsync = !gui->vsync;
                printf("VSync %s\n", gui->vsync ? "enabled" : "disabled");
                break;
                
            case 'c':
            case 'C':
                printf("Enter scale factor (1-4): ");
                fflush(stdout);
                if (fgets(input, sizeof(input), stdin)) {
                    int scale = atoi(input);
                    if (scale >= 1 && scale <= 4) {
                        gui->scale_factor = scale;
                        printf("Scale factor set to %dx\n", gui->scale_factor);
                    }
                }
                break;
                
            case 'q':
            case 'Q':
                gui->settings_visible = false;
                printf("Settings closed\n");
                break;
                
            default:
                printf("Unknown command\n");
                break;
        }
        
        if (gui->settings_visible) {
            gui_render_settings(gui);
        }
    }
}
