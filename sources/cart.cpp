#include <../headers/cart.hpp>
#include <map>
#include <string>
#include <string.h>


static cart_context ctx;

cart_context *cart_get_context() {
    return &ctx;
}

bool cart_need_save(){
    return ctx.need_save;
}

bool cart_mbc1(){
    return BETWEEN(ctx.header->type, 0x01, 0x03);
}

bool cart_mbc3() {
    return (ctx.header->type >= 0x0F && ctx.header->type <= 0x13);
}

bool cart_battery() {
    // Only deal with MBC1
    return ctx.header->type == 0x03;
}


// Taken from pandocs
static const char *ROM_TYPES[] = {
    "ROM ONLY",
    "MBC1",
    "MBC1+RAM",
    "MBC1+RAM+BATTERY",
    "0x04 ???",
    "MBC2",
    "MBC2+BATTERY",
    "0x07 ???",
    "ROM+RAM 1",
    "ROM+RAM+BATTERY 1",
    "0x0A ???",
    "MMM01",
    "MMM01+RAM",
    "MMM01+RAM+BATTERY",
    "0x0E ???",
    "MBC3+TIMER+BATTERY",
    "MBC3+TIMER+RAM+BATTERY 2",
    "MBC3",
    "MBC3+RAM 2",
    "MBC3+RAM+BATTERY 2",
    "0x14 ???",
    "0x15 ???",
    "0x16 ???",
    "0x17 ???",
    "0x18 ???",
    "MBC5",
    "MBC5+RAM",
    "MBC5+RAM+BATTERY",
    "MBC5+RUMBLE",
    "MBC5+RUMBLE+RAM",
    "MBC5+RUMBLE+RAM+BATTERY",
    "0x1F ???",
    "MBC6",
    "0x21 ???",
    "MBC7+SENSOR+RUMBLE+RAM+BATTERY",
};

static const std::map<u8, std::string> LIC_CODE = {
    {0x00, "None"},
    {0x01, "Nintendo R&D1"},
    {0x08, "Capcom"},
    {0x13, "Electronic Arts"},
    {0x18, "Hudson Soft"},
    {0x19, "b-ai"},
    {0x20, "kss"},
    {0x22, "pow"},
    {0x24, "PCM Complete"},
    {0x25, "san-x"},
    {0x28, "Kemco Japan"},
    {0x29, "seta"},
    {0x30, "Viacom"},
    {0x31, "Nintendo"},
    {0x32, "Bandai"},
    {0x33, "Ocean/Acclaim"},
    {0x34, "Konami"},
    {0x35, "Hector"},
    {0x37, "Taito"},
    {0x38, "Hudson"},
    {0x39, "Banpresto"},
    {0x41, "Ubi Soft"},
    {0x42, "Atlus"},
    {0x44, "Malibu"},
    {0x46, "angel"},
    {0x47, "Bullet-Proof"},
    {0x49, "irem"},
    {0x50, "Absolute"},
    {0x51, "Acclaim"},
    {0x52, "Activision"},
    {0x53, "American sammy"},
    {0x54, "Konami"},
    {0x55, "Hi tech entertainment"},
    {0x56, "LJN"},
    {0x57, "Matchbox"},
    {0x58, "Mattel"},
    {0x59, "Milton Bradley"},
    {0x60, "Titus"},
    {0x61, "Virgin"},
    {0x64, "LucasArts"},
    {0x67, "Ocean"},
    {0x69, "Electronic Arts"},
    {0x70, "Infogrames"},
    {0x71, "Interplay"},
    {0x72, "Broderbund"},
    {0x73, "sculptured"},
    {0x75, "sci"},
    {0x78, "THQ"},
    {0x79, "Accolade"},
    {0x80, "misawa"},
    {0x83, "lozc"},
    {0x86, "Tokuma Shoten Intermedia"},
    {0x87, "Tsukuda Original"},
    {0x91, "Chunsoft"},
    {0x92, "Video system"},
    {0x93, "Ocean/Acclaim"},
    {0x95, "Varie"},
    {0x96, "Yonezawa/s’pal"},
    {0x97, "Kaneko"},
    {0x99, "Pack in soft"},
    {0xA4, "Konami (Yu-Gi-Oh!)"}
};

const char *cart_lic_name() {
    if (ctx.header->new_lic_code <= 0xA4) {
        auto it = LIC_CODE.find(ctx.header->lic_code);
        if (it != LIC_CODE.end()) {
            return it->second.c_str();
        }
    }

    return "unkown licensee";
}

const char *cart_type_name() {
    if (ctx.header->type <= 0x22) {
        return ROM_TYPES[ctx.header->type];
    }

    return "unkown type";
}

