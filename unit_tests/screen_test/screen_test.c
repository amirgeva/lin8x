#include <screen.h>
#include <stdio.h>


int main(int argc, char* argv[])
{
	if (!screen_init())
	{
		printf("Failed to open screen\n");
		fflush(stdout);
		return -1;
	}
	screen_print_info();
	fflush(stdout);
	screen_clear(0xF800); // Red
	screen_scroll(50, 0x1F); // Blue
	screen_draw_string(10, 10, "Hello, World!", 0, 0x7C0); // Black text on green background
	screen_shut();
	return 0;
}

