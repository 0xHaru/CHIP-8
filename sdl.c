#include <time.h>

#include "SDL2/SDL.h"
#include "chip8.h"

typedef struct {
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *texture;
} GfxContext;

void
gfx_create(GfxContext *ctx,
           const char *title,
           int width,
           int height,
           int scale_factor)
{
    SDL_Init(SDL_INIT_VIDEO);

    ctx->window = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED,
                                   SDL_WINDOWPOS_CENTERED, width * scale_factor,
                                   height * scale_factor, SDL_WINDOW_SHOWN);

    ctx->renderer = SDL_CreateRenderer(ctx->window, -1, 0);

    ctx->texture = SDL_CreateTexture(ctx->renderer, SDL_PIXELFORMAT_RGBA8888,
                                     SDL_TEXTUREACCESS_STREAMING, width,
                                     height);
}

void
gfx_update(GfxContext *ctx, const void *pixels, int pitch)
{
    SDL_UpdateTexture(ctx->texture, NULL, pixels, pitch);
    SDL_RenderClear(ctx->renderer);
    SDL_RenderCopy(ctx->renderer, ctx->texture, NULL, NULL);
    SDL_RenderPresent(ctx->renderer);
}

void
gfx_destroy(GfxContext *ctx)
{
    SDL_DestroyTexture(ctx->texture);
    SDL_DestroyRenderer(ctx->renderer);
    SDL_DestroyWindow(ctx->window);
    SDL_Quit();
}

bool
handle_input_event(Chip8 *vm)
{
    static const SDL_Scancode scancodes[] = {
        SDL_SCANCODE_X, SDL_SCANCODE_1, SDL_SCANCODE_2, SDL_SCANCODE_3,
        SDL_SCANCODE_Q, SDL_SCANCODE_W, SDL_SCANCODE_E, SDL_SCANCODE_A,
        SDL_SCANCODE_S, SDL_SCANCODE_D, SDL_SCANCODE_Z, SDL_SCANCODE_C,
        SDL_SCANCODE_4, SDL_SCANCODE_R, SDL_SCANCODE_F, SDL_SCANCODE_V,
    };

    SDL_Event event;
    bool quit = false;

    while (SDL_PollEvent(&event)) {
        switch (event.type) {
        case SDL_QUIT:
            quit = true;
            break;

        case SDL_KEYDOWN:
            if (event.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
                quit = true;
                break;
            }

            for (int i = 0; i < KEYPAD_SIZE; i++) {
                if (event.key.keysym.scancode == scancodes[i])
                    c8_press_key(vm, i);
            }
            break;

        case SDL_KEYUP:
            for (int i = 0; i < KEYPAD_SIZE; i++) {
                if (event.key.keysym.scancode == scancodes[i])
                    c8_release_key(vm, i);
            }
            break;

        default:
            break;
        }
    }

    return quit;
}

void
mono_to_rgba(Chip8 *vm, uint32_t *pixels, int size)
{
    SDL_assert(size >= SCREEN_SIZE * 8);
    for (int row = 0; row < SCREEN_HEIGHT; row++) {
        for (int col = 0; col < SCREEN_WIDTH; col++) {
            int i = SCREEN_WIDTH * row + col;
            pixels[i] = c8_get_pixel(vm, row, col) ? 0xFFFFFFFF : 0x0;
        }
    }
}

int
main(int argc, char *argv[])
{
    if (argc != 4) {
        SDL_Log("Usage: %s <scale-factor> <emulator-frequency> <rom-file>",
                argv[0]);
        return 1;
    }

    // No. instructions per second executed by the emulator
    const int emu_freq = SDL_atoi(argv[2]);
    const int scale_factor = SDL_atoi(argv[1]);

    Chip8 vm;
    c8_init(&vm, emu_freq, P_CHIP8, time(NULL));

    SDL_RWops *file = SDL_RWFromFile(argv[3], "rb");
    if (!file) {
        SDL_Log("Error: couldn't open ROM file");
        return 1;
    }
    SDL_RWread(file, &vm.RAM[PC_OFFSET], 1, MAX_ROM_SIZE);
    SDL_RWclose(file);

    GfxContext ctx;
    gfx_create(&ctx, "CHIP-8", SCREEN_WIDTH, SCREEN_HEIGHT, scale_factor);

    uint32_t pixels[SCREEN_SIZE * 8];
    const int pitch = sizeof(pixels[0]) * SCREEN_WIDTH;
    const double performance_freq = (double) SDL_GetPerformanceFrequency();

    while (true) {
        Uint64 start = SDL_GetPerformanceCounter();

        if (handle_input_event(&vm) || c8_ended(&vm)) break;
        if (c8_cycle(&vm) != 0) goto unknown_opcode;

        c8_decrement_timers(&vm);

        if (c8_screen_updated(&vm)) {
            // Convert monochrome pixels to RGBA pixels
            mono_to_rgba(&vm, pixels, SCREEN_SIZE * 8);
            gfx_update(&ctx, pixels, pitch);
        }

        Uint64 end = SDL_GetPerformanceCounter();
        double elapsed_time = ((end - start) * 1000) / performance_freq;

        if (elapsed_time < GAME_LOOP_DELAY)
            SDL_Delay((Uint32) (GAME_LOOP_DELAY - elapsed_time + 0.5));
    }

    gfx_destroy(&ctx);
    return 0;

unknown_opcode:
    SDL_Log("Error: unknown opcode \"0x%x\"\n", c8_get_opcode(&vm));
    gfx_destroy(&ctx);
    return 1;
}
