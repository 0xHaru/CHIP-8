#include "chip8.h"

// Source: skeeto/handmade hero
#ifdef DEBUG
#define ASSERT(expr)             \
    if (!(expr)) {               \
        *(volatile int *) 0 = 0; \
    }
#else
#define ASSERT(expr)
#endif

// Source: libgcc
static void *
memset_(void *dest, int val, uint64_t len)
{
    unsigned char *ptr = dest;
    while (len-- > 0)
        *ptr++ = val;
    return dest;
}

// Source: libgcc
static void *
memcpy_(void *dest, const void *src, uint64_t len)
{
    char *d = dest;
    const char *s = src;
    while (len--)
        *d++ = *s++;
    return dest;
}

// Source: skeeto
static uint8_t
rand_byte(uint64_t *s)
{
    *s = *s * 0x3243f6a8885a308d + 1;
    return *s >> 56;
}

static const uint8_t font[] = {
    // Standard 8x5 font
    0xF0, 0x90, 0x90, 0x90, 0xF0,  // 0
    0x20, 0x60, 0x20, 0x20, 0x70,  // 1
    0xF0, 0x10, 0xF0, 0x80, 0xF0,  // 2
    0xF0, 0x10, 0xF0, 0x10, 0xF0,  // 3
    0x90, 0x90, 0xF0, 0x10, 0x10,  // 4
    0xF0, 0x80, 0xF0, 0x10, 0xF0,  // 5
    0xF0, 0x80, 0xF0, 0x90, 0xF0,  // 6
    0xF0, 0x10, 0x20, 0x40, 0x40,  // 7
    0xF0, 0x90, 0xF0, 0x90, 0xF0,  // 8
    0xF0, 0x90, 0xF0, 0x10, 0xF0,  // 9
    0xF0, 0x90, 0xF0, 0x90, 0x90,  // A
    0xE0, 0x90, 0xE0, 0x90, 0xE0,  // B
    0xF0, 0x80, 0x80, 0x80, 0xF0,  // C
    0xE0, 0x90, 0x90, 0x90, 0xE0,  // D
    0xF0, 0x80, 0xF0, 0x80, 0xF0,  // E
    0xF0, 0x80, 0xF0, 0x80, 0x80,  // F

    // Hi-res 8x10 font (S-CHIP)
    0x3C, 0x7E, 0xE7, 0xC3, 0xC3, 0xC3, 0xC3, 0xE7, 0x7E, 0x3C,  // 0
    0x18, 0x38, 0x58, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x3C,  // 1
    0x3E, 0x7F, 0xC3, 0x06, 0x0C, 0x18, 0x30, 0x60, 0xFF, 0xFF,  // 2
    0x3C, 0x7E, 0xC3, 0x03, 0x0E, 0x0E, 0x03, 0xC3, 0x7E, 0x3C,  // 3
    0x06, 0x0E, 0x1E, 0x36, 0x66, 0xC6, 0xFF, 0xFF, 0x06, 0x06,  // 4
    0xFF, 0xFF, 0xC0, 0xC0, 0xFC, 0xFE, 0x03, 0xC3, 0x7E, 0x3C,  // 5
    0x3E, 0x7C, 0xC0, 0xC0, 0xFC, 0xFE, 0xC3, 0xC3, 0x7E, 0x3C,  // 6
    0xFF, 0xFF, 0x03, 0x06, 0x0C, 0x18, 0x30, 0x60, 0x60, 0x60,  // 7
    0x3C, 0x7E, 0xC3, 0xC3, 0x7E, 0x7E, 0xC3, 0xC3, 0x7E, 0x3C,  // 8
    0x3C, 0x7E, 0xC3, 0xC3, 0x7F, 0x3F, 0x03, 0x03, 0x3E, 0x7C   // 9
};

void
c8_reset(Chip8 *vm)
{
    memset_(vm, 0, sizeof(Chip8));
    memcpy_(&vm->RAM[FONT_OFFSET], font, sizeof(font));
    vm->PC = PC_OFFSET;
}

void
c8_soft_reset(Chip8 *vm)
{
    *vm = (Chip8){
        .PC = PC_OFFSET,
        .screen_updated = true,
    };
}

void
c8_init(Chip8 *vm, int emu_freq, Platform plt, uint64_t seed)
{
    c8_reset(vm);
    vm->IPF = (int) ((double) emu_freq / GAME_LOOP_FREQ + 0.5);
    vm->platform = plt;
    vm->rng = seed;
}

void
c8_load_rom(Chip8 *vm, unsigned char *rom, int size)
{
    ASSERT(size <= MAX_ROM_SIZE);
    memcpy_(&vm->RAM[PC_OFFSET], rom, size);
}

