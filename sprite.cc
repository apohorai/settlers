#include "sprite.h"
#include <string.h>

#define SPRITE_DEST ((uint8_t*)0x2000)

// 1. Define these variables at the top of the file
static uint16_t sprite_x = 160;
static uint8_t  sprite_y = 120;

const uint8_t ball_sprite[64] = {
    0x00, 0x7e, 0x00, 0x03, 0xff, 0xc0, 0x07, 0xff, 0xe0, 0x0f, 0xff, 0xf0,
    0x1f, 0xff, 0xf8, 0x1f, 0xff, 0xf8, 0x3f, 0xff, 0xfc, 0x3f, 0xff, 0xfc,
    0x3f, 0xff, 0xfc, 0x3f, 0xff, 0xfc, 0x3f, 0xff, 0xfc, 0x1f, 0xff, 0xf8,
    0x1f, 0xff, 0xf8, 0x0f, 0xff, 0xf0, 0x07, 0xff, 0xe0, 0x03, 0xff, 0xc0,
    0x00, 0x7e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00 
};

void initSprite0() {
    memcpy(SPRITE_DEST, ball_sprite, 64);
    SPRITE_POINTERS[0] = 0x80;

    // 2. Use the variables to set initial hardware state
    SPRITE_0_X = (uint8_t)sprite_x; 
    SPRITE_0_Y = sprite_y;
    SPRITE_MSB_X = 0; 
    SPRITE_COLOR_0 = 1; 
    SPRITE_ENABLE = 0x01; 
}

void moveSprite(int8_t dx, int8_t dy) {
    // 1. Update X with boundaries (don't go below 0 or above 344)
    int16_t new_x = (int16_t)sprite_x + dx;
    if (new_x < 0) new_x = 0;
    if (new_x > 344) new_x = 344;
    sprite_x = (uint16_t)new_x;

    // 2. Update Y with boundaries (don't go below 0 or above 255)
    int16_t new_y = (int16_t)sprite_y + dy;
    if (new_y < 0) new_y = 0;
    if (new_y > 255) new_y = 255;
    sprite_y = (uint8_t)new_y;

    // 3. Update Hardware Registers
    SPRITE_0_X = (uint8_t)(sprite_x & 0xFF);
    SPRITE_0_Y = sprite_y;

    // Handle MSB (9th bit)
    if (sprite_x > 255) {
        SPRITE_MSB_X |= 0x01;
    } else {
        SPRITE_MSB_X &= ~0x01;
    }
}