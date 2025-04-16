#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <errno.h>

#define SHM_SIZE 640*480*2
char* virtfb_buffer = 0;

int virtfb_init()
{
	key_t key;
	int shmid;
	char *data;

	key = ftok("/tmp/virtfb.shm", 's');
	if (key == -1)
	{
		perror("ftok");
		return 1;
	}

	/* attach to the segment */
	shmid = shmget(key, SHM_SIZE, 0644);
	if (shmid == -1)
	{
		perror("shmget");
		return 1;
	}

	/* attach to the segment to get a pointer to it */
	virtfb_buffer = shmat(shmid, (void *)0, 0);
	if (virtfb_buffer == (char *)(-1))
	{
		perror("shmat");
		return 1;
	}
}

int virtfb_shut()
{
  /* detach from the segment */
  if (shmdt(virtfb_buffer) == -1) {
    perror("shmdt");
    return 1;
  }

  return 0;
}