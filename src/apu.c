/*
 * apu.c - Audio Processing Unit implementation
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/apu.h"

void apu_init(APU *apu) {
    int i;
    
    memset(apu, 0, sizeof(APU));
    
    /* Allocate audio buffer (enough for 1 second at 32kHz stereo) */
    apu->buffer_size = 32000 * 2;  /* Stereo samples */
    apu->audio_buffer = (s16 *)calloc(apu->buffer_size, sizeof(s16));
    
    /* Allocate DSP sample buffer */
    apu->dsp.sample_buffer = (s16 *)calloc(32000, sizeof(s16));
    apu->dsp.sample_rate = 32000;
    
    /* Initialize voices */
    for (i = 0; i < DSP_NUM_VOICES; i++) {
        apu->dsp.voices[i].enabled = false;
        apu->dsp.voices[i].key_on = false;
        apu->dsp.voices[i].key_off = false;
    }
    
    apu_reset(apu);
}

void apu_reset(APU *apu) {
    int i;
    
    /* Reset SPC-700 CPU */
    apu->cpu.pc = 0xFFC0;  /* Boot ROM entry point */
    apu->cpu.a = 0;
    apu->cpu.x = 0;
    apu->cpu.y = 0;
    apu->cpu.sp = 0xFF;
    apu->cpu.psw = 0;
    apu->cpu.cycles = 0;
    apu->cpu.stopped = false;
    
    /* Clear RAM */
    memset(apu->ram, 0, SPC_RAM_SIZE);
    
    /* Reset communication ports */
    for (i = 0; i < 4; i++) {
        apu->port_in[i] = 0;
        apu->port_out[i] = 0;
    }
    
    /* Reset timers */
    for (i = 0; i < 3; i++) {
        apu->timer[i] = 0;
        apu->timer_target[i] = 0;
        apu->timer_enabled[i] = false;
    }
    
    /* Reset DSP */
    apu->dsp.main_volume_left = 127;
    apu->dsp.main_volume_right = 127;
    apu->dsp.key_on = 0;
    apu->dsp.key_off = 0;
    apu->dsp.sample_count = 0;
    
    /* Reset audio buffer */
    apu->buffer_pos = 0;
    
    apu->enabled = true;
}

void apu_run(APU *apu, u32 cycles) {
    u32 cycles_run = 0;
    
    if (!apu->enabled) {
        return;
    }
    
    /* Execute SPC-700 instructions until we've used up the cycle budget */
    while (cycles_run < cycles && !apu->cpu.stopped) {
        u32 instruction_cycles = spc700_execute_instruction(apu);
        cycles_run += instruction_cycles;
    }
    
    /* Generate audio samples proportional to cycles executed */
    /* Approximately 1 sample per 21 cycles at 1.024 MHz / 32kHz */
    u32 samples = cycles / 21;
    if (samples > 0) {
        apu_generate_samples(apu, samples);
    }
}

void apu_write_port(APU *apu, u8 port, u8 value) {
    if (port < 4) {
        apu->port_in[port] = value;
    }
}

u8 apu_read_port(APU *apu, u8 port) {
    if (port < 4) {
        return apu->port_out[port];
    }
    return 0;
}

u8 apu_read_ram(const APU *apu, u16 address) {
    return apu->ram[address];
}

void apu_write_ram(APU *apu, u16 address, u8 value) {
    apu->ram[address] = value;
}

u8 apu_read_dsp(const APU *apu, u8 address) {
    u8 voice;
    
    /* DSP registers are mapped to specific addresses */
    if (address <= 0x7F) {
        voice = (address >> 4) & 0x07;
        
        switch (address & 0x0F) {
            case 0x00: return apu->dsp.voices[voice].volume_left;
            case 0x01: return apu->dsp.voices[voice].volume_right;
            case 0x02: return apu->dsp.voices[voice].pitch & 0xFF;
            case 0x03: return (apu->dsp.voices[voice].pitch >> 8) & 0xFF;
            case 0x08: return apu->dsp.voices[voice].envx;
            case 0x09: return apu->dsp.voices[voice].outx;
            default: return 0;
        }
    }
    
    /* Global DSP registers */
    switch (address) {
        case 0x0C: return apu->dsp.main_volume_left;
        case 0x1C: return apu->dsp.main_volume_right;
        case 0x4C: return apu->dsp.key_on;
        case 0x5C: return apu->dsp.key_off;
        default: return 0;
    }
}

