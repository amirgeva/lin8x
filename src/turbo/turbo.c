#include <stdio.h>
#include <screen.h>
#include <keyboard.h>
#include <keycodes.h>
//#include <hal.h>
#include <vector.h>
#include <types.h>
#include <utils.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>
#include <time.h>

FILE *log_file=0;

#define CONTROL(x) (x - 0x40)
#define min(x,y)	((x) < (y) ? (x) : (y))
#define max(x,y)	((x) > (y) ? (x) : (y))

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
char current_filename[256];

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
	current_filename[0] = 0;
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
	return x;
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

/* forward declarations */
void hal_color(Color fg, Color bg);
void hal_move(uint x, uint y);
void hal_draw_char(char c);
void hal_rept_char(char c, uint n);
void draw_str(const char *s);
void draw_frame();
void redraw_all();
uint getkey();
void clipboard_copy();
void clipboard_cut();
void clipboard_paste();
#define SPECIAL(x) (KEY_##x << 8)

void save_file(const char *filename)
{
	FILE *file = fopen(filename, "w");
	if (!file) return;
	uint n = vector_size(document);
	for (uint i = 0; i < n; ++i)
	{
		Vector *line;
		vector_get(document, i, &line);
		uint len = vector_size(line);
		if (len > 0)
		{
			byte *data = vector_access(line, 0);
			fwrite(data, 1, len, file);
		}
		fputc('\n', file);
	}
	fclose(file);
}

/* ---- Dropdown menu system ---- */

#define MENU_BOX_W 20
#define MENU_FG RGB(0, 0, 0)
#define MENU_BG RGB(168, 168, 168)
#define MENU_SEL_FG RGB(255, 255, 255)
#define MENU_SEL_BG RGB(0, 0, 0)
#define MENU_HOT RGB(168, 0, 0)

static void draw_menu_box(uint x, uint y, uint w, uint h)
{
	hal_color(MENU_FG, MENU_BG);
	for (uint row = 0; row < h; row++)
	{
		hal_move(x, y + row);
		hal_rept_char(' ', w);
	}
}

static void draw_menu_entry(uint x, uint y, const char *text, const char *shortcut, byte selected)
{
	if (text == NULL)
	{
		/* separator */
		hal_color(MENU_FG, MENU_BG);
		hal_move(x, y);
		hal_draw_char(0xCC);
		hal_rept_char(0xCD, MENU_BOX_W - 2);
		hal_draw_char(0xB9);
		return;
	}
	hal_color(MENU_FG, MENU_BG);
	hal_move(x, y);
	hal_draw_char(0xBA);
	if (selected)
		hal_color(MENU_SEL_FG, MENU_SEL_BG);
	hal_draw_char(' ');
	/* draw text with first char highlighted */
	if (!selected)
		hal_color(MENU_HOT, MENU_BG);
	hal_draw_char(text[0]);
	if (selected)
		hal_color(MENU_SEL_FG, MENU_SEL_BG);
	else
		hal_color(MENU_FG, MENU_BG);
	for (const char *p = text + 1; *p; p++)
		hal_draw_char(*p);
	/* pad and draw shortcut */
	uint text_len = strlen(text);
	uint sc_len = shortcut ? strlen(shortcut) : 0;
	uint pad = MENU_BOX_W - 4 - text_len - sc_len;
	hal_rept_char(' ', pad);
	if (shortcut)
	{
		for (const char *p = shortcut; *p; p++)
			hal_draw_char(*p);
	}
	hal_draw_char(' ');
	hal_color(MENU_FG, MENU_BG);
	hal_draw_char(0xBA);
}

static void draw_menu_border_top(uint x, uint y)
{
	hal_color(MENU_FG, MENU_BG);
	hal_move(x, y);
	hal_draw_char(0xC9);
	hal_rept_char(0xCD, MENU_BOX_W - 2);
	hal_draw_char(0xBB);
}

static void draw_menu_border_bottom(uint x, uint y)
{
	hal_color(MENU_FG, MENU_BG);
	hal_move(x, y);
	hal_draw_char(0xC8);
	hal_rept_char(0xCD, MENU_BOX_W - 2);
	hal_draw_char(0xBC);
}

typedef struct {
	const char *text;
	const char *shortcut;
} MenuItem;

