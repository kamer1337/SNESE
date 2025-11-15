/*
 * input.c - Input/Controller emulation implementation
 */

#include <string.h>
#include "../include/input.h"

void input_init(InputSystem *input) {
    memset(input, 0, sizeof(InputSystem));
    input_reset(input);
}

void input_reset(InputSystem *input) {
    /* Reset joypad 1 */
    input->joypad1.buttons = 0;
    input->joypad1.buttons_previous = 0;
    input->joypad1.shift_register = 0;
    input->joypad1.bit_count = 0;
    input->joypad1.strobe = false;
    
    /* Reset joypad 2 */
    input->joypad2.buttons = 0;
    input->joypad2.buttons_previous = 0;
    input->joypad2.shift_register = 0;
    input->joypad2.bit_count = 0;
    input->joypad2.strobe = false;
    
    /* Reset auto-read */
    input->auto_read = false;
    input->auto_joy1 = 0;
    input->auto_joy2 = 0;
}

void input_set_button(InputSystem *input, u8 controller, u16 button, bool pressed) {
    Joypad *joy;
    
    if (controller == 0) {
        joy = &input->joypad1;
    } else {
        joy = &input->joypad2;
    }
    
    /* Update button state */
    if (pressed) {
        joy->buttons |= button;
    } else {
        joy->buttons &= ~button;
    }
}

void input_set_buttons(InputSystem *input, u8 controller, u16 button_mask) {
    if (controller == 0) {
        input->joypad1.buttons = button_mask;
    } else {
        input->joypad2.buttons = button_mask;
    }
}

void input_write_strobe(InputSystem *input, u8 value) {
    bool new_strobe = (value & 0x01) != 0;
    
    /* Strobe high: latch current button state */
    if (new_strobe) {
        input->joypad1.shift_register = input->joypad1.buttons & 0xFF;
        input->joypad1.bit_count = 0;
        
        input->joypad2.shift_register = input->joypad2.buttons & 0xFF;
        input->joypad2.bit_count = 0;
    }
    
    input->joypad1.strobe = new_strobe;
    input->joypad2.strobe = new_strobe;
}

static u8 read_joypad_bit(Joypad *joy) {
    u8 result;
    
    /* If strobe is high, continuously return first bit */
    if (joy->strobe) {
        return (joy->buttons & 0x80) ? 1 : 0;
    }
    
    /* Return current bit and shift */
    if (joy->bit_count < 16) {
        result = (joy->buttons >> (15 - joy->bit_count)) & 0x01;
        joy->bit_count++;
    } else {
        result = 1;  /* After all bits, return 1 */
    }
    
    return result;
}

u8 input_read_joypad1(InputSystem *input) {
    return read_joypad_bit(&input->joypad1);
}

u8 input_read_joypad2(InputSystem *input) {
    return read_joypad_bit(&input->joypad2);
}

void input_auto_read(InputSystem *input) {
    /* Auto-read captures the current controller state during VBlank */
    if (input->auto_read) {
        input->auto_joy1 = input->joypad1.buttons;
        input->auto_joy2 = input->joypad2.buttons;
    }
}

bool input_is_pressed(const InputSystem *input, u8 controller, u16 button) {
    if (controller == 0) {
        return (input->joypad1.buttons & button) != 0;
    } else {
        return (input->joypad2.buttons & button) != 0;
    }
}

bool input_is_pressed_edge(const InputSystem *input, u8 controller, u16 button) {
    if (controller == 0) {
        return (input->joypad1.buttons & button) != 0 &&
               (input->joypad1.buttons_previous & button) == 0;
    } else {
        return (input->joypad2.buttons & button) != 0 &&
               (input->joypad2.buttons_previous & button) == 0;
    }
}