void apu_write_dsp(APU *apu, u8 address, u8 value) {
    u8 voice;
    
    /* DSP registers */
    if (address <= 0x7F) {
        voice = (address >> 4) & 0x07;
        
        switch (address & 0x0F) {
            case 0x00: apu->dsp.voices[voice].volume_left = value; break;
            case 0x01: apu->dsp.voices[voice].volume_right = value; break;
            case 0x02:
                apu->dsp.voices[voice].pitch = (apu->dsp.voices[voice].pitch & 0xFF00) | value;
                break;
            case 0x03:
                apu->dsp.voices[voice].pitch = (apu->dsp.voices[voice].pitch & 0x00FF) | (value << 8);
                break;
            case 0x04:
                apu->dsp.voices[voice].sample_address = (value << 8);
                break;
            case 0x05: apu->dsp.voices[voice].adsr1 = value; break;
            case 0x06: apu->dsp.voices[voice].adsr2 = value; break;
            case 0x07: apu->dsp.voices[voice].gain = value; break;
        }
    }
    
    /* Global DSP registers */
    switch (address) {
        case 0x0C: apu->dsp.main_volume_left = value; break;
        case 0x1C: apu->dsp.main_volume_right = value; break;
        case 0x4C:  /* Key On */
            apu->dsp.key_on = value;
            /* Enable voices */
            for (int i = 0; i < DSP_NUM_VOICES; i++) {
                if (value & (1 << i)) {
                    apu->dsp.voices[i].key_on = true;
                    apu->dsp.voices[i].enabled = true;
                }
            }
            break;
        case 0x5C:  /* Key Off */
            apu->dsp.key_off = value;
            /* Disable voices */
            for (int i = 0; i < DSP_NUM_VOICES; i++) {
                if (value & (1 << i)) {
                    apu->dsp.voices[i].key_off = true;
                    apu->dsp.voices[i].enabled = false;
                }
            }
            break;
    }
}

void apu_generate_samples(APU *apu, u32 num_samples) {
    u32 i, v;
    s32 sample_left, sample_right;
    
    for (i = 0; i < num_samples && apu->buffer_pos < apu->buffer_size; i++) {
        sample_left = 0;
        sample_right = 0;
        
        /* Mix all active voices */
        for (v = 0; v < DSP_NUM_VOICES; v++) {
            if (apu->dsp.voices[v].enabled) {
                /* Generate simple waveform (placeholder) */
                /* Real implementation would decode BRR samples */
                s16 sample = (s16)((v * 1000) % 4000 - 2000);  /* Placeholder */
                
                sample_left += (sample * apu->dsp.voices[v].volume_left) >> 7;
                sample_right += (sample * apu->dsp.voices[v].volume_right) >> 7;
            }
        }
        
        /* Apply main volume and clamp */
        sample_left = (sample_left * apu->dsp.main_volume_left) >> 7;
        sample_right = (sample_right * apu->dsp.main_volume_right) >> 7;
        
        if (sample_left > 32767) sample_left = 32767;
        if (sample_left < -32768) sample_left = -32768;
        if (sample_right > 32767) sample_right = 32767;
        if (sample_right < -32768) sample_right = -32768;
        
        /* Store stereo sample */
        apu->audio_buffer[apu->buffer_pos++] = (s16)sample_left;
        apu->audio_buffer[apu->buffer_pos++] = (s16)sample_right;
    }
    
    apu->dsp.sample_count += num_samples;
}

