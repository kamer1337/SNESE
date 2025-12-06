/*
 * script.c - Simple scripting layer implementation
 * Compatible with GCC 15.2+ on Windows 11 x64 and Linux
 */

/* Enable POSIX functions on Linux (must be before any includes) */
#ifndef _WIN32
    #ifndef _POSIX_C_SOURCE
        #define _POSIX_C_SOURCE 200809L
    #endif
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "../include/script.h"
#include "../include/cartridge.h"

/* Platform-specific string functions */
#ifdef _WIN32
    #define strcasecmp _stricmp
    
    /* Windows implementation of strdup */
    static char *strdup_impl(const char *s) {
        size_t len = strlen(s) + 1;
        char *copy = (char *)malloc(len);
        if (copy) {
            memcpy(copy, s, len);
        }
        return copy;
    }
    #define strdup strdup_impl
    
    /* Windows implementation of strtok_r */
    static char *strtok_r_impl(char *str, const char *delim, char **saveptr) {
        char *token;
        if (str == NULL) {
            str = *saveptr;
        }
        /* Skip leading delimiters */
        str += strspn(str, delim);
        if (*str == '\0') {
            *saveptr = str;
            return NULL;
        }
        /* Find end of token */
        token = str;
        str = strpbrk(token, delim);
        if (str == NULL) {
            *saveptr = token + strlen(token);
        } else {
            *str = '\0';
            *saveptr = str + 1;
        }
        return token;
    }
    #define strtok_r strtok_r_impl
#else
    /* POSIX systems have strings.h and native strtok_r/strdup */
    #include <strings.h>
#endif

/* Helper to trim whitespace */
static char *trim(char *str) {
    char *end;
    
    /* Trim leading space */
    while (isspace((unsigned char)*str)) str++;
    
    if (*str == '\0') return str;
    
    /* Trim trailing space */
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    
    end[1] = '\0';
    return str;
}

void script_init(ScriptContext *ctx, Cartridge *cart, Memory *mem) {
    memset(ctx, 0, sizeof(ScriptContext));
    ctx->cart = cart;
    ctx->mem = mem;
    ctx->line_number = 0;
    ctx->error_occurred = false;
    ctx->label_count = 0;
}

