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

#include <android/log.h>

#include "SDL2/SDL.h"

#define LOG_TAG "DEBUG_BTCB"

void init_game() {
    __android_log_print(ANDROID_LOG_INFO, LOG_TAG, "I'm at g0");
    random_init();
    __android_log_print(ANDROID_LOG_INFO, LOG_TAG, "I'm at g1");
    graphics_init("Bup the Catboy", WIDTH * 2, HEIGHT * 2);
    __android_log_print(ANDROID_LOG_INFO, LOG_TAG, "I'm at g2");
    graphics_set_resolution(WIDTH, HEIGHT);
    __android_log_print(ANDROID_LOG_INFO, LOG_TAG, "I'm at g3");
    controller_init();
    __android_log_print(ANDROID_LOG_INFO, LOG_TAG, "I'm at g4");
    audio_init();
    __android_log_print(ANDROID_LOG_INFO, LOG_TAG, "I'm at g5");
    load_assets();
    __android_log_print(ANDROID_LOG_INFO, LOG_TAG, "I'm at g6");
    init_data();
    __android_log_print(ANDROID_LOG_INFO, LOG_TAG, "I'm at g7");
    menu_init();
    __android_log_print(ANDROID_LOG_INFO, LOG_TAG, "I'm at g8");
    savefile_load();
    __android_log_print(ANDROID_LOG_INFO, LOG_TAG, "I'm at g9");
    //load_level(GET_ASSET(struct Binary, "levels/level100.lvl"));
    __android_log_print(ANDROID_LOG_INFO, LOG_TAG, "I'm at g10");
    load_menu(title_screen);
    __android_log_print(ANDROID_LOG_INFO, LOG_TAG, "I'm at g11");
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

#define ANDROID_APPNAME "com.maniscat2.btcb"

const char *get_gamedir(void) {
    SDL_bool privileged_write = SDL_FALSE, privileged_manage = SDL_FALSE;
    static char gamedir_unprivileged[512] = { 0 }, gamedir_privileged[12] = { 0 };
    const char *basedir_unprivileged = SDL_AndroidGetExternalStoragePath();
    const char *basedir_privileged = SDL_AndroidGetTopExternalStoragePath();

    snprintf(gamedir_unprivileged, sizeof(gamedir_unprivileged), 
             "%s", basedir_unprivileged);
    snprintf(gamedir_privileged, sizeof(gamedir_privileged), 
             "%s/%s", basedir_privileged, ANDROID_APPNAME);

    //Android 10 and below
    privileged_write = SDL_AndroidRequestPermission("android.permission.WRITE_EXTERNAL_STORAGE");
    //Android 11 and up
    privileged_manage = SDL_AndroidRequestPermission("android.permission.MANAGE_EXTERNAL_STORAGE");
    return (privileged_write || privileged_manage) ? gamedir_privileged : gamedir_unprivileged;
}

int SDL_main(int argc, char** argv) {
    FILE* maingame = fopen(get_gamedir(), "w");
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
        __android_log_print(ANDROID_LOG_INFO, LOG_TAG, "I'm at Q0");
        graphics_get_size(&windoww, &windowh);
        __android_log_print(ANDROID_LOG_INFO, LOG_TAG, "I'm at Q1");
        graphics_start_frame();
        __android_log_print(ANDROID_LOG_INFO, LOG_TAG, "I'm at Q2");
        render_interpolation = min((ticks() - game_start_ticks) / STEP_TIME * TIME_SCALE, 1);
        __android_log_print(ANDROID_LOG_INFO, LOG_TAG, "I'm at Q3");
        render_level(drawlist, WIDTH, HEIGHT, render_interpolation);
        __android_log_print(ANDROID_LOG_INFO, LOG_TAG, "I'm at Q4");
        LE_Render(drawlist, gfxcmd_process);
        __android_log_print(ANDROID_LOG_INFO, LOG_TAG, "I'm at Q5");
        LE_ClearDrawList(drawlist);
        __android_log_print(ANDROID_LOG_INFO, LOG_TAG, "I'm at Q6");
        graphics_end_frame();
        __android_log_print(ANDROID_LOG_INFO, LOG_TAG, "I'm at Q7");
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
