/*
 * script.h - Simple scripting layer for ROM modifications
 * 
 * Provides a simple assembly-like DSL for ROM patching and modifications.
 */

#ifndef SCRIPT_H
#define SCRIPT_H

#include "types.h"
#include "cartridge.h"
#include "memory.h"

/* Maximum script line length */
#define MAX_SCRIPT_LINE 256

/* Script command types */
typedef enum {
    SCRIPT_CMD_NONE = 0,
    SCRIPT_CMD_SET,        /* SET addr value - Set byte at address */
    SCRIPT_CMD_SET16,      /* SET16 addr value - Set 16-bit word */
    SCRIPT_CMD_COPY,       /* COPY src dest size - Copy memory */
    SCRIPT_CMD_FILL,       /* FILL addr size value - Fill memory */
    SCRIPT_CMD_PATCH,      /* PATCH addr bytes... - Patch multiple bytes */
    SCRIPT_CMD_LOAD,       /* LOAD addr file - Load data from file */
    SCRIPT_CMD_SAVE,       /* SAVE addr size file - Save data to file */
    SCRIPT_CMD_CHECKSUM,   /* CHECKSUM - Update ROM checksum */
    SCRIPT_CMD_COMMENT,    /* ; comment - Comment line */
    SCRIPT_CMD_LABEL,      /* :label - Label for jumps */
    SCRIPT_CMD_GOTO,       /* GOTO label - Jump to label */
    SCRIPT_CMD_ERROR       /* Parse error */
} ScriptCommandType;

/* Script command structure */
typedef struct {
    ScriptCommandType type;
    u32 address;
    u32 address2;
    u32 size;
    u8 value;
    u16 value16;
    u8 data[256];
    char label[64];
    char filename[256];
    char error_msg[128];
} ScriptCommand;

/* Script execution context */
typedef struct {
    Cartridge *cart;
    Memory *mem;
    char current_file[256];
    u32 line_number;
    bool error_occurred;
    char last_error[256];
    
    /* Label table for GOTO */
    struct {
        char name[64];
        u32 line;
    } labels[32];
    u8 label_count;
} ScriptContext;

/* Function declarations */

/*
 * Initialize script context
 */
void script_init(ScriptContext *ctx, Cartridge *cart, Memory *mem);

/*
 * Parse a single script line
 */
ScriptCommand script_parse_line(const char *line);

/*
 * Execute a script command
 */
int script_execute(ScriptContext *ctx, const ScriptCommand *cmd);

/*
 * Execute script from file
 */
int script_execute_file(ScriptContext *ctx, const char *filename);

/*
 * Execute script from string
 */
int script_execute_string(ScriptContext *ctx, const char *script);

/*
 * Get last error message
 */
const char *script_get_error(const ScriptContext *ctx);

/*
 * Set error message
 */
void script_set_error(ScriptContext *ctx, const char *message);

/*
 * Print command help
 */
void script_print_help(void);

#endif /* SCRIPT_H */
