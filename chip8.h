#ifndef CHIP8_H
#define CHIP8_H

#define DEBUG  // Turns on ASSERT macro

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

_Static_assert(sizeof(u8) == 1, "u8 must be 1 byte long");
_Static_assert(sizeof(u16) == 2, "u16 must be 2 bytes long");
_Static_assert(sizeof(u32) == 4, "u32 must be 4 bytes long");
_Static_assert(sizeof(u64) == 8, "u64 must be 8 bytes long");

// Delay in milliseconds needed to achieve 60fps (1000/60=16.666)
#define GAME_LOOP_DELAY 16.666
#define GAME_LOOP_FREQ 60

#define RAM_SIZE 4096
#define MAX_ROM_SIZE 3584  // RAM_SIZE - PC_OFFSET

#define FONT_OFFSET 0x50
#define HFONT_OFFSET 0xA0
#define PC_OFFSET 0x200

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define SCREEN_SIZE 1024  // 128x64 pixels = 8192 bits = 1024 bytes

#define KEYPAD_SIZE 16

typedef enum {
    P_CHIP8,      // Enable "modern" CHIP-8 behavior
    P_SCHIP_1_0,  // Enable CHIP-48/S-CHIP 1.0 behavior
    P_SCHIP_1_1,  // Enable S-CHIP 1.1 behavior
} Platform;

typedef struct {
    u8 RAM[RAM_SIZE];

    u16 I;   // Index register
    u16 PC;  // Program counter

    u16 stack[16];
    u8 SP;  // Stack pointer

    u8 V[16];          // Variable registers
    u8 hp48_flags[8];  // HP-48's "RPL user flag" registers (S-CHIP)

    u8 screen[SCREEN_SIZE];
    u8 keypad[KEYPAD_SIZE];
    u8 wait_for_key;

    u8 DT;  // Delay timer
    u8 ST;  // Sound timer

    u16 opcode;  // Current opcode
    u64 rng;     // PRNG state
    int IPF;     // No. instructions executed each frame

    _Bool hi_res;          // Enable 128x64 hi-res mode (S-CHIP)
    _Bool screen_updated;  // Was the screen updated?

    Platform platform;  // CHIP-8, CHIP-48/S-CHIP 1.0 or S-CHIP 1.1 behavior?
} Chip8;

// Fully resets state of the emulator.
void c8_reset(Chip8 *vm);

// Resets emulator but not its memory.
void c8_soft_reset(Chip8 *vm);

// Fully resets emulator and initializes it
// - emu_freq: frequency of the emulator (or its "speed")
// - plt: CHIP-8, CHIP-48/S-CHIP 1.0 or S-CHIP 1.1 behavior?
// - seed: initial seed for the PRNG
void c8_init(Chip8 *vm, int emu_freq, Platform plt, unsigned long long seed);

// Loads ROM into the memory of the emulator.
// ASSERT: size <= 3584
void c8_load_rom(Chip8 *vm, unsigned char *rom, int size);

// Fetch-decode-execute N instructions, where N = vm->IPF.
int c8_cycle(Chip8 *vm);

// Decrement timers if they are non-zero.
// This function should be called at a constant frequency of 60Hz.
void c8_decrement_timers(Chip8 *vm);

// Returns true if sound timer is non-zero and sound should be played.
_Bool c8_sound(Chip8 *vm);

// Returns true if one of the instructions executed by c8_cycle() modified
// the screen, in which case the display should be updated.
_Bool c8_screen_updated(Chip8 *vm);

// Returns true if the last executed instruction was 00FD. (S-CHIP)
_Bool c8_ended(Chip8 *vm);

// Returns 1 if the pixel is set, 0 if the pixel is cleared.
// ASSERT: 0 <= row <= 63 && 0 <= col <= 127
int c8_get_pixel(Chip8 *vm, int row, int col);

// Returns last executed opcode.
int c8_get_opcode(Chip8 *vm);

// Sets selected key to 1.
// ASSERT: 0 <= key <= 15
void c8_press_key(Chip8 *vm, int key);

// Sets selected key to 0.
// ASSERT: 0 <= key <= 15
void c8_release_key(Chip8 *vm, int key);

// Sets behavior (CHIP-8, CHIP-48/S-CHIP 1.0 or S-CHIP 1.1) of the emulator.
void c8_set_platform(Chip8 *vm, Platform plt);

#endif
