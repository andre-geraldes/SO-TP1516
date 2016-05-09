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

// functions' headers
int executeRequest(int pid, char *command);

// execute request function
int executeRequest(int pid, char *command) {
  int files;
  char *token;
  char *delimiters = " ";

  // get the first token
  token = strtok(command,delimiters);
  if(strcmp(token,"backup")) { //backup

  } else { // restore

  }

  files = 0;
  // iterates all over the others tokens
  token = strtok(NULL, delimiters);
  while(token != NULL) {
    files++;
    kill(pid,SIGUSR1);
    sleep(1);
    token = strtok(NULL, delimiters);
  }

  return files;
}

// server main
int main(int argc, char const *argv[]) {
    // set fifo location
    char *fifo = strcat(getenv("HOME"),"/.Backup/fifo");
    int r,fd,pid,er;
    char buffer[SIZE],command[SIZE];

    // clear screen
    system("clear");
    puts("_____ SOBUSRV _____");

    // creates named pipe
    mkfifo(fifo,0666);

    // keeps reading from the named pipe
    while(1) {
      fd = open(fifo,O_RDONLY);
      while((r = read(fd,&buffer,SIZE)) > 0) {
        write(1,&buffer,r);
        sscanf(buffer,"%d\t%[^\n]s",&pid,command); // PID and command are OK
        // server handles at maximun 5 request simultaneously
        if(running < 5) {
          if(fork() == 0) {
            er = executeRequest(pid,command);
            _exit(0);
          } else {

          }
        } else {
          // enqueue request
        }


      }
      close(fd);
    }
    return 0;
}