ScriptCommand script_parse_line(const char *line) {
    ScriptCommand cmd;
    char line_copy[MAX_SCRIPT_LINE];
    char *token;
    char *rest;
    
    memset(&cmd, 0, sizeof(cmd));
    cmd.type = SCRIPT_CMD_NONE;
    
    /* Copy and trim line */
    strncpy(line_copy, line, sizeof(line_copy) - 1);
    line_copy[sizeof(line_copy) - 1] = '\0';
    rest = trim(line_copy);
    
    /* Skip empty lines */
    if (rest[0] == '\0') {
        return cmd;
    }
    
    /* Check for comment */
    if (rest[0] == ';' || rest[0] == '#') {
        cmd.type = SCRIPT_CMD_COMMENT;
        return cmd;
    }
    
    /* Check for label */
    if (rest[0] == ':') {
        cmd.type = SCRIPT_CMD_LABEL;
        strncpy(cmd.label, rest + 1, sizeof(cmd.label) - 1);
        return cmd;
    }
    
    /* Parse command token */
    token = strtok(rest, " \t");
    if (!token) {
        cmd.type = SCRIPT_CMD_ERROR;
        snprintf(cmd.error_msg, sizeof(cmd.error_msg), "Empty command");
        return cmd;
    }
    
    /* SET command: SET addr value */
    if (strcasecmp(token, "SET") == 0) {
        cmd.type = SCRIPT_CMD_SET;
        
        token = strtok(NULL, " \t");
        if (!token || sscanf(token, "%x", &cmd.address) != 1) {
            cmd.type = SCRIPT_CMD_ERROR;
            snprintf(cmd.error_msg, sizeof(cmd.error_msg), "Invalid address");
            return cmd;
        }
        
        token = strtok(NULL, " \t");
        if (!token) {
            cmd.type = SCRIPT_CMD_ERROR;
            snprintf(cmd.error_msg, sizeof(cmd.error_msg), "Missing value");
            return cmd;
        }
        
        unsigned int val;
        if (sscanf(token, "%x", &val) != 1) {
            cmd.type = SCRIPT_CMD_ERROR;
            snprintf(cmd.error_msg, sizeof(cmd.error_msg), "Invalid value");
            return cmd;
        }
        cmd.value = (u8)val;
    }
    /* SET16 command: SET16 addr value */
    else if (strcasecmp(token, "SET16") == 0) {
        cmd.type = SCRIPT_CMD_SET16;
        
        token = strtok(NULL, " \t");
        if (!token || sscanf(token, "%x", &cmd.address) != 1) {
            cmd.type = SCRIPT_CMD_ERROR;
            snprintf(cmd.error_msg, sizeof(cmd.error_msg), "Invalid address");
            return cmd;
        }
        
        token = strtok(NULL, " \t");
        if (!token) {
            cmd.type = SCRIPT_CMD_ERROR;
            snprintf(cmd.error_msg, sizeof(cmd.error_msg), "Missing value");
            return cmd;
        }
        
        unsigned int val;
        if (sscanf(token, "%x", &val) != 1) {
            cmd.type = SCRIPT_CMD_ERROR;
            snprintf(cmd.error_msg, sizeof(cmd.error_msg), "Invalid value");
            return cmd;
        }
        cmd.value16 = (u16)val;
    }
    /* FILL command: FILL addr size value */
    else if (strcasecmp(token, "FILL") == 0) {
        cmd.type = SCRIPT_CMD_FILL;
        
        token = strtok(NULL, " \t");
        if (!token || sscanf(token, "%x", &cmd.address) != 1) {
            cmd.type = SCRIPT_CMD_ERROR;
            snprintf(cmd.error_msg, sizeof(cmd.error_msg), "Invalid address");
            return cmd;
        }
        
        token = strtok(NULL, " \t");
        if (!token || sscanf(token, "%x", &cmd.size) != 1) {
            cmd.type = SCRIPT_CMD_ERROR;
            snprintf(cmd.error_msg, sizeof(cmd.error_msg), "Invalid size");
            return cmd;
        }
        
        token = strtok(NULL, " \t");
        if (!token) {
            cmd.type = SCRIPT_CMD_ERROR;
            snprintf(cmd.error_msg, sizeof(cmd.error_msg), "Missing value");
            return cmd;
        }
        
        unsigned int val;
        if (sscanf(token, "%x", &val) != 1) {
            cmd.type = SCRIPT_CMD_ERROR;
            snprintf(cmd.error_msg, sizeof(cmd.error_msg), "Invalid value");
            return cmd;
        }
        cmd.value = (u8)val;
    }
    /* COPY command: COPY src dest size */
    else if (strcasecmp(token, "COPY") == 0) {
        cmd.type = SCRIPT_CMD_COPY;
        
        token = strtok(NULL, " \t");
        if (!token || sscanf(token, "%x", &cmd.address) != 1) {
            cmd.type = SCRIPT_CMD_ERROR;
            snprintf(cmd.error_msg, sizeof(cmd.error_msg), "Invalid source address");
            return cmd;
        }
        
        token = strtok(NULL, " \t");
        if (!token || sscanf(token, "%x", &cmd.address2) != 1) {
            cmd.type = SCRIPT_CMD_ERROR;
            snprintf(cmd.error_msg, sizeof(cmd.error_msg), "Invalid dest address");
            return cmd;
        }
        
        token = strtok(NULL, " \t");
        if (!token || sscanf(token, "%x", &cmd.size) != 1) {
            cmd.type = SCRIPT_CMD_ERROR;
            snprintf(cmd.error_msg, sizeof(cmd.error_msg), "Invalid size");
            return cmd;
        }
    }
    /* CHECKSUM command */
    else if (strcasecmp(token, "CHECKSUM") == 0) {
        cmd.type = SCRIPT_CMD_CHECKSUM;
    }
    /* Unknown command */
    else {
        cmd.type = SCRIPT_CMD_ERROR;
        snprintf(cmd.error_msg, sizeof(cmd.error_msg), "Unknown command: %s", token);
    }
    
    return cmd;
}

int script_execute(ScriptContext *ctx, const ScriptCommand *cmd) {
    if (!ctx || !cmd) {
        return ERROR;
    }
    
    switch (cmd->type) {
        case SCRIPT_CMD_NONE:
        case SCRIPT_CMD_COMMENT:
            /* Do nothing */
            break;
            
        case SCRIPT_CMD_LABEL:
            /* Store label for later GOTO */
            if (ctx->label_count < 32) {
                strncpy(ctx->labels[ctx->label_count].name, cmd->label, 63);
                ctx->labels[ctx->label_count].name[63] = '\0';
                ctx->labels[ctx->label_count].line = ctx->line_number;
                ctx->label_count++;
            }
            break;
            
        case SCRIPT_CMD_SET:
            if (ctx->cart && cmd->address < ctx->cart->rom_size) {
                cartridge_write_rom(ctx->cart, cmd->address, cmd->value);
            } else {
                script_set_error(ctx, "SET: Address out of range");
                return ERROR;
            }
            break;
            
        case SCRIPT_CMD_SET16:
            if (ctx->cart && cmd->address + 1 < ctx->cart->rom_size) {
                cartridge_write_rom(ctx->cart, cmd->address, cmd->value16 & 0xFF);
                cartridge_write_rom(ctx->cart, cmd->address + 1, (cmd->value16 >> 8) & 0xFF);
            } else {
                script_set_error(ctx, "SET16: Address out of range");
                return ERROR;
            }
            break;
            
        case SCRIPT_CMD_FILL:
            if (ctx->cart && cmd->address + cmd->size <= ctx->cart->rom_size) {
                for (u32 i = 0; i < cmd->size; i++) {
                    cartridge_write_rom(ctx->cart, cmd->address + i, cmd->value);
                }
            } else {
                script_set_error(ctx, "FILL: Address/size out of range");
                return ERROR;
            }
            break;
            
        case SCRIPT_CMD_COPY:
            if (ctx->cart && 
                cmd->address + cmd->size <= ctx->cart->rom_size &&
                cmd->address2 + cmd->size <= ctx->cart->rom_size) {
                for (u32 i = 0; i < cmd->size; i++) {
                    u8 byte = cartridge_read(ctx->cart, cmd->address + i);
                    cartridge_write_rom(ctx->cart, cmd->address2 + i, byte);
                }
            } else {
                script_set_error(ctx, "COPY: Address/size out of range");
                return ERROR;
            }
            break;
            
        case SCRIPT_CMD_CHECKSUM:
            if (ctx->cart) {
                cartridge_update_checksum(ctx->cart);
            } else {
                script_set_error(ctx, "CHECKSUM: No ROM loaded");
                return ERROR;
            }
            break;
            
        case SCRIPT_CMD_ERROR:
            script_set_error(ctx, cmd->error_msg);
            return ERROR;
            
        default:
            script_set_error(ctx, "Unimplemented command");
            return ERROR;
    }
    
    return SUCCESS;
}

