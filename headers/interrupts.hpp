
#pragma once

#include <../headers/cpu.hpp>
#include <../headers/common.hpp>

typedef enum {
    IT_VBLANK = 1,          // Vertical Blank Interrupt
    IT_LCD_STAT = 2,        // LCD Status Interrupt
    IT_TIMER = 4,           // Timer Interrupt
    IT_SERIAL = 8,          // Serial Transfer Interrupt    
    IT_JOYPAD = 16          // Joypad Input Interrupt
} interrupt_type;

void cpu_request_interrupt(interrupt_type t);

void cpu_handle_interrupts(cpu_context *ctx);
