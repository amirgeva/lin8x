#include <screen.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char* argv[])
{
	if (!screen_init())
	{
		printf("Failed to open screen\n");
		fflush(stdout);
		return -1;
	}
	if (!screen_load_sprites("sprites.png", 32, 0))
	{
		screen_shut();
		printf("Failed to load sprites\n");
		fflush(stdout);
		return -1;
	}
	screen_clear(0);
	short x=0, y=100, dx=1;
	for(int i=0;i<1000;++i)
	{
		screen_clear(0);
		screen_draw_sprite(x,y,0);
		x+=dx;
		if (x>400) dx=-1;
		if (x==0) dx=1;
		usleep(10000);
	}

	screen_shut();
	return 0;
}

