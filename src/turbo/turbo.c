#include <stdio.h>
#include <screen.h>
#include <keyboard.h>
#include <keycodes.h>
//#include <hal.h>
#include <vector.h>
#include <types.h>
#include <stdlib.h>
#include <memory.h>
#include <termios.h>
#include <unistd.h>
#include <time.h>

FILE *log_file=0;

#define CONTROL(x) (x - 0x40)
#define min(x,y)	((x) < (y) ? (x) : (y))
#define max(x,y)	((x) > (y) ? (x) : (y))

uint millis()
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (uint)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}

Color fg = 0xFFFF;
Color bg = 0x0000;
const byte TL = 0xC9;
const byte TR = 0xBB;
const byte BL = 0xC8;
const byte BR = 0xBC;
const byte HORZ = 0xCD;
const byte VERT = 0xBA;
uint W, H;
byte insert = 1;
Vector *document;
Vector *clipboard;

typedef struct _cursor
{
	short x, y;
} Cursor;

typedef struct _mapping
{
	short logical, visual;
} Mapping;

typedef struct _range
{
	byte start, stop;
} Range;

Mapping line_mapping[1024];

Cursor cursor, offset, select_start, select_stop;
short redraw_start, redraw_stop;

void init_state()
{
	fg = 0xFFFF;
	bg = 0;
	redraw_start = 0;
	redraw_stop = 0x7000;
	insert = 1;
	document = 0;
	cursor.x = 0;
	cursor.y = 0;
	offset.x = 0;
	offset.y = 0;
	select_start.x = 0;
	select_start.y = 0;
	select_stop.x = 0;
	select_stop.y = 0;
}

byte in_selection(short x, short y)
{
	if (y > select_start.y || (y == select_start.y && x >= select_start.x))
	{
		if (y < select_stop.y || (y == select_stop.y && x < select_stop.x))
		{
			return 1;
		}
	}
	return 0;
}

byte visual_mapping(Vector *line, Mapping *mapping, short start, short stop)
{
	short n = vector_size(line);
	const byte *data = vector_access(line, 0);
	short visual = -offset.x;
	short x = 0;

	byte index = 0;
	for (short logical = 0; logical < n; ++logical)
	{
		if (visual >= 0 && visual < (W - 2) && logical >= start && logical < stop)
		{
			mapping[index].logical = logical;
			mapping[index].visual = visual;
			index++;
		}
		++visual;
		++x;
		if (data[logical] == 9)
		{
			while ((x & 3) != 0)
			{
				++visual;
				++x;
			}
		}
	}
	return index;
}

short logical_to_visual(Vector *line, uint logical)
{
	uint n = min(logical, vector_size(line));
	const byte *data = vector_access(line, 0);
	short x = 0;
	for (uint i = 0; i < n; ++i)
	{
		++x;
		if (data[i] == 9)
		{
			while ((x & 3) != 0)
				++x;
		}
	}
	return x - offset.x;
}

byte cursor_lt(Cursor *a, Cursor *b)
{
	if (a->y == b->y)
		return a->x < b->x;
	return a->y < b->y;
}

byte valid_select()
{
	if (select_start.y == select_stop.y)
		return select_start.x < select_stop.x;
	return select_start.y < select_stop.y;
}

void redraw_line(short y)
{
	redraw_start = y;
	redraw_stop = y + 1;
}

void redraw_from(short y)
{
	redraw_start = y;
	redraw_stop = 0x7000;
}

void redraw_range(short y1, short y2)
{
	redraw_start = min(y1, y2);
	redraw_stop = max(y1, y2) + 1;
}

void clear_select()
{
	redraw_start = select_start.y;
	redraw_stop = select_stop.y + 1;
	select_start.x = 0;
	select_start.y = 0;
	select_stop.x = 0;
	select_stop.y = 0;
}

void start_select()
{
	select_stop = select_start = cursor;
}