void
c8_decrement_timers(Chip8 *vm)
{
    if (vm->DT > 0) vm->DT -= 1;
    if (vm->ST > 0) vm->ST -= 1;
}

bool
c8_sound(Chip8 *vm)
{
    return vm->ST > 0;
}

bool
c8_screen_updated(Chip8 *vm)
{
    return vm->screen_updated;
}

bool
c8_ended(Chip8 *vm)
{
    return vm->opcode == 0x00FD;
}

int
c8_get_pixel(Chip8 *vm, int row, int col)
{
    ASSERT(row >= 0 && row <= 63 && col >= 0 && col <= 127);
    int byte = 128 * row + col;            // Row major order
    byte /= 8;                             // Convert 128x64 to 16x64
    uint8_t bitmask = 1 << (7 - col % 8);  // Bitmask to get pixel
    return (vm->screen[byte] & bitmask) != 0;
}

void
set_pixel(Chip8 *vm, int row, int col)
{
    ASSERT(row >= 0 && row <= 63 && col >= 0 && col <= 127);
    int byte = 128 * row + col;            // Row major order
    byte /= 8;                             // Convert 128x64 to 16x64
    uint8_t bitmask = 1 << (7 - col % 8);  // Bitmask to set pixel
    vm->screen[byte] |= bitmask;
}

void
clear_pixel(Chip8 *vm, int row, int col)
{
    ASSERT(row >= 0 && row <= 63 && col >= 0 && col <= 127);
    int byte = 128 * row + col;            // Row major order
    byte /= 8;                             // Convert 128x64 to 16x64
    uint8_t bitmask = 1 << (7 - col % 8);  // Bitmask to clear pixel
    vm->screen[byte] &= ~bitmask;
}

int
c8_get_opcode(Chip8 *vm)
{
    return vm->opcode;
}

void
c8_press_key(Chip8 *vm, int key)
{
    ASSERT(key >= 0 && key <= 15);
    vm->keypad[key] = 1;
}

void
c8_release_key(Chip8 *vm, int key)
{
    ASSERT(key >= 0 && key <= 15);
    vm->keypad[key] = 0;
}

void
c8_set_freq(Chip8 *vm, int emu_freq)
{
    vm->IPF = (int) ((double) emu_freq / GAME_LOOP_FREQ + 0.5);
}

void
c8_set_platform(Chip8 *vm, Platform plt)
{
    vm->platform = plt;
}

// Display n-byte sprite starting at memory location I at (Vx, Vy),
// set VF = collision
static void
op_Dxyn(Chip8 *vm, uint8_t x, uint8_t y, uint8_t n)
{
    vm->V[0xF] = 0;
    int screen_width = vm->hi_res ? 128 : 64;
    int screen_height = vm->hi_res ? 64 : 32;

    // Top-left coordinate of the sprite (origin)
    int xo = vm->V[x] % screen_width;   // X origin (column)
    int yo = vm->V[y] % screen_height;  // Y origin (row)

    // Draw an n pixels tall sprite
    for (int row = 0; row < n; row++) {
        if (yo + row >= screen_height) break;
        uint8_t sprite_row = vm->RAM[vm->I + row];

        // Sprites are guaranteed to be 8 pixels wide
        // (where each pixel is represented by a single bit)
        for (int col = 0; col < 8; col++) {
            if (xo + col >= screen_width) break;
            int xc = xo + col;  // X origin + X offset
            int yc = yo + row;  // Y origin + Y offset

            uint8_t sprite_pixel = (sprite_row & (1 << (7 - col))) != 0;

            if (vm->hi_res) {
                int screen_pixel = c8_get_pixel(vm, yc, xc);
                vm->V[0xF] |= screen_pixel & sprite_pixel;

                if (screen_pixel ^ sprite_pixel)
                    set_pixel(vm, yc, xc);
                else
                    clear_pixel(vm, yc, xc);
            } else {
                // Scale coordinates
                xc *= 2;
                yc *= 2;

                int screen_pixel = c8_get_pixel(vm, yc, xc);
                vm->V[0xF] |= screen_pixel & sprite_pixel;

                // Scale 64x32 up to 128x64
                if (screen_pixel ^ sprite_pixel) {
                    set_pixel(vm, yc, xc);
                    set_pixel(vm, yc, xc + 1);
                    set_pixel(vm, yc + 1, xc);
                    set_pixel(vm, yc + 1, xc + 1);
                } else {
                    clear_pixel(vm, yc, xc);
                    clear_pixel(vm, yc, xc + 1);
                    clear_pixel(vm, yc + 1, xc);
                    clear_pixel(vm, yc + 1, xc + 1);
                }
            }
        }
    }
}

