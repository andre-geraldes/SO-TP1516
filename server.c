#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/wait.h>
#include<sys/stat.h>
#include<sys/types.h>
#include<fcntl.h>

#define SIZE 512

char *fifo = "/home/barbosa/.Backup/fifo";

int main(int argc, char const *argv[]) {
    int read,fd;
    char b[SIZE];

    // creates named pipe
    if(mkfifo(fifo,0666) == -1) {
      printf("Error: Could not create the named pipe!\n");
      exit(1);
    };

    // reads continualy from the named pipe
    while(1) {
      fd = open(fifo,O_RDONLY);
      // ends with Ctrl + D
      while((read = read(fd,&b,SIZE)) > 0) {
        write(1,&b,read);
      }
      close(fd);
    }
    return 0;
}
