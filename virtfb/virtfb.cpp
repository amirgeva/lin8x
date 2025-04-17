#include <opencv2/opencv.hpp>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define SHM_SIZE 640*480*2
int key_wait=100;
int scale=1;

int main(int argc, char* argv[])
{
  for(int i=1;i<argc;++i)
  {
    if (strcmp(argv[i], "-s") == 0 && (i + 1) < argc)
    {
      scale = atoi(argv[++i]);
      if (scale < 1) scale = 1;
      if (scale > 4) scale = 4;
    }
    else if (strcmp(argv[i], "-w") == 0 && (i + 1) < argc)
    {
      key_wait = atoi(argv[++i]);
      if (key_wait < 1) key_wait = 1;
    }
    else
    {
      printf("Usage: %s [-s <scale>] [-w <wait_time>]\n", argv[0]);
      return 1;
    }
  }

  key_t key;
  int shmid;
  void *data;
  int mode = S_IRUSR | S_IWUSR;  /* read/write for user */

  const char* filepath="/tmp/virtfb.shm"; // Path to the file used for ftok

  // Check if the file exists. If not, create it.
  if (access(filepath, F_OK) == -1) {
    // File doesn't exist, create it.
    int fd = open(filepath, O_CREAT, 0644); // Create the file with read/write permissions for the owner
    if (fd == -1) {
      perror("open");
      return 1; // Exit if file creation fails
    }
    close(fd); // Close the file descriptor
    printf("File '%s' created.\n", filepath);
  }

  /* set the key */
  key = ftok("/tmp/virtfb.shm", 's');
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

  cv::Mat image(480, 640, CV_8UC2, data);
  cv::Mat color_image(480, 640, CV_8UC3);
  cv::Mat scaled_image;
  if (scale > 1)
  {
    scaled_image = cv::Mat(480*scale, 640*scale, CV_8UC3);
  }

  cv::namedWindow("virtfb", cv::WINDOW_AUTOSIZE);
  int kb=0;
  while ((kb=cv::waitKey(key_wait)) != 201) // F12
  {
    //if (kb>0) std::cout << kb << std::endl;
    cv::cvtColor(image, color_image, cv::COLOR_BGR5652BGR);
    if (scale>1)
    {
      cv::resize(color_image, scaled_image, cv::Size(640*scale, 480*scale), 0, 0, cv::INTER_NEAREST);
      cv::imshow("virtfb", scaled_image);
    }
    else
      cv::imshow("virtfb", color_image);
  }

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
