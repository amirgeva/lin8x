#include <stdio.h>


int main(int argc, char* argv[])
{
	if (argc>1)
	{
		FILE* f = fopen(argv[1], "r");
		if (f)
		{
			char c;
			while ((c = fgetc(f)) != EOF)
			{
				putchar(c);
			}
			fclose(f);
		}
		else
		{
			perror("Failed to open file");
			return 1;
		}
	}
	else
	{
		printf("No file specified\n");
		return 1;
	}
	return 0;
}


