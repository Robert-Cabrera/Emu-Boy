#include <../headers/common.hpp>
#include <../headers/ui.hpp>
#include "../headers/main.hpp"
#include "../headers/ppu.hpp"
#include "../headers/bus.hpp"
#include "../headers/gamepad.hpp"
#include "../headers/cart.hpp"

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>

#include <dirent.h>
#include <algorithm>
#include <vector>
#include <string>
#include <unistd.h>

SDL_Window *sdlWindow;
SDL_Renderer *sdlRenderer;
SDL_Texture *sdlTexture;
SDL_Surface *screen;

static SDL_Texture* bgTexture = nullptr;

#define GB_WIDTH 160
#define GB_HEIGHT 144

// White, Light Gray, Dark Gray, Black
static unsigned long tile_colors[4] = {0xFFFFFFFF, 0xFFAAAAAA, 0xFF555555, 0xFF000000};

// Lowercase helper
std::string to_lower(const std::string& s) {
    std::string out = s;
    std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c){ return std::tolower(c); });
    return out;
}

// Returns background asset filename
std::string pick_background(const std::string& title) {
    std::string low = to_lower(title);
    if (low.find("pokemon") != std::string::npos || low.find("pokémon") != std::string::npos || low.find("pkmn") != std::string::npos)
        return "../assets/pokemon.jpg";
    if (low.find("mario") != std::string::npos)
        return "../assets/mario.jpg";
    if (low.find("tetris") != std::string::npos)
        return "../assets/tetris.jpg";
    if (low.find("metroid") != std::string::npos)
        return "../assets/metroid.jpg";
    if (low.find("zelda") != std::string::npos || low.find("link") != std::string::npos)
        return "../assets/zelda.jpg";
    return "../assets/default.jpg";
}

void darken_surface(SDL_Surface* surface, float factor) {
    if (!surface) return;

    // Lock the surface if needed
    if (SDL_MUSTLOCK(surface)) SDL_LockSurface(surface);

    Uint8 r, g, b, a = 255;
    for (int y = 0; y < surface->h; ++y) {
        Uint8* row = (Uint8*)surface->pixels + y * surface->pitch;
        for (int x = 0; x < surface->w; ++x) {
            Uint32 pixel_color = 0;

            // Read the pixel (safe for any BytesPerPixel)
            switch (surface->format->BytesPerPixel) {
                case 1:
                    pixel_color = row[x];
                    break;
                case 2:
                    pixel_color = ((Uint16*)row)[x];
                    break;
                case 3: {
                    Uint8 *p = row + x * 3;
                    pixel_color = (p[0]) | (p[1] << 8) | (p[2] << 16);
                    break;
                }
                case 4:
                    pixel_color = ((Uint32*)row)[x];
                    break;
            }

            SDL_GetRGBA(pixel_color, surface->format, &r, &g, &b, &a);
            r = static_cast<Uint8>(r * factor);
            g = static_cast<Uint8>(g * factor);
            b = static_cast<Uint8>(b * factor);
            Uint32 dark_pixel = SDL_MapRGBA(surface->format, r, g, b, a);

            // Write the pixel back
            switch (surface->format->BytesPerPixel) {
                case 1:
                    row[x] = (Uint8)dark_pixel;
                    break;
                case 2:
                    ((Uint16*)row)[x] = (Uint16)dark_pixel;
                    break;
                case 3: {
                    Uint8 *p = row + x * 3;
                    p[0] = (dark_pixel) & 0xFF;
                    p[1] = (dark_pixel >> 8) & 0xFF;
                    p[2] = (dark_pixel >> 16) & 0xFF;
                    break;
                }
                case 4:
                    ((Uint32*)row)[x] = dark_pixel;
                    break;
            }
        }
    }

    if (SDL_MUSTLOCK(surface)) SDL_UnlockSurface(surface);
}


// Calculate integer scale and center
void get_display_rect(SDL_Rect *dest_rect, int win_w, int win_h) {
    int scale_w = win_w / GB_WIDTH;
    int scale_h = win_h / GB_HEIGHT;
    int scale = (scale_w < scale_h ? scale_w : scale_h);
    if (scale < 1) scale = 1;

    int draw_w = GB_WIDTH * scale;
    int draw_h = GB_HEIGHT * scale;

    dest_rect->w = draw_w;
    dest_rect->h = draw_h;
    dest_rect->x = (win_w - draw_w) / 2;
    dest_rect->y = (win_h - draw_h) / 2;
}

