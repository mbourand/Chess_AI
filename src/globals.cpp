#include "globals.h"

uint64_t invert_bits(uint64_t bits)
{
    uint64_t result = 0;
    for (int8_t i = 0; i < 64; i++)
    {
        if (bits & (1ULL << i))
            result |= 1ULL << (63 - i);
    }
    return result;
}

uint64_t invert_bytes(uint64_t n)
{
    n = ((n << 8) & 0xFF00FF00FF00FF00ULL) | ((n >> 8) & 0x00FF00FF00FF00FFULL);
    n = ((n << 16) & 0xFFFF0000FFFF0000ULL) | ((n >> 16) & 0x0000FFFF0000FFFFULL);
    return (n << 32) | (n >> 32);
}
