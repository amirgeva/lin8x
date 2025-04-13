#include <screen.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/fb.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <errno.h>

static int screen_fd = -1;
static int screen_width = 0;
static int screen_height = 0;
static int screen_bpp = 0;
static int screen_size = 0;
static int screen_pitch = 0;
static Color* screen_buffer = 0;
static const byte font_data[] = {
#include "font.inl"
};

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
	screen_bpp = vinfo.bits_per_pixel;
	screen_size = screen_width * screen_height * (screen_bpp / 8);
	screen_pitch = screen_width * (screen_bpp / 8);
	screen_buffer = (Color *)mmap(0, screen_size, PROT_READ | PROT_WRITE, MAP_SHARED, screen_fd, 0);
	//printf("Screen buffer mapped successfully\n"); fflush(stdout);
	return 1;
}

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

void screen_shut()
{
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
			for (uint j = x; j < right; j++)
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
	if (x>=(screen_width - 8) || y>=(screen_height - 16))
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
