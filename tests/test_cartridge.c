/*
 * test_cartridge.c - Unit tests for cartridge module
 */

#include "test_framework.h"
#include "../include/cartridge.h"
#include "../include/types.h"
#include <string.h>
#include <stdlib.h>

void test_cartridge_init(void) {
    TEST("Cartridge initialization");
    
    Cartridge cart;
    memset(&cart, 0, sizeof(cart));
    
    ASSERT(cart.rom_data == NULL);
    ASSERT(cart.rom_size == 0);
    ASSERT(cart.has_backup == false);
    
    TEST_PASS();
}

void test_cartridge_write_rom(void) {
    TEST("Cartridge ROM write");
    
    Cartridge cart;
    memset(&cart, 0, sizeof(cart));
    
    /* Allocate test ROM */
    cart.rom_size = 1024;
    cart.rom_data = (u8 *)calloc(cart.rom_size, 1);
    ASSERT(cart.rom_data != NULL);
    
    /* Write some data */
    cartridge_write_rom(&cart, 0x100, 0xAB);
    cartridge_write_rom(&cart, 0x101, 0xCD);
    
    ASSERT_EQ(cart.rom_data[0x100], 0xAB);
    ASSERT_EQ(cart.rom_data[0x101], 0xCD);
    
    /* Test boundary */
    cartridge_write_rom(&cart, 0x3FF, 0xFF);
    ASSERT_EQ(cart.rom_data[0x3FF], 0xFF);
    
    /* Test out of bounds (should not crash) */
    cartridge_write_rom(&cart, 0x10000, 0x00);
    
    free(cart.rom_data);
    TEST_PASS();
}

void test_cartridge_backup_restore(void) {
    TEST("Cartridge backup and restore");
    
    Cartridge cart;
    memset(&cart, 0, sizeof(cart));
    
    /* Allocate test ROM */
    cart.rom_size = 256;
    cart.rom_data = (u8 *)calloc(cart.rom_size, 1);
    ASSERT(cart.rom_data != NULL);
    
    /* Set initial data */
    cart.rom_data[0] = 0x11;
    cart.rom_data[1] = 0x22;
    cart.rom_data[255] = 0xFF;
    
    /* Create backup */
    ASSERT_EQ(cartridge_backup_rom(&cart), SUCCESS);
    ASSERT(cart.has_backup == true);
    ASSERT(cart.rom_backup != NULL);
    
    /* Modify ROM */
    cart.rom_data[0] = 0xAA;
    cart.rom_data[1] = 0xBB;
    
    /* Restore from backup */
    ASSERT_EQ(cartridge_restore_rom(&cart), SUCCESS);
    ASSERT_EQ(cart.rom_data[0], 0x11);
    ASSERT_EQ(cart.rom_data[1], 0x22);
    ASSERT_EQ(cart.rom_data[255], 0xFF);
    
    /* Cleanup */
    free(cart.rom_data);
    free(cart.rom_backup);
    TEST_PASS();
}

void test_cartridge_checksum(void) {
    TEST("Cartridge checksum calculation");
    
    Cartridge cart;
    memset(&cart, 0, sizeof(cart));
    
    /* Allocate test ROM */
    cart.rom_size = 256;
    cart.rom_data = (u8 *)calloc(cart.rom_size, 1);
    ASSERT(cart.rom_data != NULL);
    
    /* Fill with known pattern */
    for (int i = 0; i < 256; i++) {
        cart.rom_data[i] = (u8)i;
    }
    
    u16 checksum = cartridge_calculate_checksum(&cart);
    
    /* Sum of 0 to 255 = 255 * 256 / 2 = 32640 = 0x7F80 */
    ASSERT_EQ(checksum, 0x7F80);
    
    free(cart.rom_data);
    TEST_PASS();
}

void test_cartridge_suite(void) {
    TEST_SUITE("Cartridge Module");
    
    test_cartridge_init();
    test_cartridge_write_rom();
    test_cartridge_backup_restore();
    test_cartridge_checksum();
}
