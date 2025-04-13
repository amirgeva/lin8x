#pragma once

#include <types.h>

#define MODIFIER_SHIFT 0x10000000
#define MODIFIER_CTRL  0x20000000
#define MODIFIER_ALT   0x40000000

// Returns 0 if a keyboard is found, -1 if not
int open_keyboard();
void close_keyboard();
// Returns number of keyboard events found, -1 if an error has occurred
int poll_keyboard_event();

bool is_key_pressed(byte key_code);

// Returns a key from buffer, or 0 if no key is available
// Does not block.
// If the key is an ASCII character, it is returned as is.
// If the key is a special key, it is returned as a key code shifted left by 8 bits.
// In addition, modifiers are returned in the high 8 bits, except for the case of
// alpha characters with a shift modifier, where the shift modifier is ignored,
// and the character is returned as a capital.
uint get_key();

