#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
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

uint16_t keypad;
uint16_t prev_keypad;

uint8_t memory[4096];
uint8_t V[16];
uint16_t I;
uint16_t pc;
uint16_t stack[16];
uint8_t sp;
uint8_t delay_timer, sound_timer;

uint64_t last_time, cur_time;

struct termios orig_termios;

void p_memo(uint16_t address) {
    printf("First 4 bytes: %02X%02X %02X%02X\n", memory[address], memory[address+1], memory[address+2], memory[address+3]);
}

uint64_t get_time_ms() {
    struct  timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000 + (ts.tv_nsec / 1000000);
}

void disableRawMode() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

void enableRawMode() {
    tcgetattr(STDIN_FILENO, &orig_termios);
    atexit(disableRawMode);

    struct termios raw = orig_termios;

    raw.c_lflag &= ~(ICANON | ECHO); 
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

void setNonBlocking() {
    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
}

void pollInput(uint16_t *keypad)
{
    char c;

    while (read(STDIN_FILENO, &c, 1) > 0)
    {
        switch (c)
        {
            case '1': *keypad |= (1 << 0x1); break;
            case '2': *keypad |= (1 << 0x2); break;
            case '3': *keypad |= (1 << 0x3); break;
            case '4': *keypad |= (1 << 0xC); break;

            case 'q': *keypad |= (1 << 0x4); break;
            case 'w': *keypad |= (1 << 0x5); break;
            case 'e': *keypad |= (1 << 0x6); break;
            case 'r': *keypad |= (1 << 0xD); break;

            case 'a': *keypad |= (1 << 0x7); break;
            case 's': *keypad |= (1 << 0x8); break;
            case 'd': *keypad |= (1 << 0x9); break;
            case 'f': *keypad |= (1 << 0xE); break;

            case 'z': *keypad |= (1 << 0xA); break;
            case 'x': *keypad |= (1 << 0x0); break;
            case 'c': *keypad |= (1 << 0xB); break;
            case 'v': *keypad |= (1 << 0xF); break;
        }
    }
}

int main(int argc, char *argv[]) {
    srand(time(NULL));

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
    init_console();
    enableRawMode();
    setNonBlocking();
    sp = 0;
    last_time = get_time_ms();
    
    while(true) {
        cur_time = get_time_ms();
        if (cur_time - last_time >= 16) {
            last_time = cur_time;
            if (delay_timer > 0) delay_timer--;
            if (sound_timer > 0) sound_timer--;
        }
        prev_keypad = keypad;
        keypad = 0;
        pollInput(&keypad);
        uint16_t opcode = (memory[pc] << 8) | memory[pc + 1];
        pc += 2;
        
        uint8_t type = (opcode & 0xF000) >> 12;
        uint8_t X = (opcode & 0x0F00) >> 8;
        uint8_t Y = (opcode & 0x00F0) >> 4;
        uint8_t N = (opcode & 0x000F);
        uint8_t NN = (opcode & 0x00FF);
        uint16_t NNN = (opcode & 0x0FFF);
        
        uint16_t result;
        
        switch (type) {
            case 0x0:
                if (opcode == 0x00E0) {
                    init_display();
                    draw_display();
                } else if (opcode == 0x00EE) {
                    sp -= 1;
                    pc = stack[sp];
                }
                break;
            case 0x1:
                pc = NNN;
                break;
            case 0x2:
                stack[sp] = pc;
                sp++;
                pc = NNN;
                break;
            case 0x3:
                if (V[X] == NN) 
                    pc += 2;
                break;
            case 0x4:
                if (V[X] != NN)
                    pc += 2;
                break;
            case 0x5:
                if (V[X] == V[Y])
                    pc += 2;
                break;
            case 0x9:
                if (V[X] != V[Y])
                    pc += 2;
                break;
            case 0x6:
                V[X] = NN;
                break;
            case 0x7:
                V[X] += NN;
                break;
            case 0x8:
                switch (N)
                {
                case 0:
                    V[X] = V[Y];
                    break;
                case 1:
                    V[X] = V[X] | V[Y];
                    break;
                case 2:
                    V[X] = V[X] & V[Y];
                    break;
                case 3:
                    V[X] = V[X] ^ V[Y];
                    break;
                case 4:
                    result = V[X] + V[Y];
                    V[0xF] = result > 0xFF;
                    V[X] = result & 0xFF;
                    break;
                case 5:
                    V[0xF] = (V[X] >= V[Y]);
                    V[X] -= V[Y];
                    break;
                case 7:
                    V[0xF] = (V[Y] >= V[X]);
                    V[X] = V[Y] - V[X];
                    break;
                case 6:
                    V[0xF] = V[X] & 1;
                    V[X] = V[X] >> 1;
                    break;
                case 0xE:
                    V[0xF] = (V[X] >> 7) & 1;
                    V[X] = V[X] << 1;
                    break;
                default:
                    printf("Unknown instruction %2X\n", opcode);
                    break;
                }
                break;
            case 0xA:
                I = NNN;
                break;
            case 0xB:
                pc = NNN + V[0x0];
                break;
            case 0xC:
                uint8_t random_num;
                random_num = rand() % 256;
                V[X] = random_num & NN;
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
            case 0xE:
                if (NN == 0x9E) {
                    if (keypad & (1 << V[X]))
                        pc += 2;
                } else if (NN == 0xA1) {
                    if (!(keypad & (1 << V[X])))
                        pc += 2;
                }
                break;
            case 0xF:
                switch (NN)
                {
                case 0x07:
                    V[X] = delay_timer;
                    break;
                case 0x15:
                    delay_timer = V[X];
                    break;
                case 0x18:
                    sound_timer = V[X];
                    break;
                case 0x1E:
                    I += V[X];
                    break;
                case 0x0A:
                    uint16_t new_keys = keypad & ~prev_keypad;
                    if (!new_keys) {
                        pc -= 2;
                    } else {
                        V[X] = __builtin_ctz(new_keys);
                    }
                    break;
                case 0x29:
                    I = FONTSET_STA_ADDRESS + V[X] * 5;
                    break;
                case 0x33:
                    memory[I] = V[X] / 100;
                    memory[I+1] = (V[X] / 10) % 10;
                    memory[I+2] = V[X] % 10;
                    break;
                case 0x55:
                    for (int i=0; i<=X; i++) {
                        memory[I+i] = V[i];
                    }
                    break;
                case 0x65:
                    for (int i=0; i<=X; i++) {
                        V[i] = memory[I+i];
                    }
                    break;
                default:
                    printf("Unknown instruction %2X\n", opcode);
                    break;
                }
                break;
            default:
                printf("Unknown instruction %02X\n", opcode);
                break;
        }
        sleep_ms(2000);
    }
    restore_console();
}