static int run_dropdown(uint menu_x, uint menu_y, MenuItem *items, int count)
{
	int sel = 0;
	/* count non-separator items to skip separators */
	uint box_h = count + 2;
	draw_menu_border_top(menu_x, menu_y);
	for (int i = 0; i < count; i++)
		draw_menu_entry(menu_x, menu_y + 1 + i, items[i].text, items[i].shortcut, i == sel);
	draw_menu_border_bottom(menu_x, menu_y + count + 1);

	while (1)
	{
		poll_keyboard_event();
		uint key = getkey();
		if (key == 0)
		{
			usleep(1000);
			continue;
		}
		key &= ~(MODIFIER_CTRL | MODIFIER_ALT | MODIFIER_SHIFT);
		if (key == SPECIAL(ESC))
			return -1;
		if (key == SPECIAL(ENTER))
			return sel;
		if (key == SPECIAL(UP))
		{
			do {
				sel = (sel + count - 1) % count;
			} while (items[sel].text == NULL);
		}
		else if (key == SPECIAL(DOWN))
		{
			do {
				sel = (sel + 1) % count;
			} while (items[sel].text == NULL);
		}
		/* redraw entries */
		for (int i = 0; i < count; i++)
			draw_menu_entry(menu_x, menu_y + 1 + i, items[i].text, items[i].shortcut, i == sel);
	}
}

/* ---- File dialog (Turbo C style) ---- */

#define DLG_W 50
#define DLG_H 20
#define DLG_LIST_H 12
#define DLG_MAX_FILES 256
#define DLG_NAME_MAX 64

#define DLG_CYAN   RGB(0, 168, 168)
#define DLG_DCYAN  RGB(0, 84, 84)
#define DLG_WHITE  RGB(255, 255, 255)
#define DLG_BLACK  RGB(0, 0, 0)
#define DLG_YELLOW RGB(255, 255, 84)
#define DLG_GREEN  RGB(84, 255, 84)

static void dlg_box(uint x, uint y, uint w, uint h, Color fg, Color bg)
{
	hal_color(fg, bg);
	hal_move(x, y);
	hal_draw_char(0xC9);
	hal_rept_char(0xCD, w - 2);
	hal_draw_char(0xBB);
	for (uint i = 1; i < h - 1; i++)
	{
		hal_move(x, y + i);
		hal_draw_char(0xBA);
		hal_rept_char(' ', w - 2);
		hal_draw_char(0xBA);
	}
	hal_move(x, y + h - 1);
	hal_draw_char(0xC8);
	hal_rept_char(0xCD, w - 2);
	hal_draw_char(0xBC);
}

static void dlg_inner_box(uint x, uint y, uint w, uint h, Color fg, Color bg)
{
	hal_color(fg, bg);
	hal_move(x, y);
	hal_draw_char(0xDA);
	hal_rept_char(0xC4, w - 2);
	hal_draw_char(0xBF);
	for (uint i = 1; i < h - 1; i++)
	{
		hal_move(x, y + i);
		hal_draw_char(0xB3);
		hal_rept_char(' ', w - 2);
		hal_draw_char(0xB3);
	}
	hal_move(x, y + h - 1);
	hal_draw_char(0xC0);
	hal_rept_char(0xC4, w - 2);
	hal_draw_char(0xD9);
}

static void dlg_label(uint x, uint y, const char *text, Color fg, Color bg)
{
	hal_color(fg, bg);
	hal_move(x, y);
	for (const char *p = text; *p; p++)
		hal_draw_char(*p);
}

static void dlg_button(uint x, uint y, const char *text, byte selected)
{
	if (selected)
		hal_color(DLG_BLACK, DLG_GREEN);
	else
		hal_color(DLG_BLACK, DLG_CYAN);
	hal_move(x, y);
	hal_draw_char(' ');
	for (const char *p = text; *p; p++)
		hal_draw_char(*p);
	hal_draw_char(' ');
}

typedef struct {
	char name[DLG_NAME_MAX];
	byte is_dir;
} FileEntry;

static int file_entry_cmp(const void *a, const void *b)
{
	const FileEntry *fa = (const FileEntry *)a;
	const FileEntry *fb = (const FileEntry *)b;
	/* directories first */
	if (fa->is_dir != fb->is_dir)
		return fb->is_dir - fa->is_dir;
	return strcmp(fa->name, fb->name);
}

