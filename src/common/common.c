#include "common/common.h"

uint8_t calculate_fourth_mean(uint8_t parent_mean, uint8_t error, uint8_t m1, uint8_t m2, uint8_t m3)
{
    int32_t sum = m1 + m2 + m3;
    int32_t m4 = (4 * parent_mean + error) - sum;

    return (uint8_t)m4;
}