// If n=0 and extended mode, show 16x16 sprite (S-CHIP)
static void
op_Dxy0(Chip8 *vm, uint8_t x, uint8_t y, uint8_t n)
{
    ASSERT(n == 0 && vm->hi_res);

    vm->V[0xF] = 0;
    int screen_width = 128;
    int screen_height = 64;

    // Top-left coordinate of the sprite (origin)
    int xo = vm->V[x] % screen_width;   // X origin (column)
    int yo = vm->V[y] % screen_height;  // Y origin (row)

    // Draw a 16 pixels tall hi-res sprite
    for (int row = 0; row < 16; row++) {
        if (yo + row >= screen_height) break;
        uint16_t sprite_row = vm->RAM[vm->I + 2 * row] << 8 |
                              vm->RAM[vm->I + 2 * row + 1];

        // Hi-res sprites are guaranteed to be 16 pixels wide
        // (where each pixel is represented by a single bit)
        for (int col = 0; col < 16; col++) {
            if (xo + col >= screen_width) break;
            int xc = xo + col;  // X origin + X offset
            int yc = yo + row;  // Y origin + Y offset

            uint8_t sprite_pixel = (sprite_row & (1 << (15 - col))) != 0;
            int screen_pixel = c8_get_pixel(vm, yc, xc);
            vm->V[0xF] |= screen_pixel & sprite_pixel;

            if (screen_pixel ^ sprite_pixel)
                set_pixel(vm, yc, xc);
            else
                clear_pixel(vm, yc, xc);
        }
    }
}

// Scroll display n lines down (S-CHIP)
static void
op_00Cn(Chip8 *vm, uint8_t n)
{
    int rows = vm->hi_res ? 64 : 32;
    int cols = vm->hi_res ? (128 / 8) : (64 / 8);
    rows--;
    while (rows >= n) {
        memcpy_(&vm->screen[rows * cols], &vm->screen[(rows - n) * cols], cols);
        rows--;
    }
    memset_(vm->screen, 0, n * cols);
}

// Scroll display 4 pixels right (S-CHIP)
static void
op_00FB(Chip8 *vm)
{
    int rows = vm->hi_res ? 64 : 32;
    int cols = vm->hi_res ? (128 / 8) : (64 / 8);
    for (int y = 0; y < rows; y++) {
        for (int x = cols - 1; x > 0; x--) {
            vm->screen[y * cols + x] = vm->screen[y * cols + x] >> 4 |
                                       vm->screen[y * cols + x - 1] << 4;
        }
        vm->screen[y * cols] >>= 4;
    }
}

// Scroll display 4 pixels left
static void
op_00FC(Chip8 *vm)
{
    int rows = vm->hi_res ? 64 : 32;
    int cols = vm->hi_res ? (128 / 8) : (64 / 8);
    for (int y = 0; y < rows; y++) {
        for (int x = 0; x < cols - 1; x++) {
            vm->screen[y * cols + x] = vm->screen[y * cols + x] << 4 |
                                       vm->screen[y * cols + x + 1] >> 4;
        }
        vm->screen[y * cols + (cols - 1)] <<= 4;
    }
}

// Wait for a key press, store the value of the key in Vx
static void
op_Fx0A(Chip8 *vm, uint8_t x)
{
    // On the original COSMAC VIP, the key was only registered when it was
    // pressed and then *released*
    switch (vm->wait_for_key) {
    case 0:
        vm->PC -= 2;
        for (int i = 0; i < KEYPAD_SIZE; i++)
            if (vm->keypad[i]) return;
        vm->wait_for_key = 1;
        break;
    case 1:
        vm->PC -= 2;
        for (int i = 0; i < KEYPAD_SIZE; i++) {
            if (vm->keypad[i]) {
                vm->V[x] = i;
                vm->wait_for_key = 2;
                return;
            }
        }
        break;
    case 2:
        for (int i = 0; i < KEYPAD_SIZE; i++) {
            if (vm->keypad[i]) {
                vm->PC -= 2;
                return;
            }
        }
        vm->wait_for_key = 0;
        break;
    }
}

