#include "main.h"

#include "game/input.h"
#include "io/assets/assets.h"
#include "io/audio/audio.h"
#include "game/data.h"
#include "game/level.h"
#include "game/overlay/transition.h"
#include "game/savefile.h"
#include "game/overlay/menu.h"
#include "io/io.h"
#include "rng.h"

#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <pthread.h>

#define STEPS_PER_SECOND 60
#define STEP_TIME (TICKS_PER_SEC / STEPS_PER_SECOND)
#define TIME_SCALE 1

double internal_global_timer = 0;
uint64_t global_timer = 0;
uint64_t game_start_ticks = 0;
float delta_time = 0;
bool running = true;
pthread_t game_loop_thread;
LE_DrawList* drawlist;
float render_interpolation = 0;

int windoww, windowh;

void init_game() {
    random_init();
    graphics_init("Bup the Catboy", WIDTH * 2, HEIGHT * 2);
    graphics_set_resolution(WIDTH, HEIGHT);
    controller_init();
    audio_init();
    load_assets();
    init_data();
    menu_init();
    savefile_load();
    load_level(GET_ASSET(struct Binary, "levels/level100.lvl"));
    load_menu(title_screen);
}

void* game_loop(void* _) {
    while (running) {
        uint64_t num_ticks = ticks();
        if (game_start_ticks == 0) game_start_ticks = num_ticks;
        delta_time = (num_ticks - game_start_ticks) / TICKS_PER_SEC * STEPS_PER_SECOND * TIME_SCALE;
        game_start_ticks = num_ticks;
        update_input();
        if (current_level != NULL) {
            update_transition();
            update_level(delta_time);
        }
        sync(game_start_ticks, STEP_TIME / TIME_SCALE);
        internal_global_timer += delta_time;
        global_timer = internal_global_timer;
    }
    return NULL;
}

#include <android/log.h>

#define LOG_TAG "DEBUG_BTCB"

int SDL_main(int argc, char** argv) {
    __android_log_print(ANDROID_LOG_INFO, LOG_TAG, "I'm at 0");
    if (argc >= 2) {
        if (strcmp(argv[1], "--extract") == 0) {
            extract_assets();
            return 0;
        }
    }
    __android_log_print(ANDROID_LOG_INFO, LOG_TAG, "I'm at 1");
    drawlist = LE_CreateDrawList();
    __android_log_print(ANDROID_LOG_INFO, LOG_TAG, "I'm at 2");
    init_game();
    __android_log_print(ANDROID_LOG_INFO, LOG_TAG, "I'm at 3");
    pthread_create(&game_loop_thread, NULL, game_loop, NULL);
    __android_log_print(ANDROID_LOG_INFO, LOG_TAG, "I'm at 4");
    while (true) {
        if (requested_quit()) break;
        graphics_get_size(&windoww, &windowh);
        graphics_start_frame();
        render_interpolation = min((ticks() - game_start_ticks) / STEP_TIME * TIME_SCALE, 1);
        render_level(drawlist, WIDTH, HEIGHT, render_interpolation);
        LE_Render(drawlist, gfxcmd_process);
        LE_ClearDrawList(drawlist);
        graphics_end_frame();
    }
    __android_log_print(ANDROID_LOG_INFO, LOG_TAG, "I'm at 5");
    running = false;
    __android_log_print(ANDROID_LOG_INFO, LOG_TAG, "I'm at 6");
    pthread_join(game_loop_thread, NULL);
    __android_log_print(ANDROID_LOG_INFO, LOG_TAG, "I'm at 7");
    graphics_deinit();
    __android_log_print(ANDROID_LOG_INFO, LOG_TAG, "I'm at 8");
    controller_deinit();
    audio_deinit();
    io_deinit();
    return 0;
}
