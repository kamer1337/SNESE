/*
 * cpu.h - 65c816 CPU emulation
 * 
 * Emulates the Ricoh 5A22 (65c816 core) processor used in the SNES.
 */

#ifndef CPU_H
#define CPU_H

#include "types.h"

/* CPU Status flags (Processor Status Register - P) */
#define FLAG_C  0x01  /* Carry */
#define FLAG_Z  0x02  /* Zero */
#define FLAG_I  0x04  /* IRQ Disable */
#define FLAG_D  0x08  /* Decimal mode */
#define FLAG_X  0x10  /* Index register size (0=16-bit, 1=8-bit) */
#define FLAG_M  0x20  /* Accumulator size (0=16-bit, 1=8-bit) */
#define FLAG_V  0x40  /* Overflow */
#define FLAG_N  0x80  /* Negative */

/* CPU Emulation mode flag */
#define FLAG_E  0x01  /* Emulation mode (when 1, CPU acts like 6502) */

/* Interrupt vectors (in bank $00) */
#define VECTOR_NATIVE_COP   0xFFE4
#define VECTOR_NATIVE_BRK   0xFFE6
#define VECTOR_NATIVE_ABORT 0xFFE8
#define VECTOR_NATIVE_NMI   0xFFEA
#define VECTOR_NATIVE_IRQ   0xFFEE

#define VECTOR_EMULATION_COP   0xFFF4
#define VECTOR_EMULATION_ABORT 0xFFF8
#define VECTOR_EMULATION_NMI   0xFFFA
#define VECTOR_EMULATION_RESET 0xFFFC
#define VECTOR_EMULATION_IRQ   0xFFFE

/* CPU structure */
typedef struct {
    /* Registers */
    u16 a;      /* Accumulator (16-bit in native mode, 8-bit in emulation) */
    u16 x;      /* X index register */
    u16 y;      /* Y index register */
    u16 sp;     /* Stack pointer */
    u16 pc;     /* Program counter */
    u16 d;      /* Direct page register */
    
    u8 dbr;     /* Data bank register */
    u8 pbr;     /* Program bank register */
    u8 p;       /* Processor status register */
    u8 e;       /* Emulation mode flag */
    
    /* Internal state */
    u64 cycles;          /* Total cycles executed */
    u32 cycles_per_frame; /* Cycles to execute per frame (~89342 for 60Hz) */
    bool stopped;        /* CPU stopped flag */
    bool waiting;        /* CPU waiting for interrupt */
    
    /* Interrupt flags */
    bool nmi_pending;
    bool irq_pending;
    
    /* Timing */
    u32 instruction_cycles; /* Cycles for current instruction */
    
    /* Debug/Breakpoint support */
    u32 breakpoints[8];  /* Up to 8 breakpoints (24-bit addresses) */
    u8 breakpoint_count; /* Number of active breakpoints */
    bool breakpoint_hit; /* Flag indicating breakpoint was hit */
} CPU;

/* Function declarations */

/*
 * Initialize CPU to power-on state
 */
void cpu_init(CPU *cpu);

/*
 * Reset CPU to reset vector
 */
void cpu_reset(CPU *cpu);

/*
 * Execute one CPU instruction
 * Returns number of cycles executed
 */
u32 cpu_step(CPU *cpu);

/*
 * Execute CPU for specified number of cycles
 */
void cpu_run(CPU *cpu, u32 cycles);

/*
 * Trigger NMI interrupt
 */
void cpu_nmi(CPU *cpu);

/*
 * Trigger IRQ interrupt
 */
void cpu_irq(CPU *cpu);

/*
 * Get/Set status flags */
u8 cpu_get_flag(const CPU *cpu, u8 flag);
void cpu_set_flag(CPU *cpu, u8 flag, bool value);

/*
 * Print CPU state for debugging
 */
void cpu_print_state(const CPU *cpu);

/*
 * Disassemble instruction at current PC
 */
void cpu_disassemble(const CPU *cpu, char *buffer, size_t size);

/* Breakpoint management */

/*
 * Add a breakpoint at the specified 24-bit address
 * Returns true if added successfully, false if breakpoint table is full
 */
bool cpu_add_breakpoint(CPU *cpu, u32 address);

/*
 * Remove a breakpoint at the specified 24-bit address
 * Returns true if removed successfully, false if not found
 */
bool cpu_remove_breakpoint(CPU *cpu, u32 address);

/*
 * Clear all breakpoints
 */
void cpu_clear_breakpoints(CPU *cpu);

/*
 * Check if there's a breakpoint at the current PC
 * Returns true if breakpoint exists at current address
 */
bool cpu_check_breakpoint(const CPU *cpu);

/* Addressing modes - internal functions */
u32 cpu_addr_immediate(CPU *cpu);
u32 cpu_addr_absolute(CPU *cpu);
u32 cpu_addr_absolute_x(CPU *cpu);
u32 cpu_addr_absolute_y(CPU *cpu);
u32 cpu_addr_direct(CPU *cpu);
u32 cpu_addr_direct_x(CPU *cpu);
u32 cpu_addr_direct_y(CPU *cpu);
u32 cpu_addr_indirect(CPU *cpu);
u32 cpu_addr_indexed_indirect(CPU *cpu);
u32 cpu_addr_indirect_indexed(CPU *cpu);
u32 cpu_addr_stack_relative(CPU *cpu);

#endif /* CPU_H */
