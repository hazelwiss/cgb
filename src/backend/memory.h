#pragma once
#include <limits.h>
#include <utility.h>
#include <backend/scheduler.h>
#define VRAM_BEG 0x8000
#define VRAM_END 0x9FFF
#define ERAM_BEG 0xA000
#define ERAM_END 0xBFFF
#define OAM_BEG 0xFE00
#define OAM_END 0xFE9F
#define IO_BEG 0xFF00
#define IO_END 0xFF7F
#define IE 0xFFFF

typedef struct {
    struct {
        struct {
            uint8_t vram[KB(8)];
            uint8_t eram[KB(8)];
            uint8_t oam[OAM_END + 1 - OAM_BEG];
            struct {
                uint8_t r_if, r_ie;
                uint8_t div, tima, tma, tac;
                uint8_t data[IO_END + 1 - IO_BEG];
            } io;
        } slowmem;
        uint8_t fastmem[UINT16_MAX];
    } mmap;
    char boot_rom_path[PATH_MAX];
    char rom_path[PATH_MAX];
    uint8_t boot_rom[256];
    uint8_t unmapped_rom[256];
    Scheduler *sched;
} Memory;

uint8_t memRead(Memory *mem, uint16_t adr);
void memWrite(Memory *mem, uint16_t adr, uint8_t val);

void loadROM(Memory *mem, const char *path);
void setBootROM(Memory *mem, const char *path);
void unmountBootROM(Memory *mem);