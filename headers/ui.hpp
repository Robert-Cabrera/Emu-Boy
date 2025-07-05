#pragma once

#include <../headers/ui.hpp>
#include <string>

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

static const int SCREEN_WIDTH = 1024;
static const int SCREEN_HEIGHT = 768;

void ui_init(std::string);
void ui_handle_events();
void ui_update();

std::string rom_picker_sdl2(SDL_Window *window, SDL_Renderer *renderer, const std::string &directory);