int script_execute_file(ScriptContext *ctx, const char *filename) {
    FILE *file;
    char line[MAX_SCRIPT_LINE];
    
    if (!ctx || !filename) {
        return ERROR;
    }
    
    file = fopen(filename, "r");
    if (!file) {
        script_set_error(ctx, "Cannot open script file");
        return ERROR;
    }
    
    strncpy(ctx->current_file, filename, sizeof(ctx->current_file) - 1);
    ctx->line_number = 0;
    ctx->error_occurred = false;
    
    while (fgets(line, sizeof(line), file)) {
        ctx->line_number++;
        
        ScriptCommand cmd = script_parse_line(line);
        if (script_execute(ctx, &cmd) != SUCCESS) {
            fclose(file);
            return ERROR;
        }
    }
    
    fclose(file);
    return SUCCESS;
}

int script_execute_string(ScriptContext *ctx, const char *script) {
    char *script_copy;
    char *line;
    char *saveptr;
    
    if (!ctx || !script) {
        return ERROR;
    }
    
    /* Copy script string for strtok_r */
    script_copy = strdup(script);
    if (!script_copy) {
        script_set_error(ctx, "Memory allocation failed");
        return ERROR;
    }
    
    ctx->line_number = 0;
    ctx->error_occurred = false;
    
    line = strtok_r(script_copy, "\n", &saveptr);
    while (line) {
        ctx->line_number++;
        
        ScriptCommand cmd = script_parse_line(line);
        if (script_execute(ctx, &cmd) != SUCCESS) {
            free(script_copy);
            return ERROR;
        }
        
        line = strtok_r(NULL, "\n", &saveptr);
    }
    
    free(script_copy);
    return SUCCESS;
}

const char *script_get_error(const ScriptContext *ctx) {
    if (ctx && ctx->error_occurred) {
        return ctx->last_error;
    }
    return NULL;
}

void script_set_error(ScriptContext *ctx, const char *message) {
    if (ctx && message) {
        snprintf(ctx->last_error, sizeof(ctx->last_error),
                 "Line %u: %s", ctx->line_number, message);
        ctx->error_occurred = true;
    }
}

void script_print_help(void) {
    printf("\n=== SNESE Scripting Language Help ===\n\n");
    printf("Commands:\n");
    printf("  SET addr value        - Set byte at address to value (hex)\n");
    printf("  SET16 addr value      - Set 16-bit word at address (hex, little-endian)\n");
    printf("  FILL addr size value  - Fill memory region with value\n");
    printf("  COPY src dest size    - Copy memory from src to dest\n");
    printf("  CHECKSUM              - Update ROM checksum\n");
    printf("  ; comment             - Comment line (ignored)\n");
    printf("  :label                - Define a label\n");
    printf("\n");
    printf("Examples:\n");
    printf("  SET 10000 FF          - Set byte at 0x10000 to 0xFF\n");
    printf("  SET16 7FFC 8000       - Set word at 0x7FFC to 0x8000\n");
    printf("  FILL 20000 100 00     - Fill 256 bytes at 0x20000 with 0x00\n");
    printf("  COPY 10000 20000 100  - Copy 256 bytes from 0x10000 to 0x20000\n");
    printf("  CHECKSUM              - Recalculate and update ROM checksum\n");
    printf("\n");
    printf("Notes:\n");
    printf("  - All numbers are in hexadecimal\n");
    printf("  - Addresses are ROM addresses (not SNES addresses)\n");
    printf("  - Commands are case-insensitive\n");
    printf("  - Lines starting with ; or # are comments\n");
    printf("\n");
}
