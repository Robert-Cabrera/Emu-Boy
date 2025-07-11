#pragma once

#include <../headers/common.hpp>

// PANDOCS - Rendering - Pixel FIFO
static const int LINES_PER_FRAME = 154;
static const int TICKS_PER_LINE  = 456;
static const int YRES            = 144;
static const int XRES            = 160;

typedef enum {
    FS_TILE,
    FS_DATA0,
    FS_DATA1,
    FS_IDLE,
    FS_PUSH
} fetch_state;


// FIFO Structure:

typedef struct _fifo_entry{
    struct _fifo_entry *next;
    u32 value;
} fifo_entry;

typedef struct {
    fifo_entry *head;
    fifo_entry *tail;
    u32 size;
} fifo;

typedef struct{
    
    fetch_state cur_fetch_state;
    fifo pixel_fifo;
    u8 line_x;
    u8 pushed_x;
    u8 fetch_x;

    u8 bgw_fetch_data[3];
    u8 fetch_entry_data[6];

    u8 map_y;
    u8 map_x;

    u8 tile_y;
    u8 fifo_x;

    
} pixel_fifo_context;


typedef struct {
    u8 y;
    u8 x;
    u8 tile;
    
    // Bit fields for flags:

    /*
    
    	7	            6	        5	        4	        3	        2	        1	        0
    BG attributes	Priority      Y flip	  X flip	   Bank	       |-     Color palette     -|

    */

    u8 f_cgb_pn : 3;          // GameBoy Color Palette Number
    u8 f_cgb_vram_bank : 1;   // GameBoy Color VRAM
    u8 f_pn : 1;              // Palette Number
    u8 f_x_flip: 1;           // Should the sprite be flipped on the x-axis?
    u8 f_y_flip: 1;           // Should it be flipped on the y-axis?
    u8 f_bgp: 1;              // Background priority

} oam_entry;

typedef struct _oam_line_entry{
    oam_entry entry;
    struct _oam_line_entry *next;
} oam_line_entry;

typedef struct {
    
    oam_entry oam_ram[40];
    u8 vram[0x2000];
    
    pixel_fifo_context pfc;
    
    // 0 - 10 sprites
    u8 line_sprite_count;

    // Current sprites in line
    oam_line_entry *line_sprites;

    // OAM Memory
    oam_line_entry line_entry_array[10];

    u8 fetched_entry_count;
    oam_entry fetched_entries[3];
    u8 window_line;


    u32 current_frame;
    u32 line_ticks;
    u32 *video_buffer;

} ppu_context;

ppu_context *ppu_get_context();

void ppu_init();
void ppu_tick();
 
void ppu_oam_write(u16 address, u8 value);
u8 ppu_oam_read(u16 address);

void ppu_vram_write(u16 address, u8 value);
u8 ppu_vram_read(u16 address);

void pipeline_process();
void pipeline_fifo_reset();


bool window_visible();