void extend_select()
{
	if (cursor_lt(&cursor, &select_start))
		select_start = cursor;
	else
		select_stop = cursor;
}

void clear()
{
	uint n = vector_size(document);
	Vector *line;
	for (uint i = 0; i < n; ++i)
	{
		vector_get(document, i, &line);
		vector_shut(line);
	}
	vector_clear(document);
}

uint round_up(uint value, uint margin)
{
	return (value + margin) & ~(margin - 1);
}

Vector *get_line(uint line_index)
{
	uint size = vector_size(document);
	if (line_index < size)
	{
		Vector *line;
		vector_get(document, line_index, &line);
		return line;
	}
	vector_reserve(document, round_up(line_index + 1, 16));
	vector_resize(document, line_index + 1);
	uint new_size = vector_size(document);
	while (size < new_size)
	{
		Vector *new_line = vector_new(1);
		vector_set(document, size++, &new_line);
	}
	Vector *line;
	vector_get(document, line_index, &line);
	return line;
}

void append_line(Vector *line, byte b)
{
	vector_push(line, &b);
}

void load_file(const char *filename)
{
	byte buffer[4096];
	clear();
	FILE* file = fopen(filename, "r");
	if (!file)
		return;
	uint line_index = 0;
	while (fgets(buffer, sizeof(buffer), file))
	{
		Vector *line = get_line(line_index++);
		uint n = strlen(buffer);
		if (buffer[n - 1] == '\n')
		{
			buffer[n - 1] = 0;
			--n;
		}
		vector_resize(line, n);
		char *ptr = vector_access(line, 0);
		for (uint i = 0; i < n; ++i)
		{
			ptr[i] = buffer[i];
		}
	}
	fclose(file);
}

Color hal_fg = 0, hal_bg = 0;
uint hal_x = 0, hal_y = 0;

void hal_color(Color fg, Color bg)
{
	hal_fg = fg;
	hal_bg = bg;
}

void hal_move(uint x, uint y)
{
	hal_x = x;
	hal_y = y;
}

void hal_draw_char(char c)
{
	text_draw_char(hal_x, hal_y, c, hal_fg, hal_bg);
	hal_x++;
}

void hal_rept_char(char c, uint n)
{
	for (uint i = 0; i < n; ++i)
	{
		hal_draw_char(c);
	}
}

void draw_str(const char *s)
{
	for (; *s; ++s)
	{
		hal_draw_char(*s);
	}
}

Color RGB(uint r, uint g, uint b)
{
	r = ((r&255) >> 3);
	g = ((g&255) >> 2);
	b = ((b&255) >> 3);
	return (r<<11) | (g<<5) | b;
}

void draw_menu_item(const char *s)
{
	hal_color(RGB(168,0,0), RGB(168,168,168));
	hal_draw_char(' ');
	hal_draw_char(' ');
	hal_draw_char(*s);
	++s;
	if (*s)
	{
		hal_color(0, RGB(168,168,168));
		draw_str(s);
	}
}

void draw_status_item(const char *key, const char *name)
{
	hal_color(RGB(168, 0, 0), RGB(168, 168, 168));
	hal_draw_char(' ');
	draw_str(key);
	hal_draw_char(' ');
	hal_color(0, RGB(168, 168, 168));
	draw_str(name);
	hal_draw_char(' ');
}

void draw_menu()
{
	hal_move(0, 0);
	hal_color(0, RGB(168, 168, 168));
	hal_rept_char(' ', W);
	hal_move(0, 0);
	draw_menu_item("\xF0");
	draw_menu_item("File");
	draw_menu_item("Edit");
	draw_menu_item("Search");
	draw_menu_item("Run");
	draw_menu_item("Compile");
}

void draw_status()
{
	hal_move(0, H - 1);
	hal_color(0, RGB(168, 168, 168));
	hal_rept_char(' ', W);
	hal_move(0, H - 1);
	draw_status_item("F1", "Help");
	draw_status_item("F2", "Save");
	draw_status_item("F3", "Open");
	draw_status_item("F9", "Compile");
}

