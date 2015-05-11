#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <pthread.h>
#include <SDL2/SDL.h>


#define MEMSIZE   4096
#define MEMSTART  0x200
#define REGS      16
#define STACKSIZE 16

#define REGDUMP 0
#define MEMDUMP 1

#define FR 255
#define FG 255
#define FB 255

#define BR 0
#define BG 0
#define BB 0

#define SCREEN_WIDTH  64
#define SCREEN_HEIGHT 32

#define case_KEY(x, state)\
case KEY_##x:\
    keys[0x##x] = state;\
    break;


// Functions
double hertz(int);
bool isallnum(char *);
void draw_sprite(void);
void clear_screen(void);
void *update_timers(void *);

void sdl_init(void);
void sdl_quit(void);

void print_usage(void);
void print_version(void);
void print_help(void);
void parse_args(int, char **);
void error(char *);
void unknown(unsigned int);

void load(void);
void fetch(void);
void decode(void);
void execute(void);
void handle_events(void);
void finish(void);

int main(int, char **);

// CHIP-8 specific variables
unsigned char memory[MEMSIZE] = {0};
unsigned char reg[REGS] = {0};
unsigned char inst = 0, x = 0, y = 0, kk = 0, n = 0,
                DT = 0, ST = 0, SP = 0;
unsigned int addr = 0, opcode = 0, I = 0,
                PC = 0;
unsigned int stack[STACKSIZE] = {0};

// Interpreter specific variables
FILE *fpin = NULL;
char *fnin = NULL;
char *prog = NULL;
char *version = "1.0";
char *HELP_MSG = "\n"
"Interpreter options:\n"
"\t-h\tprint this help message and exit\n"
"\t-v\tprint version and exit\n"
"\t-r\tprint a register dump at the end\n"
"\t-m\tstore a memory dump in file `memory.bin' at the end\n\n"
"SDL options:\n"
"\t-s\tset scale factor\n"
"\t-i\tset interpreter speed (in Hz)\n";
bool DEBUG[2] = {0}; /* REGDUMP, MEMDUMP */
bool keys[16] = {0};
unsigned int temp = 0;
pthread_t updater;
bool running = true;

unsigned char font[80] = {
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

// SDL specific variables
SDL_Window *window = NULL;
SDL_Surface *surface = NULL;
SDL_Event evt;
SDL_Rect bounds = {0};
unsigned int scale = 2;
unsigned int speed = 60;
unsigned char graphics[SCREEN_HEIGHT * SCREEN_WIDTH] = {0};
typedef enum {
    KEY_0='X',

    KEY_1='1',
    KEY_2='2',
    KEY_3='3',

    KEY_4='Q',
    KEY_5='W',
    KEY_6='E',

    KEY_7='A',
    KEY_8='S',
    KEY_9='D',

    KEY_A='Z',
    KEY_B='C',
    KEY_C='4',
    KEY_D='R',
    KEY_E='F',
    KEY_F='V'
} Key;


void clear_screen(void) {
    
    bounds.x = 0;
    bounds.y = 0;
    bounds.w = SCREEN_WIDTH * scale;
    bounds.h = SCREEN_HEIGHT * scale;
    SDL_FillRect(surface,
                 &bounds,
                 SDL_MapRGB(surface->format,
                            BR, BG, BB));
    
    SDL_UpdateWindowSurface(window);
}

double hertz(int hz) {
    return (1.0/hz) * 1000;
}

bool isallnum(char *s) {
    size_t i;
    for (i=0; s[i] != '\0'; i++) {
        if (!
           ((s[i] - 48) >= 0 &&
            (s[i] - 48) <= 9))
            return false;
    }
    return true;
}

void draw_sprite(void) {
    int iy = 0, ix = 0;
    int dy = 0, dx = 0;
    unsigned char bit = 0, oldbit = 0;
    unsigned int data = 0;
    reg[0xF] = 0;
    
    for (iy = 0; iy < n; iy++) {
        data = memory[I + iy];
        dy = (reg[y] + iy) % SCREEN_HEIGHT;
        for (ix = 0; ix < 8; ix++) {
            dx = (reg[x] + ix) % SCREEN_WIDTH;
            bit = (data >> (7 - ix)) & 1;
            oldbit = graphics[dy * SCREEN_WIDTH + dx];
            if (bit && oldbit)
                reg[0xF] = 1;
            
            bit ^= oldbit;
            graphics[dy * SCREEN_WIDTH + dx] = bit;
            bounds.x = dx * scale;
            bounds.y = dy * scale;
            bounds.w = scale;
            bounds.h = scale;
            if (bit)
                SDL_FillRect(surface,
                             &bounds,
                             SDL_MapRGB(surface->format,
                                        FR, FG, FB));
            else
                SDL_FillRect(surface,
                             &bounds,
                             SDL_MapRGB(surface->format,
                                        BR, BG, BB));
        }
    }
    SDL_UpdateWindowSurface(window);
}

void *update_timers(void *data) {
    while (running) {
        if (ST) ST--;
        if (DT) DT--;
        SDL_Delay(hertz(60));
    }
    
    return NULL;
}


void sdl_init(void) {
    if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
        fprintf(stderr, "error: failed to initialize SDL: %s\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }
    
    window = SDL_CreateWindow("CHIP 8 interpreter",
                              SDL_WINDOWPOS_CENTERED,
                              SDL_WINDOWPOS_CENTERED,
                              SCREEN_WIDTH * scale,
                              SCREEN_HEIGHT * scale,
                              SDL_WINDOW_SHOWN);
    if (!window) {
        fprintf(stderr, "error: failed to create window: %s\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }
    SDL_Surface *icon = SDL_LoadBMP("chip8.bmp");
    SDL_SetWindowIcon(window, icon);
    SDL_FreeSurface(icon);
    
    surface = SDL_GetWindowSurface(window);
    SDL_UpdateWindowSurface(window);
}

void sdl_quit(void) {
    if (window)
        SDL_DestroyWindow(window);
    
    SDL_Quit();
}


void print_usage(void) {
    fprintf(stderr, "usage: %s [-h] [-v] FILE\n", prog);
}

void print_version(void) {
    fprintf(stderr, "CHIP8 interpreter version %s\n", version);
}

void print_help(void) {
    print_usage();
    fprintf(stderr, "%s", HELP_MSG);
    exit(EXIT_FAILURE);
}

void parse_args(int argc, char *argv[]) {
    int index, c;
    
    opterr = 0;
    prog = argv[0];
    
    while ((c = getopt(argc, argv, "hvrms:i:")) != -1)
        switch (c) {
            case 'h':
                print_help();
                break;
            case 'v':
                print_version();
                exit(EXIT_FAILURE);
                break;
            case 'r':
                DEBUG[REGDUMP] = true;
                break;
            case 'm':
                DEBUG[MEMDUMP] = true;
                break;
            case 's':
                if (!isallnum(optarg))
                    error("option `-s' requires an intereger argument.");
                
                scale = atoi(optarg);
                break;
            case 'i':
                if (!isallnum(optarg))
                    error("option `-i' requires an intereger argument.");
                
                speed = atoi(optarg);
                break;
            case '?':
                fprintf(stderr, "Unknown option `-%c'.\n", optopt);
                exit(EXIT_FAILURE);
                break;
            default:
                exit(EXIT_FAILURE);
                break;
        }
        
        index = optind;
        if (index < argc) {
            fnin = argv[index++];
            if (index < argc) {
                print_usage();
                exit(EXIT_FAILURE);
            }
            fpin = fopen(fnin, "rb");
            if (!fpin)
                error("failed to open file.");
        }
        else {
            print_usage();
            exit(EXIT_FAILURE);
        }
}

void error(char *msg) {
    fprintf(stderr, "error: %s\n", msg);
    exit(EXIT_FAILURE);
}

void unknown(unsigned int opcode) {
    fprintf(stderr, "unknown opcode: %04X\n", opcode);
    exit(EXIT_FAILURE);
}


void load(void) {
    fseek(fpin, 0, SEEK_END);
    size_t size = ftell(fpin);
    if (size > MEMSIZE)
        error("file is too large (over 4 KB).");
    
    rewind(fpin);
    int index, c;
    for (index = MEMSTART; (c = fgetc(fpin)) != EOF; index++)
        memory[index] = c;
    
    for (index = 0; index < 80; index++)
        memory[index] = font[index];
}

void fetch(void) {
    /* Fetch a single opcode from the memory */
    opcode = (memory[PC] << 8) | memory[PC + 1];
    PC += 2;
}

void decode(void) {
    /* Break the opcode into all possible pieces */
    inst = (opcode & 0xF000) >> 12;
    x    = (opcode & 0x0F00) >>  8;
    y    = (opcode & 0x00F0) >>  4;
    addr = (opcode & 0x0FFF) >>  0;
    kk   = (opcode & 0x00FF) >>  0;
    n    = (opcode & 0x000F) >>  0;
}

void execute(void) {
    /* Execute a single opcode */
    switch (inst) {
        case 0x00:
            switch (addr) {
                case 0x00E0:
                    /* CLS
                     * Clear the display.
                     */
                    for (temp=0; temp < SCREEN_HEIGHT*SCREEN_WIDTH; temp++)
                        graphics[temp] = 0;
                    clear_screen();
                    break;
                case 0x00EE:
                    /* RET
                     * Return from a subroutine.
                     */
                    PC = stack[--SP];
                    break;
                default:
                    /* SYS addr
                     * Jump to a machine code routine at nnn.
                     */
                    break;
            }
            break;
        case 0x01:
            /* JP addr
             * Jump to location nnn.
             */
            PC = addr;
            break;
        case 0x02:
            /* CALL addr
             * Call subroutine at nnn.
             */
            stack[SP++] = PC;
            PC = addr;
            break;
        case 0x03:
            /* SE Vx, byte
             * Skip next instruction if Vx == kk.
             */
            if (reg[x] == kk)
                PC += 2;
            break;
        case 0x04:
            /* SNE Vx, byte
             * Skip next instruction if Vx != kk.
             */
            if (reg[x] != kk)
                PC += 2;
            break;
        case 0x05:
            /* SE Vx, Vy
             * Skip next instruction of Vx == Vy.
             */
            if (reg[x] == reg[y])
                PC += 2;
            break;
        case 0x06:
            /* LD Vx, byte
             * Set Vx = kk.
             */
            reg[x] = kk;
            break;
        case 0x07:
            /* ADD Vx, byte
             * Set Vx = Vx + kk
             */
            reg[x] += kk;
            break;
        case 0x08:
            switch (n) {
                case 0x00:
                    /* LD Vx, Vy
                     * Set Vx = Vy.
                     */
                    reg[x] = reg[y];
                    break;
                case 0x01:
                    /* OR Vx, Vy
                     * Set Vx = Vx OR Vy.
                     */
                    reg[x] |= reg[y];
                    break;
                case 0x02:
                    /* AND Vx, Vy
                     * Set Vx = Vx AND Vy.
                     */
                    reg[x] &= reg[y];
                    break;
                case 0x03:
                    /* XOR Vx, Vy
                     * Set Vx = Vx XOR Vy.
                     */
                    reg[x] ^= reg[y];
                    break;
                case 0x04:
                    /* ADD Vx, Vy
                     * Set Vx = Vx + Vy, set VF = carry.
                     */
                    temp = reg[x];
                    temp += reg[y];
                    reg[0xF] = temp > 0xFF;
                    reg[x] = (unsigned char) temp;
                    break;
                case 0x05:
                    /* SUB Vx, Vy
                     * Set Vx = Vx - Vy, set VF = NOT borrow.
                     */
                    reg[0xF] = reg[x] > reg[y];
                    reg[x] -= reg[y];
                    break;
                case 0x06:
                    /* SHR Vx
                     * Set Vx = Vx SHR 1.
                     */
                    reg[0xF] = reg[x] & 1;
                    reg[x] >>= 1;
                    break;
                case 0x07:
                    /* SUBN Vx, Vy
                     * Set Vx = Vy - Vx, set VF = NOT borrow.
                     */
                    reg[0xF] = reg[y] > reg[x];
                    reg[x] = reg[y] - reg[x];
                    break;
                case 0x0E:
                    /* SHL Vx
                     * Set Vx = Vx SHL 1.
                     */
                    reg[0xF] = (reg[x] & 0b10000000) >> 7;
                    reg[x] <<= 1;
                    break;
                default:
                    /* Something else?
                     */
                    unknown(opcode);
                    break;
            }
            break;
        case 0x09:
            /* SNE Vx, Vy
             * Skip next instruction if Vx != Vy.
             */
            if (reg[x] != reg[y])
                PC += 2;
            break;
        case 0x0A:
            /* LD I, addr
             * Set I = nnn
             */
            I = addr;
            break;
        case 0x0B:
            /* JP V0, addr
             * Jump to location nnn + V0.
             */
            PC = addr + reg[0x0];
            break;
        case 0x0C:
            /* RND Vx, byte
             * Set Vx = random byte AND kk.
             */
            reg[x] = random() & kk;
            break;
        case 0x0D:
            /* DRW Vx, Vy, nibble
             * Display n-byte sprite starting at memory location I at (Vx, Vy),
             *  set VF = collision.
             */
            draw_sprite();
            break;
        case 0x0E:
            switch (kk) {
                case 0x9E:
                    /* SKP Vx
                     * Skip next instruction if key with the value of Vx is pressed.
                     */
                    if (keys[reg[x]])
                        PC += 2;
                    break;
                case 0xA1:
                    /* SKNP Vx
                     * Skip next instruction if key with the value of Vx is not pressed.
                     */
                    if (!keys[reg[x]])
                        PC += 2;
                    break;
                default:
                    /* Something else?
                     */
                    unknown(opcode);
                    break;
            }
            break;
        case 0x0F:
            switch (kk) {
                case 0x07:
                    /* LD Vx, DT
                     * Set Vx = delay timer value.
                     */
                    reg[x] = DT;
                    break;
                case 0x0A:
                    /* LD Vx, K
                     * Wait for a key press, store the value of the key in Vx.
                     */
                    if (!keys[reg[x]])
                        PC -= 2;
                    break;
                case 0x15:
                    /* LD DT, Vx
                     * DT is set equal to the value of Vx.
                     */
                    DT = reg[x];
                    break;
                case 0x18:
                    /* LD ST, Vx
                     * ST is set equal to the value of Vx.
                     */
                    ST = reg[x];
                    break;
                case 0x1E:
                    /* ADD I, Vx
                     * Set I = I + Vx/
                     */
                    I += reg[x];
                    break;
                case 0x29:
                    /* LD F, Vx
                     * Set I = location of sprite for digit Vx.
                     */
                    I = reg[x] * 5;
                    break;
                case 0x33:
                    /* LD B, Vx
                     * Store BCD representation of Vx in memory locations I, I+1, and I+2.
                     */
                    memory[I] = reg[x] / 100;
                    memory[I + 1] = (reg[x] / 10) % 10;
                    memory[I + 2] = reg[x] % 10;
                    break;
                case 0x55:
                    /* LD [I], Vx
                     * Store registers V0 trough Vx in memory starting at location I.
                     */
                    for (n=0; n <= x; n++)
                        memory[I + n] = reg[n];
                    break;
                case 0x65:
                    /* LD Vx, [I]
                     * Read registers V0 trough Vx from memory starting at location I.
                     */
                    for (n=0; n <= x; n++)
                        reg[n] = memory[I + n];
                    break;
                default:
                    /* Something else?
                     */
                    unknown(opcode);
                    break;
            }
            break;
        default:
            /* Something else?
             */
            unknown(opcode);
            break;
    }
}

void handle_events(void) {
    char chr = 0;
    const char *key = NULL;
    
    while (SDL_PollEvent(&evt) != 0) {
        switch (evt.type) {
            case SDL_QUIT:
                exit(EXIT_SUCCESS);
                break;
            case SDL_KEYDOWN: case SDL_KEYUP:
                key = SDL_GetKeyName(evt.key.keysym.sym);
                if (key[1] != '\0') break;
                chr = key[0];
                bool state = (evt.type == SDL_KEYDOWN);
                switch(chr) {
                    case_KEY(0, state);

                    case_KEY(1, state);
                    case_KEY(2, state);
                    case_KEY(3, state);

                    case_KEY(4, state);
                    case_KEY(5, state);
                    case_KEY(6, state);

                    case_KEY(7, state);
                    case_KEY(8, state);
                    case_KEY(9, state);

                    case_KEY(A, state);
                    case_KEY(B, state);
                    case_KEY(C, state);
                    case_KEY(D, state);
                    case_KEY(E, state);
                    case_KEY(F, state);
                }
                break;
            default:
                break;
        }
    }
}

void finish(void) {
    running = false;
    
    if (fpin)
        fclose(fpin);
    
    sdl_quit();
    
    if (DEBUG[REGDUMP]) {
        /* Do a register dump */
        int i;
        printf("+-------REGISTER DUMP-------+\n");
        for (i = 0; i < REGS; i++) {
            printf("| register 0x%X - 0x%02X (%03d) |\n", i, reg[i], reg[i]);
        }
        printf("+----END OF REGISTER DUMP---+\n");
    }
    
    if (DEBUG[MEMDUMP]) {
        FILE *fp = fopen("memory.bin", "wb");
        if (!fp)
            error("failed to open file `memory.bin' for memdump.");
        
        if (fwrite(memory, sizeof(unsigned char), MEMSIZE, fp) != MEMSIZE)
            error("failed to write memdump.");
        fclose(fp);
        printf("Memdump saved into memory.bin.\n");
    }
}

int main(int argc, char *argv[]) {
    atexit(finish);
    parse_args(argc, argv);
    load();
    sdl_init();
    clear_screen();
    double hz = hertz(speed);
    
    PC = 0x200;
    
    if (pthread_create(&updater, NULL, update_timers, NULL))
        error("failed to create thread");
    
    while (PC < MEMSIZE) {
        fetch();
        decode();
        execute();
        handle_events();
        SDL_Delay(hz);
    }
    
    return EXIT_SUCCESS;
}
