#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <errno.h>

#define SHM_SIZE 1024  /* make it a 1K chunk */

int main() {
  key_t key;
  int shmid;
  char *data;
  int mode = S_IRUSR | S_IWUSR;  /* read/write for user */

  /* set the key */
  key = ftok("/tmp/shm.example", 's');
  if (key == -1) {
    perror("ftok");
    return 1;
  }

  /* create (or attach to) the segment */
  shmid = shmget(key, SHM_SIZE, 0644 | IPC_CREAT);
  if (shmid == -1) {
    perror("shmget");
    return 1;
  }

  /* attach to the segment to get a pointer to it */
  data = shmat(shmid, (void *)0, 0);
  if (data == (char *)(-1)) {
    perror("shmat");
    return 1;
  }

  /* now put some things into the memory for other process to read */
  sprintf(data, "Hello, shared memory world!\n");

  /* detach from the segment */
  if (shmdt(data) == -1) {
    perror("shmdt");
    return 1;
  }

  /* remove the segment */
  if (shmctl(shmid, IPC_RMID, 0) == -1) {
    perror("shmctl");
    return 1;
  }

  return 0;
}