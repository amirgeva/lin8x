#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>

int main(int argc, char *argv[])
{
	mknod("/dev/fb0", S_IFCHR | 0666, makedev(29, 0));
	mkdir("/tmp", 0777);
	mkdir("/proc", 0755);
	mkdir("/dev/input", 0755);
	for (int i = 0; i < 4; i++)
	{
		char path[32];
		snprintf(path, sizeof(path), "/dev/input/event%d", i);
		mknod(path, S_IFCHR | 0666, makedev(13, 64 + i));
	}
	mkdir("/etc", 0755);
	FILE *f = fopen("/etc/resolv.conf", "w");
	if (f)
	{
		fputs("nameserver 10.0.2.3\n", f);
		fputs("nameserver 8.8.8.8\n", f);
		fclose(f);
	}
	execl("/bin/sh", "sh", (char *)0);
	return 1;
}
