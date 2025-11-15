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
    
    /* Simple cycle execution - actual SPC-700 emulation would go here */
    while (cycles_run < cycles && !apu->cpu.stopped) {
        /* Placeholder for SPC-700 instruction execution */
        /* This would fetch, decode, and execute SPC-700 opcodes */
        
        cycles_run++;
        apu->cpu.cycles++;
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
