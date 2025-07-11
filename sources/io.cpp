#include <../headers/io.hpp>
#include <../headers/bus.hpp>
#include <../headers/timer.hpp>
#include <../headers/dma.hpp>
#include <../headers/lcd.hpp>
#include <../headers/cpu.hpp>
#include <../headers/gamepad.hpp>

static char serial_data[2];

u8 io_read(u16 address) {

    if (address == 0xFF00){
        return gamepad_get_output();
    }

    if (address == 0xFF01) {
        return serial_data[0];
    }

    if (address == 0xFF02) {
        return serial_data[1];
    }

    if (BETWEEN(address, 0xFF04, 0xFF07)) {
        return timer_read(address);
    }

    if (address == 0xFF0F) {
        return cpu_get_int_flags();
    }

    if (BETWEEN(address, 0xFF40, 0xFF4B)) {
        return lcd_read(address);
    }

    if (BETWEEN(address, 0xFF10, 0xFF3F)) {
        // Sound registers
        return 0;
    } 

    if (address == 0xFF7F) {
        // Unused I/O port. Ignore reads.
        return 0;
    }

    printf("Unsupported Bus Read (%04X)\n", address);


    return 0;
}

void io_write(u16 address, u8 value) {

    if (address == 0xFF00) {
        gamepad_set_selected(value);
        return;
    }

    if (address == 0xFF01) {
        serial_data[0] = value;
        return;
    }

    if (address == 0xFF02) {
        serial_data[1] = value;
        return;
    }

    if (BETWEEN(address, 0xFF04, 0xFF07)) {
        timer_write(address, value);
        return;
    }
    
    if (address == 0xFF0F) {
        cpu_set_int_flags(value);
        return;
    }

    if (BETWEEN(address, 0xFF40, 0xFF4B)) {
        lcd_write(address, value);
        return;
    }

    if (BETWEEN(address, 0xFF10, 0xFF3F)) {
        // Sound registers
        return;
    } 

    if (address == 0xFF7F) {
        // Unused I/O port. Ignore writes.
        return;
    }

    printf("Unsupported Bus Write (%04X)\n", address);


}
