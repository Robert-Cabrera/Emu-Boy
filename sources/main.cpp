#include <stdio.h>

#include "../headers/main.hpp"
#include "../headers/cart.hpp"
#include "../headers/cpu.hpp"
#include "../headers/ui.hpp"
#include "../headers/dma.hpp"
#include "../headers/ppu.hpp"
#include "../headers/timer.hpp"


#include <pthread.h>
#include <unistd.h>
#include <dirent.h>
#include <string>
#include <array>
#include <libgen.h>
#include <limits.h>

/* 
  Emu components:

  Cart
  CPU
  Address Bus
  PPU
  Timer

*/

std::string g_rom_path;

static emu_context ctx;

emu_context *emu_get_context() {
    return &ctx;
}

void *cpu_run(void *p) {

    timer_init();
    cpu_init();
    ppu_init();

    ctx.running = true;
    ctx.paused = false;
    ctx.ticks = 0;

    while(ctx.running) {
        if (ctx.paused) {
            delay(10);
            continue;
        }

        if (!cpu_step()) {
            printf("CPU Stopped\n");
            return 0;
        }
    }

    return 0;
}


int emu_run(std::string g_rom_path) {
   
    printf("Cart loaded..\n");
    printf("ROM: %s\n", g_rom_path.c_str());

    ui_init(g_rom_path);

    pthread_t t1;

    if (pthread_create(&t1, NULL, cpu_run, NULL)) {
        fprintf(stderr, "FAILED TO START MAIN CPU THREAD!\n");
        return -1;
    }


    u32 prev_frame = 0;


    while(!ctx.die) {
        usleep(1000);
        ui_handle_events();

        // Only when there's a frame change we change the UI
        if (prev_frame != ppu_get_context()->current_frame){
            ui_update();
        }


        prev_frame = ppu_get_context()->current_frame;

    }

    return 0;
}

void emu_cycles(int cpu_cycles) {
    
    for (int i = 0; i < cpu_cycles ; i++){
        
        // Timer ticks 4 TIMES per CPU Cycle
        for (int n = 0 ; n < 4 ; n++){
            ctx.ticks++;
            timer_tick();
            ppu_tick();
        }
        
        // DMA ticks ONCE per CPU Cycle
        dma_tick();
    }
}



std::string zenity_select_folder() {
    std::array<char, 512> buffer;
    std::string result;
    const char* display = getenv("DISPLAY");
    const char* xauth = getenv("XAUTHORITY");
    char cwd[PATH_MAX];
    getcwd(cwd, sizeof(cwd)); 

    // Get parent directory
    char parent[PATH_MAX];
    strncpy(parent, cwd, sizeof(parent));
    parent[sizeof(parent) - 1] = '\0';
    dirname(parent); 

    std::string cmd = "env -i ";
    if (display) cmd += std::string("DISPLAY=") + display + " ";
    if (xauth)   cmd += std::string("XAUTHORITY=") + xauth + " ";
    cmd += "PATH=/usr/bin:/bin /usr/bin/zenity --file-selection --directory --filename=\"";
    cmd += parent;
    cmd += "/\" 2>&1";

    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) {
        printf("Could not start zenity.\n");
        return "";
    }
    while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
        result += buffer.data();
    }
    int code = pclose(pipe);
    if (!result.empty() && result.back() == '\n') result.pop_back();
    return result;
}



// Entry point of the program
int main(int argc, char **argv) {
    
    std::string rom_folder = zenity_select_folder();
    if (rom_folder.empty()) {
        printf("No ROM folder selected. Exiting.\n");
        return 0;
    }
    printf("Selected ROM folder: %s\n", rom_folder.c_str());
    
    // Initialize SDL2 andROM picker UI as normal 
    SDL_Init(SDL_INIT_VIDEO);
    TTF_Init();

    SDL_Window *pickerWindow;
    SDL_Renderer *pickerRenderer;
    SDL_CreateWindowAndRenderer(600, 400, 0, &pickerWindow, &pickerRenderer);

    // SDL2 picker with the selected folder
    std::string rom_path = rom_picker_sdl2(pickerWindow, pickerRenderer, rom_folder);

    SDL_DestroyRenderer(pickerRenderer);
    SDL_DestroyWindow(pickerWindow);

    if (rom_path.empty()) {
        printf("No ROM selected. Exiting.\n");
        return 0;
    }
    
    g_rom_path = rom_path;

    if (!cart_load((char*)rom_path.c_str())) {
        printf("Failed to load ROM!\n");
        return 1; // <-- or exit(-1)
    }

    // Continue to launch the main emulator UI and core
    return emu_run(g_rom_path);
}