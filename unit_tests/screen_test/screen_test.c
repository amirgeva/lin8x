#include <screen.h>
#include <stdio.h>
#include <stdlib.h>

Color RGB(uint r, uint g, uint b)
{
	r = ((r & 255) >> 3);
	g = ((g & 255) >> 2);
	b = ((b & 255) >> 3);
	return (r << 11) | (g << 5) | b;
}

int main(int argc, char* argv[])
{
	Color fill_color = 0;
	if (argc > 3)
	{
		fill_color = RGB(atoi(argv[1]), atoi(argv[2]), atoi(argv[3]));
	}
	if (!screen_init())
	{
		printf("Failed to open screen\n");
		fflush(stdout);
		return -1;
	}
	screen_clear(fill_color);
	screen_print_info();
	fflush(stdout);
	uint color = 0;
	for (int i = 0; i < 16; i++)
	{
		screen_fill_rect(0, i*16, 300, 16, RGB(color,color,color));
		color += 16;
	}
	screen_shut();
	return 0;
}

