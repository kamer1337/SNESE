/*
 * cartridge.c - ROM cartridge loading and management implementation
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/cartridge.h"

/* SMC header size (some ROMs have a 512-byte copier header) */
#define SMC_HEADER_SIZE 512

/* Region names for display */
static const char *region_names[] = {
    "Japan", "North America", "Europe", "Scandinavia",
    "Unknown", "Unknown", "French", "Dutch",
    "Spanish", "German", "Italian", "Unknown",
    "Unknown", "Korean"
};

int cartridge_load(Cartridge *cart, const char *filename) {
    FILE *file;
    long file_size;
    size_t bytes_read;
    u8 *data;
    bool has_smc_header = false;
    
    /* Initialize cartridge structure */
    memset(cart, 0, sizeof(Cartridge));
    strncpy(cart->filename, filename, sizeof(cart->filename) - 1);
    
    /* Open ROM file */
    file = fopen(filename, "rb");
    if (!file) {
        fprintf(stderr, "Error: Cannot open ROM file '%s'\n", filename);
        return ERROR;
    }
    
    /* Get file size */
    fseek(file, 0, SEEK_END);
    file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    if (file_size < 0x8000) {
        fprintf(stderr, "Error: ROM file too small (less than 32KB)\n");
        fclose(file);
        return ERROR;
    }
    
    /* Check for SMC header (512 bytes) */
    if ((file_size % 1024) == 512) {
        has_smc_header = true;
        file_size -= SMC_HEADER_SIZE;
    }
    
    /* Allocate memory for ROM data */
    data = (u8 *)malloc(file_size);
    if (!data) {
        fprintf(stderr, "Error: Cannot allocate memory for ROM\n");
        fclose(file);
        return ERROR;
    }
    
    /* Skip SMC header if present */
    if (has_smc_header) {
        fseek(file, SMC_HEADER_SIZE, SEEK_SET);
    }
    
    /* Read ROM data */
    bytes_read = fread(data, 1, file_size, file);
    fclose(file);
    
    if (bytes_read != (size_t)file_size) {
        fprintf(stderr, "Error: Failed to read complete ROM file\n");
        free(data);
        return ERROR;
    }
    
    cart->rom_data = data;
    cart->rom_size = file_size;
    
    /* Parse ROM header */
    if (cartridge_parse_header(cart) != SUCCESS) {
        fprintf(stderr, "Warning: ROM header parsing failed, attempting to continue...\n");
    }
    
    /* Allocate SRAM if present */
    if (cart->has_sram && cart->sram_size > 0) {
        cart->sram_data = (u8 *)calloc(cart->sram_size, 1);
        if (!cart->sram_data) {
            fprintf(stderr, "Warning: Cannot allocate SRAM\n");
            cart->has_sram = false;
        }
    }
    
    return SUCCESS;
}

void cartridge_unload(Cartridge *cart) {
    if (cart->rom_data) {
        free(cart->rom_data);
        cart->rom_data = NULL;
    }
    
    if (cart->sram_data) {
        free(cart->sram_data);
        cart->sram_data = NULL;
    }
    
    if (cart->rom_backup) {
        free(cart->rom_backup);
        cart->rom_backup = NULL;
    }
    
    cart->rom_size = 0;
    cart->sram_size = 0;
    cart->has_backup = false;
}

