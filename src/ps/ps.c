#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mount.h>

int main(int argc, char* argv[]) {
    mount("proc", "/proc", "proc", 0, "");

    DIR *proc_dir = opendir("/proc");
    if (!proc_dir)
	{
        perror("opendir /proc");
        return 1;
    }

    printf("%5s  %s\n", "PID", "COMMAND");
    struct dirent *entry;
    while ((entry = readdir(proc_dir)) != NULL) {
        char *endptr;
        long pid = strtol(entry->d_name, &endptr, 10);
        if (*endptr != '\0')
            continue;

        char path[256];
        char name[256];
        name[0] = '\0';

        /* try cmdline first */
        snprintf(path, sizeof(path), "/proc/%s/cmdline", entry->d_name);
        FILE *f = fopen(path, "r");
        if (f)
        {
            int n = fread(name, 1, sizeof(name) - 1, f);
            fclose(f);
            if (n > 0)
            {
                /* cmdline uses \0 as separator between args */
                for (int i = 0; i < n; i++)
                    if (name[i] == '\0') name[i] = ' ';
                name[n] = '\0';
            }
        }

        if (name[0] != '\0')
            printf("%5ld  %s\n", pid, name);
    }

    closedir(proc_dir);
    return 0;
}