void draw_brackets(byte b)
{

	hal_draw_char('[');
	hal_color(RGB(84,252,84), RGB(0,0,168));
	hal_draw_char(b);
	hal_color(RGB(255, 255, 255), RGB(0, 0, 168));
	hal_draw_char(']');
}

void draw_frame()
{
	draw_menu();
	fg = RGB(255,255,255);
	bg = RGB(0,0,168);
	byte i;
	hal_color(RGB(255, 255, 255), RGB(0, 0, 168));
	hal_move(0, 1);
	hal_draw_char(TL);
	hal_draw_char(HORZ);
	draw_brackets(254);
	hal_rept_char(HORZ, W - 10);
	draw_brackets(0x12);
	hal_draw_char(HORZ);
	hal_draw_char(TR);

	// hal_draw_char(' ');
	// hal_draw_str("fib.sl");

	for (i = 2; i < (H - 2); ++i)
	{
		hal_color(RGB(255, 255, 255), RGB(0, 0, 168));
		hal_move(0, i);
		hal_draw_char(VERT);
		hal_rept_char(' ', W - 2);
		hal_color(RGB(0,0,168), RGB(0,168,168));
		if (i == 2)
			hal_draw_char(0x1E);
		else if (i == 3)
			hal_draw_char(0xFE);
		else if (i == (H - 3))
			hal_draw_char(0x1F);
		else
			hal_draw_char(0xB1);
		// hal_draw_char(VERT);
	}
	hal_color(RGB(255, 255, 255), RGB(0, 0, 168));
	hal_move(0, H - 2);
	hal_draw_char(BL);
	hal_rept_char(HORZ, 20);
	hal_color(RGB(0, 0, 168), RGB(0, 168, 168));
	hal_draw_char(0x11);
	hal_draw_char(0xFE);
	hal_rept_char(0xB1, W - 26);
	hal_draw_char(0x10);
	hal_color(RGB(84, 255, 84), RGB(0, 0, 168));
	hal_draw_char(0xC4);
	hal_draw_char(0xD9);
	// hal_draw_char(BR);
	draw_status();
	hal_color(fg, bg);
}

/**
 * Draw a single text line, from start character index to stop index.
 * Returns true if draw was successful (valid indices)
 */
byte draw_line(short logical_y)
{
	if (logical_y < 0 || logical_y >= vector_size(document))
		return 0;
	short visual_y = logical_y - offset.y;
	if (visual_y < 0 || visual_y >= (H - 4))
		return 0;

	fprintf(log_file, "draw_line %hd %hd  selection: %hd,%hd - %hd,%hd\n", logical_y, visual_y, 
		select_start.x, select_start.y, select_stop.x, select_stop.y);
	if (in_selection(0, logical_y))
		hal_color(bg, fg);
	Vector *line = get_line(logical_y);
	byte *data = vector_access(line, 0);
	byte n = 0;
	hal_move(1, visual_y + 2);
	if (data)
	{
		n = visual_mapping(line, line_mapping, 0, 0x7000);
	}
	else
	{
		hal_rept_char(' ', W - 2);
		return 1;
	}

	short x = 0, i = 0;
	while (x < (W - 2))
	{
		if (i < n)
		{
			if (x < line_mapping[i].visual)
			{
				byte m = (byte)(line_mapping[i].visual - x);
				hal_rept_char(' ', m);
				x = line_mapping[i].visual;
			}
			if (select_start.x == line_mapping[i].logical && select_start.y == logical_y)
				hal_color(bg, fg);
			if (select_stop.x == line_mapping[i].logical && select_stop.y == logical_y)
				hal_color(fg, bg);
			byte c = data[line_mapping[i].logical];
			hal_draw_char(c >= 32 ? c : ' ');
			++x;
			++i;
		}
		else
		{
			if (select_stop.y == logical_y)
				hal_color(fg, bg);
			hal_rept_char(' ', (byte)(W - 2 - x));
			break;
		}
	}
	return 1;
}