static int scan_directory(const char *path, FileEntry *entries, int max)
{
	DIR *dir = opendir(path);
	if (!dir) return 0;
	int count = 0;
	/* add parent directory entry */
	strcpy(entries[count].name, "..");
	entries[count].is_dir = 1;
	count++;
	struct dirent *ent;
	while ((ent = readdir(dir)) != NULL && count < max)
	{
		if (ent->d_name[0] == '.')
			continue;
		strncpy(entries[count].name, ent->d_name, DLG_NAME_MAX - 1);
		entries[count].name[DLG_NAME_MAX - 1] = 0;
		char full[512];
		snprintf(full, sizeof(full), "%s/%s", path, ent->d_name);
		struct stat st;
		entries[count].is_dir = 0;
		if (stat(full, &st) == 0 && S_ISDIR(st.st_mode))
			entries[count].is_dir = 1;
		count++;
	}
	closedir(dir);
	qsort(entries, count, sizeof(FileEntry), file_entry_cmp);
	return count;
}

/* focus: 0=name field, 1=file list, 2=Open button, 3=Cancel button */
#define FOCUS_NAME 0
#define FOCUS_LIST 1
#define FOCUS_OK   2
#define FOCUS_CANCEL 3

static int input_dialog(const char *title, char *buf, int bufsize)
{
	uint dx = (W - DLG_W) / 2;
	uint dy = (H - DLG_H) / 2;
	uint name_x = dx + 2;
	uint name_y = dy + 2;
	uint name_w = DLG_W - 4;
	uint list_x = dx + 2;
	uint list_y = dy + 5;
	uint list_w = DLG_W - 4;
	uint btn_y = dy + DLG_H - 2;

	FileEntry files[DLG_MAX_FILES];
	char cwd[256];
	getcwd(cwd, sizeof(cwd));
	int file_count = scan_directory(cwd, files, DLG_MAX_FILES);
	int list_sel = 0;
	int list_scroll = 0;
	int focus = FOCUS_NAME;
	int name_len = strlen(buf);

	while (1)
	{
		/* draw dialog */
		dlg_box(dx, dy, DLG_W, DLG_H, DLG_YELLOW, DLG_DCYAN);
		/* title */
		uint title_x = dx + (DLG_W - strlen(title) - 2) / 2;
		hal_color(DLG_YELLOW, DLG_DCYAN);
		hal_move(title_x, dy);
		hal_draw_char(' ');
		draw_str(title);
		hal_draw_char(' ');

		/* Name label and field */
		dlg_label(name_x, dy + 1, "Name", DLG_YELLOW, DLG_DCYAN);
		if (focus == FOCUS_NAME)
			hal_color(DLG_BLACK, DLG_WHITE);
		else
			hal_color(DLG_WHITE, DLG_BLACK);
		hal_move(name_x, name_y);
		hal_rept_char(' ', name_w);
		hal_move(name_x, name_y);
		for (int i = 0; i < name_len && i < name_w; i++)
			hal_draw_char(buf[i]);

		/* File list label */
		dlg_label(list_x, dy + 3, "Files", DLG_YELLOW, DLG_DCYAN);
		/* cwd display */
		hal_color(DLG_WHITE, DLG_DCYAN);
		hal_move(list_x + 7, dy + 3);
		{
			int cw = DLG_W - 12;
			int cl = strlen(cwd);
			const char *show = cwd;
			if (cl > cw) show = cwd + cl - cw;
			for (const char *p = show; *p; p++)
				hal_draw_char(*p);
		}

		/* File list box */
		dlg_inner_box(list_x, list_y - 1, list_w, DLG_LIST_H + 2, DLG_WHITE, DLG_DCYAN);
		for (int i = 0; i < DLG_LIST_H; i++)
		{
			int fi = list_scroll + i;
			hal_move(list_x + 1, list_y + i);
			if (fi < file_count)
			{
				byte sel = (focus == FOCUS_LIST && fi == list_sel);
				if (sel)
					hal_color(DLG_BLACK, DLG_CYAN);
				else
					hal_color(DLG_WHITE, DLG_DCYAN);
				int nlen = strlen(files[fi].name);
				int avail = list_w - 2;
				for (int j = 0; j < avail; j++)
				{
					if (j < nlen)
						hal_draw_char(files[fi].name[j]);
					else if (j == nlen && files[fi].is_dir)
						hal_draw_char('/');
					else
						hal_draw_char(' ');
				}
			}
			else
			{
				hal_color(DLG_WHITE, DLG_DCYAN);
				hal_rept_char(' ', list_w - 2);
			}
		}

		/* Buttons */
		dlg_button(dx + DLG_W / 2 - 12, btn_y, "Open", focus == FOCUS_OK);
		dlg_button(dx + DLG_W / 2 + 4, btn_y, "Cancel", focus == FOCUS_CANCEL);

		/* scrollbar indicator */
		if (file_count > DLG_LIST_H)
		{
			hal_color(DLG_WHITE, DLG_DCYAN);
			uint sb_pos = list_sel * (DLG_LIST_H - 1) / (file_count - 1);
			for (int i = 0; i < DLG_LIST_H; i++)
			{
				hal_move(list_x + list_w - 1, list_y + i);
				hal_draw_char(i == sb_pos ? 0xDB : 0xB1);
			}
		}

		/* wait for key */
		uint key = 0;
		while (key == 0)
		{
			poll_keyboard_event();
			key = getkey();
			if (key == 0) usleep(1000);
		}
		uint raw = key & ~(MODIFIER_CTRL | MODIFIER_ALT | MODIFIER_SHIFT);

		if (raw == SPECIAL(ESC))
			return -1;

		if (raw == SPECIAL(TAB))
		{
			focus = (focus + 1) % 4;
			continue;
		}

		if (focus == FOCUS_NAME)
		{
			if (raw == SPECIAL(ENTER))
			{
				buf[name_len] = '\0';
				return 0;
			}
			else if (raw == SPECIAL(BACKSPACE))
			{
				if (name_len > 0) buf[--name_len] = '\0';
			}
			else if (key >= 32 && key < 127 && name_len < bufsize - 1)
			{
				buf[name_len++] = (char)key;
				buf[name_len] = '\0';
			}
		}
		else if (focus == FOCUS_LIST)
		{
			if (raw == SPECIAL(UP) && list_sel > 0)
			{
				list_sel--;
				if (list_sel < list_scroll)
					list_scroll = list_sel;
			}
			else if (raw == SPECIAL(DOWN) && list_sel < file_count - 1)
			{
				list_sel++;
				if (list_sel >= list_scroll + DLG_LIST_H)
					list_scroll = list_sel - DLG_LIST_H + 1;
			}
			else if (raw == SPECIAL(PAGEUP))
			{
				list_sel -= DLG_LIST_H;
				if (list_sel < 0) list_sel = 0;
				list_scroll = list_sel;
			}
			else if (raw == SPECIAL(PAGEDOWN))
			{
				list_sel += DLG_LIST_H;
				if (list_sel >= file_count) list_sel = file_count - 1;
				if (list_sel >= list_scroll + DLG_LIST_H)
					list_scroll = list_sel - DLG_LIST_H + 1;
			}
			else if (raw == SPECIAL(ENTER))
			{
				if (list_sel < file_count && files[list_sel].is_dir)
				{
					/* navigate into directory */
					chdir(files[list_sel].name);
					getcwd(cwd, sizeof(cwd));
					file_count = scan_directory(cwd, files, DLG_MAX_FILES);
					list_sel = 0;
					list_scroll = 0;
				}
				else if (list_sel < file_count)
				{
					/* select file */
					strncpy(buf, files[list_sel].name, bufsize - 1);
					buf[bufsize - 1] = 0;
					name_len = strlen(buf);
					return 0;
				}
			}
		}
		else if (focus == FOCUS_OK)
		{
			if (raw == SPECIAL(ENTER))
			{
				buf[name_len] = '\0';
				return 0;
			}
		}
		else if (focus == FOCUS_CANCEL)
		{
			if (raw == SPECIAL(ENTER))
				return -1;
		}
	}
}

