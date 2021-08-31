#pragma once
#include<limits.h>
#include<stdint.h>
#include<stddef.h>

typedef struct{
    uint8_t mmap[UINT16_MAX];
} Memory;

uint8_t read(Memory* mem, uint16_t adr);
void write(Memory* mem, uint16_t adr, uint8_t val);