void cart_setup_banking(){
    for (int i = 0 ; i < 16 ; i++){
        ctx.ram_banks[i] = 0;

        // Only has 1 bank if RAM size is 2
        // or 4 banks if RAM size is 3
        if (ctx.header->ram_size == 2 && i == 0 ||
            ctx.header->ram_size == 3 && i < 4  ||
            ctx.header->ram_size == 4 && i < 16 ||
            ctx.header->ram_size == 5 && i < 8) {

            ctx.ram_banks[i] = new u8[0x2000];      // 8KB
            memset(ctx.ram_banks[i], 0, 0x2000);
        }
    }

    ctx.ram_bank = ctx.ram_banks[0];
    ctx.rom_bank_x = ctx.rom_data + 0x4000;         // 16KB ROM bank 1

}

bool cart_load(char *cart) {
    
    snprintf(ctx.filename, sizeof(ctx.filename), "%s", cart);

    FILE *fp = fopen(cart, "rb");
    if (!fp) {printf("Failed to open: %s\n", cart); return false;}

    printf("Opened: %s\n", ctx.filename);

    // Get file size
    fseek(fp, 0, SEEK_END);
    ctx.rom_size = ftell(fp);

    // Go back to start of file
    rewind(fp);

    // Allocate memory and read file
    ctx.rom_data = new u8[ctx.rom_size];
    memset(ctx.rom_data, 0, ctx.rom_size);
    
    fread(ctx.rom_data, ctx.rom_size, 1, fp);
    fclose(fp);


    // Get header and banking
    ctx.header = (rom_header *)(ctx.rom_data + 0x100); // 0x100 is the start of the header in GB carts
    ctx.header->title[15] = 0;

    ctx.battery = cart_battery();
    ctx.need_save = false;

    // Print info tabulated
    char type_str[32];
    snprintf(type_str, sizeof(type_str), "%02X (%-17s)", ctx.header->type, cart_type_name());
    
    char rom_size_str[32];
    snprintf(rom_size_str, sizeof(rom_size_str), "%d KB", 32 << ctx.header->rom_size);
    
    char ram_size_str[32];
    snprintf(ram_size_str, sizeof(ram_size_str), "0x%X", ctx.header->ram_size);
    
    char lic_str[32];
    snprintf(lic_str, sizeof(lic_str), "%02X (%-17s)", ctx.header->lic_code, cart_lic_name());
    
    char rom_ver_str[32];
    snprintf(rom_ver_str, sizeof(rom_ver_str), "0x%X", ctx.header->version);
    
    // Print info tabulated
    printf("===============================================\n");
    printf("| %-12s | %-25s |\n", "Field", "Value");
    printf("===============================================\n");
    printf("| %-12s | %-25s |\n", "Title", ctx.header->title);
    printf("| %-12s | %-25s |\n", "Type", type_str);
    printf("| %-12s | %-25s |\n", "ROM Size", rom_size_str);
    printf("| %-12s | %-25s |\n", "RAM Size", ram_size_str);
    printf("| %-12s | %-25s |\n", "LIC Code", lic_str);
    printf("| %-12s | %-25s |\n", "ROM Vers", rom_ver_str);
    printf("===============================================\n");
    

    // Setup banking
    cart_setup_banking();

    // Nintendo's checksum algorithm
    u16 x = 0;
    for (u16 i=0x0134; i<=0x014C; i++) {
        x = x - ctx.rom_data[i] - 1;
    }

    printf("\t Checksum : %2.2X (%s)\n", ctx.header->checksum, (x & 0xFF) ? "PASSED" : "FAILED");

    if (ctx.battery){
        cart_battery_load();
    }

    return true;
}

void cart_battery_load(){
    char fn[1048];

    char *dot = strrchr(ctx.filename, '.');
    if (dot) {
        char *filename_only = strrchr(ctx.filename, '/');
        if (!filename_only) filename_only = strrchr(ctx.filename, '\\');
        filename_only = filename_only ? filename_only + 1 : ctx.filename;
        
        snprintf(fn, sizeof(fn), "../roms/saves/%.*s.battery", (int)(dot - filename_only), filename_only);
    } else {
        char *filename_only = strrchr(ctx.filename, '/');
        if (!filename_only) filename_only = strrchr(ctx.filename, '\\');
        filename_only = filename_only ? filename_only + 1 : ctx.filename;
        
        snprintf(fn, sizeof(fn), "../roms/saves/%s.battery", filename_only);
    }

    FILE *fp = fopen(fn, "rb");


    if (!fp) {
        // fprintf(stderr, "Failed to open battery file: %s\n", fn);
        return;
    }
    
    fread(ctx.ram_bank, 0x2000, 1, fp); // Save the current RAM bank
    fclose(fp);

}

