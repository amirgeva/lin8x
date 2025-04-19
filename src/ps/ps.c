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

    struct dirent *entry;
    while ((entry = readdir(proc_dir)) != NULL) {
        // Check if the directory entry is a number (PID)
        char *endptr;
        long pid = strtol(entry->d_name, &endptr, 10);
        if (*endptr == '\0')
		{
            // It's a PID directory
            char cmdline_path[4096];
            snprintf(cmdline_path, sizeof(cmdline_path), "/proc/%s/cmdline", entry->d_name);

            FILE *cmdline_file = fopen(cmdline_path, "r");
            if (cmdline_file)
			{
                char cmdline[4096];
                if (fgets(cmdline, sizeof(cmdline), cmdline_file))
				{
                    // Print the PID and command line
                    printf("PID: %ld, Command: %s\n", pid, cmdline);
                }
                fclose(cmdline_file);
            }
        }
    }

    closedir(proc_dir);
    return 0;
}

