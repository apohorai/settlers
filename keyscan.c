#include <stdint.h>

// CIA #1
#define CIA1_PRA (*(volatile uint8_t*)0xDC00)
#define CIA1_PRB (*(volatile uint8_t*)0xDC01)

// Screen
#define SCREEN ((volatile uint8_t*)0x0400)

// Convert nybble to hex char
static uint8_t hex(uint8_t v)
{
    v &= 0x0F;
    return (v < 10) ? ('0' + v) : ('A' + v - 10);
}

int main(void)
{
    // Clear screen
    for (uint16_t i = 0; i < 1000; i++)
        SCREEN[i] = ' ';

    while (1)
    {
        for (uint8_t row = 0; row < 8; row++)
        {
            // Select exactly ONE row (active low)
            CIA1_PRA = (uint8_t)~(1 << row);

            // Small settle delay
            __asm__("nop");

            // Read columns
            uint8_t cols = CIA1_PRB;

            // Display: "Rn: xx"
            uint16_t pos = row * 40;

            SCREEN[pos + 0] = 'R';
            SCREEN[pos + 1] = '0' + row;
            SCREEN[pos + 2] = ':';
            SCREEN[pos + 3] = ' ';

            SCREEN[pos + 4] = hex(cols >> 4);
            SCREEN[pos + 5] = hex(cols);
        }
    }
}
