#include "savefile.h"

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <sys/stat.h>

#include "game/overlay/menu.h"

#include "SDL2/SDL.h"

#include <android/log.h>

#define LOG_TAG "DEBUG_BTCB"

#ifdef WINDOWS
#define BINARY "b"
#else
#define BINARY
#endif

#define SAVEFILE_FILENAME "btcb.sav"

struct {
    uint8_t selected_savefile;
    struct SaveFile savefiles[NUM_SAVEFILES];
} savedata;
struct SaveFile* savefile = NULL;

enum {
    Context_Select,
    Context_Erase,
    Context_CopyWhat,
    Context_CopyTo
} curr_context;
int copy_what = -1;

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

void savefile_load() {
    char* game = get_gamedir();
    char path[512] = {0};
    snprintf(path, 512, "%s/%s", game, SAVEFILE_FILENAME);
     __android_log_print(ANDROID_LOG_INFO, LOG_TAG, "I'm at f0");
    FILE* f = fopen(path, "r" BINARY);
     __android_log_print(ANDROID_LOG_INFO, LOG_TAG, "I'm at f1");
    if (!f) {
        savedata.selected_savefile = 0;
        for (int i = 0; i < NUM_SAVEFILES; i++) {
            savefile_erase(i);
        }
         __android_log_print(ANDROID_LOG_INFO, LOG_TAG, "I'm at f2");
        savefile_save();
         __android_log_print(ANDROID_LOG_INFO, LOG_TAG, "I'm at f3");
    }
    else {
        fread(&savedata, sizeof(savedata), 1, f);
         __android_log_print(ANDROID_LOG_INFO, LOG_TAG, "I'm at f4");
        fclose(f);
         __android_log_print(ANDROID_LOG_INFO, LOG_TAG, "I'm at f6");
    }
     __android_log_print(ANDROID_LOG_INFO, LOG_TAG, "I'm at f7");
    savefile_select(savedata.selected_savefile);
     __android_log_print(ANDROID_LOG_INFO, LOG_TAG, "I'm at f8");
}

void savefile_select(int file) {
    savedata.selected_savefile = file;
    savefile = &savedata.savefiles[file];
}

void savefile_erase(int file) {
    memset(&savedata.savefiles[file], 0, sizeof(struct SaveFile));
}

void savefile_copy(int from, int to) {
    memcpy(&savedata.savefiles[to], &savedata.savefiles[from], sizeof(struct SaveFile));
}

void savefile_save() {
    /*char* game = get_gamedir();
    __android_log_print(ANDROID_LOG_INFO, LOG_TAG, "I'm at s0");
    char path[512] = {0};
    snprintf(path, 512, "%s/%s", game, SAVEFILE_FILENAME);
    __android_log_print(ANDROID_LOG_INFO, LOG_TAG, "I'm at s1");
    FILE* f = fopen(path, "w" BINARY);
    __android_log_print(ANDROID_LOG_INFO, LOG_TAG, "I'm at s2");
    //fwrite(&savedata, sizeof(savedata), 1, f);
    __android_log_print(ANDROID_LOG_INFO, LOG_TAG, "I'm at s3");
    fclose(f);*/
}

struct SaveFile* savefile_get(int file) {
    return &savedata.savefiles[file];
}

void menubtn_file_select(int selected_index) {
    int file = selected_index - 1;
    switch (curr_context) {
        case Context_Select:
            savefile_select(file);
            pop_menu();
            break;
        case Context_CopyWhat:
            curr_context = Context_CopyTo;
            copy_what = file;
            push_menu(file_to);
            break;
        case Context_CopyTo:
            savefile_copy(copy_what, file);
            pop_menu_multi(2);
            break;
        case Context_Erase:
            savefile_erase(file);
            pop_menu();
            break;
    }
}

void menubtn_file_copy(int selected_index) {
    curr_context = Context_CopyWhat;
    push_menu(file_what);
}

void menubtn_file_erase(int selected_index) {
    curr_context = Context_Erase;
    push_menu(file_what);
}

void menubtn_file_cancel(int selected_index) {
    if (curr_context == Context_CopyTo) pop_menu_multi(2);
    else pop_menu();
    curr_context = Context_Select;
}