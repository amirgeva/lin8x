#include <keyboard.h>
#include <stdio.h>


int main(int argc, char* argv[])
{
    if (open_keyboard()<0) {
        printf("No keyboard found\n");
        return -1;
    }
    printf("Keyboard found\n");

    while (1)
    {
        int event = poll_keyboard_event();
        if (event < 0) {
            printf("Error polling keyboard event\n");
            break;
        }
        // if (event > 0) {
        //     printf("Keyboard event: %d\n", event);
        // }

        uint key = get_key();
        if (key != 0) {
            if (key<256)
                printf("%c", key);
            else
                printf("\nKey pressed: %08x\n", key);
        }
    }

    close_keyboard();
    printf("Keyboard closed\n");
    return 0;
}

