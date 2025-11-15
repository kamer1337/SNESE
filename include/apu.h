/*
 * apu.h - Audio Processing Unit (SPC-700 + DSP) emulation
 * 
 * Handles audio synthesis, SPC-700 processor, and DSP.
 */

#ifndef APU_H
#define APU_H

#include "types.h"

/* SPC-700 RAM size */
#define SPC_RAM_SIZE 0x10000  /* 64KB */

/* DSP Registers */
#define DSP_NUM_VOICES 8

/* Voice structure for DSP */
typedef struct {
    s16 sample_buffer[16];  /* BRR sample buffer */
    u16 pitch;              /* Pitch (frequency) */
    u8 volume_left;         /* Left volume */
    u8 volume_right;        /* Right volume */
    u8 adsr1;               /* ADSR envelope 1 */
    u8 adsr2;               /* ADSR envelope 2 */
    u8 gain;                /* Gain */
    u8 envx;                /* Current envelope value */
    u8 outx;                /* Current sample output */
    
    u16 sample_address;     /* Current sample address */
    u16 loop_address;       /* Loop point address */
    u8 sample_offset;       /* Offset within current BRR block */
    
    bool key_on;            /* Key on flag */
    bool key_off;           /* Key off flag */
    bool enabled;           /* Voice enabled */
} DSPVoice;

/* DSP structure */
typedef struct {
    DSPVoice voices[DSP_NUM_VOICES];  /* 8 audio voices */
    
    /* Global DSP registers */
    u8 main_volume_left;    /* Main volume left */
    u8 main_volume_right;   /* Main volume right */
    u8 echo_volume_left;    /* Echo volume left */
    u8 echo_volume_right;   /* Echo volume right */
    u8 key_on;              /* Key on flags */
    u8 key_off;             /* Key off flags */
    u8 flags;               /* Control flags */
    u8 noise_clock;         /* Noise frequency */
    u8 echo_feedback;       /* Echo feedback */
    u16 echo_buffer_addr;   /* Echo buffer address */
    u16 echo_delay;         /* Echo delay */
    
    s16 *sample_buffer;     /* Output sample buffer */
    u32 sample_count;       /* Number of samples generated */
    u32 sample_rate;        /* Sample rate (32000 Hz typical) */
} DSP;

/* SPC-700 CPU registers */
typedef struct {
    u16 pc;                 /* Program counter */
    u8 a;                   /* Accumulator */
    u8 x;                   /* X register */
    u8 y;                   /* Y register */
    u8 sp;                  /* Stack pointer */
    u8 psw;                 /* Program status word */
    
    u64 cycles;             /* Cycle counter */
    bool stopped;           /* CPU stopped */
} SPC700;

/* APU structure */
typedef struct {
    SPC700 cpu;             /* SPC-700 CPU */
    DSP dsp;                /* DSP for audio synthesis */
    
    u8 ram[SPC_RAM_SIZE];   /* 64KB audio RAM */
    
    /* Communication ports with main CPU */
    u8 port_in[4];          /* Input ports from main CPU */
    u8 port_out[4];         /* Output ports to main CPU */
    
    /* Timers */
    u8 timer[3];            /* 3 timers */
    u8 timer_target[3];     /* Timer target values */
    bool timer_enabled[3];  /* Timer enable flags */
    
    /* Audio output */
    s16 *audio_buffer;      /* Audio output buffer */
    u32 buffer_size;        /* Buffer size in samples */
    u32 buffer_pos;         /* Current buffer position */
    
    bool enabled;           /* APU enabled */
} APU;

/* Function declarations */

/*
 * Initialize APU
 */
void apu_init(APU *apu);

/*
 * Reset APU to power-on state
 */
void apu_reset(APU *apu);

/*
 * Execute APU for specified number of cycles
 */
void apu_run(APU *apu, u32 cycles);

/*
 * Write to APU communication port (from main CPU)
 */
void apu_write_port(APU *apu, u8 port, u8 value);

/*
 * Read from APU communication port (from main CPU)
 */
u8 apu_read_port(APU *apu, u8 port);

/*
 * Read from SPC RAM
 */
u8 apu_read_ram(const APU *apu, u16 address);

/*
 * Write to SPC RAM
 */
void apu_write_ram(APU *apu, u16 address, u8 value);

/*
 * Read from DSP register
 */
u8 apu_read_dsp(const APU *apu, u8 address);

/*
 * Write to DSP register
 */
void apu_write_dsp(APU *apu, u8 address, u8 value);

/*
 * Generate audio samples
 */
void apu_generate_samples(APU *apu, u32 num_samples);

/*
 * Output audio to WAV file
 */
void apu_output_wav(const APU *apu, const char *filename);

/*
 * Get audio buffer for playback
 */
const s16 *apu_get_audio_buffer(const APU *apu, u32 *size);

/*
 * Execute a single SPC-700 instruction
 */
u32 spc700_execute_instruction(APU *apu);

/*
 * Decode BRR (Bit Rate Reduction) audio sample block
 */
void brr_decode_block(const u8 *brr_block, s16 *samples, s16 *old, s16 *older);

/*
 * SPC-700 PSW flag accessors
 */
#define SPC_FLAG_C 0x01  /* Carry */
#define SPC_FLAG_Z 0x02  /* Zero */
#define SPC_FLAG_I 0x04  /* Interrupt enable */
#define SPC_FLAG_H 0x08  /* Half carry */
#define SPC_FLAG_B 0x10  /* Break */
#define SPC_FLAG_P 0x20  /* Direct page */
#define SPC_FLAG_V 0x40  /* Overflow */
#define SPC_FLAG_N 0x80  /* Negative */

#endif /* APU_H */
