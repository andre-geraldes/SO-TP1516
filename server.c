#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/wait.h>
#include<sys/stat.h>
#include<sys/types.h>
#include<fcntl.h>

#define SIZE 512

int running = 0; // number of requests running
int pids[5]; // holds the PIDs

// queue structure

// server main
int main(int argc, char const *argv[]) {
    // set fifo location
    char *fifo = strcat(getenv("HOME"),"/.Backup/fifo");
    int r,fd,pid;
    char b[SIZE],command[SIZE];

    // clear screen
    system("clear");
    puts("_____ SOBUSRV _____");

    // creates named pipe
    mkfifo(fifo,0666);

    // reads continualy from the named pipe
    while(1) {
      fd = open(fifo,O_RDONLY);
      while((r = read(fd,&b,SIZE)) > 0) {
        write(1,&b,r);
        sscanf(b,"%d\t%[^\n]s",&pid,command);
        printf("\t%d\t%s\n",pid,command);
      }
      close(fd);
    }
    return 0;
}