void apu_output_wav(const APU *apu, const char *filename) {
    FILE *f;
    u32 data_size, file_size;
    u16 num_channels = 2;  /* Stereo */
    u32 sample_rate = 32000;
    u16 bits_per_sample = 16;
    u32 byte_rate;
    u16 block_align;
    u32 i;
    
    if (!apu->audio_buffer || apu->buffer_pos == 0) {
        fprintf(stderr, "Error: No audio data to output\n");
        return;
    }
    
    f = fopen(filename, "wb");
    if (!f) {
        fprintf(stderr, "Error: Cannot create WAV file '%s'\n", filename);
        return;
    }
    
    data_size = apu->buffer_pos * sizeof(s16);
    file_size = 36 + data_size;
    byte_rate = sample_rate * num_channels * bits_per_sample / 8;
    block_align = num_channels * bits_per_sample / 8;
    
    /* Write RIFF header */
    fwrite("RIFF", 1, 4, f);
    fwrite(&file_size, 4, 1, f);
    fwrite("WAVE", 1, 4, f);
    
    /* Write fmt chunk */
    fwrite("fmt ", 1, 4, f);
    i = 16;  /* Chunk size */
    fwrite(&i, 4, 1, f);
    i = 1;   /* PCM format */
    fwrite(&i, 2, 1, f);
    fwrite(&num_channels, 2, 1, f);
    fwrite(&sample_rate, 4, 1, f);
    fwrite(&byte_rate, 4, 1, f);
    fwrite(&block_align, 2, 1, f);
    fwrite(&bits_per_sample, 2, 1, f);
    
    /* Write data chunk */
    fwrite("data", 1, 4, f);
    fwrite(&data_size, 4, 1, f);
    fwrite(apu->audio_buffer, 1, data_size, f);
    
    fclose(f);
    printf("Audio output to %s (%u samples)\n", filename, apu->buffer_pos / 2);
}

const s16 *apu_get_audio_buffer(const APU *apu, u32 *size) {
    *size = apu->buffer_pos;
    return apu->audio_buffer;
}

/* BRR (Bit Rate Reduction) audio decoding */
void brr_decode_block(const u8 *brr_block, s16 *samples, s16 *old, s16 *older) {
    u8 header;
    u8 filter, shift;
    int i;
    s16 s1, s2;
    
    /* BRR block format:
     * Byte 0: Header (EESSFFFF)
     *   E = End flag
     *   S = Shift amount (0-12)
     *   F = Filter type (0-3)
     * Bytes 1-8: Sample data (2 4-bit samples per byte, 16 samples total)
     */
    
    header = brr_block[0];
    shift = (header >> 4) & 0x0F;
    filter = (header >> 2) & 0x03;
    
    /* Previous samples for filtering */
    s1 = *old;
    s2 = *older;
    
    /* Clamp shift to prevent overflow */
    if (shift > 12) {
        shift = 12;
    }
    
    /* Decode 16 samples (8 bytes, 2 samples per byte) */
    for (i = 0; i < 8; i++) {
        u8 byte = brr_block[1 + i];
        s16 sample1, sample2;
        s32 s;
        
        /* First sample (high nibble) */
        sample1 = (s8)(byte & 0xF0) >> 4;  /* Sign-extend 4-bit to 8-bit */
        if (sample1 > 7) sample1 -= 16;     /* Convert to signed */
        
        /* Apply shift */
        s = (s32)(sample1 << shift) >> 1;
        
        /* Apply filter */
        switch (filter) {
            case 0:  /* No filter */
                break;
            case 1:  /* Filter: out = in + old * 15/16 */
                s += s1 + ((-s1) >> 4);
                break;
            case 2:  /* Filter: out = in + old * 61/32 - older * 15/16 */
                s += (s1 * 2) + ((-s1 * 3) >> 5) - s2 + ((s2) >> 4);
                break;
            case 3:  /* Filter: out = in + old * 115/64 - older * 13/16 */
                s += (s1 * 2) + ((-s1 * 13) >> 6) - s2 + ((s2 * 3) >> 4);
                break;
        }
        
        /* Clamp to 16-bit range */
        if (s > 32767) s = 32767;
        if (s < -32768) s = -32768;
        
        samples[i * 2] = (s16)s;
        s2 = s1;
        s1 = (s16)s;
        
        /* Second sample (low nibble) */
        sample2 = (s8)((byte & 0x0F) << 4) >> 4;  /* Sign-extend 4-bit to 8-bit */
        if (sample2 > 7) sample2 -= 16;
        
        /* Apply shift */
        s = (s32)(sample2 << shift) >> 1;
        
        /* Apply filter */
        switch (filter) {
            case 0:
                break;
            case 1:
                s += s1 + ((-s1) >> 4);
                break;
            case 2:
                s += (s1 * 2) + ((-s1 * 3) >> 5) - s2 + ((s2) >> 4);
                break;
            case 3:
                s += (s1 * 2) + ((-s1 * 13) >> 6) - s2 + ((s2 * 3) >> 4);
                break;
        }
        
        /* Clamp to 16-bit range */
        if (s > 32767) s = 32767;
        if (s < -32768) s = -32768;
        
        samples[i * 2 + 1] = (s16)s;
        s2 = s1;
        s1 = (s16)s;
    }
    
    /* Save state for next block */
    *old = s1;
    *older = s2;
}

