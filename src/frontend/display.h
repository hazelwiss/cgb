#pragma once
#include <utility.h>
#include <SDL2/SDL.h>

typedef struct CPU CPU;

void initDisplay(struct CPU *cpu, const char *name, size_t width,
                 size_t height);

void updateWindows(const void *pixeldata, size_t row_length);
