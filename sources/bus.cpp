#include <../headers/bus.hpp>
#include <../headers/cart.hpp>
#include <../headers/ram.hpp>
#include <../headers/cpu.hpp>
#include <../headers/io.hpp>
#include <../headers/dma.hpp>
#include <../headers/ppu.hpp>
#include <stdexcept>

// 0x0000 - 0x3FFF : ROM Bank 0
// 0x4000 - 0x7FFF : ROM Bank 1 - Switchable
// 0x8000 - 0x97FF : CHR RAM
// 0x9800 - 0x9BFF : BG Map 1
// 0x9C00 - 0x9FFF : BG Map 2
// 0xA000 - 0xBFFF : Cartridge RAM
// 0xC000 - 0xCFFF : RAM Bank 0
// 0xD000 - 0xDFFF : RAM Bank 1-7 - switchable - Color only
// 0xE000 - 0xFDFF : Reserved - Echo RAM
// 0xFE00 - 0xFE9F : Object Attribute Memory
// 0xFEA0 - 0xFEFF : Reserved - Unusable
// 0xFF00 - 0xFF7F : I/O Registers
// 0xFF80 - 0xFFFE : Zero Page

u8 bus_read(u16 address){

    if (address < 0x8000){

        return cart_read(address);          // Reading from ROM

    } else if (address < 0xA000) {

        return ppu_vram_read(address);      // CHR RAM or BG Map (PPU VRAM)

    } else if (address < 0xC000) {

        return cart_read(address);          // Cartridge RAM
        
    } else if (address < 0xE000){

        return wram_read(address);          // WRAM (Working RAM)

    } else if (address < 0xFE00){

        return 0;                           // Reserved - Echo RAM (Don't really need it)

    } else if (address < 0xFEA0){

        // Check for DMA transfer
        if (dma_transferring()){
            return 0xFF;
        }

        return ppu_oam_read(address);       // Object Attribute Memory

    } else if (address < 0xFF00){

        return 0;                           // Reserved - (This is not usable)

    } else if (address < 0xFF80){

        return io_read(address);            // I/O Registers

    } else if (address == 0xFFFF){

        return cpu_get_ie_register();       // Interrupt Enable Register
    }

    return hram_read(address);              // Zero Page

}

void bus_write(u16 address, u8 value) {

    if (address >= 0xFF4C && address <= 0xFF7F) {
        // Unused I/O ports, ignore writes and suppress warning
        return;
    }

    if (address < 0x8000) {

        cart_write(address, value);         // ROM Data
    
    
    } else if (address < 0xA000) {
 
        ppu_vram_write(address, value);      // CHR RAM or BG Map (PPU VRAM)

    
    } else if (address < 0xC000) {

        cart_write(address, value);         // EXT-RAM
    
    
    } else if (address < 0xE000) {

        wram_write(address, value);         // WRAM
    
    
    } else if (address < 0xFE00) {

        // Reserved - Echo RAM
    
    } else if (address < 0xFEA0) {
        
        // Check for DMA Transferring, return if active
        if (dma_transferring()){
            return;
        }

        ppu_oam_write(address, value);      // OAM
    
    } else if (address < 0xFF00) {

                                            // Reserved - Unusable

    } else if (address < 0xFF80) {

        io_write(address, value);           // IO Registers
        

    } else if (address == 0xFFFF) {

        cpu_set_ie_register(value);         // Interrupt Enable Register

    } else {

        hram_write(address, value);         // Zero Page
    }
}

u16 bus_read16(u16 address) {
    u16 lo = bus_read(address);
    u16 hi = bus_read(address + 1);

    return lo | (hi << 8);
}

void bus_write16(u16 address, u16 value) {
    bus_write(address + 1, (value >> 8) & 0xFF);
    bus_write(address, value & 0xFF);
}