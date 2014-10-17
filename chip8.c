#include <stdio.h>
#include <stdlib.h>
#include "SDL/SDL.h"
#include "SDL/SDL_keysym.h"
#include "SDL/SDL_gfxPrimitives.h"
#define dopcode memory[pc] << 8 | memory[pc + 1]
#define X       (opcode & 0x0F00) >> 8
#define Y       (opcode & 0x00F0) >> 4
#define DX      (px * SCALE)
#define DY      (py * SCALE)
#define R       (rand() % 256)
#define N       (opcode & 0x000F)
#define KK      (opcode & 0x00FF)
#define NNN     (opcode & 0x0FFF)
#define SCALE   5
#define WIDTH   64
#define HEIGHT  32
#define INST_PER_CYCLE 5

unsigned char memory[4096];
char gfx[32][64];
int pc = 0x200;
unsigned int I = 0;
unsigned char V[16];
int opcode, inst, kk, x, y, px, py, i, ii, temp, cinst;
char color, ccolor, running, paused, debug, space_pressed;
unsigned char sp;
int DT, ST;
int stack[16];
int keys[16];

void load_rom(FILE* fp, int start) {
    int c, i;
    for (; (c = fgetc(fp)) != EOF; i++) {
        memory[i + start] = c;
    }
}

void beep() {
    printf("BEEP!\n");
}

