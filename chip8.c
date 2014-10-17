#include <stdio.h>
#include <stdlib.h>
#include "SDL/SDL.h"
#include "SDL/SDL_keysym.h"
#include "chip8.h"
#include "time.h"

void load_rom(FILE* fp, int start) {
    int c, i;
    for (; (c = fgetc(fp)) != EOF; i++) {
        memory[i + start] = c;
    }
}

void beep() {
    if (sound)
        printf("BEEP!\n");
}

main(int argc, char* argv[]) {
    /* Parse command line arguments */
    if (argc != 2) {
        fprintf(stderr, "usage: chip8 ROM\n");
        exit(-1);
    }
    
    /* Load ROM */
    FILE* fp = fopen(argv[1], "rb");
    if (!fp) {
        fprintf(stderr, "error: failed to open file: %s\n", argv[1]);
        exit(-1);
    }
    load_rom(fp, 0x200);
    fclose(fp);
    
    /* Load hex font */
    for (temp=0; temp < 80; temp++) memory[temp] = chip8_font[temp];
    
    /* Init SDL */
    int SX, SY;
    SX = SCALE * WIDTH;
    SY = SCALE * HEIGHT;
    
    if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
        fprintf(stderr, "Could not initialize SDL: %s\n", SDL_GetError());
        exit(-1);
    }
    SDL_Surface* screen;
    SDL_Rect bounds;
    if (!(screen = SDL_SetVideoMode(SX, SY, 0, SDL_SWSURFACE))) {
        fprintf(stderr, "Could not set video mode: %s\n", SDL_GetError());
        exit(-1);
    }
    SDL_Event event;
    SDL_WM_SetCaption("CHIP8 emulator - running", 0);
    SDL_EnableUNICODE(1);
    
    /* Setup some variables */
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
                    if (strlen(key) > 1) break;
                    chr = key[0];
                    switch(chr) {
                        case KEY_1:
                            keys[0] = 1;
                            break;
                        case KEY_2:
                            keys[1] = 1;
                            break;
                        case KEY_3:
                            keys[2] = 1;
                            break;
                        case KEY_C:
                            keys[3] = 1;
                            break;
                            
                        case KEY_4:
                            keys[4] = 1;
                            break;
                        case KEY_5:
                            keys[5] = 1;
                            break;
                        case KEY_6:
                            keys[6] = 1;
                            break;
                        case KEY_D:
                            keys[7] = 1;
                            break;
                            
                        case KEY_7:
                            keys[8] = 1;
                            break;
                        case KEY_8:
                            keys[9] = 1;
                            break;
                        case KEY_9:
                            keys[10] = 1;
                            break;
                        case KEY_E:
                            keys[11] = 1;
                            break;
                            
                        case KEY_A:
                            keys[12] = 1;
                            break;
                        case KEY_0:
                            keys[13] = 1;
                            break;
                        case KEY_B:
                            keys[14] = 1;
                            break;
                        case KEY_F:
                            keys[15] = 1;
                            break;
                        
                        case KEY_NEXT_OPCODE:
                            next_opcode = 1;
                            SDL_Delay(250);
                    }
                    break;
                case (SDL_KEYUP):
                    key = SDL_GetKeyName(event.key.keysym.sym);
                    if (strlen(key) > 1) break;
                    chr = key[0];
                    switch(chr) {
                        case KEY_1:
                            keys[0] = 0;
                            break;
                        case KEY_2:
                            keys[1] = 0;
                            break;
                        case KEY_3:
                            keys[2] = 0;
                            break;
                        case KEY_C:
                            keys[3] = 0;
                            break;
                            
                        case KEY_4:
                            keys[4] = 0;
                            break;
                        case KEY_5:
                            keys[5] = 0;
                            break;
                        case KEY_6:
                            keys[6] = 0;
                            break;
                        case KEY_D:
                            keys[7] = 0;
                            break;
                            
                        case KEY_7:
                            keys[8] = 0;
                            break;
                        case KEY_8:
                            keys[9] = 0;
                            break;
                        case KEY_9:
                            keys[10] = 0;
                            break;
                        case KEY_E:
                            keys[11] = 0;
                            break;
                            
                        case KEY_A:
                            keys[12] = 0;
                            break;
                        case KEY_0:
                            keys[13] = 0;
                            break;
                        case KEY_B:
                            keys[14] = 0;
                            break;
                        case KEY_F:
                            keys[15] = 0;
                            break;
                        case KEY_PAUSE:
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
                        case KEY_NEXT_OPCODE:
                            if (paused) {
                                next_opcode = 0;
                                SDL_Delay(2);
                            }
                            break;
                        case KEY_DEBUG:
                            // Debug
                            debug = !debug;
                            if (debug)
                                printf("Debugging: ON\n");
                            else
                                printf("Debugging: OFF\n");
                            break;
                        case KEY_INFO:
                            // Print info
                            for (temp=0; temp<16; temp++) {
                                printf("V%-2d ", temp);
                            }
                            printf("\n");
                            for (temp=0; temp<16; temp++) {
                                printf("%-3d ", V[temp]);
                            }
                            printf("\n");
                            printf("I: %d\n", I);
                            printf("pc: %d\n", pc);
                            printf("stack pointer: %d\n", sp);
                            printf("Stimer: %d\n", ST);
                            printf("Dtimer: %d\n", DT);
                            break;
                        case KEY_HALT:
                            // Halt
                            running = 0;
                            break;
                    }
                    break;
            }
            if (!running) break;
        }
        if (paused && next_opcode) cinst = 1;
        for (ii = 0; ii < cinst; ii++) {
        if (paused) cinst = 0;
        // It looks better without identation
        if (pc >= 4096) {
            break;
        }
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
                srand(time(NULL));
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
                        if (color && ccolor) V[0xF] = 1;
                        color ^= ccolor;
                        gfx[py][px] = color;
                        bounds.x = DX;
                        bounds.y = DY;
                        bounds.w = SCALE;
                        bounds.h = SCALE;
                        if (color)
                            SDL_FillRect(screen,
                                         &bounds,
                                         SDL_MapRGB(screen->format,
                                                    COLOR_ON));
                        else
                            SDL_FillRect(screen,
                                         &bounds,
                                         SDL_MapRGB(screen->format,
                                                    COLOR_OFF));
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
        SDL_Delay(15);
    }
    SDL_Quit();
}
