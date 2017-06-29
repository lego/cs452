#ifndef __TERMINAL_H__
#define __TERMINAL_H__

// Terminal codes
#define CLEAR_TERMINAL "\x1b" "\x5b" "\x32" "\x4a"
#define SAVE_CURSOR "\x1b" "7"
#define RECOVER_CURSOR "\x1b" "8"
#define HOME_CURSOR "\x1b" "[H"
#define CLEAR_LINE_BEFORE "\x1b" "[1K"
#define CLEAR_LINE_AFTER "\x1b" "[K"
#define CLEAR_LINE "\x1b" "[2K"

#define SAVE_TERMINAL "\x1b" "\x5b" "\x3f" "\x31" "\x30" "\x34" "\x39" "\x68"
#define RECOVER_TERMINAL "\x1b" "\x5b" "\x3f" "\x31" "\x30" "\x34" "\x39" "\x6c"

// Colors

#define RESET_ATTRIBUTES "\x1b" "[0m"

#define HIDE_CURSOR "\x1b" "[?25l"
#define SHOW_CURSOR "\x1b" "[?25h"

#define WHITE_BG "\x1b" "[47m"
#define RED_BG "\x1b" "[41m"
#define GREEN_BG "\x1b" "[42m"

#define BLACK_FG "\x1b" "[30m"
#define GREY_FG "\x1b" "[37m"


// Characters
#define ESCAPE_CH 0x1b
#define BACKSPACE_CH '\b'
#define DELETE_CH 0x7f
#define NEWLINE_CH '\n'
#define CARRIAGE_RETURN_CH '\r'

#endif