void visual_cursor(byte with_offset, Cursor *vc)
{
	Vector *line = get_line(cursor.y);
	short visual_x = logical_to_visual(line, cursor.x);
	vc->x = visual_x;
	vc->y = cursor.y;
	if (with_offset)
	{
		vc->x -= offset.x;
		vc->y -= offset.y;
	}
}

byte cursor_in_view()
{
	Cursor v;
	visual_cursor(1, &v);
	if (v.x >= 0 && v.y >= 0 && v.x < (W - 2) && v.y < (H - 4))
		return 1;
	return 0;
}

void eod_cursor(Cursor *c)
{
	c->x = 0;
	c->y = vector_size(document);
}

void redraw_all()
{
	redraw_start = 0;
	redraw_stop = 0x7000;
}

void calculate_new_offset()
{
	Cursor v;
	visual_cursor(0, &v);
	if (v.x < (W >> 1))
		v.x = 0;
	else
		offset.x = v.x - (W >> 1);
	if (v.y < (H >> 1))
		offset.y = 0;
	else
		offset.y = v.y - (H >> 1);
	draw_frame();
	redraw_all();
}

void place_cursor()
{
	if (cursor_in_view() == 0)
		calculate_new_offset();
	Cursor v;
	visual_cursor(1, &v);
	hal_move(v.x + 1, v.y + 2);
}

void decimal_string(byte *buffer, short length, uint value)
{
	sprintf(buffer, "%u", value);
}

void draw_number(uint value)
{
	byte buffer[8];
	buffer[7] = 0;
	decimal_string(buffer, 7, value);
	byte *ptr = buffer;
	for (; *ptr == 48; ++ptr)
		;
	for (; *ptr; ++ptr)
		hal_draw_char(*ptr);
}

void draw_cursor_position()
{
	hal_move(5, H - 2);
	// hal_draw_char(0xB9);
	hal_draw_char(' ');
	draw_number(cursor.x + 1);
	hal_draw_char(':');
	draw_number(cursor.y + 1);
	hal_draw_char(' ');
	// hal_draw_char(0xCC);
	for (byte i = 0; i < 5; ++i)
		hal_draw_char(HORZ);
}

void draw_screen()
{
	if (offset.y > redraw_start)
	{
		redraw_start = offset.y;
	}
	for (short y = redraw_start; y < redraw_stop; ++y)
	{
		if (!draw_line(y))
			break;
	}
	redraw_stop = redraw_start;
	draw_cursor_position();
	place_cursor();
}

uint visual_to_logical(Vector *line, uint visual)
{
	uint n = vector_size(line);
	const byte *data = vector_access(line, 0);
	uint x = 0;
	for (uint i = 0; i < n; ++i)
	{
		if (x >= visual)
			return i;
		++x;
		if (data[i] == 9)
		{
			while ((x & 3) != 0)
				++x;
		}
	}
	return n;
}

void move_x_cursor(short dx)
{
	if (dx == 0 || cursor.y < 0 || cursor.y >= vector_size(document))
		return;
	Vector *line = get_line(cursor.y);
	cursor.x += dx;
	if (cursor.x < 0)
		cursor.x = 0;
	short n = vector_size(line);
	if (cursor.x >= n)
		cursor.x = n;
	place_cursor();
}

byte is_move_key(uint key)
{
	if (key >= 0x100)
	{
		key >>= 8;
		key &= 0xFFFF;
		return (key == KEY_LEFT || key == KEY_RIGHT || key == KEY_UP || key == KEY_DOWN ||
				key == KEY_PAGEUP || key == KEY_PAGEDOWN || key == KEY_HOME || key == KEY_END ||
				key == KEY_INSERT || key == KEY_DELETE);
	}
	return 0;
}