void cart_battery_save(){
    char fn[1048];

    char *dot = strrchr(ctx.filename, '.');
    if (dot) {
        char *filename_only = strrchr(ctx.filename, '/');
        if (!filename_only) filename_only = strrchr(ctx.filename, '\\');
        filename_only = filename_only ? filename_only + 1 : ctx.filename;
        
        snprintf(fn, sizeof(fn), "../roms/saves/%.*s.battery", (int)(dot - filename_only), filename_only);
    } else {
        char *filename_only = strrchr(ctx.filename, '/');
        if (!filename_only) filename_only = strrchr(ctx.filename, '\\');
        filename_only = filename_only ? filename_only + 1 : ctx.filename;
        
        snprintf(fn, sizeof(fn), "../roms/saves/%s.battery", filename_only);
    }

    FILE *fp = fopen(fn, "wb");

    if (!fp) {
        // fprintf(stderr, "Failed to open battery file: %s\n", fn);
        return;
    }

    fwrite(ctx.ram_bank, 0x2000, 1, fp); // Save the current RAM bank
    fclose(fp);

}

u8 cart_read(u16 address) {
    // Always support ROM bank 0 at 0x0000–0x3FFF
    if (address < 0x4000)
        return ctx.rom_data[address];

    // Cart MBC1
    if (cart_mbc1()) {
        if ((address & 0xE000) == 0xA000) {
            if (!ctx.ram_enabled || !ctx.ram_bank) return 0xFF;
            return ctx.ram_bank[address - 0xA000];
        }
        return ctx.rom_bank_x[address - 0x4000];
    }

    // Cart MBC3
    if (cart_mbc3()) {
        if ((address & 0xE000) == 0xA000) {
            if (!ctx.ram_enabled || !ctx.ram_bank) return 0xFF;
            return ctx.ram_bank[address - 0xA000];
        }
        return ctx.rom_bank_x[address - 0x4000];
    }

    if (ctx.header->type == 0x00) { // ROM ONLY
        if (address < ctx.rom_size)
            return ctx.rom_data[address];
        else
            return 0xFF; // Out of bounds, open bus
    }

    // If unsupported, return open bus
    return 0xFF;
}

void cart_write(u16 address, u8 value) {
    
        // Cart MBC1
    if (cart_mbc1()){
        if (address < 0x2000){
            ctx.ram_enabled = (value & 0x0F) == 0x0A; 
        }
        
        if ((address & 0xE000) == 0x2000){
            // Rom Bank Number
            if (value == 0){
                value = 1; // Bank 0 is not allowed
            }

            value &= 0b11111;

            ctx.rom_bank_value = value;
            ctx.rom_bank_x = ctx.rom_data + (0x4000 * ctx.rom_bank_value);
        }

        if ((address & 0xE000) == 0x4000){
            // Ram Bank Number (2-bit register)
            ctx.ram_bank_value = value & 0b11;
        
            if (ctx.ram_banking){
                if (cart_need_save()){cart_battery_save();}
                ctx.ram_bank = ctx.ram_banks[ctx.ram_bank_value];
            }
        }

        if ((address & 0xE000) == 0x6000){
            // Banking Mode Selection (1-bit register)
            ctx.banking_mode = value & 1; 

            ctx.ram_banking = ctx.banking_mode;
            
            if (ctx.ram_banking){
                ctx.ram_bank = ctx.ram_banks[ctx.ram_bank_value];
            }
        }

        if ((address & 0xE000) == 0xA000){
            
            if (!ctx.ram_enabled){
                return;
            }
            
            if (!ctx.ram_bank) {
                return;
            }

            ctx.ram_bank[(address - 0xA000)] = value;
        
            if (ctx.battery) {
                ctx.need_save = true; // We need to save the battery
                }
            }
    }

    // Cart MBC3
    if (cart_mbc3()) {
        // RAM enable
        if (address < 0x2000) {
            ctx.ram_enabled = ((value & 0x0F) == 0x0A);
            return;
        }
        // ROM bank number (7 bits, never 0)
        if (address >= 0x2000 && address < 0x4000) {
            u8 bank = value & 0x7F;
            if (bank == 0) bank = 1;
            ctx.rom_bank_value = bank;
            ctx.rom_bank_x = ctx.rom_data + (0x4000 * ctx.rom_bank_value);
            return;
        }
        // RAM bank number (2 bits, 0–3)
        if (address >= 0x4000 && address < 0x6000) {
            ctx.ram_bank_value = value & 0x03;
            ctx.ram_bank = ctx.ram_banks[ctx.ram_bank_value];
            // RTC select ignored (value 0x08–0x0C) for now
            return;
        }
        // Latch clock (RTC) is not implemented, ignore 0x6000–0x7FFF
        // RAM write
        if ((address & 0xE000) == 0xA000) {
            if (!ctx.ram_enabled || !ctx.ram_bank) return;
            ctx.ram_bank[address - 0xA000] = value;
            if (ctx.battery) ctx.need_save = true;
            return;
        }
        return;
    }

    if (ctx.header->type == 0x00) {
        return;
    }
}