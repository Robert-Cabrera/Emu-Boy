#include "../headers/ppu.hpp"
#include "../headers/lcd.hpp"
#include <../headers/cpu.hpp>
#include <../headers/interrupts.hpp>
#include <../headers/cart.hpp>
#include <string.h>

static u32 target_frame_time = 1000 / 60;
static long prev_frame_time = 0;
static long start_timer = 0;
static long frame_count = 0;

void increment_ly(){

    auto lcdgc = lcd_get_context();
    auto ppuc  = ppu_get_context();

    if (window_visible() && lcdgc->ly >= lcdgc->win_y && lcdgc->ly < lcdgc->win_y + YRES){
        ppuc->window_line++;
    }

    lcd_get_context()->ly++;
    
    if (lcd_get_context()->ly == lcd_get_context()->ly_compare){
        LCDS_LYC_SET(1);

        if (LCDS_STAT_INT(SS_LYC)){
            cpu_request_interrupt(IT_LCD_STAT);
        }
    
    } else {
        LCDS_LYC_SET(0);
    }
}

void load_line_sprites (){
    int cur_y = lcd_get_context()->ly;

    u8 sprite_height = LCDC_OBJ_HEIGHT;
    memset(ppu_get_context()->line_entry_array, 0, 
          sizeof(ppu_get_context()->line_entry_array));
    
    for (int i = 0 ; i < 40 ; i++){
        oam_entry e = ppu_get_context()->oam_ram[i];
        
        if (e.x == 0){
            // X = 0 is always invisible
            continue;
        }

        if (ppu_get_context()->line_sprite_count >= 10){
            // Only up to 10 sprites 
            break;
        }

        if (e.y <= cur_y + 16 && e.y + sprite_height > cur_y + 16){
            // The sprite is on the current line
            oam_line_entry *entry = &ppu_get_context()->line_entry_array[ppu_get_context()->line_sprite_count++];

            entry-> entry = e;
            entry-> next = nullptr;

            if(!ppu_get_context()->line_sprites || ppu_get_context()->line_sprites->entry.x > e.x){
                entry-> next = ppu_get_context()->line_sprites;
                ppu_get_context()->line_sprites = entry;
                continue;
            }

            // Sort the sprites (Some games cannot render if sprites do not show up on the right order)
            
            oam_line_entry *le = ppu_get_context()->line_sprites;
            oam_line_entry *prev = le;

            while (le){
                if (le->entry.x > e.x){
                    prev->next = entry;
                    entry->next = le;
                    break;
                }

                if (!le->next){
                    le->next = entry;
                    break;
                }

                prev = le;
                le = le -> next;
            }
        }
    }

}

void ppu_mode_oam(){
    if (ppu_get_context()->line_ticks >= 80){
        LCDS_MODE_SET(MODE_TRANSFER);

        ppu_get_context()->pfc.cur_fetch_state  = FS_TILE;
        ppu_get_context()->pfc.line_x           = 0;
        ppu_get_context()->pfc.fetch_x          = 0;
        ppu_get_context()->pfc.pushed_x         = 0;
        ppu_get_context()->pfc.fifo_x           = 0;
    }

    if(ppu_get_context()->line_ticks == 1){
        // Read the OAM on the first tick only
        ppu_get_context()-> line_sprites = 0;
        ppu_get_context()-> line_sprite_count = 0;

        load_line_sprites();
    }
}

void ppu_mode_transfer(){

    pipeline_process();

    if (ppu_get_context()->pfc.pushed_x >= XRES){
        
        pipeline_fifo_reset();
        LCDS_MODE_SET(MODE_HBLANK);

        if (LCDS_STAT_INT(SS_HBLANK)){
            cpu_request_interrupt(IT_LCD_STAT);
        }
    }
}

void ppu_mode_hblank(){
    if (ppu_get_context()->line_ticks >= TICKS_PER_LINE){
        increment_ly();

        if (lcd_get_context()->ly >= YRES){
            LCDS_MODE_SET(MODE_VBLANK);

            cpu_request_interrupt(IT_VBLANK);

            if (LCDS_STAT_INT(SS_VBLANK)){
                cpu_request_interrupt(IT_LCD_STAT);
            }

            ppu_get_context()->current_frame++;

            // FPS calculation and timing
            u32 end = get_ticks();
            u32 frame_time = end - prev_frame_time;

            if (frame_time < target_frame_time){
                delay((target_frame_time - frame_time));
            }

            if (end - start_timer >= 1000) {
                u32 fps = frame_count;
                start_timer = end;
                frame_count = 0;

                printf("FPS: %d\n", fps);

                // This block runs every second thus we can check if we need to save the cart
                if (cart_need_save()){cart_battery_save();}
            }

            frame_count++;
            prev_frame_time = get_ticks();

        } else {
            LCDS_MODE_SET(MODE_OAM);
        }

        ppu_get_context()->line_ticks = 0;
    }
  }

void ppu_mode_vblank(){
    if (ppu_get_context()->line_ticks >= TICKS_PER_LINE){
        increment_ly();

        if (lcd_get_context()->ly >= LINES_PER_FRAME){
            LCDS_MODE_SET(MODE_OAM);
            lcd_get_context()->ly = 0;
            ppu_get_context()-> window_line = 0;
        }

        ppu_get_context()->line_ticks = 0;
    }
}