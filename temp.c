void debugHandler(){
    // --- FIXED T DUMP HANDLER ---
        (*(volatile uint8_t*)0xDC00) = 0xFB; 
        if (!((*(volatile uint8_t*)0xDC01) & 0x40)) { 
            __asm__("sei"); 
            (*(volatile uint8_t*)0xd011) = 0x1B; (*(volatile uint8_t*)0xd018) = 0x14; 
            (*(volatile uint8_t*)0xd020) = 1; (*(volatile uint8_t*)0xd021) = 0;    
            for(uint16_t i = 0; i < 1000; i++) { SCREEN_RAM[i] = 0x20; COLOR_RAM[i] = 0x01; }
            
            // Show Forward Path at the top
            for(uint8_t i = 0; i < npc[1].path_len; i++) {
                uint8_t m = npc[1].path_actions[i];
                if (m == 0) SCREEN_RAM[i] = 21; else if (m == 1) SCREEN_RAM[i] = 4;
                else if (m == 2) SCREEN_RAM[i] = 12; else if (m == 3) SCREEN_RAM[i] = 18;
            }
            // Show Backward Path slightly below
            for(uint8_t i = 0; i < npc[1].back_len; i++) {
                uint8_t m = npc[1].path_back[i];
                if (m == 0) SCREEN_RAM[80+i] = 21; else if (m == 1) SCREEN_RAM[80+i] = 4;
                else if (m == 2) SCREEN_RAM[80+i] = 12; else if (m == 3) SCREEN_RAM[80+i] = 18;
            }
            while(1); 
        }
}