/* ---- File menu actions ---- */

static byte do_file_new()
{
	clear();
	current_filename[0] = 0;
	cursor.x = 0;
	cursor.y = 0;
	offset.x = 0;
	offset.y = 0;
	draw_frame();
	redraw_all();
	return 0;
}

static byte do_file_open()
{
	char buf[256];
	buf[0] = 0;
	if (input_dialog("Open file:", buf, sizeof(buf)) == 0 && buf[0] != 0)
	{
		load_file(buf);
		strncpy(current_filename, buf, sizeof(current_filename) - 1);
		cursor.x = 0;
		cursor.y = 0;
		offset.x = 0;
		offset.y = 0;
		draw_frame();
		redraw_all();
	}
	else
	{
		draw_frame();
		redraw_all();
	}
	return 0;
}

static byte do_file_save()
{
	if (current_filename[0] == 0)
	{
		char buf[256];
		buf[0] = 0;
		if (input_dialog("Save as:", buf, sizeof(buf)) == 0 && buf[0] != 0)
		{
			strncpy(current_filename, buf, sizeof(current_filename) - 1);
		}
		else
		{
			draw_frame();
			redraw_all();
			return 0;
		}
	}
	save_file(current_filename);
	draw_frame();
	redraw_all();
	return 0;
}

