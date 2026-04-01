#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>

int main(int argc, char *argv[])
{
	const char *path = (argc > 1) ? argv[1] : ".";
	DIR *dir = opendir(path);
	if (!dir)
	{
		perror("list");
		return 1;
	}
	struct dirent *entry;
	while ((entry = readdir(dir)) != NULL)
	{
		if (entry->d_name[0] == '.')
			continue;
		char full[512];
		snprintf(full, sizeof(full), "%s/%s", path, entry->d_name);
		struct stat st;
		if (stat(full, &st) == 0 && S_ISDIR(st.st_mode))
			printf("%s/\n", entry->d_name);
		else
			printf("%s\n", entry->d_name);
	}
	closedir(dir);
	return 0;
}
