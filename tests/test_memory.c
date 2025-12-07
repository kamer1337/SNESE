/*
 * test_memory.c - Memory management tests
 * 
 * Tests for SNES memory operations and DMA
 */

#include "test_framework.h"
#include "../include/memory.h"
#include "../include/cartridge.h"
#include <string.h>
#include <stdlib.h>

/* Test memory initialization */
void test_memory_initialization(void) {
    TEST("Memory initialization");
    
    Memory mem;
    memory_init(&mem);
    
    /* Verify memory regions are cleared */
    bool wram_clear = true;
    for (int i = 0; i < 256 && wram_clear; i++) {
        if (mem.wram[i] != 0) wram_clear = false;
    }
    ASSERT(wram_clear);
    
    bool vram_clear = true;
    for (int i = 0; i < 256 && vram_clear; i++) {
        if (mem.vram[i] != 0) vram_clear = false;
    }
    ASSERT(vram_clear);
    
    /* Verify DMA channels are initialized */
    for (int i = 0; i < 8; i++) {
        ASSERT(mem.dma[i].enabled == false);
        ASSERT(mem.dma[i].hdma_enabled == false);
    }
    
    TEST_PASS();
}

/* Test memory reset */
void test_memory_reset(void) {
    TEST("Memory reset operation");
    
    Memory mem;
    memory_init(&mem);
    
    /* Modify some memory */
    mem.wram[0x1000] = 0x42;
    mem.vram[0x1000] = 0x55;
    
    /* Reset */
    memory_reset(&mem);
    
    /* Memory should be cleared */
    ASSERT_EQ(mem.wram[0x1000], 0);
    ASSERT_EQ(mem.vram[0x1000], 0);
    
    TEST_PASS();
}

/* Test WRAM access */
void test_memory_wram_basic(void) {
    TEST("WRAM basic access");
    
    Memory mem;
    memory_init(&mem);
    
    /* Direct WRAM access */
    mem.wram[0x1000] = 0x42;
    ASSERT_EQ(mem.wram[0x1000], 0x42);
    
    /* Test different addresses */
    mem.wram[0] = 0xFF;
    mem.wram[WRAM_SIZE - 1] = 0xAA;
    
    ASSERT_EQ(mem.wram[0], 0xFF);
    ASSERT_EQ(mem.wram[WRAM_SIZE - 1], 0xAA);
    
    TEST_PASS();
}

/* Test VRAM access */
void test_memory_vram_basic(void) {
    TEST("VRAM basic access");
    
    Memory mem;
    memory_init(&mem);
    
    /* Direct VRAM access */
    mem.vram[0x2000] = 0x42;
    ASSERT_EQ(mem.vram[0x2000], 0x42);
    
    /* Test boundaries */
    mem.vram[0] = 0x11;
    mem.vram[VRAM_SIZE - 1] = 0x22;
    
    ASSERT_EQ(mem.vram[0], 0x11);
    ASSERT_EQ(mem.vram[VRAM_SIZE - 1], 0x22);
    
    TEST_PASS();
}

/* Test CGRAM access */
void test_memory_cgram_basic(void) {
    TEST("CGRAM basic access");
    
    Memory mem;
    memory_init(&mem);
    
    /* CGRAM stores palette colors */
    mem.cgram[0] = 0x1F;  /* Red component */
    mem.cgram[1] = 0x00;  /* Green/Blue component */
    
    ASSERT_EQ(mem.cgram[0], 0x1F);
    ASSERT_EQ(mem.cgram[1], 0x00);
    
    /* Test full range */
    for (int i = 0; i < CGRAM_SIZE; i++) {
        mem.cgram[i] = (u8)(i & 0xFF);
    }
    
    for (int i = 0; i < CGRAM_SIZE; i++) {
        ASSERT_EQ(mem.cgram[i], (u8)(i & 0xFF));
    }
    
    TEST_PASS();
}

/* Test OAM access */
void test_memory_oam_basic(void) {
    TEST("OAM basic access");
    
    Memory mem;
    memory_init(&mem);
    
    /* OAM stores sprite attributes */
    mem.oam[0] = 0x50;   /* X position */
    mem.oam[1] = 0x60;   /* Y position */
    mem.oam[2] = 0x10;   /* Tile */
    mem.oam[3] = 0x01;   /* Attributes */
    
    ASSERT_EQ(mem.oam[0], 0x50);
    ASSERT_EQ(mem.oam[1], 0x60);
    ASSERT_EQ(mem.oam[2], 0x10);
    ASSERT_EQ(mem.oam[3], 0x01);
    
    TEST_PASS();
}

/* Test memory read/write 8-bit */
void test_memory_read_write_8(void) {
    TEST("Memory read/write 8-bit");
    
    Memory mem;
    memory_init(&mem);
    
    /* Write to WRAM region */
    u32 addr = 0x7E0100;
    memory_write(&mem, addr, 0x42);
    
    u8 value = memory_read(&mem, addr);
    ASSERT_EQ(value, 0x42);
    
    /* Test different addresses */
    memory_write(&mem, 0x7E0200, 0x55);
    value = memory_read(&mem, 0x7E0200);
    ASSERT_EQ(value, 0x55);
    
    TEST_PASS();
}

