#ifndef CHIP8_H
#define CHIP8_H

#include <stdbool.h>
#include <stdint.h>

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
    uint8_t RAM[RAM_SIZE];

    uint16_t I;   // Index register
    uint16_t PC;  // Program counter

    uint16_t stack[16];
    uint8_t SP;  // Stack pointer

    uint8_t V[16];          // Variable registers
    uint8_t hp48_flags[8];  // HP-48's "RPL user flag" registers (S-CHIP)

    uint8_t screen[SCREEN_SIZE];
    uint8_t keypad[KEYPAD_SIZE];
    uint8_t wait_for_key;

    uint8_t DT;  // Delay timer
    uint8_t ST;  // Sound timer

    uint16_t opcode;  // Current opcode
    uint64_t rng;     // PRNG state
    int IPF;          // No. instructions executed each frame

    bool hi_res;          // Enable 128x64 hi-res mode (S-CHIP)
    bool screen_updated;  // Was the screen updated?

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
void c8_init(Chip8 *vm, int emu_freq, Platform plt, uint64_t seed);

// Loads ROM into the memory of the emulator.
// ASSERT: size <= 3584
void c8_load_rom(Chip8 *vm, unsigned char *rom, int size);

// Fetch-decode-execute N instructions, where N = vm->IPF.
int c8_cycle(Chip8 *vm);

// Decrement timers if they are non-zero.
// This function should be called at a constant frequency of 60Hz.
void c8_decrement_timers(Chip8 *vm);

// Returns true if sound timer is non-zero and sound should be played.
bool c8_sound(Chip8 *vm);

// Returns true if one of the instructions executed by c8_cycle() modified
// the screen, in which case the display should be updated.
bool c8_screen_updated(Chip8 *vm);

// Returns true if the last executed instruction was 00FD. (S-CHIP)
bool c8_ended(Chip8 *vm);

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

// Sets frequency of the emulator (or its "speed").
void c8_set_freq(Chip8 *vm, int emu_freq);

// Sets behavior (CHIP-8, CHIP-48/S-CHIP 1.0 or S-CHIP 1.1) of the emulator.
void c8_set_platform(Chip8 *vm, Platform plt);

#endif