main(int argc, char* argv[]) {
    int SX, SY;
    SX = SCALE * WIDTH;
    SY = SCALE * HEIGHT;
    
    if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
        fprintf(stderr, "Could not initialize SDL: %s\n", SDL_GetError());
        exit(-1);
    }
    SDL_Surface* screen;
    if (!(screen = SDL_SetVideoMode(SX, SY, 0, SDL_SWSURFACE))) {
        fprintf(stderr, "Could not set video mode: %s\n", SDL_GetError());
        exit(-1);
    }
    SDL_Event event;
    SDL_WM_SetCaption("CHIP8 emulator - running", 0);
    SDL_EnableUNICODE(1);
    
    FILE* fp = fopen(argv[1], "rb");
    load_rom(fp, 0x200);
    fclose(fp);
    
    fp = fopen("FONTS.ch8", "rb");
    load_rom(fp, 0);
    fclose(fp);
    
    running = 1;
    char* keyname;
    char chr;
    char* key;
    cinst = INST_PER_CYCLE;
    for (;;) {
        if (SDL_PollEvent(&event)) {
            switch (event.type) {
                case (SDL_QUIT):
                    running = 0;
                    break;
                case (SDL_KEYDOWN):
                    key = SDL_GetKeyName(event.key.keysym.sym);
                    if (strlen(key) > 1) {
                        if (!strcmp(key, "space")) {
                            // Next instruction
                            if (paused) {
                                space_pressed = 1;
                                SDL_Delay(250); /* Incase you want to 
                                    execute just one opcode */
                            }
                        }
                        break;
                    }
                    chr = key[0];
                    switch(chr) {
                        case '1':
                        case '2':
                        case '3':
                        case '4':
                            keys[chr - '0' - 1] = 1;
                            break;
                            
                        case 'q':
                            keys[4] = 1;
                            break;
                        case 'w':
                            keys[5] = 1;
                            break;
                        case 'e':
                            keys[6] = 1;
                            break;
                        case 'r':
                            keys[7] = 1;
                            break;
                            
                        case 'a':
                            keys[8] = 1;
                            break;
                        case 's':
                            keys[9] = 1;
                            break;
                        case 'd':
                            keys[10] = 1;
                            break;
                        case 'f':
                            keys[11] = 1;
                            break;
                            
                        case 'z':
                            keys[12] = 1;
                            break;
                        case 'x':
                            keys[13] = 1;
                            break;
                        case 'c':
                            keys[14] = 1;
                            break;
                        case 'v':
                            keys[15] = 1;
                            break;
                    }
                    break;
                case (SDL_KEYUP):
                    key = SDL_GetKeyName(event.key.keysym.sym);
                    if (strlen(key) > 1) {
                        if (!strcmp(key, "space")) {
                            // Next instruction
                            if (paused) {
                                space_pressed = 0;
                                SDL_Delay(2);
                            }
                        }
                        break;
                    }
                    chr = key[0];
                    switch(chr) {
                        case '1':
                        case '2':
                        case '3':
                        case '4':
                            keys[chr - '0' - 1] = 0;
                            break;
                            
                        case 'q':
                            keys[4] = 0;
                        case 'w':
                            keys[5] = 0;
                            break;
                        case 'e':
                            keys[6] = 0;
                            break;
                        case 'r':
                            keys[7] = 0;
                            break;
                            
                        case 'a':
                            keys[8] = 0;
                            break;
                        case 's':
                            keys[9] = 0;
                            break;
                        case 'd':
                            keys[10] = 0;
                            break;
                        case 'f':
                            keys[11] = 0;
                            break;
                            
                        case 'z':
                            keys[12] = 0;
                            break;
                        case 'x':
                            keys[13] = 0;
                            break;
                        case 'c':
                            keys[14] = 0;
                            break;
                        case 'v':
                            keys[15] = 0;
                            break;
                        case 'p':
                            // Pause
                            paused = !paused;
                            if (paused) {
                                SDL_WM_SetCaption(
                                    "CHIP8 emulator - paused", 0);
                                cinst = 0;
                            }
                            else {
                                SDL_WM_SetCaption(
                                    "CHIP8 emulator - running", 0);
                                cinst = INST_PER_CYCLE;
                            }
                            break;
                        case 'b':
                            // Debug
                            debug = !debug;
                            break;
                        case 'h':
                            // Halt
                            running = 0;
                            break;
                    }
                    break;
            }
            if (!running) break;
        }
        if (paused && space_pressed) cinst = 1;
        for (ii = 0; ii < cinst; ii++) {
        if (paused) cinst = 0;
        // It looks better without identation
        if (pc >= 4096) break;
        opcode = dopcode;
        inst   = opcode & 0xF000;
        if (opcode != 0x0 && debug) {
            printf("0x%X (%d): 0x%X\n", pc, pc, opcode);
        }
        pc += 2;
        switch(inst) {
            case 0x0000:
                switch (KK) {
                    case 0x00E0:
                        for (y=0; y<32; y++) {
                            for (x=0; x<64; x++) {
                                gfx[y][x] = 0;
                            }
                        }
                        boxRGBA(screen, 0, 0, SX, SY, 0, 0, 0, 255);
                        SDL_Flip(screen);
                        break;
                    case 0x00EE:
                        pc = stack[sp--];
                        break;
                    default:
                        // 0NNN - ignored
                        break;
                }
                break;
            case 0x1000:
                pc = NNN;
                break;
            case 0x2000:
                stack[++sp] = pc;
                pc = NNN;
                break;
            case 0x3000:
                if (V[X] == KK)
                    pc += 2;
                break;
            case 0x4000:
                if (V[X] != KK)
                    pc += 2;
                break;
            case 0x5000:
                if (V[X] == V[Y])
                    pc += 2;
                break;
            case 0x6000:
                V[X] = KK;
                break;
            case 0x7000:
                V[X] += KK;
                break;
            case 0x8000:
                switch (opcode & 0x000F) {
                    case 0x0000:
                        V[X] = V[Y];
                        break;
                    case 0x0001:
                        V[X] |= V[Y];
                        break;
                    case 0x0002:
                        V[X] &= V[Y];
                        break;
                    case 0x0003:
                        V[X] ^= V[Y];
                        break;
                    case 0x0004:
                        temp = V[X] + V[Y];
                        V[0xF] = (temp > 0xFF);
                        V[X] = temp;
                        break;
                    case 0x0005:
                        temp = V[X] - V[Y];
                        V[0xF] = (V[X] > V[Y]);
                        V[X] = temp;
                        break;
                    case 0x0006:
                        V[0xF] = V[X] & 1;
                        V[X] >>= 1;
                        break;
                    case 0x0007:
                        temp = V[Y] - V[X];
                        V[0xF] = (V[Y] > V[X]);
                        V[X] = temp;
                        break;
                    case 0x000E:
                        V[0xF] = (V[X] & 128) >> 7;
                        V[X] <<= 1;
                        break;
                    default:
                        printf("Unknown opcode: 0x%X\n", opcode);
                        break;
                }
                break;
            case 0x9000:
                if (V[X] != V[Y])
                    pc += 2;
                break;
            case 0xA000:
                I = NNN;
                break;
            case 0xBa000:
                pc = NNN + V[0x0];
                break;
            case 0xC000:
                V[X] = R & KK;
                break;
            case 0xD000:
                // Das drawing!
                V[0xF] = 0;
                px = py = 0;
                for (y=0; y < N; y++) {
                    py = ((V[Y] + y) % 32);
                    for (x=0; x < 8; x++) {
                        px = ((V[X] + x) % 64);
                        color = (memory[I + y] >> (7 - x)) & 1;
                        ccolor = gfx[py][px];
                        if (color == ccolor == 1)
                            V[0xF] = 1;
                        color ^= ccolor;
                        gfx[py][px] = color;
                        if (color) {
                            boxRGBA(screen,
                                    DX,
                                    DY,
                                    DX + SCALE,
                                    DY + SCALE,
                                    0,
                                    255,
                                    0,
                                    255);
                        }
                        else {
                            boxRGBA(screen,
                                    DX,
                                    DY,
                                    DX + SCALE,
                                    DY + SCALE,
                                    0,
                                    0,
                                    0,
                                    255);
                        }
                    }
                }
                SDL_Flip(screen);
                break;
            case 0xE000:
                switch (KK) {
                    case 0x009E:
                        if (keys[V[X]] == 1)
                            pc += 2;
                        break;
                    case 0x00A1:
                        if (keys[V[X]] == 0)
                            pc += 2;
                        break;
                }
                break;
            case 0xF000:
                switch (KK) {
                    case 0x0007:
                        V[X] = DT;
                        break;
                    case 0x000A:
                        temp = -1;
                        for (i=0; i<16; i++) {
                            if (keys[i]) {
                                temp = i;
                                break;
                            }
                        }
                        if (temp != -1)
                            V[X] = temp;
                        else
                            pc -= 2;
                        break;
                    case 0x0015:
                        DT = V[X];
                        break;
                    case 0x0018:
                        ST = V[X];
                        break;
                    case 0x001E:
                        I += V[X];
                        break;
                    case 0x0029:
                        I = V[X] * 5;
                        break;
                    case 0x0033:
                        memory[I] = V[X] / 100;
                        memory[I + 1] = (V[X] / 10) % 10;
                        memory[I + 2] = V[X] % 10;
                        break;
                    case 0x0055:
                        for (i=0; i<=X; i++)
                            memory[I + i] = V[i];
                        break;
                    case 0x0065:
                        for (i=0; i<=X; i++)
                            V[i] = memory[I + i];
                        break;
                    default:
                        printf("Unknown opcode: 0x%X\n", opcode);
                        break;
                }
                break;
            default:
                printf("Unknown opcode: 0x%X\n", opcode);
                break;
        }
        if (DT > 0) {
            DT--;
        }
        if (ST > 0) {
            ST--;
            if (!ST) beep();
        }
        }
        SDL_Delay(10);
    }
    SDL_Quit();
}
