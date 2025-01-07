#include "utils/utils.h"

bool is_power_of_two(uint32_t x)
{
    return x && !(x & (x - 1));
}