void ui_init(std::string g_rom_path) {
    printf("[ui_init] Start\n");

    // Window/Renderer
    sdlWindow = SDL_CreateWindow("Game Boy Emulator", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                 160*5, 144*5, SDL_WINDOW_RESIZABLE);
    if (!sdlWindow) {
        printf("[ui_init] SDL_CreateWindow failed: %s\n", SDL_GetError());
        exit(1);
    }
    printf("[ui_init] Window OK\n");

    sdlRenderer = SDL_CreateRenderer(sdlWindow, -1, SDL_RENDERER_ACCELERATED);
    if (!sdlRenderer) {
        printf("[ui_init] SDL_CreateRenderer failed: %s\n", SDL_GetError());
        exit(1);
    }
    printf("[ui_init] Renderer OK\n");

    // Surface
    screen = SDL_CreateRGBSurface(0, 160, 144, 32,
                                  0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
    if (!screen) {
        printf("[ui_init] SDL_CreateRGBSurface failed: %s\n", SDL_GetError());
        exit(1);
    }
    printf("[ui_init] Surface OK\n");

    // Texture
    sdlTexture = SDL_CreateTexture(sdlRenderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, 160, 144);
    if (!sdlTexture) {
        printf("[ui_init] SDL_CreateTexture failed: %s\n", SDL_GetError());
        exit(1);
    }
    printf("[ui_init] Texture OK\n");
    // --- Use ROM filename for background ---
    std::string bg_file;

    // Find the last '/' or '\\' and '.' in g_rom_path
    size_t last_slash = g_rom_path.find_last_of("/\\");
    size_t last_dot = g_rom_path.find_last_of(".");
    std::string rom_basename;

    if (last_slash == std::string::npos)
        rom_basename = g_rom_path;
    else
        rom_basename = g_rom_path.substr(last_slash + 1);

    if (last_dot != std::string::npos && last_dot > last_slash)
        rom_basename = rom_basename.substr(0, last_dot - last_slash - 1);

    // Now pick the background based on the filename (without extension)
    bg_file = pick_background(rom_basename);

    SDL_Surface* bgSurface = IMG_Load(bg_file.c_str());
    if (!bgSurface) {
        printf("[ui_init] Could not load background %s: %s\n", bg_file.c_str(), IMG_GetError());
    } else {
        darken_surface(bgSurface, 0.35f);
        bgTexture = SDL_CreateTextureFromSurface(sdlRenderer, bgSurface);
        SDL_FreeSurface(bgSurface);
        if (!bgTexture) {
            printf("[ui_init] SDL_CreateTextureFromSurface failed: %s\n", SDL_GetError());
        } else {
            printf("[ui_init] Background texture OK\n");
        }
    }
    printf("[ui_init] Finished successfully\n");
}


void delay(u32 ms) {
    SDL_Delay(ms);
}

u32 get_ticks(){
    return SDL_GetTicks();
}

void ui_update() {
    int win_w, win_h;
    SDL_GetWindowSize(sdlWindow, &win_w, &win_h);

    u32 *video_buffer = ppu_get_context()->video_buffer;

    for (int line_num = 0; line_num < GB_HEIGHT; line_num++) {
        for (int x = 0; x < GB_WIDTH; x++) {
            ((u32*)screen->pixels)[x + (line_num * GB_WIDTH)] = video_buffer[x + (line_num * GB_WIDTH)];
        }
    }

    SDL_UpdateTexture(sdlTexture, NULL, screen->pixels, screen->pitch);

    SDL_Rect dest_rect;
    get_display_rect(&dest_rect, win_w, win_h);

    // --- Render background ---
    if (bgTexture) {
        SDL_Rect bgRect = {0, 0, win_w, win_h};
        SDL_RenderCopy(sdlRenderer, bgTexture, NULL, &bgRect);
    } else {
        SDL_SetRenderDrawColor(sdlRenderer, 0,0,0,255);
        SDL_RenderClear(sdlRenderer);
    }

    // --- Render Game Boy screen (centered & scaled) ---
    SDL_RenderCopy(sdlRenderer, sdlTexture, NULL, &dest_rect);
    SDL_RenderPresent(sdlRenderer);
}

void ui_on_key(bool down, u32 key_code){
    switch (key_code){
        case SDLK_z:      gamepad_get_state()->a = down; break;
        case SDLK_x:      gamepad_get_state()->b = down; break;
        case SDLK_RETURN:
        case SDLK_KP_ENTER:
                          gamepad_get_state()->start = down; break;
        case SDLK_TAB:    gamepad_get_state()->select = down; break;
        case SDLK_UP:     gamepad_get_state()->up = down; break;
        case SDLK_DOWN:   gamepad_get_state()->down = down; break;
        case SDLK_LEFT:   gamepad_get_state()->left = down; break;
        case SDLK_RIGHT:  gamepad_get_state()->right = down; break;
    }
}

void ui_handle_events() {
    SDL_Event e;
    while (SDL_PollEvent(&e) > 0) {
        if (e.type == SDL_KEYDOWN){
            ui_on_key(true, e.key.keysym.sym);
        }
        if (e.type == SDL_KEYUP){
            ui_on_key(false, e.key.keysym.sym);
        }
        if (e.type == SDL_WINDOWEVENT && e.window.event == SDL_WINDOWEVENT_CLOSE) {
            emu_get_context()->die = true;
        }
    }
}
static std::vector<std::string> list_roms(const std::string &directory) {
    std::vector<std::string> files;
    DIR *dir = opendir(directory.c_str());
    if (!dir) return files;
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        std::string name = entry->d_name;
        std::string low = name;
        std::transform(low.begin(), low.end(), low.begin(), ::tolower);
        if (low.size() > 3 && (low.substr(low.size()-3)==".gb" || low.substr(low.size()-4)==".gbc"))
            files.push_back(name);
    }
    closedir(dir);
    std::sort(files.begin(), files.end());
    return files;
}

