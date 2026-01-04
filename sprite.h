#ifndef SPRITE_H
#define SPRITE_H

#include <stdint.h>

// VIC-II Sprite Register Definitions
#define VIC_BASE        0xd000
#define SPRITE_0_X      (*(volatile uint8_t*)(VIC_BASE + 0x00))
#define SPRITE_0_Y      (*(volatile uint8_t*)(VIC_BASE + 0x01))
#define SPRITE_MSB_X    (*(volatile uint8_t*)(VIC_BASE + 0x10))
#define SPRITE_ENABLE   (*(volatile uint8_t*)(VIC_BASE + 0x15))
#define SPRITE_COLOR_0  (*(volatile uint8_t*)(VIC_BASE + 0x27))
#define SPRITE_POINTERS ((volatile uint8_t*)0x07f8)

// Function prototypes
void initSprite0();
void moveSprite(int8_t dx, int8_t dy);

#endif