MapperType cartridge_detect_mapper(const u8 *rom_data, u32 rom_size) {
    u16 lorom_checksum, hirom_checksum;
    u16 lorom_complement, hirom_complement;
    
    if (rom_size < 0x10000) {
        return MAPPER_UNKNOWN;
    }
    
    /* Read checksums from both possible header locations */
    lorom_checksum = rom_data[LOROM_HEADER_OFFSET + 0x1C] |
                     (rom_data[LOROM_HEADER_OFFSET + 0x1D] << 8);
    lorom_complement = rom_data[LOROM_HEADER_OFFSET + 0x1E] |
                       (rom_data[LOROM_HEADER_OFFSET + 0x1F] << 8);
    
    hirom_checksum = rom_data[HIROM_HEADER_OFFSET + 0x1C] |
                     (rom_data[HIROM_HEADER_OFFSET + 0x1D] << 8);
    hirom_complement = rom_data[HIROM_HEADER_OFFSET + 0x1E] |
                       (rom_data[HIROM_HEADER_OFFSET + 0x1F] << 8);
    
    /* Check if complement and checksum add up to 0xFFFF */
    bool lorom_valid = (lorom_checksum + lorom_complement) == 0xFFFF;
    bool hirom_valid = (hirom_checksum + hirom_complement) == 0xFFFF;
    
    /* Prefer HiROM if both appear valid and ROM is large enough */
    if (hirom_valid && rom_size >= 0x400000) {
        return MAPPER_HIROM;
    }
    
    if (lorom_valid) {
        return MAPPER_LOROM;
    }
    
    if (hirom_valid) {
        return MAPPER_HIROM;
    }
    
    /* Default to LoROM if uncertain */
    return MAPPER_LOROM;
}

int cartridge_parse_header(Cartridge *cart) {
    u32 header_offset;
    const u8 *header_data;
    
    /* Detect mapper type */
    cart->mapper = cartridge_detect_mapper(cart->rom_data, cart->rom_size);
    
    /* Get header offset based on mapper */
    header_offset = (cart->mapper == MAPPER_HIROM) ? 
                    HIROM_HEADER_OFFSET : LOROM_HEADER_OFFSET;
    
    if (header_offset + 0x30 > cart->rom_size) {
        return ERROR;
    }
    
    header_data = &cart->rom_data[header_offset];
    
    /* Parse header fields */
    memcpy(cart->header.title, &header_data[0x00], 21);
    cart->header.title[20] = '\0';  /* Ensure null termination */
    
    cart->header.map_mode = header_data[0x15];
    cart->header.rom_type = header_data[0x16];
    cart->header.rom_size = header_data[0x17];
    cart->header.sram_size = header_data[0x18];
    cart->header.country_code = header_data[0x19];
    cart->header.license_code = header_data[0x1A];
    cart->header.version = header_data[0x1B];
    
    cart->header.checksum_complement = header_data[0x1C] | 
                                       (header_data[0x1D] << 8);
    cart->header.checksum = header_data[0x1E] | 
                           (header_data[0x1F] << 8);
    
    /* Calculate SRAM size */
    if (cart->header.sram_size > 0 && cart->header.sram_size < 16) {
        cart->sram_size = 1024 << cart->header.sram_size;
        cart->has_sram = true;
        cart->sram_battery = (cart->header.rom_type & 0x0F) == 0x02 ||
                            (cart->header.rom_type & 0x0F) == 0x05;
    }
    
    return SUCCESS;
}

void cartridge_print_info(const Cartridge *cart) {
    const char *mapper_name;
    const char *region_name;
    
    printf("\n=== ROM Information ===\n");
    printf("File: %s\n", cart->filename);
    printf("Title: %s\n", cart->header.title);
    
    /* Mapper type */
    switch (cart->mapper) {
        case MAPPER_LOROM:   mapper_name = "LoROM"; break;
        case MAPPER_HIROM:   mapper_name = "HiROM"; break;
        case MAPPER_EXHIROM: mapper_name = "ExHiROM"; break;
        default:             mapper_name = "Unknown"; break;
    }
    printf("Mapper: %s\n", mapper_name);
    
    /* ROM size */
    printf("ROM Size: %u KB (%u bytes)\n", cart->rom_size / 1024, cart->rom_size);
    
    /* SRAM info */
    if (cart->has_sram) {
        printf("SRAM Size: %u KB (%s)\n", 
               cart->sram_size / 1024,
               cart->sram_battery ? "Battery-backed" : "No battery");
    } else {
        printf("SRAM: None\n");
    }
    
    /* Region */
    if (cart->header.country_code < sizeof(region_names) / sizeof(region_names[0])) {
        region_name = region_names[cart->header.country_code];
    } else {
        region_name = "Unknown";
    }
    printf("Region: %s (0x%02X)\n", region_name, cart->header.country_code);
    
    /* Version */
    printf("Version: 1.%u\n", cart->header.version);
    
    /* Checksum */
    printf("Checksum: 0x%04X (Complement: 0x%04X)\n",
           cart->header.checksum, cart->header.checksum_complement);
    
    /* Verify checksum */
    u16 calculated = cartridge_calculate_checksum(cart);
    if (calculated == cart->header.checksum) {
        printf("Checksum: VALID\n");
    } else {
        printf("Checksum: INVALID (calculated: 0x%04X)\n", calculated);
    }
    
    printf("=======================\n\n");
}

