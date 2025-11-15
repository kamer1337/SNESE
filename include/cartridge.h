/*
 * cartridge.h - ROM cartridge loading and management
 * 
 * Handles loading SNES ROM files (.sfc/.smc format), parsing headers,
 * and managing cartridge memory mapping.
 */

#ifndef CARTRIDGE_H
#define CARTRIDGE_H

#include "types.h"

/* Cartridge mapper types */
typedef enum {
    MAPPER_LOROM = 0,
    MAPPER_HIROM = 1,
    MAPPER_EXHIROM = 5,
    MAPPER_UNKNOWN = 0xFF
} MapperType;

/* ROM region codes */
typedef enum {
    REGION_JAPAN = 0,
    REGION_NORTH_AMERICA = 1,
    REGION_EUROPE = 2,
    REGION_SCANDINAVIA = 3,
    REGION_FRENCH = 6,
    REGION_DUTCH = 7,
    REGION_SPANISH = 8,
    REGION_GERMAN = 9,
    REGION_ITALIAN = 10,
    REGION_KOREAN = 13,
    REGION_UNKNOWN = 0xFF
} RegionCode;

/* SNES ROM header structure (located at $FFB0-$FFFF in header bank) */
typedef struct {
    char title[21];           /* Game title (ASCII) */
    u8 map_mode;              /* ROM speed and mapper type */
    u8 rom_type;              /* Cartridge type (ROM, ROM+RAM, ROM+RAM+Battery, etc.) */
    u8 rom_size;              /* ROM size (log2(size in KB) - 10) */
    u8 sram_size;             /* SRAM size (log2(size in KB) - 10) */
    u8 country_code;          /* Destination region */
    u8 license_code;          /* Publisher code */
    u8 version;               /* ROM version */
    u16 checksum_complement;  /* Bitwise NOT of checksum */
    u16 checksum;             /* ROM checksum */
} ROMHeader;

/* Cartridge structure */
typedef struct {
    u8 *rom_data;             /* Raw ROM data */
    u32 rom_size;             /* Size of ROM in bytes */
    u8 *sram_data;            /* Save RAM (if present) */
    u32 sram_size;            /* Size of SRAM in bytes */
    
    ROMHeader header;         /* Parsed ROM header */
    MapperType mapper;        /* Detected mapper type */
    bool has_sram;            /* Whether cartridge has save RAM */
    bool sram_battery;        /* Whether SRAM is battery-backed */
    
    char filename[256];       /* Original ROM filename */
    
    /* For ROM editing/backup */
    u8 *rom_backup;           /* Backup of original ROM data */
    bool has_backup;          /* Whether backup exists */
} Cartridge;

/* Function declarations */

/*
 * Load a ROM file from disk
 * Returns SUCCESS on success, ERROR on failure
 */
int cartridge_load(Cartridge *cart, const char *filename);

/*
 * Free cartridge memory
 */
void cartridge_unload(Cartridge *cart);

/*
 * Parse ROM header from loaded data
 * Returns SUCCESS if header is valid, ERROR otherwise
 */
int cartridge_parse_header(Cartridge *cart);

/*
 * Print cartridge information to console
 */
void cartridge_print_info(const Cartridge *cart);

/*
 * Read byte from cartridge address
 */
u8 cartridge_read(const Cartridge *cart, u32 address);

/*
 * Write byte to cartridge (SRAM if present)
 */
void cartridge_write(Cartridge *cart, u32 address, u8 value);

/*
 * Calculate ROM checksum
 */
u16 cartridge_calculate_checksum(const Cartridge *cart);

/*
 * Detect mapper type from ROM data
 */
MapperType cartridge_detect_mapper(const u8 *rom_data, u32 rom_size);

/*
 * Write byte to ROM data (for editing)
 */
void cartridge_write_rom(Cartridge *cart, u32 address, u8 value);

/*
 * Update ROM checksum after modifications
 */
void cartridge_update_checksum(Cartridge *cart);

/*
 * Save modified ROM to file
 */
int cartridge_save_rom(const Cartridge *cart, const char *filename);

/*
 * Create backup of ROM data
 */
int cartridge_backup_rom(Cartridge *cart);

/*
 * Restore ROM from backup
 */
int cartridge_restore_rom(Cartridge *cart);

#endif /* CARTRIDGE_H */
