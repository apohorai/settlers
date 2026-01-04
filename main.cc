#include <stdint.h>
#include <string.h>
#include "sprite.h"
#include "charset_data.h"
#include "map_data.h"

// Hardware Registers
#define CIA1_PRA        (*(volatile uint8_t*)0xDC00)
#define CIA1_PRB        (*(volatile uint8_t*)0xDC01)
#define VIC_MEM_CONTROL (*(volatile uint8_t*)0xd018)
#define BORDER_COLOR    (*(volatile uint8_t*)0xd020)
#define BG_COLOR        (*(volatile uint8_t*)0xd021)

// Memory
#define SCREEN_RAM      ((uint8_t*)0x0400)
#define COLOR_RAM       ((uint8_t*)0xD800)
#define CHARSET_DEST    ((uint8_t*)0x3000)

void draw_map() {
    memcpy((void*)SCREEN_RAM, settlers_map, 1000);
    memset((void*)COLOR_RAM, 1, 1000); // Make all tiles white
}

void init_system() {
    memcpy(CHARSET_DEST, settlers_charset, 2048);
    VIC_MEM_CONTROL = 0x1C; // Screen $0400, Charset $3000
    BORDER_COLOR = 0;
    BG_COLOR = 6;
}

void readKeys() {
    for (uint8_t row = 0; row < 8; row++) {
        CIA1_PRA = (uint8_t)~(1 << row);
        uint8_t cols = CIA1_PRB;

        if (row == 1) {
            if (cols == 0xFD) moveSprite(0, -1); // W
            if (cols == 0xFB) moveSprite(-1, 0); // A
            if (cols == 0xDF) moveSprite(0, 1);  // S
            if (cols == 0xF9) moveSprite(-1, -1); 
        } else if (row == 2) {
            if (cols == 0xFB) moveSprite(1, 0);  // D
        }
    }
}

void delay(uint16_t cycles) {
    while(cycles--) { __asm__ volatile ("nop"); }
}

int main(void) {
    init_system();
    draw_map();
    initSprite0();

    while (1) {
        readKeys();
        delay(100);
    }
    return 0;
}