void join_prev_line()
{
	Vector *prev_line = get_line(cursor.y - 1);
	Vector *cur_line = get_line(cursor.y);
	uint m = vector_size(prev_line);
	uint n = vector_size(cur_line);
	byte *data = vector_access(cur_line, 0);
	for (uint i = 0; i < n; ++i)
		vector_push(prev_line, &data[i]);
	vector_shut(cur_line);
	vector_erase(document, cursor.y);
	cursor.y--;
	cursor.x = logical_to_visual(prev_line, m);
}

byte delete_single_line_selection()
{
	Vector *line = get_line(select_start.y);
	vector_erase_range(line, select_start.x, select_stop.x);
	cursor = select_start;
	redraw_line(select_start.y);
	select_stop = select_start;
	return 1;
}

byte delete_selection()
{
	if (select_start.y == select_stop.y)
		return delete_single_line_selection();
	Vector *line = get_line(select_start.y);
	uint n = vector_size(line);
	vector_erase_range(line, select_start.x, n);
	if ((select_stop.y - select_start.y) > 1)
	{
		for (short y = select_start.y + 1; y < select_stop.y; ++y)
		{
			line = get_line(y);
			vector_shut(line);
		}
		vector_erase_range(document, select_start.y + 1, select_stop.y);
		select_stop.y = select_start.y + 1;
	}
	if (select_stop.x > 0)
	{
		line = get_line(select_stop.y);
		n = vector_size(line);
		vector_erase_range(line, 0, select_stop.x);
	}
	if (select_stop.y > select_start.y)
	{
		cursor.y = select_stop.y;
		cursor.x = 0;
		join_prev_line();
	}
	cursor = select_start;
	redraw_start = 0;
	redraw_stop = 0x7000;
	select_stop = select_start;
	return 0;
}

byte do_delete()
{
	if (valid_select())
		return delete_selection();
	Vector *line = get_line(cursor.y);
	if (cursor.x < vector_size(line))
	{
		vector_erase(line, cursor.x);
	}
	else if (cursor.y < (vector_size(document) - 1))
	{
		Vector *next_line = get_line(cursor.y + 1);
		uint n = vector_size(next_line);
		byte *data = vector_access(next_line, 0);
		for (uint j = 0; j < n; ++j)
			vector_push(line, &data[j]);
		vector_shut(next_line);
		vector_erase(document, cursor.y + 1);
		return 0; /* Indicate redraw from line to end of screen */
	}
	return 1;
}

void backspace()
{
	move_x_cursor(-1);
	do_delete();
}

void add_enter()
{
	if (insert)
	{
		Vector *cur_line = get_line(cursor.y);
		uint n = vector_size(cur_line);
		byte *data = vector_access(cur_line, 0);
		Vector *next_line = vector_new(1);
		vector_insert(document, cursor.y + 1, &next_line);
		for (uint j = cursor.x; j < n; ++j)
			vector_push(next_line, &data[j]);
		vector_erase_range(cur_line, cursor.x, n);
		cursor.y++;
		cursor.x = 0;
	}
}

void add_char(byte c)
{
	if (c == 10 || c == 13)
	{
		add_enter();
	}
	else
	{
		Vector *line = get_line(cursor.y);
		if (cursor.x >= vector_size(line))
			vector_push(line, &c);
		else
		{
			if (insert)
			{
				vector_insert(line, cursor.x, &c);
			}
			else
			{
				vector_set(line, cursor.x, &c);
			}
		}
		++cursor.x;
	}
}

static bool blink_state = 0;
static uint last_blink_ts = 0;
static bool blink_drawn = 0;
void hal_blink(bool state)
{
	blink_state = state;
}

void draw_blink()
{
	Cursor v;
	visual_cursor(1, &v);
	int x = (v.x+1)*8;
	int y = (v.y+2)*16;
	screen_xor_rect(x,y,8,16,0xFFFF);
}