std::string rom_picker_sdl2(SDL_Window *window, SDL_Renderer *renderer, const std::string &directory) {
    auto roms = list_roms(directory);
    if (roms.empty()) return "";

    TTF_Font *font = TTF_OpenFont("../assets/fonts/NotoSansMono-Medium.ttf", 18);
    if (!font) { printf("Font load error: %s\n", TTF_GetError()); }

    int selected = 0;
    bool done = false;
    int win_w=500, win_h=280;  // A bit taller for more items

    SDL_SetWindowSize(window, win_w, win_h);

    int visible_lines = (win_h - 60) / 28; // 28 px per line, leave space for padding
    int scroll_offset = 0;

    while (!done) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) { if (font) TTF_CloseFont(font); return ""; }
            if (e.type == SDL_KEYDOWN) {
                if (e.key.keysym.sym == SDLK_DOWN)  {
                    selected = (selected + 1) % roms.size();
                    if (selected - scroll_offset >= visible_lines) scroll_offset++;
                }
                if (e.key.keysym.sym == SDLK_UP)    {
                    selected = (selected - 1 + roms.size()) % roms.size();
                    if (selected < scroll_offset) scroll_offset = selected;
                }
                if (e.key.keysym.sym == SDLK_RETURN) { if (font) TTF_CloseFont(font); return directory + "/" + roms[selected]; }
                if (e.key.keysym.sym == SDLK_ESCAPE) { if (font) TTF_CloseFont(font); return ""; }
            }
        }

        SDL_SetRenderDrawColor(renderer, 16,16,16,255); // Background: black
        SDL_RenderClear(renderer);

        // Title
        if (font) {
            SDL_Color fg = {220,220,220,255};
            SDL_Surface* txt = TTF_RenderUTF8_Blended(font, "Select ROM", fg);
            if (txt) {
                SDL_Texture* txtTex = SDL_CreateTextureFromSurface(renderer, txt);
                int tw=0, th=0;
                SDL_QueryTexture(txtTex, NULL, NULL, &tw, &th);
                SDL_Rect tr = { (win_w-tw)/2, 18, tw, th };
                SDL_RenderCopy(renderer, txtTex, NULL, &tr);
                SDL_DestroyTexture(txtTex);
                SDL_FreeSurface(txt);
            }
        }

        int start_y = 60;
        int line_h = 28;

        for (size_t vi=0; vi < visible_lines && (vi+scroll_offset)<roms.size(); ++vi) {
            int i = vi + scroll_offset;
            SDL_Rect r = {40, start_y + int(vi)*line_h, win_w-80, line_h};
            if (i == selected) {
                // Draw light rounded rectangle highlight
                SDL_SetRenderDrawColor(renderer, 220, 220, 220, 255); // White(ish) highlight
                SDL_RenderFillRect(renderer, &r);
                SDL_SetRenderDrawColor(renderer, 70, 70, 70, 255); // Border
                SDL_RenderDrawRect(renderer, &r);
            } else {
                SDL_SetRenderDrawColor(renderer, 30,30,30,255); // Slightly lighter than bg
                SDL_RenderFillRect(renderer, &r);
            }

            // Draw text
            if (font) {
                SDL_Color fg = (i==selected) ? SDL_Color{16,16,16,255} : SDL_Color{200,200,200,255};
                SDL_Surface* txt = TTF_RenderUTF8_Blended(font, roms[i].c_str(), fg);
                if (txt) {
                    SDL_Texture* txtTex = SDL_CreateTextureFromSurface(renderer, txt);
                    int tw=0, th=0;
                    SDL_QueryTexture(txtTex, NULL, NULL, &tw, &th);
                    SDL_Rect tr = { r.x+12, r.y+(line_h-th)/2, tw, th };
                    SDL_RenderCopy(renderer, txtTex, NULL, &tr);
                    SDL_DestroyTexture(txtTex);
                    SDL_FreeSurface(txt);
                }
            }
        }
        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }
    if (font) TTF_CloseFont(font);
    return "";
}