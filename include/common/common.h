/**
 * @file common.h
 * @brief Common utilities for quadtree operations
 */

#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>

/**
 * @brief How we order quadrants when processing the tree
 */
typedef enum
{
    QUADRANT_TOP_LEFT = 0,     /* Start with top left */
    QUADRANT_TOP_RIGHT = 1,    /* Then top right */
    QUADRANT_BOTTOM_RIGHT = 2, /* Then bottom right */
    QUADRANT_BOTTOM_LEFT = 3   /* Finish with bottom left */
} quadrant_order_t;

/* Array to make traversal easier */
static const int quadrant_order[4] = {
    QUADRANT_TOP_LEFT,
    QUADRANT_TOP_RIGHT,
    QUADRANT_BOTTOM_RIGHT,
    QUADRANT_BOTTOM_LEFT};

/**
 * @brief Figures out the last mean value we need
 * @param parent_mean Mean of the parent node
 * @param error Rounding error stored
 * @param m1 First mean
 * @param m2 Second mean
 * @param m3 Third mean
 * @return The fourth mean value
 */
uint8_t calculate_fourth_mean(uint8_t parent_mean, uint8_t error,
                              uint8_t m1, uint8_t m2, uint8_t m3);

#endif /* COMMON_H */