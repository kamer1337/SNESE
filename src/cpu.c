/*
 * cpu.c - 65c816 CPU emulation implementation
 */

#include <stdio.h>
#include <string.h>
#include "../include/cpu.h"
#include "../include/memory.h"

/* External memory system (to be linked) */
extern Memory g_memory;

void cpu_init(CPU *cpu) {
    memset(cpu, 0, sizeof(CPU));
    cpu_reset(cpu);
}

void cpu_reset(CPU *cpu) {
    /* Reset state to power-on defaults */
    cpu->a = 0;
    cpu->x = 0;
    cpu->y = 0;
    cpu->d = 0;
    cpu->dbr = 0;
    cpu->pbr = 0;
    
    /* Set emulation mode */
    cpu->e = 1;
    
    /* Set processor status flags */
    cpu->p = FLAG_I | FLAG_M | FLAG_X;  /* IRQ disabled, 8-bit registers */
    
    /* Stack pointer set to $01FF in emulation mode */
    cpu->sp = 0x01FF;
    
    /* Read reset vector from $FFFC */
    cpu->pc = memory_read16(&g_memory, VECTOR_EMULATION_RESET);
    
    /* Reset cycle counter */
    cpu->cycles = 0;
    cpu->stopped = false;
    cpu->waiting = false;
    
    /* Clear interrupt flags */
    cpu->nmi_pending = false;
    cpu->irq_pending = false;
    
    printf("CPU reset: PC=$%04X\n", cpu->pc);
}

u8 cpu_get_flag(const CPU *cpu, u8 flag) {
    return (cpu->p & flag) ? 1 : 0;
}

void cpu_set_flag(CPU *cpu, u8 flag, bool value) {
    if (value) {
        cpu->p |= flag;
    } else {
        cpu->p &= ~flag;
    }
}

void cpu_print_state(const CPU *cpu) {
    printf("CPU State:\n");
    printf("  PC: $%02X:%04X\n", cpu->pbr, cpu->pc);
    printf("  A: $%04X  X: $%04X  Y: $%04X\n", cpu->a, cpu->x, cpu->y);
    printf("  SP: $%04X  D: $%04X\n", cpu->sp, cpu->d);
    printf("  DBR: $%02X  P: $%02X [", cpu->dbr, cpu->p);
    
    /* Print flag states */
    printf("%c", cpu_get_flag(cpu, FLAG_N) ? 'N' : '-');
    printf("%c", cpu_get_flag(cpu, FLAG_V) ? 'V' : '-');
    printf("%c", cpu_get_flag(cpu, FLAG_M) ? 'M' : '-');
    printf("%c", cpu_get_flag(cpu, FLAG_X) ? 'X' : '-');
    printf("%c", cpu_get_flag(cpu, FLAG_D) ? 'D' : '-');
    printf("%c", cpu_get_flag(cpu, FLAG_I) ? 'I' : '-');
    printf("%c", cpu_get_flag(cpu, FLAG_Z) ? 'Z' : '-');
    printf("%c", cpu_get_flag(cpu, FLAG_C) ? 'C' : '-');
    printf("]\n");
    
    printf("  E: %d (", cpu->e);
    printf(cpu->e ? "Emulation" : "Native");
    printf(" mode)\n");
    
    printf("  Cycles: %lu\n", (unsigned long)cpu->cycles);
}

/* Push byte to stack */
static void cpu_push8(CPU *cpu, u8 value) {
    if (cpu->e) {
        /* Emulation mode: stack in page 1 ($0100-$01FF) */
        memory_write(&g_memory, 0x0100 | (cpu->sp & 0xFF), value);
        cpu->sp = 0x0100 | ((cpu->sp - 1) & 0xFF);
    } else {
        /* Native mode: full 16-bit stack pointer */
        memory_write(&g_memory, cpu->sp, value);
        cpu->sp--;
    }
}

/* Push 16-bit word to stack */
static void cpu_push16(CPU *cpu, u16 value) {
    cpu_push8(cpu, (value >> 8) & 0xFF);  /* High byte first */
    cpu_push8(cpu, value & 0xFF);         /* Low byte second */
}

/* Pull byte from stack */
static u8 cpu_pull8(CPU *cpu) {
    if (cpu->e) {
        /* Emulation mode */
        cpu->sp = 0x0100 | ((cpu->sp + 1) & 0xFF);
        return memory_read(&g_memory, 0x0100 | (cpu->sp & 0xFF));
    } else {
        /* Native mode */
        cpu->sp++;
        return memory_read(&g_memory, cpu->sp);
    }
}

/* Pull 16-bit word from stack (used later for RTI/RTS instructions) */
__attribute__((unused))
static u16 cpu_pull16(CPU *cpu) {
    u8 low = cpu_pull8(cpu);
    u8 high = cpu_pull8(cpu);
    return low | (high << 8);
}

void cpu_nmi(CPU *cpu) {
    cpu->nmi_pending = true;
}

void cpu_irq(CPU *cpu) {
    if (!cpu_get_flag(cpu, FLAG_I)) {
        cpu->irq_pending = true;
    }
}

