#pragma once

#include <../headers/common.hpp>
#include <string>

typedef struct {
    bool paused;
    bool running;
    bool die;
    u64 ticks;
} emu_context;

int emu_run(std::string);

emu_context *emu_get_context();

void emu_cycles(int cpu_cycles);

