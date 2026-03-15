#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "display.h"

#define FONTSET_STA_ADDRESS 0x50
#define PROGRAM_STA_ADDRESS 0x200

const uint8_t FONTSET[] = {
    0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
    0x20, 0x60, 0x20, 0x20, 0x70, // 1
    0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
    0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
    0x90, 0x90, 0xF0, 0x10, 0x10, // 4
    0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
    0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
    0xF0, 0x10, 0x20, 0x40, 0x40, // 7
    0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
    0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
    0xF0, 0x90, 0xF0, 0x90, 0x90, // A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
    0xF0, 0x80, 0x80, 0x80, 0xF0, // C
    0xE0, 0x90, 0x90, 0x90, 0xE0, // D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
    0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

uint8_t memory[4096];
uint8_t V[16];
uint16_t I;
uint16_t pc;
uint16_t stack[16];
uint8_t sp;
uint8_t delay_timer, sound_timer;

void p_memo(uint16_t address) {
    printf("First 4 bytes: %02X%02X %02X%02X\n", memory[address], memory[address+1], memory[address+2], memory[address+3]);
}


int main(int argc, char *argv[]) {
    memset(memory, 0, sizeof(memory));
    memcpy(&memory[FONTSET_STA_ADDRESS], FONTSET, sizeof(FONTSET));
    char *rom_path;
    FILE *fp;
    
    if (argc != 2) {
        fprintf(stderr, "Error: Need a chip-8 ROM\n");
        printf("Usage: chip8 rom.ch8\n");
        return -1;
    } else {
        rom_path = *++argv;
    }
    
    if (NULL == (fp = fopen(rom_path, "rb"))) {
        fprintf(stderr, "Error: Could not open ROM\n");
        return -1;
    }
    
    fseek(fp, 0, SEEK_END);
    long rom_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    fread(&memory[PROGRAM_STA_ADDRESS], 1, rom_size, fp);
    fclose(fp);
    pc = PROGRAM_STA_ADDRESS;
    
    init_display();
    
    while(true) {
        uint16_t opcode = (memory[pc] << 8) | memory[pc + 1];
        pc += 2;
        
        uint8_t type = (opcode & 0xF000) >> 12;
        uint8_t X = (opcode & 0x0F00) >> 8;
        uint8_t Y = (opcode & 0x00F0) >> 4;
        uint8_t N = (opcode & 0x000F);
        uint8_t NN = (opcode & 0x00FF);
        uint16_t NNN = (opcode & 0x0FFF);
        
        switch (type) {
            case 0x0:
                if (opcode == 0x00E0) {
                    init_display();
                    draw_display();
                }
                break;
            case 0x1:
                pc = NNN;
                break;
            case 0x6:
                V[X] = NN;
                break;
            case 0x7:
                V[X] += NN;
                break;
            case 0xA:
                I = NNN;
                break;
            case 0xD: {
                uint8_t x_coor = V[X] & 0x3F;
                uint8_t y_coor = V[Y] & 0x1F;
                V[0xF] = 0;
                for (int i = 0; i < N; i++) {
                    uint8_t cur_y = y_coor + i;
                    if (cur_y >= 32) {
                        break;
                    }
                    uint8_t sprite_byte = memory[I + i];
                    for (int j = 0; j < 8; j++) {
                        uint8_t cur_x = x_coor + j;
                        if (cur_x >= 64) {
                            break;
                        }
                        if ((sprite_byte & (0x80 >> j)) != 0) {
                            if (gfx[cur_y][cur_x] == 1) 
                                V[0xF] = 1;
                            gfx[cur_y][cur_x] ^= 1;
                        }
                    }
                }
                draw_display();
                break;
            }
        }
        sleep_ms(2000);
    }
}
