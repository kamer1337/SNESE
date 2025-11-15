/*
 * types.h - Common type definitions for SNESE
 * 
 * Provides standard integer types and common definitions
 * used throughout the emulator.
 */

#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>
#include <stdbool.h>

/* Standard integer types (for clarity) */
typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t   s8;
typedef int16_t  s16;
typedef int32_t  s32;
typedef int64_t  s64;

/* Boolean type */
#ifndef bool
typedef uint8_t bool;
#define true  1
#define false 0
#endif

/* Common constants */
#define KB 1024
#define MB (1024 * KB)

/* Memory sizes */
#define WRAM_SIZE      (128 * KB)  /* Work RAM */
#define VRAM_SIZE      (64 * KB)   /* Video RAM */
#define CGRAM_SIZE     512         /* Color Generator RAM (palette) */
#define OAM_SIZE       544         /* Object Attribute Memory (sprites) */
#define SRAM_MAX_SIZE  (256 * KB)  /* Max cartridge save RAM */

/* ROM header location (for LoROM/HiROM detection) */
#define LOROM_HEADER_OFFSET 0x7FC0
#define HIROM_HEADER_OFFSET 0xFFC0

/* Return codes */
#define SUCCESS  0
#define ERROR   -1

#endif /* TYPES_H */