void reset_blink()
{
	if (blink_drawn)
		draw_blink();
	blink_drawn = 0;
}

void process_blink()
{
	if (!blink_state)
		return;
	uint ts = millis();
	if (ts-last_blink_ts > 500)
	{
		last_blink_ts = ts;
		draw_blink();
		blink_drawn ^= 1;
	}
}

uint getkey()
{
	return get_key();
}

void move_y(short dy)
{
	Vector *line = get_line(cursor.y);
	short vx = logical_to_visual(line, cursor.x);
	cursor.y += dy;
	line = get_line(cursor.y);
	cursor.x = visual_to_logical(line, vx);
}

void clipboard_copy()
{
	if (!valid_select())
		return;
	vector_clear(clipboard);
	for (short y = select_start.y; y <= select_stop.y; ++y)
	{
		Vector *line = get_line(y);
		if (!line)
			continue;
		short start = 0, stop = vector_size(line);
		if (y == select_start.y)
		{
			start = select_start.x;
		}
		if (y == select_stop.y && select_stop.x < stop)
			stop = select_stop.x;
		byte *data = vector_access(line, 0);
		for (short x = start; x < stop; ++x)
		{
			vector_push(clipboard, &(data[x]));
		}
		if (y < select_stop.y)
		{
			byte enter = 10;
			vector_push(clipboard, &enter);
		}
	}
}

void clipboard_cut()
{
	if (!valid_select())
		return;
	clipboard_copy();
	do_delete();
}

void clipboard_paste()
{
	if (valid_select())
		do_delete();
	uint n = vector_size(clipboard);
	const byte *data = vector_access(clipboard, 0);
	for (uint i = 0; i < n; ++i)
	{
		add_char(data[i]);
	}
	redraw_start = 0;
	redraw_stop = 0x7000;
}

#define SPECIAL(x) (KEY_##x << 8)