u8 cartridge_read(const Cartridge *cart, u32 address) {
    /* Simplified read - full banking to be implemented with memory system */
    if (address < cart->rom_size) {
        return cart->rom_data[address];
    }
    return 0xFF;
}

void cartridge_write(Cartridge *cart, u32 address, u8 value) {
    /* SRAM write handling - to be properly implemented with memory mapper */
    if (cart->has_sram && address < cart->sram_size) {
        cart->sram_data[address] = value;
    }
}

u16 cartridge_calculate_checksum(const Cartridge *cart) {
    u32 sum = 0;
    u32 i;
    
    /* Sum all bytes in ROM */
    for (i = 0; i < cart->rom_size; i++) {
        sum += cart->rom_data[i];
    }
    
    /* Mask to 16 bits */
    return (u16)(sum & 0xFFFF);
}

void cartridge_write_rom(Cartridge *cart, u32 address, u8 value) {
    if (address < cart->rom_size) {
        cart->rom_data[address] = value;
    }
}

void cartridge_update_checksum(Cartridge *cart) {
    u32 header_offset;
    u16 new_checksum;
    u16 new_complement;
    
    /* Calculate new checksum */
    new_checksum = cartridge_calculate_checksum(cart);
    new_complement = ~new_checksum;
    
    /* Update header in ROM data */
    header_offset = (cart->mapper == MAPPER_HIROM) ? 
                    HIROM_HEADER_OFFSET : LOROM_HEADER_OFFSET;
    
    if (header_offset + 0x30 <= cart->rom_size) {
        /* Write checksum complement (offset 0x1C-0x1D) */
        cart->rom_data[header_offset + 0x1C] = new_complement & 0xFF;
        cart->rom_data[header_offset + 0x1D] = (new_complement >> 8) & 0xFF;
        
        /* Write checksum (offset 0x1E-0x1F) */
        cart->rom_data[header_offset + 0x1E] = new_checksum & 0xFF;
        cart->rom_data[header_offset + 0x1F] = (new_checksum >> 8) & 0xFF;
        
        /* Update header structure */
        cart->header.checksum = new_checksum;
        cart->header.checksum_complement = new_complement;
    }
}

int cartridge_save_rom(const Cartridge *cart, const char *filename) {
    FILE *file;
    size_t bytes_written;
    
    if (!cart->rom_data) {
        return ERROR;
    }
    
    file = fopen(filename, "wb");
    if (!file) {
        return ERROR;
    }
    
    bytes_written = fwrite(cart->rom_data, 1, cart->rom_size, file);
    fclose(file);
    
    if (bytes_written != cart->rom_size) {
        return ERROR;
    }
    
    return SUCCESS;
}

int cartridge_backup_rom(Cartridge *cart) {
    if (!cart->rom_data || cart->rom_size == 0) {
        return ERROR;
    }
    
    /* Free existing backup if present */
    if (cart->rom_backup) {
        free(cart->rom_backup);
        cart->rom_backup = NULL;
    }
    
    /* Allocate and copy ROM data */
    cart->rom_backup = (u8 *)malloc(cart->rom_size);
    if (!cart->rom_backup) {
        return ERROR;
    }
    
    memcpy(cart->rom_backup, cart->rom_data, cart->rom_size);
    cart->has_backup = true;
    
    return SUCCESS;
}

int cartridge_restore_rom(Cartridge *cart) {
    if (!cart->rom_backup || !cart->has_backup) {
        return ERROR;
    }
    
    if (!cart->rom_data || cart->rom_size == 0) {
        return ERROR;
    }
    
    /* Restore ROM data from backup */
    memcpy(cart->rom_data, cart->rom_backup, cart->rom_size);
    
    /* Re-parse header */
    cartridge_parse_header(cart);
    
    return SUCCESS;
}
