/*
 * test_script.c - Unit tests for scripting module
 */

#include "test_framework.h"
#include "../include/script.h"
#include "../include/cartridge.h"
#include "../include/memory.h"
#include <string.h>
#include <stdlib.h>

void test_script_parse_set(void) {
    TEST("Script parse SET command");
    
    ScriptCommand cmd = script_parse_line("SET 1000 FF");
    
    ASSERT_EQ(cmd.type, SCRIPT_CMD_SET);
    ASSERT_EQ(cmd.address, 0x1000);
    ASSERT_EQ(cmd.value, 0xFF);
    
    TEST_PASS();
}

void test_script_parse_set16(void) {
    TEST("Script parse SET16 command");
    
    ScriptCommand cmd = script_parse_line("SET16 2000 ABCD");
    
    ASSERT_EQ(cmd.type, SCRIPT_CMD_SET16);
    ASSERT_EQ(cmd.address, 0x2000);
    ASSERT_EQ(cmd.value16, 0xABCD);
    
    TEST_PASS();
}

void test_script_parse_fill(void) {
    TEST("Script parse FILL command");
    
    ScriptCommand cmd = script_parse_line("FILL 3000 100 AA");
    
    ASSERT_EQ(cmd.type, SCRIPT_CMD_FILL);
    ASSERT_EQ(cmd.address, 0x3000);
    ASSERT_EQ(cmd.size, 0x100);
    ASSERT_EQ(cmd.value, 0xAA);
    
    TEST_PASS();
}

void test_script_parse_copy(void) {
    TEST("Script parse COPY command");
    
    ScriptCommand cmd = script_parse_line("COPY 1000 2000 200");
    
    ASSERT_EQ(cmd.type, SCRIPT_CMD_COPY);
    ASSERT_EQ(cmd.address, 0x1000);
    ASSERT_EQ(cmd.address2, 0x2000);
    ASSERT_EQ(cmd.size, 0x200);
    
    TEST_PASS();
}

void test_script_parse_comment(void) {
    TEST("Script parse comment");
    
    ScriptCommand cmd1 = script_parse_line("; This is a comment");
    ASSERT_EQ(cmd1.type, SCRIPT_CMD_COMMENT);
    
    ScriptCommand cmd2 = script_parse_line("# Another comment");
    ASSERT_EQ(cmd2.type, SCRIPT_CMD_COMMENT);
    
    TEST_PASS();
}

void test_script_parse_label(void) {
    TEST("Script parse label");
    
    ScriptCommand cmd = script_parse_line(":mylabel");
    
    ASSERT_EQ(cmd.type, SCRIPT_CMD_LABEL);
    ASSERT_STR_EQ(cmd.label, "mylabel");
    
    TEST_PASS();
}

void test_script_execute_set(void) {
    TEST("Script execute SET command");
    
    Cartridge cart;
    Memory mem;
    ScriptContext ctx;
    
    memset(&cart, 0, sizeof(cart));
    memset(&mem, 0, sizeof(mem));
    
    cart.rom_size = 4096;
    cart.rom_data = (u8 *)calloc(cart.rom_size, 1);
    ASSERT(cart.rom_data != NULL);
    
    script_init(&ctx, &cart, &mem);
    
    ScriptCommand cmd = script_parse_line("SET 100 42");
    ASSERT_EQ(script_execute(&ctx, &cmd), SUCCESS);
    ASSERT_EQ(cart.rom_data[0x100], 0x42);
    
    free(cart.rom_data);
    TEST_PASS();
}

void test_script_execute_fill(void) {
    TEST("Script execute FILL command");
    
    Cartridge cart;
    Memory mem;
    ScriptContext ctx;
    
    memset(&cart, 0, sizeof(cart));
    memset(&mem, 0, sizeof(mem));
    
    cart.rom_size = 4096;
    cart.rom_data = (u8 *)calloc(cart.rom_size, 1);
    ASSERT(cart.rom_data != NULL);
    
    script_init(&ctx, &cart, &mem);
    
    ScriptCommand cmd = script_parse_line("FILL 200 10 AA");
    ASSERT_EQ(script_execute(&ctx, &cmd), SUCCESS);
    
    /* Check filled bytes */
    for (int i = 0; i < 0x10; i++) {
        ASSERT_EQ(cart.rom_data[0x200 + i], 0xAA);
    }
    
    /* Check boundary */
    ASSERT_EQ(cart.rom_data[0x1FF], 0x00);
    ASSERT_EQ(cart.rom_data[0x210], 0x00);
    
    free(cart.rom_data);
    TEST_PASS();
}

void test_script_execute_string(void) {
    TEST("Script execute from string");
    
    Cartridge cart;
    Memory mem;
    ScriptContext ctx;
    
    memset(&cart, 0, sizeof(cart));
    memset(&mem, 0, sizeof(mem));
    
    cart.rom_size = 4096;
    cart.rom_data = (u8 *)calloc(cart.rom_size, 1);
    ASSERT(cart.rom_data != NULL);
    
    script_init(&ctx, &cart, &mem);
    
    const char *script = 
        "; Test script\n"
        "SET 100 11\n"
        "SET 101 22\n"
        "SET 102 33\n";
    
    ASSERT_EQ(script_execute_string(&ctx, script), SUCCESS);
    ASSERT_EQ(cart.rom_data[0x100], 0x11);
    ASSERT_EQ(cart.rom_data[0x101], 0x22);
    ASSERT_EQ(cart.rom_data[0x102], 0x33);
    
    free(cart.rom_data);
    TEST_PASS();
}

void test_script_suite(void) {
    TEST_SUITE("Script Module");
    
    test_script_parse_set();
    test_script_parse_set16();
    test_script_parse_fill();
    test_script_parse_copy();
    test_script_parse_comment();
    test_script_parse_label();
    test_script_execute_set();
    test_script_execute_fill();
    test_script_execute_string();
}
