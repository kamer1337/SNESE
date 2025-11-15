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
    cpu->breakpoint_count = 0;
    cpu->breakpoint_hit = false;
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
    
    /* Check for breakpoint */
    if (cpu_check_breakpoint(cpu)) {
        cpu->breakpoint_hit = true;
        cpu->stopped = true;
        printf("Breakpoint hit at $%02X:%04X\n", cpu->pbr, cpu->pc);
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
            
        /* Flag instructions */
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
            
        case 0xD8:  /* CLD - Clear Decimal */
            cpu_set_flag(cpu, FLAG_D, false);
            cpu->instruction_cycles = 2;
            break;
            
        case 0xF8:  /* SED - Set Decimal */
            cpu_set_flag(cpu, FLAG_D, true);
            cpu->instruction_cycles = 2;
            break;
            
        case 0xB8:  /* CLV - Clear Overflow */
            cpu_set_flag(cpu, FLAG_V, false);
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
            
        /* Load/Store instructions - 8-bit immediate */
        case 0xA9:  /* LDA immediate */
            {
                if (cpu_get_flag(cpu, FLAG_M)) {
                    /* 8-bit mode */
                    cpu->a = (cpu->a & 0xFF00) | memory_read(&g_memory, (cpu->pbr << 16) | cpu->pc);
                    cpu->pc++;
                    cpu->instruction_cycles = 2;
                } else {
                    /* 16-bit mode */
                    u16 value = memory_read16(&g_memory, (cpu->pbr << 16) | cpu->pc);
                    cpu->a = value;
                    cpu->pc += 2;
                    cpu->instruction_cycles = 3;
                }
                cpu_set_flag(cpu, FLAG_Z, (cpu->a & (cpu_get_flag(cpu, FLAG_M) ? 0xFF : 0xFFFF)) == 0);
                cpu_set_flag(cpu, FLAG_N, (cpu->a & (cpu_get_flag(cpu, FLAG_M) ? 0x80 : 0x8000)) != 0);
            }
            break;
            
        case 0xA2:  /* LDX immediate */
            {
                if (cpu_get_flag(cpu, FLAG_X)) {
                    /* 8-bit mode */
                    cpu->x = memory_read(&g_memory, (cpu->pbr << 16) | cpu->pc);
                    cpu->pc++;
                    cpu->instruction_cycles = 2;
                } else {
                    /* 16-bit mode */
                    cpu->x = memory_read16(&g_memory, (cpu->pbr << 16) | cpu->pc);
                    cpu->pc += 2;
                    cpu->instruction_cycles = 3;
                }
                cpu_set_flag(cpu, FLAG_Z, cpu->x == 0);
                cpu_set_flag(cpu, FLAG_N, (cpu->x & (cpu_get_flag(cpu, FLAG_X) ? 0x80 : 0x8000)) != 0);
            }
            break;
            
        case 0xA0:  /* LDY immediate */
            {
                if (cpu_get_flag(cpu, FLAG_X)) {
                    /* 8-bit mode */
                    cpu->y = memory_read(&g_memory, (cpu->pbr << 16) | cpu->pc);
                    cpu->pc++;
                    cpu->instruction_cycles = 2;
                } else {
                    /* 16-bit mode */
                    cpu->y = memory_read16(&g_memory, (cpu->pbr << 16) | cpu->pc);
                    cpu->pc += 2;
                    cpu->instruction_cycles = 3;
                }
                cpu_set_flag(cpu, FLAG_Z, cpu->y == 0);
                cpu_set_flag(cpu, FLAG_N, (cpu->y & (cpu_get_flag(cpu, FLAG_X) ? 0x80 : 0x8000)) != 0);
            }
            break;
            
        /* Transfer instructions */
        case 0xAA:  /* TAX - Transfer A to X */
            if (cpu_get_flag(cpu, FLAG_X)) {
                cpu->x = cpu->a & 0xFF;
            } else {
                cpu->x = cpu->a;
            }
            cpu_set_flag(cpu, FLAG_Z, cpu->x == 0);
            cpu_set_flag(cpu, FLAG_N, (cpu->x & (cpu_get_flag(cpu, FLAG_X) ? 0x80 : 0x8000)) != 0);
            cpu->instruction_cycles = 2;
            break;
            
        case 0xA8:  /* TAY - Transfer A to Y */
            if (cpu_get_flag(cpu, FLAG_X)) {
                cpu->y = cpu->a & 0xFF;
            } else {
                cpu->y = cpu->a;
            }
            cpu_set_flag(cpu, FLAG_Z, cpu->y == 0);
            cpu_set_flag(cpu, FLAG_N, (cpu->y & (cpu_get_flag(cpu, FLAG_X) ? 0x80 : 0x8000)) != 0);
            cpu->instruction_cycles = 2;
            break;
            
        case 0x8A:  /* TXA - Transfer X to A */
            if (cpu_get_flag(cpu, FLAG_M)) {
                cpu->a = (cpu->a & 0xFF00) | (cpu->x & 0xFF);
            } else {
                cpu->a = cpu->x;
            }
            cpu_set_flag(cpu, FLAG_Z, (cpu->a & (cpu_get_flag(cpu, FLAG_M) ? 0xFF : 0xFFFF)) == 0);
            cpu_set_flag(cpu, FLAG_N, (cpu->a & (cpu_get_flag(cpu, FLAG_M) ? 0x80 : 0x8000)) != 0);
            cpu->instruction_cycles = 2;
            break;
            
        case 0x98:  /* TYA - Transfer Y to A */
            if (cpu_get_flag(cpu, FLAG_M)) {
                cpu->a = (cpu->a & 0xFF00) | (cpu->y & 0xFF);
            } else {
                cpu->a = cpu->y;
            }
            cpu_set_flag(cpu, FLAG_Z, (cpu->a & (cpu_get_flag(cpu, FLAG_M) ? 0xFF : 0xFFFF)) == 0);
            cpu_set_flag(cpu, FLAG_N, (cpu->a & (cpu_get_flag(cpu, FLAG_M) ? 0x80 : 0x8000)) != 0);
            cpu->instruction_cycles = 2;
            break;
            
        /* Stack instructions */
        case 0x48:  /* PHA - Push Accumulator */
            if (cpu_get_flag(cpu, FLAG_M)) {
                cpu_push8(cpu, cpu->a & 0xFF);
                cpu->instruction_cycles = 3;
            } else {
                cpu_push16(cpu, cpu->a);
                cpu->instruction_cycles = 4;
            }
            break;
            
        case 0x68:  /* PLA - Pull Accumulator */
            if (cpu_get_flag(cpu, FLAG_M)) {
                cpu->a = (cpu->a & 0xFF00) | cpu_pull8(cpu);
                cpu_set_flag(cpu, FLAG_Z, (cpu->a & 0xFF) == 0);
                cpu_set_flag(cpu, FLAG_N, (cpu->a & 0x80) != 0);
                cpu->instruction_cycles = 4;
            } else {
                cpu->a = cpu_pull16(cpu);
                cpu_set_flag(cpu, FLAG_Z, cpu->a == 0);
                cpu_set_flag(cpu, FLAG_N, (cpu->a & 0x8000) != 0);
                cpu->instruction_cycles = 5;
            }
            break;
            
        case 0x08:  /* PHP - Push Processor Status */
            cpu_push8(cpu, cpu->p);
            cpu->instruction_cycles = 3;
            break;
            
        case 0x28:  /* PLP - Pull Processor Status */
            cpu->p = cpu_pull8(cpu);
            cpu->instruction_cycles = 4;
            break;
            
        /* Increment/Decrement */
        case 0xE8:  /* INX - Increment X */
            if (cpu_get_flag(cpu, FLAG_X)) {
                cpu->x = (cpu->x + 1) & 0xFF;
                cpu_set_flag(cpu, FLAG_Z, (cpu->x & 0xFF) == 0);
                cpu_set_flag(cpu, FLAG_N, (cpu->x & 0x80) != 0);
            } else {
                cpu->x = (cpu->x + 1) & 0xFFFF;
                cpu_set_flag(cpu, FLAG_Z, cpu->x == 0);
                cpu_set_flag(cpu, FLAG_N, (cpu->x & 0x8000) != 0);
            }
            cpu->instruction_cycles = 2;
            break;
            
        case 0xC8:  /* INY - Increment Y */
            if (cpu_get_flag(cpu, FLAG_X)) {
                cpu->y = (cpu->y + 1) & 0xFF;
                cpu_set_flag(cpu, FLAG_Z, (cpu->y & 0xFF) == 0);
                cpu_set_flag(cpu, FLAG_N, (cpu->y & 0x80) != 0);
            } else {
                cpu->y = (cpu->y + 1) & 0xFFFF;
                cpu_set_flag(cpu, FLAG_Z, cpu->y == 0);
                cpu_set_flag(cpu, FLAG_N, (cpu->y & 0x8000) != 0);
            }
            cpu->instruction_cycles = 2;
            break;
            
        case 0xCA:  /* DEX - Decrement X */
            if (cpu_get_flag(cpu, FLAG_X)) {
                cpu->x = (cpu->x - 1) & 0xFF;
                cpu_set_flag(cpu, FLAG_Z, (cpu->x & 0xFF) == 0);
                cpu_set_flag(cpu, FLAG_N, (cpu->x & 0x80) != 0);
            } else {
                cpu->x = (cpu->x - 1) & 0xFFFF;
                cpu_set_flag(cpu, FLAG_Z, cpu->x == 0);
                cpu_set_flag(cpu, FLAG_N, (cpu->x & 0x8000) != 0);
            }
            cpu->instruction_cycles = 2;
            break;
            
        case 0x88:  /* DEY - Decrement Y */
            if (cpu_get_flag(cpu, FLAG_X)) {
                cpu->y = (cpu->y - 1) & 0xFF;
                cpu_set_flag(cpu, FLAG_Z, (cpu->y & 0xFF) == 0);
                cpu_set_flag(cpu, FLAG_N, (cpu->y & 0x80) != 0);
            } else {
                cpu->y = (cpu->y - 1) & 0xFFFF;
                cpu_set_flag(cpu, FLAG_Z, cpu->y == 0);
                cpu_set_flag(cpu, FLAG_N, (cpu->y & 0x8000) != 0);
            }
            cpu->instruction_cycles = 2;
            break;
            
        /* Branch instructions */
        case 0x90:  /* BCC - Branch if Carry Clear */
            {
                s8 offset = (s8)memory_read(&g_memory, (cpu->pbr << 16) | cpu->pc);
                cpu->pc++;
                if (!cpu_get_flag(cpu, FLAG_C)) {
                    cpu->pc += offset;
                    cpu->instruction_cycles = 3;
                } else {
                    cpu->instruction_cycles = 2;
                }
            }
            break;
            
        case 0xB0:  /* BCS - Branch if Carry Set */
            {
                s8 offset = (s8)memory_read(&g_memory, (cpu->pbr << 16) | cpu->pc);
                cpu->pc++;
                if (cpu_get_flag(cpu, FLAG_C)) {
                    cpu->pc += offset;
                    cpu->instruction_cycles = 3;
                } else {
                    cpu->instruction_cycles = 2;
                }
            }
            break;
            
        case 0xF0:  /* BEQ - Branch if Equal (Zero set) */
            {
                s8 offset = (s8)memory_read(&g_memory, (cpu->pbr << 16) | cpu->pc);
                cpu->pc++;
                if (cpu_get_flag(cpu, FLAG_Z)) {
                    cpu->pc += offset;
                    cpu->instruction_cycles = 3;
                } else {
                    cpu->instruction_cycles = 2;
                }
            }
            break;
            
        case 0xD0:  /* BNE - Branch if Not Equal (Zero clear) */
            {
                s8 offset = (s8)memory_read(&g_memory, (cpu->pbr << 16) | cpu->pc);
                cpu->pc++;
                if (!cpu_get_flag(cpu, FLAG_Z)) {
                    cpu->pc += offset;
                    cpu->instruction_cycles = 3;
                } else {
                    cpu->instruction_cycles = 2;
                }
            }
            break;
            
        case 0x30:  /* BMI - Branch if Minus (Negative set) */
            {
                s8 offset = (s8)memory_read(&g_memory, (cpu->pbr << 16) | cpu->pc);
                cpu->pc++;
                if (cpu_get_flag(cpu, FLAG_N)) {
                    cpu->pc += offset;
                    cpu->instruction_cycles = 3;
                } else {
                    cpu->instruction_cycles = 2;
                }
            }
            break;
            
        case 0x10:  /* BPL - Branch if Plus (Negative clear) */
            {
                s8 offset = (s8)memory_read(&g_memory, (cpu->pbr << 16) | cpu->pc);
                cpu->pc++;
                if (!cpu_get_flag(cpu, FLAG_N)) {
                    cpu->pc += offset;
                    cpu->instruction_cycles = 3;
                } else {
                    cpu->instruction_cycles = 2;
                }
            }
            break;
            
        case 0x80:  /* BRA - Branch Always */
            {
                s8 offset = (s8)memory_read(&g_memory, (cpu->pbr << 16) | cpu->pc);
                cpu->pc++;
                cpu->pc += offset;
                cpu->instruction_cycles = 3;
            }
            break;
            
        /* Jump instructions */
        case 0x4C:  /* JMP absolute */
            {
                u16 addr = memory_read16(&g_memory, (cpu->pbr << 16) | cpu->pc);
                cpu->pc = addr;
                cpu->instruction_cycles = 3;
            }
            break;
            
        case 0x5C:  /* JMP long */
            {
                u32 addr = memory_read24(&g_memory, (cpu->pbr << 16) | cpu->pc);
                cpu->pbr = (addr >> 16) & 0xFF;
                cpu->pc = addr & 0xFFFF;
                cpu->instruction_cycles = 4;
            }
            break;
            
        /* Subroutine instructions */
        case 0x20:  /* JSR - Jump to Subroutine */
            {
                u16 addr = memory_read16(&g_memory, (cpu->pbr << 16) | cpu->pc);
                cpu->pc += 2;
                cpu_push16(cpu, cpu->pc - 1);
                cpu->pc = addr;
                cpu->instruction_cycles = 6;
            }
            break;
            
        case 0x60:  /* RTS - Return from Subroutine */
            {
                u16 addr = cpu_pull16(cpu);
                cpu->pc = addr + 1;
                cpu->instruction_cycles = 6;
            }
            break;
            
        case 0x40:  /* RTI - Return from Interrupt */
            cpu->p = cpu_pull8(cpu);
            cpu->pc = cpu_pull16(cpu);
            cpu->pbr = cpu_pull8(cpu);
            cpu->instruction_cycles = 6;
            break;
            
        case 0x6B:  /* RTL - Return from Subroutine Long */
            {
                u16 addr = cpu_pull16(cpu);
                cpu->pbr = cpu_pull8(cpu);
                cpu->pc = addr + 1;
                cpu->instruction_cycles = 6;
            }
            break;
            
        /* Store instructions - absolute */
        case 0x8D:  /* STA absolute */
            {
                u16 addr = memory_read16(&g_memory, (cpu->pbr << 16) | cpu->pc);
                cpu->pc += 2;
                if (cpu_get_flag(cpu, FLAG_M)) {
                    memory_write(&g_memory, (cpu->dbr << 16) | addr, cpu->a & 0xFF);
                    cpu->instruction_cycles = 4;
                } else {
                    memory_write16(&g_memory, (cpu->dbr << 16) | addr, cpu->a);
                    cpu->instruction_cycles = 5;
                }
            }
            break;
            
        case 0x8E:  /* STX absolute */
            {
                u16 addr = memory_read16(&g_memory, (cpu->pbr << 16) | cpu->pc);
                cpu->pc += 2;
                if (cpu_get_flag(cpu, FLAG_X)) {
                    memory_write(&g_memory, (cpu->dbr << 16) | addr, cpu->x & 0xFF);
                    cpu->instruction_cycles = 4;
                } else {
                    memory_write16(&g_memory, (cpu->dbr << 16) | addr, cpu->x);
                    cpu->instruction_cycles = 5;
                }
            }
            break;
            
        case 0x8C:  /* STY absolute */
            {
                u16 addr = memory_read16(&g_memory, (cpu->pbr << 16) | cpu->pc);
                cpu->pc += 2;
                if (cpu_get_flag(cpu, FLAG_X)) {
                    memory_write(&g_memory, (cpu->dbr << 16) | addr, cpu->y & 0xFF);
                    cpu->instruction_cycles = 4;
                } else {
                    memory_write16(&g_memory, (cpu->dbr << 16) | addr, cpu->y);
                    cpu->instruction_cycles = 5;
                }
            }
            break;
            
        case 0x9C:  /* STZ absolute */
            {
                u16 addr = memory_read16(&g_memory, (cpu->pbr << 16) | cpu->pc);
                cpu->pc += 2;
                if (cpu_get_flag(cpu, FLAG_M)) {
                    memory_write(&g_memory, (cpu->dbr << 16) | addr, 0);
                    cpu->instruction_cycles = 4;
                } else {
                    memory_write16(&g_memory, (cpu->dbr << 16) | addr, 0);
                    cpu->instruction_cycles = 5;
                }
            }
            break;
            
        /* Load instructions - absolute */
        case 0xAD:  /* LDA absolute */
            {
                u16 addr = memory_read16(&g_memory, (cpu->pbr << 16) | cpu->pc);
                cpu->pc += 2;
                if (cpu_get_flag(cpu, FLAG_M)) {
                    cpu->a = (cpu->a & 0xFF00) | memory_read(&g_memory, (cpu->dbr << 16) | addr);
                    cpu_set_flag(cpu, FLAG_Z, (cpu->a & 0xFF) == 0);
                    cpu_set_flag(cpu, FLAG_N, (cpu->a & 0x80) != 0);
                    cpu->instruction_cycles = 4;
                } else {
                    cpu->a = memory_read16(&g_memory, (cpu->dbr << 16) | addr);
                    cpu_set_flag(cpu, FLAG_Z, cpu->a == 0);
                    cpu_set_flag(cpu, FLAG_N, (cpu->a & 0x8000) != 0);
                    cpu->instruction_cycles = 5;
                }
            }
            break;
            
        case 0xAE:  /* LDX absolute */
            {
                u16 addr = memory_read16(&g_memory, (cpu->pbr << 16) | cpu->pc);
                cpu->pc += 2;
                if (cpu_get_flag(cpu, FLAG_X)) {
                    cpu->x = memory_read(&g_memory, (cpu->dbr << 16) | addr);
                    cpu_set_flag(cpu, FLAG_Z, (cpu->x & 0xFF) == 0);
                    cpu_set_flag(cpu, FLAG_N, (cpu->x & 0x80) != 0);
                    cpu->instruction_cycles = 4;
                } else {
                    cpu->x = memory_read16(&g_memory, (cpu->dbr << 16) | addr);
                    cpu_set_flag(cpu, FLAG_Z, cpu->x == 0);
                    cpu_set_flag(cpu, FLAG_N, (cpu->x & 0x8000) != 0);
                    cpu->instruction_cycles = 5;
                }
            }
            break;
            
        case 0xAC:  /* LDY absolute */
            {
                u16 addr = memory_read16(&g_memory, (cpu->pbr << 16) | cpu->pc);
                cpu->pc += 2;
                if (cpu_get_flag(cpu, FLAG_X)) {
                    cpu->y = memory_read(&g_memory, (cpu->dbr << 16) | addr);
                    cpu_set_flag(cpu, FLAG_Z, (cpu->y & 0xFF) == 0);
                    cpu_set_flag(cpu, FLAG_N, (cpu->y & 0x80) != 0);
                    cpu->instruction_cycles = 4;
                } else {
                    cpu->y = memory_read16(&g_memory, (cpu->dbr << 16) | addr);
                    cpu_set_flag(cpu, FLAG_Z, cpu->y == 0);
                    cpu_set_flag(cpu, FLAG_N, (cpu->y & 0x8000) != 0);
                    cpu->instruction_cycles = 5;
                }
            }
            break;
            
        /* Arithmetic - ADC */
        case 0x69:  /* ADC immediate */
            {
                if (cpu_get_flag(cpu, FLAG_M)) {
                    u8 operand = memory_read(&g_memory, (cpu->pbr << 16) | cpu->pc);
                    cpu->pc++;
                    u8 a = cpu->a & 0xFF;
                    u16 result = a + operand + (cpu_get_flag(cpu, FLAG_C) ? 1 : 0);
                    cpu->a = (cpu->a & 0xFF00) | (result & 0xFF);
                    cpu_set_flag(cpu, FLAG_C, result > 0xFF);
                    cpu_set_flag(cpu, FLAG_Z, (result & 0xFF) == 0);
                    cpu_set_flag(cpu, FLAG_N, (result & 0x80) != 0);
                    cpu_set_flag(cpu, FLAG_V, ((a ^ result) & (operand ^ result) & 0x80) != 0);
                    cpu->instruction_cycles = 2;
                } else {
                    u16 operand = memory_read16(&g_memory, (cpu->pbr << 16) | cpu->pc);
                    cpu->pc += 2;
                    u32 result = cpu->a + operand + (cpu_get_flag(cpu, FLAG_C) ? 1 : 0);
                    cpu_set_flag(cpu, FLAG_C, result > 0xFFFF);
                    cpu->a = result & 0xFFFF;
                    cpu_set_flag(cpu, FLAG_Z, cpu->a == 0);
                    cpu_set_flag(cpu, FLAG_N, (cpu->a & 0x8000) != 0);
                    cpu_set_flag(cpu, FLAG_V, ((cpu->a ^ result) & (operand ^ result) & 0x8000) != 0);
                    cpu->instruction_cycles = 3;
                }
            }
            break;
            
        /* Arithmetic - SBC */
        case 0xE9:  /* SBC immediate */
            {
                if (cpu_get_flag(cpu, FLAG_M)) {
                    u8 operand = memory_read(&g_memory, (cpu->pbr << 16) | cpu->pc);
                    cpu->pc++;
                    u8 a = cpu->a & 0xFF;
                    u16 result = a - operand - (cpu_get_flag(cpu, FLAG_C) ? 0 : 1);
                    cpu->a = (cpu->a & 0xFF00) | (result & 0xFF);
                    cpu_set_flag(cpu, FLAG_C, result <= 0xFF);
                    cpu_set_flag(cpu, FLAG_Z, (result & 0xFF) == 0);
                    cpu_set_flag(cpu, FLAG_N, (result & 0x80) != 0);
                    cpu_set_flag(cpu, FLAG_V, ((a ^ operand) & (a ^ result) & 0x80) != 0);
                    cpu->instruction_cycles = 2;
                } else {
                    u16 operand = memory_read16(&g_memory, (cpu->pbr << 16) | cpu->pc);
                    cpu->pc += 2;
                    u32 result = cpu->a - operand - (cpu_get_flag(cpu, FLAG_C) ? 0 : 1);
                    cpu_set_flag(cpu, FLAG_C, result <= 0xFFFF);
                    cpu->a = result & 0xFFFF;
                    cpu_set_flag(cpu, FLAG_Z, cpu->a == 0);
                    cpu_set_flag(cpu, FLAG_N, (cpu->a & 0x8000) != 0);
                    cpu_set_flag(cpu, FLAG_V, ((cpu->a ^ operand) & (cpu->a ^ result) & 0x8000) != 0);
                    cpu->instruction_cycles = 3;
                }
            }
            break;
            
        /* Logic operations */
        case 0x29:  /* AND immediate */
            {
                if (cpu_get_flag(cpu, FLAG_M)) {
                    u8 operand = memory_read(&g_memory, (cpu->pbr << 16) | cpu->pc);
                    cpu->pc++;
                    cpu->a = (cpu->a & 0xFF00) | ((cpu->a & operand) & 0xFF);
                    cpu_set_flag(cpu, FLAG_Z, (cpu->a & 0xFF) == 0);
                    cpu_set_flag(cpu, FLAG_N, (cpu->a & 0x80) != 0);
                    cpu->instruction_cycles = 2;
                } else {
                    u16 operand = memory_read16(&g_memory, (cpu->pbr << 16) | cpu->pc);
                    cpu->pc += 2;
                    cpu->a &= operand;
                    cpu_set_flag(cpu, FLAG_Z, cpu->a == 0);
                    cpu_set_flag(cpu, FLAG_N, (cpu->a & 0x8000) != 0);
                    cpu->instruction_cycles = 3;
                }
            }
            break;
            
        case 0x09:  /* ORA immediate */
            {
                if (cpu_get_flag(cpu, FLAG_M)) {
                    u8 operand = memory_read(&g_memory, (cpu->pbr << 16) | cpu->pc);
                    cpu->pc++;
                    cpu->a = (cpu->a & 0xFF00) | ((cpu->a | operand) & 0xFF);
                    cpu_set_flag(cpu, FLAG_Z, (cpu->a & 0xFF) == 0);
                    cpu_set_flag(cpu, FLAG_N, (cpu->a & 0x80) != 0);
                    cpu->instruction_cycles = 2;
                } else {
                    u16 operand = memory_read16(&g_memory, (cpu->pbr << 16) | cpu->pc);
                    cpu->pc += 2;
                    cpu->a |= operand;
                    cpu_set_flag(cpu, FLAG_Z, cpu->a == 0);
                    cpu_set_flag(cpu, FLAG_N, (cpu->a & 0x8000) != 0);
                    cpu->instruction_cycles = 3;
                }
            }
            break;
            
        case 0x49:  /* EOR immediate */
            {
                if (cpu_get_flag(cpu, FLAG_M)) {
                    u8 operand = memory_read(&g_memory, (cpu->pbr << 16) | cpu->pc);
                    cpu->pc++;
                    cpu->a = (cpu->a & 0xFF00) | ((cpu->a ^ operand) & 0xFF);
                    cpu_set_flag(cpu, FLAG_Z, (cpu->a & 0xFF) == 0);
                    cpu_set_flag(cpu, FLAG_N, (cpu->a & 0x80) != 0);
                    cpu->instruction_cycles = 2;
                } else {
                    u16 operand = memory_read16(&g_memory, (cpu->pbr << 16) | cpu->pc);
                    cpu->pc += 2;
                    cpu->a ^= operand;
                    cpu_set_flag(cpu, FLAG_Z, cpu->a == 0);
                    cpu_set_flag(cpu, FLAG_N, (cpu->a & 0x8000) != 0);
                    cpu->instruction_cycles = 3;
                }
            }
            break;
            
        /* Comparison operations */
        case 0xC9:  /* CMP immediate */
            {
                if (cpu_get_flag(cpu, FLAG_M)) {
                    u8 operand = memory_read(&g_memory, (cpu->pbr << 16) | cpu->pc);
                    cpu->pc++;
                    u8 a = cpu->a & 0xFF;
                    u8 result = a - operand;
                    cpu_set_flag(cpu, FLAG_C, a >= operand);
                    cpu_set_flag(cpu, FLAG_Z, result == 0);
                    cpu_set_flag(cpu, FLAG_N, (result & 0x80) != 0);
                    cpu->instruction_cycles = 2;
                } else {
                    u16 operand = memory_read16(&g_memory, (cpu->pbr << 16) | cpu->pc);
                    cpu->pc += 2;
                    u16 result = cpu->a - operand;
                    cpu_set_flag(cpu, FLAG_C, cpu->a >= operand);
                    cpu_set_flag(cpu, FLAG_Z, result == 0);
                    cpu_set_flag(cpu, FLAG_N, (result & 0x8000) != 0);
                    cpu->instruction_cycles = 3;
                }
            }
            break;
            
        case 0xE0:  /* CPX immediate */
            {
                if (cpu_get_flag(cpu, FLAG_X)) {
                    u8 operand = memory_read(&g_memory, (cpu->pbr << 16) | cpu->pc);
                    cpu->pc++;
                    u8 x = cpu->x & 0xFF;
                    u8 result = x - operand;
                    cpu_set_flag(cpu, FLAG_C, x >= operand);
                    cpu_set_flag(cpu, FLAG_Z, result == 0);
                    cpu_set_flag(cpu, FLAG_N, (result & 0x80) != 0);
                    cpu->instruction_cycles = 2;
                } else {
                    u16 operand = memory_read16(&g_memory, (cpu->pbr << 16) | cpu->pc);
                    cpu->pc += 2;
                    u16 result = cpu->x - operand;
                    cpu_set_flag(cpu, FLAG_C, cpu->x >= operand);
                    cpu_set_flag(cpu, FLAG_Z, result == 0);
                    cpu_set_flag(cpu, FLAG_N, (result & 0x8000) != 0);
                    cpu->instruction_cycles = 3;
                }
            }
            break;
            
        case 0xC0:  /* CPY immediate */
            {
                if (cpu_get_flag(cpu, FLAG_X)) {
                    u8 operand = memory_read(&g_memory, (cpu->pbr << 16) | cpu->pc);
                    cpu->pc++;
                    u8 y = cpu->y & 0xFF;
                    u8 result = y - operand;
                    cpu_set_flag(cpu, FLAG_C, y >= operand);
                    cpu_set_flag(cpu, FLAG_Z, result == 0);
                    cpu_set_flag(cpu, FLAG_N, (result & 0x80) != 0);
                    cpu->instruction_cycles = 2;
                } else {
                    u16 operand = memory_read16(&g_memory, (cpu->pbr << 16) | cpu->pc);
                    cpu->pc += 2;
                    u16 result = cpu->y - operand;
                    cpu_set_flag(cpu, FLAG_C, cpu->y >= operand);
                    cpu_set_flag(cpu, FLAG_Z, result == 0);
                    cpu_set_flag(cpu, FLAG_N, (result & 0x8000) != 0);
                    cpu->instruction_cycles = 3;
                }
            }
            break;
            
        /* Additional branch instructions */
        case 0x50:  /* BVC - Branch if Overflow Clear */
            {
                s8 offset = (s8)memory_read(&g_memory, (cpu->pbr << 16) | cpu->pc);
                cpu->pc++;
                if (!cpu_get_flag(cpu, FLAG_V)) {
                    cpu->pc += offset;
                    cpu->instruction_cycles = 3;
                } else {
                    cpu->instruction_cycles = 2;
                }
            }
            break;
            
        case 0x70:  /* BVS - Branch if Overflow Set */
            {
                s8 offset = (s8)memory_read(&g_memory, (cpu->pbr << 16) | cpu->pc);
                cpu->pc++;
                if (cpu_get_flag(cpu, FLAG_V)) {
                    cpu->pc += offset;
                    cpu->instruction_cycles = 3;
                } else {
                    cpu->instruction_cycles = 2;
                }
            }
            break;
            
        /* Stack pointer operations */
        case 0xBA:  /* TSX - Transfer SP to X */
            if (cpu_get_flag(cpu, FLAG_X)) {
                cpu->x = cpu->sp & 0xFF;
                cpu_set_flag(cpu, FLAG_Z, (cpu->x & 0xFF) == 0);
                cpu_set_flag(cpu, FLAG_N, (cpu->x & 0x80) != 0);
            } else {
                cpu->x = cpu->sp;
                cpu_set_flag(cpu, FLAG_Z, cpu->x == 0);
                cpu_set_flag(cpu, FLAG_N, (cpu->x & 0x8000) != 0);
            }
            cpu->instruction_cycles = 2;
            break;
            
        case 0x9A:  /* TXS - Transfer X to SP */
            if (cpu->e) {
                cpu->sp = 0x0100 | (cpu->x & 0xFF);
            } else {
                cpu->sp = cpu->x;
            }
            cpu->instruction_cycles = 2;
            break;
            
        /* Additional transfer operations */
        case 0x5B:  /* TCD - Transfer C (16-bit A) to D */
            cpu->d = cpu->a;
            cpu_set_flag(cpu, FLAG_Z, cpu->d == 0);
            cpu_set_flag(cpu, FLAG_N, (cpu->d & 0x8000) != 0);
            cpu->instruction_cycles = 2;
            break;
            
        case 0x7B:  /* TDC - Transfer D to C (16-bit A) */
            cpu->a = cpu->d;
            cpu_set_flag(cpu, FLAG_Z, cpu->a == 0);
            cpu_set_flag(cpu, FLAG_N, (cpu->a & 0x8000) != 0);
            cpu->instruction_cycles = 2;
            break;
            
        case 0x1B:  /* TCS - Transfer C (16-bit A) to SP */
            cpu->sp = cpu->a;
            cpu->instruction_cycles = 2;
            break;
            
        case 0x3B:  /* TSC - Transfer SP to C (16-bit A) */
            cpu->a = cpu->sp;
            cpu_set_flag(cpu, FLAG_Z, cpu->a == 0);
            cpu_set_flag(cpu, FLAG_N, (cpu->a & 0x8000) != 0);
            cpu->instruction_cycles = 2;
            break;
            
        /* XCE - Exchange Carry and Emulation flags */
        case 0xFB:
            {
                u8 temp = cpu->e;
                cpu->e = cpu_get_flag(cpu, FLAG_C);
                cpu_set_flag(cpu, FLAG_C, temp);
                if (cpu->e) {
                    /* Switching to emulation mode */
                    cpu_set_flag(cpu, FLAG_M, true);
                    cpu_set_flag(cpu, FLAG_X, true);
                    cpu->sp = 0x0100 | (cpu->sp & 0xFF);
                }
                cpu->instruction_cycles = 2;
            }
            break;
            
        /* Block Move Operations */
        case 0x54:  /* MVN - Block Move Next (incrementing) */
            {
                u8 dest_bank = memory_read(&g_memory, (cpu->pbr << 16) | cpu->pc);
                u8 src_bank = memory_read(&g_memory, (cpu->pbr << 16) | (cpu->pc + 1));
                cpu->pc += 2;
                
                /* Move one byte from [src_bank:X] to [dest_bank:Y] */
                u32 src_addr = (src_bank << 16) | cpu->x;
                u32 dest_addr = (dest_bank << 16) | cpu->y;
                u8 value = memory_read(&g_memory, src_addr);
                memory_write(&g_memory, dest_addr, value);
                
                /* Increment source and destination */
                cpu->x++;
                cpu->y++;
                
                /* Decrement counter (accumulator) */
                cpu->a--;
                
                /* If A != 0xFFFF, repeat this instruction */
                if (cpu->a != 0xFFFF) {
                    cpu->pc -= 3;  /* Re-execute this instruction */
                }
                
                /* Set DBR to destination bank */
                cpu->dbr = dest_bank;
                
                /* 7 cycles per byte moved */
                cpu->instruction_cycles = 7;
            }
            break;
            
        case 0x44:  /* MVP - Block Move Previous (decrementing) */
            {
                u8 dest_bank = memory_read(&g_memory, (cpu->pbr << 16) | cpu->pc);
                u8 src_bank = memory_read(&g_memory, (cpu->pbr << 16) | (cpu->pc + 1));
                cpu->pc += 2;
                
                /* Move one byte from [src_bank:X] to [dest_bank:Y] */
                u32 src_addr = (src_bank << 16) | cpu->x;
                u32 dest_addr = (dest_bank << 16) | cpu->y;
                u8 value = memory_read(&g_memory, src_addr);
                memory_write(&g_memory, dest_addr, value);
                
                /* Decrement source and destination */
                cpu->x--;
                cpu->y--;
                
                /* Decrement counter (accumulator) */
                cpu->a--;
                
                /* If A != 0xFFFF, repeat this instruction */
                if (cpu->a != 0xFFFF) {
                    cpu->pc -= 3;  /* Re-execute this instruction */
                }
                
                /* Set DBR to destination bank */
                cpu->dbr = dest_bank;
                
                /* 7 cycles per byte moved */
                cpu->instruction_cycles = 7;
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
    
    /* Simplified disassembly */
    switch (opcode) {
        case 0xEA: snprintf(buffer, size, "NOP"); break;
        case 0x18: snprintf(buffer, size, "CLC"); break;
        case 0x38: snprintf(buffer, size, "SEC"); break;
        case 0x78: snprintf(buffer, size, "SEI"); break;
        case 0x58: snprintf(buffer, size, "CLI"); break;
        case 0xD8: snprintf(buffer, size, "CLD"); break;
        case 0xF8: snprintf(buffer, size, "SED"); break;
        case 0xB8: snprintf(buffer, size, "CLV"); break;
        
        case 0xC2:
            snprintf(buffer, size, "REP #$%02X", 
                    memory_read(&g_memory, ((cpu->pbr << 16) | cpu->pc) + 1));
            break;
        case 0xE2:
            snprintf(buffer, size, "SEP #$%02X",
                    memory_read(&g_memory, ((cpu->pbr << 16) | cpu->pc) + 1));
            break;
            
        /* Load instructions */
        case 0xA9:
            if (cpu_get_flag(cpu, FLAG_M)) {
                snprintf(buffer, size, "LDA #$%02X",
                        memory_read(&g_memory, ((cpu->pbr << 16) | cpu->pc) + 1));
            } else {
                snprintf(buffer, size, "LDA #$%04X",
                        memory_read16(&g_memory, ((cpu->pbr << 16) | cpu->pc) + 1));
            }
            break;
        case 0xA2:
            if (cpu_get_flag(cpu, FLAG_X)) {
                snprintf(buffer, size, "LDX #$%02X",
                        memory_read(&g_memory, ((cpu->pbr << 16) | cpu->pc) + 1));
            } else {
                snprintf(buffer, size, "LDX #$%04X",
                        memory_read16(&g_memory, ((cpu->pbr << 16) | cpu->pc) + 1));
            }
            break;
        case 0xA0:
            if (cpu_get_flag(cpu, FLAG_X)) {
                snprintf(buffer, size, "LDY #$%02X",
                        memory_read(&g_memory, ((cpu->pbr << 16) | cpu->pc) + 1));
            } else {
                snprintf(buffer, size, "LDY #$%04X",
                        memory_read16(&g_memory, ((cpu->pbr << 16) | cpu->pc) + 1));
            }
            break;
            
        /* Transfer instructions */
        case 0xAA: snprintf(buffer, size, "TAX"); break;
        case 0xA8: snprintf(buffer, size, "TAY"); break;
        case 0x8A: snprintf(buffer, size, "TXA"); break;
        case 0x98: snprintf(buffer, size, "TYA"); break;
        
        /* Stack instructions */
        case 0x48: snprintf(buffer, size, "PHA"); break;
        case 0x68: snprintf(buffer, size, "PLA"); break;
        case 0x08: snprintf(buffer, size, "PHP"); break;
        case 0x28: snprintf(buffer, size, "PLP"); break;
        
        /* Increment/Decrement */
        case 0xE8: snprintf(buffer, size, "INX"); break;
        case 0xC8: snprintf(buffer, size, "INY"); break;
        case 0xCA: snprintf(buffer, size, "DEX"); break;
        case 0x88: snprintf(buffer, size, "DEY"); break;
        
        /* Branch instructions */
        case 0x90:
            snprintf(buffer, size, "BCC $%04X",
                    cpu->pc + 2 + (s8)memory_read(&g_memory, ((cpu->pbr << 16) | cpu->pc) + 1));
            break;
        case 0xB0:
            snprintf(buffer, size, "BCS $%04X",
                    cpu->pc + 2 + (s8)memory_read(&g_memory, ((cpu->pbr << 16) | cpu->pc) + 1));
            break;
        case 0xF0:
            snprintf(buffer, size, "BEQ $%04X",
                    cpu->pc + 2 + (s8)memory_read(&g_memory, ((cpu->pbr << 16) | cpu->pc) + 1));
            break;
        case 0xD0:
            snprintf(buffer, size, "BNE $%04X",
                    cpu->pc + 2 + (s8)memory_read(&g_memory, ((cpu->pbr << 16) | cpu->pc) + 1));
            break;
        case 0x30:
            snprintf(buffer, size, "BMI $%04X",
                    cpu->pc + 2 + (s8)memory_read(&g_memory, ((cpu->pbr << 16) | cpu->pc) + 1));
            break;
        case 0x10:
            snprintf(buffer, size, "BPL $%04X",
                    cpu->pc + 2 + (s8)memory_read(&g_memory, ((cpu->pbr << 16) | cpu->pc) + 1));
            break;
        case 0x80:
            snprintf(buffer, size, "BRA $%04X",
                    cpu->pc + 2 + (s8)memory_read(&g_memory, ((cpu->pbr << 16) | cpu->pc) + 1));
            break;
            
        /* Jump instructions */
        case 0x4C:
            snprintf(buffer, size, "JMP $%04X",
                    memory_read16(&g_memory, ((cpu->pbr << 16) | cpu->pc) + 1));
            break;
        case 0x5C:
            snprintf(buffer, size, "JML $%06X",
                    memory_read24(&g_memory, ((cpu->pbr << 16) | cpu->pc) + 1));
            break;
            
        /* Subroutine instructions */
        case 0x20:
            snprintf(buffer, size, "JSR $%04X",
                    memory_read16(&g_memory, ((cpu->pbr << 16) | cpu->pc) + 1));
            break;
        case 0x60: snprintf(buffer, size, "RTS"); break;
        case 0x40: snprintf(buffer, size, "RTI"); break;
        case 0x6B: snprintf(buffer, size, "RTL"); break;
            
        /* Store instructions */
        case 0x8D:
            snprintf(buffer, size, "STA $%04X",
                    memory_read16(&g_memory, ((cpu->pbr << 16) | cpu->pc) + 1));
            break;
        case 0x8E:
            snprintf(buffer, size, "STX $%04X",
                    memory_read16(&g_memory, ((cpu->pbr << 16) | cpu->pc) + 1));
            break;
        case 0x8C:
            snprintf(buffer, size, "STY $%04X",
                    memory_read16(&g_memory, ((cpu->pbr << 16) | cpu->pc) + 1));
            break;
        case 0x9C:
            snprintf(buffer, size, "STZ $%04X",
                    memory_read16(&g_memory, ((cpu->pbr << 16) | cpu->pc) + 1));
            break;
            
        /* Load absolute */
        case 0xAD:
            snprintf(buffer, size, "LDA $%04X",
                    memory_read16(&g_memory, ((cpu->pbr << 16) | cpu->pc) + 1));
            break;
        case 0xAE:
            snprintf(buffer, size, "LDX $%04X",
                    memory_read16(&g_memory, ((cpu->pbr << 16) | cpu->pc) + 1));
            break;
        case 0xAC:
            snprintf(buffer, size, "LDY $%04X",
                    memory_read16(&g_memory, ((cpu->pbr << 16) | cpu->pc) + 1));
            break;
            
        /* Arithmetic */
        case 0x69:
            if (cpu_get_flag(cpu, FLAG_M)) {
                snprintf(buffer, size, "ADC #$%02X",
                        memory_read(&g_memory, ((cpu->pbr << 16) | cpu->pc) + 1));
            } else {
                snprintf(buffer, size, "ADC #$%04X",
                        memory_read16(&g_memory, ((cpu->pbr << 16) | cpu->pc) + 1));
            }
            break;
        case 0xE9:
            if (cpu_get_flag(cpu, FLAG_M)) {
                snprintf(buffer, size, "SBC #$%02X",
                        memory_read(&g_memory, ((cpu->pbr << 16) | cpu->pc) + 1));
            } else {
                snprintf(buffer, size, "SBC #$%04X",
                        memory_read16(&g_memory, ((cpu->pbr << 16) | cpu->pc) + 1));
            }
            break;
            
        /* Logic operations */
        case 0x29:
            if (cpu_get_flag(cpu, FLAG_M)) {
                snprintf(buffer, size, "AND #$%02X",
                        memory_read(&g_memory, ((cpu->pbr << 16) | cpu->pc) + 1));
            } else {
                snprintf(buffer, size, "AND #$%04X",
                        memory_read16(&g_memory, ((cpu->pbr << 16) | cpu->pc) + 1));
            }
            break;
        case 0x09:
            if (cpu_get_flag(cpu, FLAG_M)) {
                snprintf(buffer, size, "ORA #$%02X",
                        memory_read(&g_memory, ((cpu->pbr << 16) | cpu->pc) + 1));
            } else {
                snprintf(buffer, size, "ORA #$%04X",
                        memory_read16(&g_memory, ((cpu->pbr << 16) | cpu->pc) + 1));
            }
            break;
        case 0x49:
            if (cpu_get_flag(cpu, FLAG_M)) {
                snprintf(buffer, size, "EOR #$%02X",
                        memory_read(&g_memory, ((cpu->pbr << 16) | cpu->pc) + 1));
            } else {
                snprintf(buffer, size, "EOR #$%04X",
                        memory_read16(&g_memory, ((cpu->pbr << 16) | cpu->pc) + 1));
            }
            break;
            
        /* Comparison operations */
        case 0xC9:
            if (cpu_get_flag(cpu, FLAG_M)) {
                snprintf(buffer, size, "CMP #$%02X",
                        memory_read(&g_memory, ((cpu->pbr << 16) | cpu->pc) + 1));
            } else {
                snprintf(buffer, size, "CMP #$%04X",
                        memory_read16(&g_memory, ((cpu->pbr << 16) | cpu->pc) + 1));
            }
            break;
        case 0xE0:
            if (cpu_get_flag(cpu, FLAG_X)) {
                snprintf(buffer, size, "CPX #$%02X",
                        memory_read(&g_memory, ((cpu->pbr << 16) | cpu->pc) + 1));
            } else {
                snprintf(buffer, size, "CPX #$%04X",
                        memory_read16(&g_memory, ((cpu->pbr << 16) | cpu->pc) + 1));
            }
            break;
        case 0xC0:
            if (cpu_get_flag(cpu, FLAG_X)) {
                snprintf(buffer, size, "CPY #$%02X",
                        memory_read(&g_memory, ((cpu->pbr << 16) | cpu->pc) + 1));
            } else {
                snprintf(buffer, size, "CPY #$%04X",
                        memory_read16(&g_memory, ((cpu->pbr << 16) | cpu->pc) + 1));
            }
            break;
            
        /* Additional branches */
        case 0x50:
            snprintf(buffer, size, "BVC $%04X",
                    cpu->pc + 2 + (s8)memory_read(&g_memory, ((cpu->pbr << 16) | cpu->pc) + 1));
            break;
        case 0x70:
            snprintf(buffer, size, "BVS $%04X",
                    cpu->pc + 2 + (s8)memory_read(&g_memory, ((cpu->pbr << 16) | cpu->pc) + 1));
            break;
            
        /* Additional transfers */
        case 0xBA: snprintf(buffer, size, "TSX"); break;
        case 0x9A: snprintf(buffer, size, "TXS"); break;
        case 0x5B: snprintf(buffer, size, "TCD"); break;
        case 0x7B: snprintf(buffer, size, "TDC"); break;
        case 0x1B: snprintf(buffer, size, "TCS"); break;
        case 0x3B: snprintf(buffer, size, "TSC"); break;
        case 0xFB: snprintf(buffer, size, "XCE"); break;
        
        /* Block Move instructions */
        case 0x54:
            snprintf(buffer, size, "MVN $%02X,$%02X",
                    memory_read(&g_memory, ((cpu->pbr << 16) | cpu->pc) + 1),
                    memory_read(&g_memory, ((cpu->pbr << 16) | cpu->pc) + 2));
            break;
        case 0x44:
            snprintf(buffer, size, "MVP $%02X,$%02X",
                    memory_read(&g_memory, ((cpu->pbr << 16) | cpu->pc) + 1),
                    memory_read(&g_memory, ((cpu->pbr << 16) | cpu->pc) + 2));
            break;
            
        default:
            snprintf(buffer, size, "??? ($%02X)", opcode);
            break;
    }
}

/* Breakpoint management functions */

bool cpu_add_breakpoint(CPU *cpu, u32 address) {
    /* Check if breakpoint table is full */
    if (cpu->breakpoint_count >= 8) {
        return false;
    }
    
    /* Check if breakpoint already exists */
    for (u8 i = 0; i < cpu->breakpoint_count; i++) {
        if (cpu->breakpoints[i] == address) {
            return true;  /* Already exists */
        }
    }
    
    /* Add new breakpoint */
    cpu->breakpoints[cpu->breakpoint_count] = address;
    cpu->breakpoint_count++;
    return true;
}

bool cpu_remove_breakpoint(CPU *cpu, u32 address) {
    /* Find and remove the breakpoint */
    for (u8 i = 0; i < cpu->breakpoint_count; i++) {
        if (cpu->breakpoints[i] == address) {
            /* Shift remaining breakpoints down */
            for (u8 j = i; j < cpu->breakpoint_count - 1; j++) {
                cpu->breakpoints[j] = cpu->breakpoints[j + 1];
            }
            cpu->breakpoint_count--;
            return true;
        }
    }
    return false;
}

void cpu_clear_breakpoints(CPU *cpu) {
    cpu->breakpoint_count = 0;
}

bool cpu_check_breakpoint(const CPU *cpu) {
    u32 current_addr = (cpu->pbr << 16) | cpu->pc;
    
    for (u8 i = 0; i < cpu->breakpoint_count; i++) {
        if (cpu->breakpoints[i] == current_addr) {
            return true;
        }
    }
    return false;
}