/* Test memory read/write 16-bit */
void test_memory_read_write_16(void) {
    TEST("Memory read/write 16-bit");
    
    Memory mem;
    memory_init(&mem);
    
    /* Write 16-bit value */
    u32 addr = 0x7E0100;
    memory_write16(&mem, addr, 0x1234);
    
    u16 value = memory_read16(&mem, addr);
    ASSERT_EQ(value, 0x1234);
    
    TEST_PASS();
}

/* Test memory read/write 24-bit */
void test_memory_read_write_24(void) {
    TEST("Memory read/write 24-bit");
    
    Memory mem;
    memory_init(&mem);
    
    /* Write 24-bit address */
    u32 addr = 0x7E0100;
    mem.wram[0x100] = 0x34;
    mem.wram[0x101] = 0x12;
    mem.wram[0x102] = 0x80;
    
    u32 value = memory_read24(&mem, addr);
    ASSERT_EQ(value, 0x801234);
    
    TEST_PASS();
}

/* Test DMA channel setup */
void test_memory_dma_channel_setup(void) {
    TEST("DMA channel setup");
    
    Memory mem;
    memory_init(&mem);
    
    /* Setup DMA channel 0 */
    u32 src_addr = (0x7E << 16) | 0x1000;  /* Bank:Offset format */
    memory_dma_setup(&mem, 0, 0x00, 0x18, src_addr, 0x100);
    
    ASSERT_EQ(mem.dma[0].dest_register, 0x18);
    ASSERT_EQ(mem.dma[0].transfer_size, 0x100);
    
    TEST_PASS();
}

/* Test DMA channel count */
void test_memory_dma_channels(void) {
    TEST("DMA channel availability");
    
    Memory mem;
    memory_init(&mem);
    
    /* SNES has 8 DMA channels (0-7) */
    for (int i = 0; i < 8; i++) {
        mem.dma[i].enabled = true;
        ASSERT(mem.dma[i].enabled == true);
    }
    
    TEST_PASS();
}

/* Test HDMA setup */
void test_memory_hdma_setup(void) {
    TEST("HDMA channel setup");
    
    Memory mem;
    memory_init(&mem);
    
    /* Setup HDMA channel */
    mem.dma[0].hdma_enabled = true;
    mem.dma[0].hdma_table_bank = 0x7E;
    mem.dma[0].hdma_table_addr = 0x2000;
    mem.dma[0].dest_register = 0x21;
    
    ASSERT(mem.dma[0].hdma_enabled == true);
    ASSERT_EQ(mem.dma[0].hdma_table_bank, 0x7E);
    ASSERT_EQ(mem.dma[0].hdma_table_addr, 0x2000);
    ASSERT_EQ(mem.dma[0].dest_register, 0x21);
    
    TEST_PASS();
}

/* Test cartridge attachment */
void test_memory_cartridge_attach(void) {
    TEST("Cartridge attachment");
    
    Memory mem;
    Cartridge cart;
    
    memory_init(&mem);
    memset(&cart, 0, sizeof(Cartridge));
    
    /* Attach cartridge */
    memory_set_cartridge(&mem, &cart);
    
    ASSERT(mem.cart != NULL);
    ASSERT(mem.cart == &cart);
    
    TEST_PASS();
}

/* Test I/O register space */
void test_memory_io_registers(void) {
    TEST("I/O register space");
    
    Memory mem;
    memory_init(&mem);
    
    /* I/O registers are at $2000-$7FFF */
    mem.io_registers[0x2100] = 0x0F;  /* INIDISP */
    ASSERT_EQ(mem.io_registers[0x2100], 0x0F);
    
    mem.io_registers[0x2105] = 0x01;  /* BGMODE */
    ASSERT_EQ(mem.io_registers[0x2105], 0x01);
    
    TEST_PASS();
}

/* Test memory size constants */
void test_memory_size_constants(void) {
    TEST("Memory size constants");
    
    /* Verify size constants are correct */
    ASSERT_EQ(WRAM_SIZE, 131072);   /* 128KB */
    ASSERT_EQ(VRAM_SIZE, 65536);    /* 64KB */
    ASSERT_EQ(CGRAM_SIZE, 512);     /* 512 bytes */
    ASSERT_EQ(OAM_SIZE, 544);       /* 544 bytes */
    
    TEST_PASS();
}

/* Test suite runner */
void test_memory_suite(void) {
    TEST_SUITE("Memory Module");
    
    test_memory_initialization();
    test_memory_reset();
    test_memory_wram_basic();
    test_memory_vram_basic();
    test_memory_cgram_basic();
    test_memory_oam_basic();
    test_memory_read_write_8();
    test_memory_read_write_16();
    test_memory_read_write_24();
    test_memory_dma_channel_setup();
    test_memory_dma_channels();
    test_memory_hdma_setup();
    test_memory_cartridge_attach();
    test_memory_io_registers();
    test_memory_size_constants();
}
