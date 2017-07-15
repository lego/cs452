#pragma once

// Terminal locations
#define SWITCH_LOCATION 3
#define SENSOR_HISTORY_LOCATION 10
#define COMMAND_LOCATION 23

// Terminal codes
#define CLEAR_TERMINAL "\e\x5b" "\x32" "\x4a"
#define SAVE_CURSOR "\e7"
#define RECOVER_CURSOR "\e8"
#define HOME_CURSOR "\e[H"
#define CLEAR_LINE_BEFORE "\e[1K"
#define CLEAR_LINE_AFTER "\e[K"
#define CLEAR_LINE "\e[2K"

#define SAVE_TERMINAL "\e\x5b" "\x3f" "\x31" "\x30" "\x34" "\x39" "\x68"
#define RECOVER_TERMINAL "\e\x5b" "\x3f" "\x31" "\x30" "\x34" "\x39" "\x6c"

// Colors

#define RESET_ATTRIBUTES "\e[0m"

#define HIDE_CURSOR "\e[?25l"
#define SHOW_CURSOR "\e[?25h"

#define WHITE_BG "\e[47m"
#define RED_BG "\e[41m"
#define GREY_BG "\e[48;5;244m"
#define GREEN_BG "\e[42m"

#define BLACK_FG "\e[30m"
#define GREY_FG "\e[37m"

#define SCROLL_DOWN_20 "\e[20B"
#define CLEAR_SCREEN "\e[2J"


// Characters
#define ESCAPE_CH 0x1b
#define ESCAPE "\e"
#define BACKSPACE_CH '\b'
#define BACKSPACE "\b"
#define DELETE_CH 0x7f
#define NEWLINE_CH '\n'
#define CARRIAGE_RETURN_CH '\r'
