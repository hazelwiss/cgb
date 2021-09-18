#pragma once
#include<utility.h>
#include<SDL2/SDL.h>

typedef struct CPU CPU;

void initDisplay(struct CPU* cpu, const char* name, size_t width, size_t height);

void updateMainWindow(const void* pixeldata, size_t row_length);

void updateSubwindows(void);

void setRenderTileMap(bool);

void setRenderBGMap(bool);