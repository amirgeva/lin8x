#include <screen.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/fb.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <errno.h>
#include <termios.h>
#include <vector.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

static int screen_fd = -1;
static int screen_width = 0;
static int screen_height = 0;
static int screen_bpp = 0;
static int screen_size = 0;
static int screen_pitch = 0;
static int screen_rows = 0;
static int screen_cols = 0;
static Color *screen_buffer = 0;
static const byte font_data[] = {
#include "font.inl"
};

typedef struct {
	uint width;
	uint height;
	uint transparency;
	Vector* data;
} Sprite;

#define SPRITE_LIMIT 256
Sprite sprites[SPRITE_LIMIT];

void init_sprites()
{
	for (int i = 0; i < 256; i++)
	{
		sprites[i].width = 0;
		sprites[i].height = 0;
		sprites[i].transparency = 0xFFFFFFFF;
		sprites[i].data = 0;
	}
}

void destroy_sprites()
{
	for (int i = 0; i < 256; i++)
	{
		if (sprites[i].data)
		{
			vector_shut(sprites[i].data);
			sprites[i].data = 0;
		}
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


#ifdef DEV

#include "virtfb.h"

bool screen_init()
{
	if (virtfb_init())
	{
		printf("Error initializing virtual framebuffer\n");
		fflush(stdout);
		return 0;
	}
	screen_buffer = (Color *)get_virtfb_buffer();
	if (!screen_buffer)
	{
		printf("Error getting virtual framebuffer buffer\n");
		fflush(stdout);
		return 0;
	}
	disable_console();
	screen_width = 640;
	screen_height = 480;
	screen_rows = screen_height / 16;
	screen_cols = screen_width / 8;
	screen_bpp = 16;
	screen_size = screen_width * screen_height * (screen_bpp / 8);
	screen_pitch = screen_width * (screen_bpp / 8);
	init_sprites();
	//printf("Virtual framebuffer initialized successfully\n"); fflush(stdout);
	return 1;
}

void screen_shut()
{
	destroy_sprites();
	if (screen_buffer)
	{
		virtfb_shut();
		screen_buffer = 0;
	}
	enable_console();
}

#else
bool screen_init()
{
	screen_fd = open("/dev/fb0", O_RDWR);
	if (screen_fd < 0)
	{
		printf("Error opening framebuffer device\n");
		fflush(stdout);
		return 0;
	}
	//printf("Framebuffer device opened successfully\n");	fflush(stdout);

	// Set the screen to a default mode
	struct fb_var_screeninfo vinfo;
	if (ioctl(screen_fd, FBIOGET_VSCREENINFO, &vinfo) == -1)
	{
		printf("Error reading variable information\n");
		fflush(stdout);
		close(screen_fd);
		screen_fd = -1;
		return 0;
	}
	//printf("Screen info retrieved successfully\n"); fflush(stdout);
	screen_width = vinfo.xres;
	screen_height = vinfo.yres;
	screen_rows = screen_height / 16;
	screen_cols = screen_width / 8;
	screen_bpp = vinfo.bits_per_pixel;
	screen_size = screen_width * screen_height * (screen_bpp / 8);
	screen_pitch = screen_width * (screen_bpp / 8);
	screen_buffer = (Color *)mmap(0, screen_size, PROT_READ | PROT_WRITE, MAP_SHARED, screen_fd, 0);
	//printf("Screen buffer mapped successfully\n"); fflush(stdout);
	init_sprites();
	return 1;
}

void screen_shut()
{
	destroy_sprites();
	if (screen_buffer)
	{
		munmap(screen_buffer, screen_size);
		screen_buffer = 0;
	}
	if (screen_fd >= 0)
	{
		close(screen_fd);
		screen_fd = -1;
	}
}

#endif

void screen_print_info()
{
	if (screen_buffer)
	{
		printf("Screen Info:\n");
		printf("Width: %d\n", screen_width);
		printf("Height: %d\n", screen_height);
		printf("Bits per pixel: %d\n", screen_bpp);
		printf("Size: %d bytes\n", screen_size);
		printf("Pitch: %d bytes\n", screen_pitch);
	}
	else
	{
		printf("Screen buffer is not initialized.\n");
	}
}

void screen_clear(Color color)
{
	if (screen_buffer)
	{
		Color *ptr = screen_buffer;
		for (int y = 0; y < screen_height; y++)
		{
			for (int x = 0; x < screen_width; x++)
			{
				*ptr++ = color;
			}
		}
	}
}

void screen_draw_pixel(uint x, uint y, Color color)
{
	if (screen_buffer && x < screen_width && y < screen_height)
	{
		Color *ptr = screen_buffer + (y * screen_width + x);
		*ptr = color;
	}
}

void screen_xor_rect(uint x, uint y, uint w, uint h, Color color)
{
	uint right = x + w;
	uint bottom = y + h;
	if (screen_buffer && x < screen_width && y < screen_height &&
		right <= screen_width && bottom <= screen_height)
	{
		Color *ptr = screen_buffer + (y * screen_width + x);
		for (uint i = 0; i < h; i++)
		{
			for (uint j = 0; j < w; j++)
			{
				ptr[j] ^= color;
			}
			ptr += screen_width;
		}
	}
}

void screen_fill_rect(uint x, uint y, uint w, uint h, Color color)
{
	uint right=x + w;
	uint bottom=y + h;
	if (screen_buffer && x < screen_width && y < screen_height && 
		right <= screen_width && bottom <= screen_height)
	{
		Color* ptr=screen_buffer + (y * screen_width + x);
		for (uint i = 0; i < h; i++)
		{
			for (uint j = 0; j < w; j++)
			{
				ptr[j] = color;
			}
			ptr+=screen_width;
		}
	}
}

void screen_draw_rect(uint x, uint y, uint w, uint h, Color color)
{
	uint right=x + w;
	uint bottom=y + h;
	if (screen_buffer && x < screen_width && y < screen_height && 
		right <= screen_width && bottom <= screen_height && h>1)
	{
		Color* ptr=screen_buffer + (y * screen_width + x);
		for (uint j = 0; j < w; j++)
			ptr[j] = color;
		ptr += screen_width;
		for (uint i = 1; i < (h - 1); i++)
		{
			ptr[0] = color;
			ptr[w - 1] = color;
			ptr+=screen_width;
		}
		for (uint j = 0; j < w; j++)
			ptr[j] = color;
	}
}

static void screen_vertical_line(uint x, uint y1, uint y2, Color color)
{
	if (y1>y2)
	{
		uint tmp=y1;
		y1=y2;
		y2=tmp;
	}
	Color* ptr=screen_buffer + (y1 * screen_width + x);
	for (uint i = y1; i <= y2; i++)
	{
		*ptr = color;
		ptr+=screen_width;
	}
}

static void screen_horizontal_line(uint x1, uint x2, uint y, Color color)
{
	if (x1>x2)
	{
		uint tmp=x1;
		x1=x2;
		x2=tmp;
	}
	Color* ptr=screen_buffer + (y * screen_width + x1);
	for (uint i = x1; i <= x2; i++)
	{
		ptr[i] = color;
	}
}

static uint udiff(uint a, uint b)
{
	if (a>b)
		return a-b;
	else
		return b-a;
}

void screen_draw_line(uint x1, uint y1, uint x2, uint y2, Color color)
{
	if (x1>=screen_width || y1>=screen_height || x2>=screen_width || y2>=screen_height)
		return;
	if (x1==x2)
		screen_vertical_line(x1, y1, y2, color);
	else if (y1==y2)
		screen_horizontal_line(x1, x2, y1, color);
	else
	{
		uint dx = udiff(x1, x2);
		uint dy = udiff(y1, y2);
		if (dx>dy)
		{
			if (x1>x2)
			{
				uint tmp=x1;
				x1=x2;
				x2=tmp;
				tmp=y1;
				y1=y2;
				y2=tmp;
			}
			uint y = y1;
			int d = 2 * dy - dx;
			for (uint x = x1; x <= x2; x++)
			{
				screen_draw_pixel(x, y, color);
				if (d > 0)
				{
					y++;
					d -= 2 * dx;
				}
				d += 2 * dy;
			}
		}
		else
		{
			if (y1>y2)
			{
				uint tmp=x1;
				x1=x2;
				x2=tmp;
				tmp=y1;
				y1=y2;
				y2=tmp;
			}
			uint x = x1;
			int d = 2 * dx - dy;
			for (uint y = y1; y <= y2; y++)
			{
				screen_draw_pixel(x, y, color);
				if (d > 0)
				{
					x++;
					d -= 2 * dy;
				}
				d += 2 * dx;
			}
		}
	}
}

void screen_scroll(uint dy, Color blank_color)
{
	if (dy>=screen_height)
	{
		screen_clear(blank_color);
		return;
	}
	Color* src=screen_buffer + (dy * screen_width);
	Color* dst=screen_buffer;
	for (uint i = 0; i < (screen_height - dy); i++)
	{
		for (uint j = 0; j < screen_width; j++)
		{
			dst[j] = src[j];
		}
		src+=screen_width;
		dst+=screen_width;
	}
	uint n = screen_width * dy;
	for (uint i = 0; i < n;++i)
	{
		dst[i] = blank_color;
	}
}

bool screen_draw_char(uint x, uint y, char c, Color fg, Color bg)
{
	if (x>(screen_width - 8) || y>(screen_height - 16))
		return 0;
	byte b = (byte)c;
	uint index = b;
	index *= (8 * 16);
	Color* ptr=screen_buffer + (y * screen_width + x);
	for(uint i = 0; i < 16; i++)
	{
		for (uint j = 0; j < 8; ++j, ++index)
		{
			if (font_data[index])
				ptr[j] = fg;
			else
				ptr[j] = bg;
		}
		ptr+=screen_width;
	}
	return 1;
}

bool screen_draw_string(uint x, uint y, const char *str, Color fg, Color bg)
{
	while (*str)
	{
		if (!screen_draw_char(x, y, *str, fg, bg))
			return 0;
		x += 8;
		str++;
	}
	return 1;
}

bool text_draw_char(uint x, uint y, char c, Color fg, Color bg)
{
	return screen_draw_char(x * 8, y * 16, c, fg, bg);
}

bool text_draw_string(uint x, uint y, const char *str, Color fg, Color bg)
{
	return screen_draw_string(x * 8, y * 16, str, fg, bg);
}

uint screen_get_rows()
{
	return screen_rows;
}

uint screen_get_cols()
{
	return screen_cols;
}

uint screen_get_width()
{
	return screen_width;
}

uint screen_get_height()
{
	return screen_height;
}

bool screen_load_sprites(const char *filename, uint size, uint start_index)
{
	int w, h, c;
	byte* data = stbi_load(filename, &w, &h, &c, 0);
	if (!data) return 0;
	if (c != 4)
	{
		printf("Error: image must be RGBA format\n");
		stbi_image_free(data);
		return 0;
	}
	uint index=start_index;
	for(uint y=0;y<=(h-size) && index<SPRITE_LIMIT;y+=size)
	{
		for(uint x=0;x<=(w-size) && index<SPRITE_LIMIT;x+=size, ++index)
		{
			Sprite* sprite = &sprites[index];
			sprite->width = size;
			sprite->height = size;
			sprite->transparency = 0;
			if (!sprite->data)
				sprite->data = vector_new(sizeof(Color));
			if (!sprite->data) break;
			vector_resize(sprite->data, size*size);
			Color* dst = (Color*)vector_access(sprite->data,0);
			byte* src = data + (y * w + x) * 4;
			const uint stride = w * 4;
			for (uint i = 0; i < size; ++i)
			{
				for (uint j = 0; j < size; ++j)
				{
					Color color = RGB(src[j*4], src[j*4+1], src[j*4+2]);
					if (src[j*4+3] == 0)
						color = 0;
					else if (color==0) color=1;
					*dst++ = color;
				}
				src += stride;
			}
		}
	}
	return 1;
}

bool screen_set_sprite(uint index, uint width, uint height, Color* data, Color* transparency)
{}

bool screen_draw_sprite(uint x, uint y, uint index)
{
	if (index >= SPRITE_LIMIT)
		return 0;
	Sprite* sprite = &sprites[index];
	if (!sprite->data)
		return 0;
	Color* src = (Color*)vector_access(sprite->data, 0);
	if (!src)
		return 0;
	uint w = sprite->width;
	uint h = sprite->height;
	Color* ptr = screen_buffer + (y * screen_width + x);
	for (uint i = 0; i < h; ++i)
	{
		for (uint j = 0; j < w; ++j)
		{
			Color color = *src++;
			if (color != sprite->transparency)
				ptr[j] = color;
		}
		ptr += screen_width;
	}
	return 1;
}


Color RGB(uint r, uint g, uint b)
{
	r = ((r&255) >> 3);
	g = ((g&255) >> 2);
	b = ((b&255) >> 3);
	return (r<<11) | (g<<5) | b;
}