static int
decode_and_execute(Chip8 *vm)
{
    // Decode
    uint8_t x = (vm->opcode & 0x0F00) >> 8;
    uint8_t y = (vm->opcode & 0x00F0) >> 4;
    uint8_t n = vm->opcode & 0x000F;
    uint8_t kk = vm->opcode & 0x00FF;
    uint16_t nnn = vm->opcode & 0x0FFF;

    // Execute
    switch (vm->opcode & 0xF000) {
    case 0x0000:
        if ((vm->opcode & 0xFFF0) == 0x00C0) {
            // SCD nibble (00Cn) - S-CHIP
            op_00Cn(vm, n);
            vm->screen_updated = true;
        } else if (vm->opcode == 0x00E0) {
            // CLS (00E0)
            memset_(vm->screen, 0, SCREEN_SIZE);
            vm->screen_updated = true;
        } else if (vm->opcode == 0x00EE) {
            // RET (00EE)
            ASSERT(vm->SP > 0);
            vm->PC = vm->stack[vm->SP--];
        } else if (vm->opcode == 0x00FB) {
            // SCR (00FB) - S-CHIP
            op_00FB(vm);
            vm->screen_updated = true;
        } else if (vm->opcode == 0x00FC) {
            // SCL (00FC) - S-CHIP
            op_00FC(vm);
            vm->screen_updated = true;
        } else if (vm->opcode == 0x00FD) {
            // EXIT (00FD) - S-CHIP
            // Subsequent calls will encounter 00FD again,
            // and c8_ended() will return true
            vm->PC -= 2;
        } else if (vm->opcode == 0x00FE) {
            // LOW (00FE) - S-CHIP
            if (vm->hi_res) vm->screen_updated = true;
            vm->hi_res = false;
        } else if (vm->opcode == 0x00FF) {
            // HIGH (00FF) - S-CHIP
            if (!vm->hi_res) vm->screen_updated = true;
            vm->hi_res = true;
        } else {
            // SYS addr (0nnn) - Not implemented
        }
        break;

    case 0x1000:
        // JP addr (1nnn)
        vm->PC = nnn;
        break;

    case 0x2000:
        // CALL addr (2nnn)
        ASSERT(vm->SP < 15);
        vm->stack[++vm->SP] = vm->PC;
        vm->PC = nnn;
        break;

    case 0x3000:
        // SE Vx, byte (3xkk)
        if (vm->V[x] == kk) vm->PC += 2;
        break;

    case 0x4000:
        // SNE Vx, byte (4xkk)
        if (vm->V[x] != kk) vm->PC += 2;
        break;

    case 0x5000:
        // SE Vx, Vy (5xy0)
        if (vm->V[x] == vm->V[y]) vm->PC += 2;
        break;

    case 0x6000:
        // LD Vx, byte (6xkk)
        vm->V[x] = kk;
        break;

    case 0x7000:
        // ADD Vx, byte (7xkk)
        vm->V[x] += kk;
        break;

    case 0x8000:
        switch (vm->opcode & 0x000F) {
        case 0x0000:
            // LD Vx, Vy (8xy0)
            vm->V[x] = vm->V[y];
            break;

        case 0x0001:
            // OR Vx, Vy (8xy1)
            vm->V[x] |= vm->V[y];
            break;

        case 0x0002:
            // AND Vx, Vy (8xy2)
            vm->V[x] &= vm->V[y];
            break;

        case 0x0003:
            // XOR Vx, Vy (8xy3)
            vm->V[x] ^= vm->V[y];
            break;

        case 0x0004: {
            // ADD Vx, Vy (8xy4)
            bool carry = (vm->V[x] + vm->V[y]) > 255;
            vm->V[x] += vm->V[y];
            vm->V[0xF] = carry;
            break;
        }

        case 0x0005: {
            // SUB Vx, Vy (8xy5)
            bool not_borrow = vm->V[x] > vm->V[y];
            vm->V[x] -= vm->V[y];
            vm->V[0xF] = not_borrow;
            break;
        }

        case 0x0006: {
            // SHR Vx {, Vy} (8xy6) - Ambiguous instruction
            if (vm->platform == P_CHIP8) vm->V[x] = vm->V[y];
            uint8_t vf = vm->V[x] & 0x01;
            vm->V[x] >>= 1;
            vm->V[0xF] = vf;
            break;
        }

        case 0x0007: {
            // SUBN Vx, Vy (8xy7)
            bool not_borrow = vm->V[y] > vm->V[x];
            vm->V[x] = vm->V[y] - vm->V[x];
            vm->V[0xF] = not_borrow;
            break;
        }

        case 0x000E: {
            // SHL Vx {, Vy} (8xyE) - Ambiguous instruction
            if (vm->platform == P_CHIP8) vm->V[x] = vm->V[y];
            uint8_t vf = vm->V[x] >> 7;
            vm->V[x] <<= 1;
            vm->V[0xF] = vf;
            break;
        }

        default:
            return -1;  // Unknown opcode
        }
        break;

    case 0x9000:
        // SNE Vx, Vy (9xy0)
        if (vm->V[x] != vm->V[y]) vm->PC += 2;
        break;

    case 0xA000:
        // LD I, addr (Annn)
        vm->I = nnn;
        break;

    case 0xB000:
        // JP V0, addr (Bnnn) - Ambiguous instruction
        if (vm->platform == P_SCHIP_1_0 || vm->platform == P_SCHIP_1_1)
            vm->PC = vm->V[x] + nnn;
        else
            vm->PC = vm->V[0x0] + nnn;
        break;

    case 0xC000:;
        // RND Vx, byte (Cxkk)
        vm->V[x] = rand_byte(&vm->rng) & kk;
        break;

    case 0xD000:
        if (vm->hi_res && n == 0) {
            // DRW Vx, Vy, 0 (Dxy0) - S-CHIP
            op_Dxy0(vm, x, y, n);
        } else {
            // DRW Vx, Vy, nibble (Dxyn)
            op_Dxyn(vm, x, y, n);
        }
        vm->screen_updated = true;
        break;
    case 0xE000:
        switch (vm->opcode & 0x00FF) {
        case 0x009E:
            // SKP Vx (Ex9E)
            if (vm->keypad[vm->V[x]]) vm->PC += 2;
            break;

        case 0x00A1:
            // SKNP Vx (ExA1)
            if (!vm->keypad[vm->V[x]]) vm->PC += 2;
            break;

        default:
            return -1;  // Unknown opcode
        }
        break;

    case 0xF000:
        switch (vm->opcode & 0x00FF) {
        case 0x0007:
            // LD Vx, DT (Fx07)
            vm->V[x] = vm->DT;
            break;

        case 0x000A:
            // LD Vx, K (Fx0A)
            op_Fx0A(vm, x);
            break;

        case 0x0015:
            // LD DT, Vx (Fx15)
            vm->DT = vm->V[x];
            break;

        case 0x0018:
            // LD ST, Vx (Fx18)
            vm->ST = vm->V[x];
            break;

        case 0x001E:
            // ADD I, Vx (Fx1E)
            vm->I += vm->V[x];
            break;

        case 0x0029:
            // LD F, Vx (Fx29)
            // A font sprite is 5 bytes long
            vm->I = FONT_OFFSET + (vm->V[x] * 5);
            break;

        case 0x0030:
            // LD HF, Vx (Fx30) - S-CHIP
            // An hi-res font sprite is 10 bytes long
            vm->I = HFONT_OFFSET + (vm->V[x] * 10);
            break;

        case 0x0033:
            // LD B, Vx (Fx33)
            vm->RAM[vm->I + 0] = (vm->V[x] / 100) % 10;
            vm->RAM[vm->I + 1] = (vm->V[x] / 10) % 10;
            vm->RAM[vm->I + 2] = (vm->V[x] / 1) % 10;
            break;

        case 0x0055:
            // LD [I], Vx (Fx55) - Ambiguous instruction
            for (int i = 0; i <= x; i++)
                vm->RAM[vm->I + i] = vm->V[i];

            if (vm->platform == P_CHIP8) vm->I += (x + 1);
            if (vm->platform == P_SCHIP_1_0) vm->I += x;
            break;

        case 0x0065:
            // LD Vx, [I] (Fx65) - Ambiguous instruction
            for (int i = 0; i <= x; i++)
                vm->V[i] = vm->RAM[vm->I + i];

            if (vm->platform == P_CHIP8) vm->I += (x + 1);
            if (vm->platform == P_SCHIP_1_0) vm->I += x;
            break;

        case 0x0075:
            // LD R, Vx (Fx75) - S-CHIP
            ASSERT(x <= 7);
            memcpy_(vm->hp48_flags, vm->V, x + 1);
            break;

        case 0x0085:
            // LD Vx, R (Fx85) - S-CHIP
            ASSERT(x <= 7);
            memcpy_(vm->V, vm->hp48_flags, x + 1);
            break;

        default:
            return -1;  // Unknown opcode
        }
        break;

    default:
        return -1;  // Unknown opcode
    }

    return 0;
}

int
c8_cycle(Chip8 *vm)
{
    vm->screen_updated = false;

    for (int i = 0; i < vm->IPF; i++) {
        // Fetch (an instruction is 2 bytes long)
        vm->opcode = (vm->RAM[vm->PC] << 8) | vm->RAM[vm->PC + 1];
        vm->PC += 2;

        if (decode_and_execute(vm) != 0) return -1;
    }

    return 0;
}
