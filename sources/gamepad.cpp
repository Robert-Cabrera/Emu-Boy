#include <../headers/gamepad.hpp>
#include <string.h>

/*

76	            5	            4	           3	        2	        1	            0
P1		Select buttons	Select d-pad	Start / Down	Select / Up	    B / Left	A / Right
*/

typedef struct {
    bool button_selected;
    bool dir_selected;
    gamepad_state controller;
} gamepad_context;

static gamepad_context ctx = {0};

bool gamepad_button_selected(){
    return ctx.button_selected;
}

bool gamepad_dir_selected(){
    return ctx.dir_selected;
}

void gamepad_set_selected(u8 value){
    ctx.button_selected = value & 0b100000;     // we select buttons with bit 5
    ctx.dir_selected = value & 0b010000;        // we select directions with bit 4
}

gamepad_state * gamepad_get_state(){
    return &ctx.controller;
}

u8 gamepad_get_output(){

    // All bottom bits are set to 1 (selected is actually 0 so everything is NOT PRESSED)
    u8 output = 0xCF;

    if (!gamepad_button_selected()){
        if (gamepad_get_state()->start){
            output &= ~(1 << 3); // Start
        } 
        
        if (gamepad_get_state()->select){
            output &= ~(1 << 2); // Select
        } 
        
        if (gamepad_get_state()->a){
            output &= ~(1 << 0); // A
        } 
        
        if (gamepad_get_state()->b){
            output &= ~(1 << 1); // B
        }
    }

    if (!gamepad_dir_selected()){
        if (gamepad_get_state()->left){
            output &= ~(1 << 1); // Left
        } 
        
        if (gamepad_get_state()->right){
            output &= ~(1 << 0); // Right
        }

        if (gamepad_get_state()->up){
            output &= ~(1 << 2); // Up
        }   

        if (gamepad_get_state()->down){
            output &= ~(1 << 3); // Down
        }
    }

    return output;

}