void event_loop()
{
	byte done = 0;
	const uint total_lines = vector_size(document);
	while (!done)
	{
		Cursor prev_cursor;
		prev_cursor.x = cursor.x;
		prev_cursor.y = cursor.y;
		process_blink();
		Vector *line = get_line(cursor.y);
		uint n = vector_size(line);
		poll_keyboard_event();
		uint key = getkey();
		if (key==0)
		{
			usleep(1000);
			continue;
		}
		reset_blink();
		bool moving = is_move_key(key);
		bool shifted = ((key & MODIFIER_SHIFT) != 0 ? 1 : 0);
		bool ctrl = ((key & MODIFIER_CTRL) != 0 ? 1 : 0);
		bool alt = ((key & MODIFIER_ALT) != 0 ? 1 : 0);
		key &= ~(MODIFIER_CTRL | MODIFIER_ALT | MODIFIER_SHIFT);
		if (moving)
		{
			if (shifted)
			{
				if (!valid_select())
					start_select();
			}
			else
				clear_select();
		}
		switch (key)
		{
		case SPECIAL(LEFT):
			if (cursor.x > 0)
				move_x_cursor(-1);
			break;
		case SPECIAL(RIGHT):
			move_x_cursor(1);
			break;
		case SPECIAL(UP):
		{
			if (ctrl)
			{
				if (offset.y > 0)
				{
					--offset.y;
					redraw_all();
				}
			}
			else
			{
				if (cursor.y > 0)
					move_y(-1);
			}
			break;
		}
		case SPECIAL(DOWN):
		{
			if (ctrl)
			{
				if (offset.y < vector_size(document))
				{
					++offset.y;
					if (!cursor_in_view())
						move_y(1);
					redraw_all();
				}
			}
			else
			{
				if (cursor.y < total_lines)
					move_y(1);
			}
			break;
		}
		case SPECIAL(HOME):
		{
			if (ctrl)
			{
				cursor.y = 0;
				cursor.x = 0;
			}
			else
				cursor.x = 0;
			break;
		}
		case SPECIAL(END):
		{
			if (ctrl)
			{
				cursor.y = vector_size(document) - 1;
				Vector *end_line = get_line(cursor.y);
				cursor.x = vector_size(end_line);
			}
			else
				cursor.x = n;
			break;
		}
		case SPECIAL(PAGEUP):
			if (cursor.y < 25)
				cursor.y = 0;
			else
				cursor.y -= 25;
			break;
		case SPECIAL(PAGEDOWN):
			cursor.y = min(cursor.y + 25, total_lines);
			break;
		case SPECIAL(ESC):
			done = 1;
			break;
		case SPECIAL(INSERT):
		{
			if (ctrl)
				clipboard_copy();
			else if (shifted)
				clipboard_paste();
			break;
		}
		/*
		case CONTROL(C):
			clipboard_copy();
			break;
		case CONTROL(X):
			clipboard_cut();
			break;
		case CONTROL(V):
			clipboard_paste();
			break;
		*/
		case SPECIAL(TAB):
		{
			add_char(9);
			redraw_line(cursor.y);
			break;
		}
		case SPECIAL(BACKSPACE):
		{
			if (cursor.x > 0)
			{
				backspace();
				redraw_line(cursor.y);
			}
			else if (cursor.y > 0)
			{
				join_prev_line();
			}
			break;
		}
		case SPECIAL(DELETE):
		{
			if (ctrl)
				clipboard_cut();
			else if (do_delete())
				redraw_line(cursor.y);
			else
			{
				redraw_from(cursor.y);
			}
			break;
		}
		case SPECIAL(ENTER):
		{
			add_enter();
			redraw_from(prev_cursor.y);
		}
		default:
		{
			if (key >= 32 && key < 127 && !ctrl && !alt)
			{
				add_char((byte)key);
				redraw_line(cursor.y);
			}
		}
		}
		line = get_line(cursor.y);
		n = vector_size(line);
		if (cursor.x > n)
			cursor.x = n;
		if (moving && shifted)
		{
			extend_select();
			redraw_range(prev_cursor.y, cursor.y);
		}
		place_cursor();
		draw_screen();
		draw_cursor_position();
		place_cursor();
	}
}

void disable_console()
{
	// Disable cursor
	printf("\x1b[?25l");
	fflush(stdout);
	// Disable echo
	struct termios t;
	tcgetattr(STDIN_FILENO, &t);
	t.c_lflag &= ~ECHO; // Disable echo
	tcsetattr(STDIN_FILENO, TCSANOW, &t);
}

void enable_console()
{
	// Enable echo
	struct termios t;
	tcgetattr(STDIN_FILENO, &t);
	t.c_lflag |= ECHO; // Enable echo
	tcsetattr(STDIN_FILENO, TCSANOW, &t);
	// Enable cursor
	printf("\x1b[?25h");
	fflush(stdout);
}

int main(int argc, char *argv[])
{
	log_file = fopen("/tmp/log.txt", "w");
	fprintf(log_file,"Turbo Editor\n");
	fflush(log_file);
	disable_console();
	if (!screen_init())
	{
		printf("Failed to open screen\n");
		fflush(stdout);
		return -1;
	}
	W = screen_get_cols();
	H = screen_get_rows();
	if (open_keyboard() < 0)
	{
		printf("Failed to open keyboard\n");
		fflush(stdout);
		return -1;
	}
	init_state();
	clipboard = vector_new(1);
	vector_reserve(clipboard, 256);
	document = vector_new(sizeof(Vector *));
	if (argc>1)
	{
		load_file(argv[1]);
	}
	fg = 0xFFFF;
	bg = 0x0;
	draw_frame();
	draw_screen();
	hal_blink(1);
	event_loop();
	clear();
	vector_shut(document);
	close_keyboard();
	screen_clear(0);
	screen_shut();
	enable_console();
	fclose(log_file);
	return 0;
}