/* SPC-700 helper functions */
static u8 spc700_read_byte(APU *apu, u16 addr) {
    return apu->ram[addr];
}

static void spc700_write_byte(APU *apu, u16 addr, u8 value) {
    apu->ram[addr] = value;
}

static void spc700_set_flag(APU *apu, u8 flag, bool value) {
    if (value) {
        apu->cpu.psw |= flag;
    } else {
        apu->cpu.psw &= ~flag;
    }
}

static bool spc700_get_flag(const APU *apu, u8 flag) {
    return (apu->cpu.psw & flag) != 0;
}

static void spc700_set_nz(APU *apu, u8 value) {
    spc700_set_flag(apu, SPC_FLAG_N, (value & 0x80) != 0);
    spc700_set_flag(apu, SPC_FLAG_Z, value == 0);
}

static void spc700_push(APU *apu, u8 value) {
    apu->ram[0x0100 + apu->cpu.sp] = value;
    apu->cpu.sp--;
}

static u8 spc700_pull(APU *apu) {
    apu->cpu.sp++;
    return apu->ram[0x0100 + apu->cpu.sp];
}

/* Execute a single SPC-700 instruction */
u32 spc700_execute_instruction(APU *apu) {
    u8 opcode;
    u8 operand1, operand2;
    u16 addr;
    u32 cycles = 2;  /* Most instructions take 2 cycles minimum */
    
    /* Fetch opcode */
    opcode = spc700_read_byte(apu, apu->cpu.pc++);
    
    /* Decode and execute */
    switch (opcode) {
        /* NOP - No Operation */
        case 0x00:
            cycles = 2;
            break;
            
        /* MOV A, #imm */
        case 0xE8:
            apu->cpu.a = spc700_read_byte(apu, apu->cpu.pc++);
            spc700_set_nz(apu, apu->cpu.a);
            cycles = 2;
            break;
            
        /* MOV X, #imm */
        case 0xCD:
            apu->cpu.x = spc700_read_byte(apu, apu->cpu.pc++);
            spc700_set_nz(apu, apu->cpu.x);
            cycles = 2;
            break;
            
        /* MOV Y, #imm */
        case 0x8D:
            apu->cpu.y = spc700_read_byte(apu, apu->cpu.pc++);
            spc700_set_nz(apu, apu->cpu.y);
            cycles = 2;
            break;
            
        /* MOV A, X */
        case 0x7D:
            apu->cpu.a = apu->cpu.x;
            spc700_set_nz(apu, apu->cpu.a);
            cycles = 2;
            break;
            
        /* MOV A, Y */
        case 0xDD:
            apu->cpu.a = apu->cpu.y;
            spc700_set_nz(apu, apu->cpu.a);
            cycles = 2;
            break;
            
        /* MOV X, A */
        case 0x5D:
            apu->cpu.x = apu->cpu.a;
            spc700_set_nz(apu, apu->cpu.x);
            cycles = 2;
            break;
            
        /* MOV Y, A */
        case 0xFD:
            apu->cpu.y = apu->cpu.a;
            spc700_set_nz(apu, apu->cpu.y);
            cycles = 2;
            break;
            
        /* MOV A, (dp) */
        case 0xE4:
            operand1 = spc700_read_byte(apu, apu->cpu.pc++);
            apu->cpu.a = spc700_read_byte(apu, operand1);
            spc700_set_nz(apu, apu->cpu.a);
            cycles = 3;
            break;
            
        /* MOV (dp), A */
        case 0xC4:
            operand1 = spc700_read_byte(apu, apu->cpu.pc++);
            spc700_write_byte(apu, operand1, apu->cpu.a);
            cycles = 4;
            break;
            
        /* MOV A, (X) */
        case 0xE6:
            apu->cpu.a = spc700_read_byte(apu, apu->cpu.x);
            spc700_set_nz(apu, apu->cpu.a);
            cycles = 3;
            break;
            
        /* MOV (X), A */
        case 0xC6:
            spc700_write_byte(apu, apu->cpu.x, apu->cpu.a);
            cycles = 4;
            break;
            
        /* MOV A, addr */
        case 0xE5:
            operand1 = spc700_read_byte(apu, apu->cpu.pc++);
            operand2 = spc700_read_byte(apu, apu->cpu.pc++);
            addr = operand1 | (operand2 << 8);
            apu->cpu.a = spc700_read_byte(apu, addr);
            spc700_set_nz(apu, apu->cpu.a);
            cycles = 4;
            break;
            
        /* MOV addr, A */
        case 0xC5:
            operand1 = spc700_read_byte(apu, apu->cpu.pc++);
            operand2 = spc700_read_byte(apu, apu->cpu.pc++);
            addr = operand1 | (operand2 << 8);
            spc700_write_byte(apu, addr, apu->cpu.a);
            cycles = 5;
            break;
            
        /* INC A */
        case 0xBC:
            apu->cpu.a++;
            spc700_set_nz(apu, apu->cpu.a);
            cycles = 2;
            break;
            
        /* INC X */
        case 0x3D:
            apu->cpu.x++;
            spc700_set_nz(apu, apu->cpu.x);
            cycles = 2;
            break;
            
        /* INC Y */
        case 0xFC:
            apu->cpu.y++;
            spc700_set_nz(apu, apu->cpu.y);
            cycles = 2;
            break;
            
        /* DEC A */
        case 0x9C:
            apu->cpu.a--;
            spc700_set_nz(apu, apu->cpu.a);
            cycles = 2;
            break;
            
        /* DEC X */
        case 0x1D:
            apu->cpu.x--;
            spc700_set_nz(apu, apu->cpu.x);
            cycles = 2;
            break;
            
        /* DEC Y */
        case 0xDC:
            apu->cpu.y--;
            spc700_set_nz(apu, apu->cpu.y);
            cycles = 2;
            break;
            
        /* ADC A, #imm */
        case 0x88: {
            u8 imm = spc700_read_byte(apu, apu->cpu.pc++);
            u16 result = apu->cpu.a + imm + (spc700_get_flag(apu, SPC_FLAG_C) ? 1 : 0);
            spc700_set_flag(apu, SPC_FLAG_C, result > 0xFF);
            spc700_set_flag(apu, SPC_FLAG_V, ((apu->cpu.a ^ result) & (imm ^ result) & 0x80) != 0);
            spc700_set_flag(apu, SPC_FLAG_H, ((apu->cpu.a & 0x0F) + (imm & 0x0F) + 
                (spc700_get_flag(apu, SPC_FLAG_C) ? 1 : 0)) > 0x0F);
            apu->cpu.a = (u8)result;
            spc700_set_nz(apu, apu->cpu.a);
            cycles = 2;
            break;
        }
            
        /* SBC A, #imm */
        case 0xA8: {
            u8 imm = spc700_read_byte(apu, apu->cpu.pc++);
            u16 result = apu->cpu.a - imm - (spc700_get_flag(apu, SPC_FLAG_C) ? 0 : 1);
            spc700_set_flag(apu, SPC_FLAG_C, result <= 0xFF);
            spc700_set_flag(apu, SPC_FLAG_V, ((apu->cpu.a ^ imm) & (apu->cpu.a ^ result) & 0x80) != 0);
            apu->cpu.a = (u8)result;
            spc700_set_nz(apu, apu->cpu.a);
            cycles = 2;
            break;
        }
            
        /* CMP A, #imm */
        case 0x68: {
            u8 imm = spc700_read_byte(apu, apu->cpu.pc++);
            u16 result = apu->cpu.a - imm;
            spc700_set_flag(apu, SPC_FLAG_C, result <= 0xFF);
            spc700_set_nz(apu, (u8)result);
            cycles = 2;
            break;
        }
            
        /* CMP X, #imm */
        case 0xC8: {
            u8 imm = spc700_read_byte(apu, apu->cpu.pc++);
            u16 result = apu->cpu.x - imm;
            spc700_set_flag(apu, SPC_FLAG_C, result <= 0xFF);
            spc700_set_nz(apu, (u8)result);
            cycles = 2;
            break;
        }
            
        /* CMP Y, #imm */
        case 0xAD: {
            u8 imm = spc700_read_byte(apu, apu->cpu.pc++);
            u16 result = apu->cpu.y - imm;
            spc700_set_flag(apu, SPC_FLAG_C, result <= 0xFF);
            spc700_set_nz(apu, (u8)result);
            cycles = 2;
            break;
        }
            
        /* AND A, #imm */
        case 0x28:
            apu->cpu.a &= spc700_read_byte(apu, apu->cpu.pc++);
            spc700_set_nz(apu, apu->cpu.a);
            cycles = 2;
            break;
            
        /* OR A, #imm */
        case 0x08:
            apu->cpu.a |= spc700_read_byte(apu, apu->cpu.pc++);
            spc700_set_nz(apu, apu->cpu.a);
            cycles = 2;
            break;
            
        /* EOR A, #imm */
        case 0x48:
            apu->cpu.a ^= spc700_read_byte(apu, apu->cpu.pc++);
            spc700_set_nz(apu, apu->cpu.a);
            cycles = 2;
            break;
            
        /* BRA rel */
        case 0x2F:
            operand1 = spc700_read_byte(apu, apu->cpu.pc++);
            apu->cpu.pc += (s8)operand1;
            cycles = 4;
            break;
            
        /* BEQ rel */
        case 0xF0:
            operand1 = spc700_read_byte(apu, apu->cpu.pc++);
            if (spc700_get_flag(apu, SPC_FLAG_Z)) {
                apu->cpu.pc += (s8)operand1;
                cycles = 4;
            } else {
                cycles = 2;
            }
            break;
            
        /* BNE rel */
        case 0xD0:
            operand1 = spc700_read_byte(apu, apu->cpu.pc++);
            if (!spc700_get_flag(apu, SPC_FLAG_Z)) {
                apu->cpu.pc += (s8)operand1;
                cycles = 4;
            } else {
                cycles = 2;
            }
            break;
            
        /* BCS rel */
        case 0xB0:
            operand1 = spc700_read_byte(apu, apu->cpu.pc++);
            if (spc700_get_flag(apu, SPC_FLAG_C)) {
                apu->cpu.pc += (s8)operand1;
                cycles = 4;
            } else {
                cycles = 2;
            }
            break;
            
        /* BCC rel */
        case 0x90:
            operand1 = spc700_read_byte(apu, apu->cpu.pc++);
            if (!spc700_get_flag(apu, SPC_FLAG_C)) {
                apu->cpu.pc += (s8)operand1;
                cycles = 4;
            } else {
                cycles = 2;
            }
            break;
            
        /* PUSH A */
        case 0x2D:
            spc700_push(apu, apu->cpu.a);
            cycles = 4;
            break;
            
        /* PUSH X */
        case 0x4D:
            spc700_push(apu, apu->cpu.x);
            cycles = 4;
            break;
            
        /* PUSH Y */
        case 0x6D:
            spc700_push(apu, apu->cpu.y);
            cycles = 4;
            break;
            
        /* PUSH PSW */
        case 0x0D:
            spc700_push(apu, apu->cpu.psw);
            cycles = 4;
            break;
            
        /* POP A */
        case 0xAE:
            apu->cpu.a = spc700_pull(apu);
            cycles = 4;
            break;
            
        /* POP X */
        case 0xCE:
            apu->cpu.x = spc700_pull(apu);
            cycles = 4;
            break;
            
        /* POP Y */
        case 0xEE:
            apu->cpu.y = spc700_pull(apu);
            cycles = 4;
            break;
            
        /* POP PSW */
        case 0x8E:
            apu->cpu.psw = spc700_pull(apu);
            cycles = 4;
            break;
            
        /* RET */
        case 0x6F:
            operand1 = spc700_pull(apu);
            operand2 = spc700_pull(apu);
            apu->cpu.pc = operand1 | (operand2 << 8);
            cycles = 5;
            break;
            
        /* SLEEP - Stop CPU */
        case 0xEF:
            apu->cpu.stopped = true;
            cycles = 3;
            break;
            
        /* STOP - Stop CPU and oscillator */
        case 0xFF:
            apu->cpu.stopped = true;
            cycles = 3;
            break;
            
        default:
            /* Unknown opcode - treat as NOP */
            cycles = 2;
            break;
    }
    
    apu->cpu.cycles += cycles;
    return cycles;
}
