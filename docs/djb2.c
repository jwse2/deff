#include <stdint.h>

// djb2 (bernstein)
uint32_t hash(char *str)
{
    char cur;
    uint32_t hash = 0;

    while (cur = *str++)
        hash = (hash * 33) ^ cur;

    return hash;
}
