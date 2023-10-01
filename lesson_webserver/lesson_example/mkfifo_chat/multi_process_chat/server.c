// server
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
 
int main() {
  int cpid = fork();
  if (cpid == -1) {
    perror("fork");
  }
 
  if (cpid == 0) {
    int fd;
    if (access("fifo2", F_OK) == -1) {
      fd = mkfifo("fifo2", 0644);
    } else {
      fd = open("fifo2", O_WRONLY);
    }
    char buf[128];
    for (;;) {
      fgets(buf, 128, stdin);
      write(fd, buf, strlen(buf) + 1);
    }
    close(fd);
  } else {
    int fd;
    if (access("fifo1", F_OK) == -1) {
      fd = mkfifo("fifo1", 0644);
    } else {
      fd = open("fifo1", O_RDONLY);
    }
    char buf[128];
    for (;;) {
      if (read(fd, buf, 128) <= 0) {
        break;
      }
      puts(buf);
    }
    close(fd);
  }
  exit(0);
}