static byte do_file_save_as()
{
	char buf[256];
	strncpy(buf, current_filename, sizeof(buf) - 1);
	buf[sizeof(buf) - 1] = 0;
	if (input_dialog("Save as:", buf, sizeof(buf)) == 0 && buf[0] != 0)
	{
		strncpy(current_filename, buf, sizeof(current_filename) - 1);
		save_file(current_filename);
	}
	draw_frame();
	redraw_all();
	return 0;
}

byte show_file_menu()
{
	MenuItem items[] = {
		{ "New",     ""    },
		{ "Open",    "F3"  },
		{ "Save",    "F2"  },
		{ "Save as", ""    },
		{ NULL,      NULL  },
		{ "Quit",    "Alt-Q" },
	};
	int count = sizeof(items) / sizeof(items[0]);
	/* File menu starts at column 4 (after the system icon item) */
	int choice = run_dropdown(4, 1, items, count);
	switch (choice)
	{
	case 0: return do_file_new();
	case 1: return do_file_open();
	case 2: return do_file_save();
	case 3: return do_file_save_as();
	case 5: return 1; /* quit */
	default:
		draw_frame();
		redraw_all();
		return 0;
	}
}

byte show_edit_menu()
{
	MenuItem items[] = {
		{ "Cut",     "Ctrl-X" },
		{ "Copy",    "Ctrl-C" },
		{ "Paste",   "Ctrl-V" },
	};
	int count = sizeof(items) / sizeof(items[0]);
	int choice = run_dropdown(9, 1, items, count);
	switch (choice)
	{
	case 0: clipboard_cut(); redraw_all(); break;
	case 1: clipboard_copy(); break;
	case 2: clipboard_paste(); redraw_all(); break;
	default: break;
	}
	draw_frame();
	redraw_all();
	return 0;
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
		offset.x = 0;
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

bool is_space(byte b)
{
	return b==' ' || b==9 || b==10 || b==13;
}

void move_word_left()
{
	Vector *line = get_line(cursor.y);
	byte *data = vector_access(line, 0);
	short n = vector_size(line);
	if (cursor.x > 0)
	{
		--cursor.x;
		if (is_space(data[cursor.x]))
		{
			while (cursor.x > 0 && is_space(data[cursor.x]))
				--cursor.x;
		}
		while (cursor.x > 0 && !is_space(data[cursor.x-1]))
			--cursor.x;
	}
}

void move_word_right()
{
	Vector *line = get_line(cursor.y);
	byte *data = vector_access(line, 0);
	short n = vector_size(line);
	if (cursor.x < n)
	{
		while (cursor.x < n && !is_space(data[cursor.x]))
			++cursor.x;
		while (cursor.x < n && is_space(data[cursor.x]))
			++cursor.x;
	}
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

void event_loop()
{
	byte done = 0;
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
		{
			if (ctrl) 
			{
				move_word_left();
			}
			else
			{
				if (cursor.x > 0)
					move_x_cursor(-1);
			}
			break;
		}
		case SPECIAL(RIGHT):
		{
			if (ctrl)
			{
				move_word_right();
			}
			else
				move_x_cursor(1);
			break;
		}
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
				if (cursor.y < vector_size(document))
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
			cursor.y = min(cursor.y + 25, vector_size(document));
			break;
		case SPECIAL(F):
		{
			if (alt)
			{
				if (show_file_menu())
					done = 1;
			}
			break;
		}
		case SPECIAL(Q):
		{
			if (alt)
				done = 1;
			break;
		}
		case SPECIAL(E):
		{
			if (alt)
				show_edit_menu();
			break;
		}
		case SPECIAL(C):
		{
			if (ctrl)
				clipboard_copy();
			break;
		}
		case SPECIAL(X):
		{
			if (ctrl)
			{
				clipboard_cut();
				redraw_all();
			}
			break;
		}
		case SPECIAL(V):
		{
			if (ctrl)
			{
				clipboard_paste();
				redraw_all();
			}
			break;
		}
		case SPECIAL(F2):
		{
			do_file_save();
			break;
		}
		case SPECIAL(F3):
		{
			do_file_open();
			break;
		}
		case SPECIAL(ESC):
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

int main(int argc, char *argv[])
{
	log_file = fopen("/tmp/log.txt", "w");
	fprintf(log_file,"Turbo Editor\n");
	fflush(log_file);
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
		strncpy(current_filename, argv[1], sizeof(current_filename) - 1);
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
	tcflush(STDIN_FILENO, TCIFLUSH);
	fclose(log_file);
	return 0;
}