/* Handle NMI interrupt */
static void cpu_handle_nmi(CPU *cpu) {
    u16 vector;
    
    /* Push return address and status */
    cpu_push8(cpu, cpu->pbr);
    cpu_push16(cpu, cpu->pc);
    cpu_push8(cpu, cpu->p);
    
    /* Get interrupt vector */
    if (cpu->e) {
        vector = memory_read16(&g_memory, VECTOR_EMULATION_NMI);
    } else {
        vector = memory_read16(&g_memory, VECTOR_NATIVE_NMI);
    }
    
    /* Set PC to interrupt handler */
    cpu->pc = vector;
    cpu->pbr = 0;
    
    /* Set interrupt disable flag */
    cpu_set_flag(cpu, FLAG_I, true);
    
    cpu->nmi_pending = false;
    cpu->instruction_cycles += 7;  /* NMI takes 7 cycles */
}

/* Handle IRQ interrupt */
static void cpu_handle_irq(CPU *cpu) {
    u16 vector;
    
    /* Push return address and status */
    cpu_push8(cpu, cpu->pbr);
    cpu_push16(cpu, cpu->pc);
    cpu_push8(cpu, cpu->p);
    
    /* Get interrupt vector */
    if (cpu->e) {
        vector = memory_read16(&g_memory, VECTOR_EMULATION_IRQ);
    } else {
        vector = memory_read16(&g_memory, VECTOR_NATIVE_IRQ);
    }
    
    /* Set PC to interrupt handler */
    cpu->pc = vector;
    cpu->pbr = 0;
    
    /* Set interrupt disable flag */
    cpu_set_flag(cpu, FLAG_I, true);
    
    cpu->irq_pending = false;
    cpu->instruction_cycles += 7;  /* IRQ takes 7 cycles */
}

u32 cpu_step(CPU *cpu) {
    u8 opcode;
    
    if (cpu->stopped) {
        return 1;
    }
    
    /* Handle pending interrupts */
    if (cpu->nmi_pending) {
        cpu_handle_nmi(cpu);
        return cpu->instruction_cycles;
    }
    
    if (cpu->irq_pending && !cpu_get_flag(cpu, FLAG_I)) {
        cpu_handle_irq(cpu);
        return cpu->instruction_cycles;
    }
    
    /* Reset instruction cycle counter */
    cpu->instruction_cycles = 0;
    
    /* Fetch opcode */
    opcode = memory_read(&g_memory, (cpu->pbr << 16) | cpu->pc);
    cpu->pc++;
    
    /* Execute instruction (simplified - only a few opcodes for now) */
    switch (opcode) {
        case 0xEA:  /* NOP */
            cpu->instruction_cycles = 2;
            break;
            
        case 0x18:  /* CLC - Clear Carry */
            cpu_set_flag(cpu, FLAG_C, false);
            cpu->instruction_cycles = 2;
            break;
            
        case 0x38:  /* SEC - Set Carry */
            cpu_set_flag(cpu, FLAG_C, true);
            cpu->instruction_cycles = 2;
            break;
            
        case 0x78:  /* SEI - Set Interrupt Disable */
            cpu_set_flag(cpu, FLAG_I, true);
            cpu->instruction_cycles = 2;
            break;
            
        case 0x58:  /* CLI - Clear Interrupt Disable */
            cpu_set_flag(cpu, FLAG_I, false);
            cpu->instruction_cycles = 2;
            break;
            
        case 0xC2:  /* REP - Reset Processor Status Bits */
            {
                u8 operand = memory_read(&g_memory, (cpu->pbr << 16) | cpu->pc);
                cpu->pc++;
                cpu->p &= ~operand;
                cpu->instruction_cycles = 3;
            }
            break;
            
        case 0xE2:  /* SEP - Set Processor Status Bits */
            {
                u8 operand = memory_read(&g_memory, (cpu->pbr << 16) | cpu->pc);
                cpu->pc++;
                cpu->p |= operand;
                cpu->instruction_cycles = 3;
            }
            break;
            
        default:
            /* Unimplemented opcode */
            printf("Unimplemented opcode: $%02X at $%02X:%04X\n", 
                   opcode, cpu->pbr, cpu->pc - 1);
            cpu->stopped = true;
            cpu->instruction_cycles = 2;
            break;
    }
    
    /* Update cycle counter */
    cpu->cycles += cpu->instruction_cycles;
    
    return cpu->instruction_cycles;
}

void cpu_run(CPU *cpu, u32 cycles) {
    u32 cycles_run = 0;
    
    while (cycles_run < cycles && !cpu->stopped) {
        cycles_run += cpu_step(cpu);
    }
}

void cpu_disassemble(const CPU *cpu, char *buffer, size_t size) {
    u8 opcode = memory_read(&g_memory, (cpu->pbr << 16) | cpu->pc);
    
    /* Simplified disassembly - only basic opcodes for now */
    switch (opcode) {
        case 0xEA:
            snprintf(buffer, size, "NOP");
            break;
        case 0x18:
            snprintf(buffer, size, "CLC");
            break;
        case 0x38:
            snprintf(buffer, size, "SEC");
            break;
        case 0x78:
            snprintf(buffer, size, "SEI");
            break;
        case 0x58:
            snprintf(buffer, size, "CLI");
            break;
        case 0xC2:
            snprintf(buffer, size, "REP #$%02X", 
                    memory_read(&g_memory, ((cpu->pbr << 16) | cpu->pc) + 1));
            break;
        case 0xE2:
            snprintf(buffer, size, "SEP #$%02X",
                    memory_read(&g_memory, ((cpu->pbr << 16) | cpu->pc) + 1));
            break;
        default:
            snprintf(buffer, size, "??? ($%02X)", opcode);
            break;
    }
}
