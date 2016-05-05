#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/wait.h>
#include<sys/stat.h>
#include<sys/types.h>
#include<fcntl.h>

#define SIZE 512

// server main
int main(int argc, char const *argv[]) {
    // set fifo location
    char *fifo = strcat(getenv("HOME"),"/.Backup/fifo");
    int r,fd;
    char b[SIZE];

    // clear screen
    system("clear");
    puts("_____ SOBUSRV _____");

    // creates named pipe
    mkfifo(fifo,0666);

    // reads continualy from the named pipe
    while(1) {
      fd = open(fifo,O_RDONLY);
      // ends with Ctrl + D
      while((r = read(fd,&b,SIZE)) > 0) {
        write(1,&b,r);
      }
      close(fd);
    }
    return 0;
}
