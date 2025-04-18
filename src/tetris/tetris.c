#include <stdio.h>
#include <screen.h>
#include <keyboard.h>
#include <stdlib.h>
#include <unistd.h>
#include <keycodes.h>

#define TILE_SIZE 32
int W, H;
int BOARD_LEFT=0, BOARD_TOP=0;
#define BOARD_WIDTH 10
#define BOARD_HEIGHT 15
#define BOARD_SIZE (BOARD_WIDTH * BOARD_HEIGHT)

const short template_offsets[] = 
{
    -2,0,-1,0,0,0,1,0,
    0,0,1,0,0,1,1,1,
    -1,0,0,0,1,0,0,1,
    -1,0,0,0,1,0,1,1,
    -1,-1,-1,0,0,0,1,0,
    -1,0,0,0,0,1,1,1,
    -1,1,-1,0,0,0,1,0
};

Color colors[7];

typedef struct piece_
{
    bool valid;
    Color color;
    short x,y;
    short offsets[8];
} Piece;

Color board[BOARD_SIZE];
Piece current;

void init()
{
    for (int i = 0; i < BOARD_SIZE; ++i)
    {
        board[i] = 0;
    }
    colors[0]=RGB(255,0,0);
    colors[1]=RGB(0,255,0);
    colors[2]=RGB(0,0,255);
    colors[3]=RGB(255,255,0);
    colors[4]=RGB(255,0,255);
    colors[5]=RGB(0,255,255);
    colors[6]=RGB(255,255,255);
    current.valid = 0;
}

bool valid_placement(Piece* piece)
{
    for (int i = 0; i < 4; ++i)
    {
        int x = piece->x + piece->offsets[i * 2];
        int y = piece->y + piece->offsets[i * 2 + 1];
        if (x < 0 || x >= BOARD_WIDTH || y < 0 || y >= BOARD_HEIGHT)
            return 0;
        if (board[y * BOARD_WIDTH + x] != 0)
            return 0;
    }
    return 1;
}

bool generate_piece()
{
    int index = rand() % 7;
    current.color = colors[index];
    current.x = BOARD_WIDTH / 2;
    current.y = 1;
    current.valid = 1;
    for (int i = 0; i < 8; ++i)
    {
        current.offsets[i] = template_offsets[index * 8 + i];
    }
    return valid_placement(&current);
}

void draw_board()
{
    for (int y = 0; y < BOARD_HEIGHT; ++y)
    {
        for (int x = 0; x < BOARD_WIDTH; ++x)
        {
            Color color = board[y * BOARD_WIDTH + x];
            screen_fill_rect(BOARD_LEFT + x * TILE_SIZE, BOARD_TOP + y * TILE_SIZE, TILE_SIZE, TILE_SIZE, color);
        }
    }
    if (current.valid)
    {
        for (int i = 0; i < 4; ++i)
        {
            int x = current.x + current.offsets[i * 2];
            int y = current.y + current.offsets[i * 2 + 1];
            screen_fill_rect(x * TILE_SIZE, y * TILE_SIZE, TILE_SIZE, TILE_SIZE, current.color);
        }
    }
}

void draw_piece(Piece* piece, Color color)
{
    for (int i = 0; i < 4; ++i)
    {
        int x = piece->x + piece->offsets[i * 2];
        int y = piece->y + piece->offsets[i * 2 + 1];
        screen_fill_rect(BOARD_LEFT + x * TILE_SIZE, BOARD_TOP + y * TILE_SIZE, TILE_SIZE, TILE_SIZE, color);
    }
}

void remove_line(short y)
{
    while (y>0)
    {
        for (int x = 0; x < BOARD_WIDTH; ++x)
        {
            board[y * BOARD_WIDTH + x] = board[(y - 1) * BOARD_WIDTH + x];
        }
        --y;
    }
    for (int x = 0; x < BOARD_WIDTH; ++x)
    {
        board[x] = 0;
    }
}

bool remove_full_lines()
{
    bool removed = 0;
    for (short y = 0; y < BOARD_HEIGHT; ++y)
    {
        bool full = 1;
        for (int x = 0; x < BOARD_WIDTH; ++x)
        {
            if (board[y * BOARD_WIDTH + x] == 0)
            {
                full = 0;
                break;
            }
        }
        if (full)
        {
            remove_line(y);
            removed=1;
        }
    }
    return removed;
}

void place_piece()
{
    for (int i = 0; i < 4; ++i)
    {
        int x = current.x + current.offsets[i * 2];
        int y = current.y + current.offsets[i * 2 + 1];
        board[y * BOARD_WIDTH + x] = current.color;
    }
    current.valid = 0;
    if (remove_full_lines())
        draw_board();
}

bool rotate_piece()
{
    Piece piece=current;
    for(int i=0;i<4;++i)
    {
        short temp = piece.offsets[i * 2];
        piece.offsets[i * 2] = -piece.offsets[i * 2 + 1];
        piece.offsets[i * 2 + 1] = temp;
    }
    if (valid_placement(&piece))
    {
        draw_piece(&current, 0);
        current=piece;
        draw_piece(&current, current.color);
        return 1;
    }
    return 0;
}

bool move_piece(short dx, short dy)
{
    Piece candidate = current;
    candidate.x += dx;
    candidate.y += dy;
    if (valid_placement(&candidate))
    {
        draw_piece(&current, 0);
        current = candidate;
        draw_piece(&current, current.color);
        return 1;
    }
    return 0;
}

int main(int argc, char* argv[])
{
    if (!screen_init())
	{
		printf("Failed to open screen\n");
		fflush(stdout);
		return -1;
	}
	W = screen_get_width();
	H = screen_get_height();
    BOARD_LEFT=(W-10*TILE_SIZE)/2;
    printf("BOARD_LEFT=%d\n", BOARD_LEFT);
	if (open_keyboard() < 0)
	{
		printf("Failed to open keyboard\n");
		fflush(stdout);
		return -1;
	}
    screen_clear(RGB(100,100,100));
    init();
    draw_board();
    bool game_over=0;
    uint counter=0;
    while (!game_over)
    {
        if (!current.valid)
        {
            if (!generate_piece())
            {
                game_over=1;
                break;
            }
            draw_piece(&current, current.color);
        }
        poll_keyboard_event();
		uint key = get_key() >> 8;
		if (key==0)
		{
			usleep(1000);
            if (++counter >= 500)
            {
                counter=0;
                if (!move_piece(0,1)) 
                    place_piece();
            }
			continue;
		}
        switch (key)
        {
            case KEY_LEFT:
                move_piece(-1,0);
                break;
            case KEY_RIGHT:
                move_piece(1,0);
                break;
            case KEY_DOWN:
                move_piece(0,1);
                break;
            case KEY_UP:
                rotate_piece();
                break;
            case KEY_ESC:
                game_over=1;
                break;
        }
    }
	close_keyboard();
	screen_clear(0);
	screen_shut();
    return 0;
}
