/*
 * input.h - Input/Controller emulation
 * 
 * Handles joypad input emulation for SNES controllers.
 */

#ifndef INPUT_H
#define INPUT_H

#include "types.h"

/* Controller button masks */
#define BUTTON_B      0x8000
#define BUTTON_Y      0x4000
#define BUTTON_SELECT 0x2000
#define BUTTON_START  0x1000
#define BUTTON_UP     0x0800
#define BUTTON_DOWN   0x0400
#define BUTTON_LEFT   0x0200
#define BUTTON_RIGHT  0x0100
#define BUTTON_A      0x0080
#define BUTTON_X      0x0040
#define BUTTON_L      0x0020
#define BUTTON_R      0x0010

/* Joypad structure */
typedef struct {
    u16 buttons;          /* Current button state */
    u16 buttons_previous; /* Previous button state for edge detection */
    u8 shift_register;    /* Shift register for serial output */
    u8 bit_count;         /* Current bit being read */
    bool strobe;          /* Strobe state (auto-read enable) */
} Joypad;

/* Input system structure */
typedef struct {
    Joypad joypad1;       /* Controller port 1 */
    Joypad joypad2;       /* Controller port 2 */
    
    bool auto_read;       /* Automatic controller reading */
    u16 auto_joy1;        /* Auto-read result for joypad 1 */
    u16 auto_joy2;        /* Auto-read result for joypad 2 */
} InputSystem;

/* Function declarations */

/*
 * Initialize input system
 */
void input_init(InputSystem *input);

/*
 * Reset input system
 */
void input_reset(InputSystem *input);

/*
 * Set button state for controller 1
 */
void input_set_button(InputSystem *input, u8 controller, u16 button, bool pressed);

/*
 * Set multiple buttons at once
 */
void input_set_buttons(InputSystem *input, u8 controller, u16 button_mask);

/*
 * Write to joypad register (strobe)
 */
void input_write_strobe(InputSystem *input, u8 value);

/*
 * Read from joypad port 1
 */
u8 input_read_joypad1(InputSystem *input);

/*
 * Read from joypad port 2
 */
u8 input_read_joypad2(InputSystem *input);

/*
 * Perform auto-read (called during VBlank)
 */
void input_auto_read(InputSystem *input);

/*
 * Check if button is pressed
 */
bool input_is_pressed(const InputSystem *input, u8 controller, u16 button);

/*
 * Check if button was just pressed (edge detection)
 */
bool input_is_pressed_edge(const InputSystem *input, u8 controller, u16 button);

#endif /* INPUT_H */
