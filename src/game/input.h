#ifndef BTCB_INPUT_H
#define BTCB_INPUT_H

#include <stdbool.h>

#ifndef NO_VSCODE
#define NO_VSCODE
#endif

#define INPUT(_) _ = (1 << (__COUNTER__)),
#define MOUSEBTN(_)
#define KEYMAP(_)
#define CONTROLLER(_)
#define JOYSTICK(_)
enum ButtonIDs {
#include "game/data/inputs.h"
    NUM_INPUTS = __COUNTER__
};
#undef INPUT
#undef MOUSEBTN
#undef KEYMAP
#undef CONTROLLER
#undef JOYSTICK

bool is_button_down(int key);
bool is_button_up(int key);
bool is_button_pressed(int key);
bool is_button_released(int key);
void get_mouse_position(int* x, int* y);
void update_input();

#endif