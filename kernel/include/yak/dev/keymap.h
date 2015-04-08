#ifndef __YAK_KEYMAP_H__
#define __YAK_KEYMAP_H__

#include <yak/lib/types.h>

const uint8_t keymap_us[2][128] =
{
    {
        0, 27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b', /*  0-14 */
        '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',  /* 15-28 */
        0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',          /* 29-41 */
        0, '\\', 'z','x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,             /* 42-54 */
        0,   /* 55 - Ctrl */
        0,   /* 56 - Alt */
        ' ', /* 57 - Space */
        0,   /* 58 - Caps lock */
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 59-68 - F1-F10 */
        0, /* 69 - Num lock */
        0, /* 70 - Scroll lock */
        0, /* 71 - Home */
        0, /* 72 - Up arrow */
        0, /* 73 - Page up */
        0,
        0, /* 75 - Left arrow */
        0,
        0, /* 77 - Right arrow */
        0,
        0, /* 79 - End */
        0, /* 80 - Down arrow */
        0, /* 81 - Page down */
        0, /* 82 - Insert */
        0, /* 83 - Delete */
        0, 0, 0,
        0, /* 87 - F11 */
        0, /* 88 - F12 */
        0, 0, 0, 0,
        0, /* 93 - Right click key */
        0, /* All other keys are undefined */
    },
    {
        0, 27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b', /*  0-14 */
        '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',  /* 15-28 */
        0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',           /* 29-41 */
        0, '\\', 'Z','X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0,             /* 42-54 */
        0,   /* 55 - Ctrl */
        0,   /* 56 - Alt */
        ' ', /* 57 - Space */
        0,   /* 58 - Caps lock */
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 59-68 - F1-F10 */
        0, /* 69 - Num lock */
        0, /* 70 - Scroll lock */
        0, /* 71 - Home */
        0, /* 72 - Up arrow */
        0, /* 73 - Page up */
        0,
        0, /* 75 - Left arrow */
        0,
        0, /* 77 - Right arrow */
        0,
        0, /* 79 - End */
        0, /* 80 - Down arrow */
        0, /* 81 - Page down */
        0, /* 82 - Insert */
        0, /* 83 - Delete */
        0, 0, 0,
        0, /* 87 - F11 */
        0, /* 88 - F12 */
        0, 0, 0, 0,
        0, /* 93 - Right click key */
        0, /* All other keys are undefined */
    }
};

#endif
