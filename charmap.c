// show_chars.c
#include <stdint.h>
#include <string.h>
#include "charset_data.h"

#define SCREEN_RAM      ((uint8_t*)0x0400)
#define COLOR_RAM       ((uint8_t*)0xD800)
#define CHARSET_DEST    ((uint8_t*)0x3000)

int main(void) {  // Changed to int
    // Copy charset to $3000
    memcpy(CHARSET_DEST, settlers_charset, 2048);
    
    // Set VIC to use charset at $3000
    *((uint8_t*)0xd018) = 0x1C;
    
    // Set colors
    *((uint8_t*)0xd020) = 0;  // Border black
    *((uint8_t*)0xd021) = 0;  // Background black
    
    // Display all 256 characters in a grid
    // First 6 rows (40x6 = 240 chars) plus part of 7th row
    for (uint16_t i = 0; i < 256; i++) {
        uint8_t row = i / 40;
        uint8_t col = i % 40;
        uint16_t offset = (row * 40) + col;
        
        SCREEN_RAM[offset] = (uint8_t)i;
        COLOR_RAM[offset] = 1;  // White
    }
    
    // Add labels for hex positions every 16 chars
    for (uint8_t i = 0; i < 16; i++) {
        // Put hex labels in the last row
        uint16_t offset = (24 * 40) + (i * 2);
        uint8_t high = i / 16;
        uint8_t low = i % 16;
        
        // This is crude - you'd need actual hex digits in your charset
        // But for now, just see the raw characters
    }
    
    while(1);
    return 0;
}