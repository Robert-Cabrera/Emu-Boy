#pragma once

#include <../headers/common.hpp>


#pragma pack(push, 1)
typedef struct{
    
    // Registers
    u8 lcdc;            // FF40
    u8 lcds;            // FF41
    
    u8 scroll_y;        // FF42
    u8 scroll_x;        // FF43
    
    u8 ly;
    u8 ly_compare;
    
    u8 dma;             // FF44
    
    u8 bg_palette;
    u8 obj_palette[2];
    
    u8 win_y;
    u8 win_x;
    
    // OTHER DATA:
    u32 bg_colors[4];
    u32 sp1_colors[4];
    u32 sp2_colors[4];
    
} lcd_context;
#pragma pack(pop)

typedef enum {

    MODE_HBLANK,
    MODE_VBLANK,
    MODE_OAM,
    MODE_TRANSFER

} lcd_mode;

lcd_context *lcd_get_context();

/* //--------------------------------------------------------------------------------------------------------------------------------
FF40 — LCDC: LCD control

        7	                6	                5	            4	            3	        2	        1	           0
LCD & PPU enable	Window tile map	    Window enable	BG & Window tiles	BG tile map	OBJ size	OBJ enable	    Priority

*/

#define LCDC_BGW_ENABLE     (BIT(lcd_get_context()->lcdc,       0))
#define LCDC_OBJ_ENABLE     (BIT(lcd_get_context()->lcdc,       1))
#define LCDC_OBJ_HEIGHT     (BIT(lcd_get_context()->lcdc,       2) ? 16 : 8)
#define LCDC_BG_MAP_AREA    (BIT(lcd_get_context()->lcdc,       3) ? 0x9C00 : 0x9800)
#define LCDC_BGW_DATA_AREA  (BIT(lcd_get_context()->lcdc,       4) ? 0x8000 : 0x8800)
#define LCDC_WIN_ENABLE     (BIT(lcd_get_context()->lcdc,       5))
#define LCDC_WIN_MAP_AREA   (BIT(lcd_get_context()->lcdc,       6) ? 0x9C00 : 0x9800)
#define LCDC_LCD_ENABLE     (BIT(lcd_get_context()->lcdc,       7))


//--------------------------------------------------------------------------------------------------------------------------------



/*//--------------------------------------------------------------------------------------------------------------------------------
FF41 — STAT: LCD status
 7	        6	                        5	                    4	              3	                2	        1	    0
NONE    LYC int select	        Mode 2 int select	    Mode 1 int select	Mode 0 int select	LYC == LY	     PPU mode

*/ 

#define LCDS_MODE               ((lcd_mode)(lcd_get_context()->lcds & 0b11))
#define LCDS_MODE_SET(mode)     { lcd_get_context()->lcds &= ~0b11; lcd_get_context()->lcds |= mode; }      // Clear first, then set

#define LCDS_LYC                (BIT(lcd_get_context()->lcds, 2))
#define LCDS_LYC_SET(b)         (BIT_SET(lcd_get_context()->lcds, 2, b))                                     // Sets the bit if equal

typedef enum {
    SS_HBLANK = (1 << 3),
    SS_VBLANK = (1 << 4),
    SS_OAM    = (1 << 5),
    SS_LYC    = (1 << 6)
}   stat_src;

#define LCDS_STAT_INT(src) (lcd_get_context()->lcds & src)

void lcd_init();

u8 lcd_read(u16 address);
void lcd_write(u16 address, u8 value);