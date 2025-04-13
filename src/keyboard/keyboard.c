#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/input.h>
#include <errno.h>
#include <keyboard.h>
#include <keycodes.h>
#include <circular.h>

#define EVENT_DEV_PATH "/dev/input/event"
#define EVENT_DEV_MAX  32
#define DEVICE_NAME_LEN 256

static char keys_state[256];
static Circular* key_buffer = 0;
static char device_path[DEVICE_NAME_LEN];
static uint modifiers = 0;

typedef struct key_mapping
{
	char ascii;
	char shifted_ascii;
} KeyMapping;

static KeyMapping mappings_lut[256];

static const byte ascii_mapping[] = {
	KEY_1, '1', '!',
	KEY_2, '2', '@',
	KEY_3, '3', '#',
	KEY_4, '4', '$',
	KEY_5, '5', '%',
	KEY_6, '6', '^',
	KEY_7, '7', '&',
	KEY_8, '8', '*',
	KEY_9, '9', '(',
	KEY_0, '0', ')',
	KEY_MINUS, '-', '_',
	KEY_EQUAL, '=', '+',
	KEY_Q, 'q', 'Q',
	KEY_W, 'w', 'W',
	KEY_E, 'e', 'E',
	KEY_R, 'r', 'R',
	KEY_T, 't', 'T',
	KEY_Y, 'y', 'Y',
	KEY_U, 'u', 'U',
	KEY_I, 'i', 'I',
	KEY_O, 'o', 'O',
	KEY_P, 'p', 'P',
	KEY_LEFTBRACE, '[', '{',
	KEY_RIGHTBRACE, ']', '}',
	KEY_A, 'a', 'A',
	KEY_S, 's', 'S',
	KEY_D, 'd', 'D',
	KEY_F, 'f', 'F',
	KEY_G, 'g', 'G',
	KEY_H, 'h', 'H',
	KEY_J, 'j', 'J',
	KEY_K, 'k', 'K',
	KEY_L, 'l', 'L',
	KEY_SEMICOLON, ';', ':',
	KEY_APOSTROPHE, '\'', '"',
	KEY_GRAVE, '`', '~',
	KEY_BACKSLASH, '\\', '|',
	KEY_Z, 'z', 'Z',
	KEY_X, 'x', 'X',
	KEY_C, 'c', 'C',
	KEY_V, 'v', 'V',
	KEY_B, 'b', 'B',
	KEY_N, 'n', 'N',
	KEY_M, 'm', 'M',
	KEY_COMMA, ',', '<',
	KEY_DOT, '.', '>',
	KEY_SLASH, '/', '?',
	KEY_SPACE, ' ', ' '
};

int find_keyboard()
{
	char device_name[DEVICE_NAME_LEN];
	int fd;
	int rc = -1;

	for (int i = 0; i < EVENT_DEV_MAX && rc<0; i++) {
		snprintf(device_path, sizeof(device_path), "%s%d", EVENT_DEV_PATH, i);

		fd = open(device_path, O_RDONLY);
		if (fd < 0) {
			if (errno != ENOENT) {
				perror("Error opening device");
			}
			continue;
		}

		if (ioctl(fd, EVIOCGNAME(sizeof(device_name)), device_name) < 0) {
			perror("Error getting device name");
			close(fd);
			continue;
		}

		if (strstr(device_name, "Keyboard") != NULL)
		{
			rc = 0;
		}

		//printf("Device: %s - %s\n", device_path, device_name);
		close(fd);
	}
	return rc;
}

static int kbd_fd=-1;

int open_keyboard()
{
	int rc = find_keyboard();
	if (rc < 0) {
		fprintf(stderr, "No keyboard found\n");
		return -1;
	}

	kbd_fd = open(device_path, O_RDONLY);
	if (kbd_fd < 0) {
		perror("Error opening keyboard device");
		return -1;
	}

	int flags = fcntl(kbd_fd, F_GETFL, 0);
	if (flags == -1) {
		perror("fcntl F_GETFL");
		close_keyboard();
		return -1;
	}

	flags |= O_NONBLOCK;
	if (fcntl(kbd_fd, F_SETFL, flags) == -1) {
		perror("fcntl F_SETFL O_NONBLOCK");
		close_keyboard();
		return -1;
	}
	key_buffer = circular_new(256,sizeof(uint));
	for(int i=0; i<256; i++)
	{
		keys_state[i] = 0;
		mappings_lut[i].ascii = 0;
		mappings_lut[i].shifted_ascii = 0;
	}
	{
		int n=sizeof(ascii_mapping)/sizeof(byte);
		for (int i = 0; i < n;i+=3)
		{
			mappings_lut[ascii_mapping[i]].ascii = ascii_mapping[i+1];
			mappings_lut[ascii_mapping[i]].shifted_ascii = ascii_mapping[i+2];
		}
	}

	return 0;
}

void close_keyboard()
{
	if (kbd_fd >= 0) {
		close(kbd_fd);
		kbd_fd = -1;
	}
}

static uint translate_key(unsigned short code)
{
	if (code<256)
	{
		KeyMapping* mapping = &mappings_lut[code];
		bool special = 0;
		if (mapping->ascii == 0) special = 1;
		if (modifiers & MODIFIER_CTRL) special = 1;
		if (modifiers & MODIFIER_ALT) special = 1;
		if (special)
		{	
			return (code << 8) | modifiers; // Special key
		}
		if (modifiers & MODIFIER_SHIFT)
		{
			return mapping->shifted_ascii;
		}
		return mapping->ascii;
	}
	else
	{
		return 0;
	}
}

static void update_modifiers()
{
	modifiers = 0;
	if (keys_state[KEY_LEFTCTRL] || keys_state[KEY_RIGHTCTRL])
		modifiers |= MODIFIER_CTRL;
	if (keys_state[KEY_LEFTALT] || keys_state[KEY_RIGHTALT])
		modifiers |= MODIFIER_ALT;
	if (keys_state[KEY_LEFTSHIFT] || keys_state[KEY_RIGHTSHIFT])
		modifiers |= MODIFIER_SHIFT;
}

static void	process_event(struct input_event *ev)
{
	if (ev->type == EV_KEY)
	{
		if (ev->value == 0) // Key released
		{
			keys_state[ev->code] = 0;
			update_modifiers();
		}
		else if (ev->value == 1) // Key pressed
		{
			keys_state[ev->code] = 1;
			update_modifiers();
			uint tranlated_key = translate_key(ev->code);
			if (tranlated_key>0)
				circular_write(key_buffer, &tranlated_key);
		}
	}
}

int poll_keyboard_event()
{
	if (kbd_fd < 0)
	{
		return -1;
	}
	int events = 0;

	while (1)
	{
		struct input_event ev;
		ssize_t bytes = read(kbd_fd, &ev, sizeof(struct input_event));
		
		if (bytes < 0)
		{
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				return events; // No event available
			}
			perror("Error reading keyboard event");
			return -1;
		}

		if (bytes != sizeof(struct input_event))
		{
			fprintf(stderr, "Unexpected size of keyboard event: %zd\n", bytes);
			return -1;
		}
		process_event(&ev);
		events++;
	}
	return events;
}

bool is_key_pressed(byte key_code)
{
	if (key_code >= 0 && key_code < 256)
	{
		return keys_state[key_code] != 0;
	}
	return 0;
}

uint get_key()
{
	uint key = 0;
	circular_read(key_buffer, &key);
	return key;
}


