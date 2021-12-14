#pragma once
#include <assert.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#define PANIC                                                                  \
    do {                                                                       \
        assert(false);                                                         \
        exit(-1);                                                              \
    } while (0)
#define KB(v) (v * 1024)
#define MB(v) (KB(v) * 1024)
#define BIT(bit) (1 << (bit))