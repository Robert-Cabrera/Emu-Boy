#pragma once

#include <../headers/common.hpp>   

typedef struct{
    bool start;
    bool select;
    bool a; 
    bool b;
    bool up;
    bool down; 
    bool left;
    bool right;
} gamepad_state;

void gamepad_init();
bool gamepad_button_selected();
bool gamepad_dir_selected();
void gamepad_set_selected(u8 value);

gamepad_state * gamepad_get_state();
u8 gamepad_get_output();