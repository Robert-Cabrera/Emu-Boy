#pragma once
#include <../headers/common.hpp>


typedef struct {
    u8 entry[4];
    u8 logo[0x30];

    char title[16];
    u16 new_lic_code;
    u8 sgb_flag;
    u8 type;
    u8 rom_size;
    u8 ram_size;
    u8 dest_code;
    u8 lic_code;
    u8 version;
    u8 checksum;
    u16 global_checksum;
} rom_header;

typedef struct{

    char filename[1024];

    u32 rom_size;
    u8 *rom_data;
    rom_header *header;

    // MBC1 FIELDS
    bool ram_enabled;
    bool ram_banking;

    u8 *rom_bank_x;
    u8 banking_mode;

    u8 rom_bank_value;
    u8 ram_bank_value;

    u8 *ram_bank;
    u8 *ram_banks[16];

    // For battery
    bool battery;   // Does the game have a battery?
    bool need_save; // Do we need to save the game?
} cart_context;

cart_context *cart_get_context();

bool cart_load(char *cart);

u8 cart_read(u16 address);
void cart_write(u16 address, u8 value);

bool cart_need_save();
void cart_battery_load();
void cart_battery_save();