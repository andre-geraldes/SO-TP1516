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
char *fifo; // path to the named pipe

// queue structure

// functions' headers
void executeRequest(int pid, char *command);
void backup(int pid, char *command);
void restore(int pid, char *command);

// backup operation
void backup(int pid, char *command) {
  int status,errno;
  char *token, *delimiters = " ";

  // token == backup
  token = strtok(command, delimiters);
  token = strtok(NULL,delimiters);
  // tokenize all files
  while(token != NULL) {
    // we know the filename
    // execute every file
    if(fork() == 0) {
      // execute the script
      errno = execlp("sh","sh","backup.sh",token,NULL);
      exit(errno);
    } else {
      wait(&status);
      if(WEXITSTATUS(status) == 0) {
        // move the file.gz to the designed folder
        kill(pid,SIGUSR1);
      } else {
        kill(pid,SIGUSR2);
      }
    }
    // guarantee that many signals arrive to the client
    sleep(1);
    // get the next token
    token = strtok(NULL, delimiters);
  }
}

// restore operation
void restore(int pid, char *command) {
  int status,errno;
  char *token, *delimiters = " ";

  // token == backup
  token = strtok(command, delimiters);
  token = strtok(NULL,delimiters);
  // tokenize all files
  while(token != NULL) {
    // we know the filename
    // execute every file
    if(fork() == 0) {
      // execute the script
      errno = execlp("sh","sh","restore.sh",token,NULL);
      exit(errno);
    } else {
      wait(&status);
      if(WEXITSTATUS(status) == 0) {
        // move the file.gz to the designed folder
        kill(pid,SIGUSR1);
      } else {
        kill(pid,SIGUSR2);
      }
    }
    // guarantee that many signals arrive to the client
    sleep(1);
    // get the next token
    token = strtok(NULL, delimiters);
  }
}

// execute request function
void executeRequest(int pid, char *command) {
  char *token,copy[512];
  char *delimiters = " ";

  // gets a copy of the commands
  strcpy(copy,command);

  // get the first token
  token = strtok(copy,delimiters);
  if(strcmp(token,"backup") == 0) { // backup
    backup(pid,command);
  } else { // restore
    restore(pid,command);
  }
}

// server main
// int main(int argc, char const *argv[])
int main() {
    // set fifo location
    fifo = strcat(getenv("HOME"),"/.Backup/fifo");
    int r,fd,pid;
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
            running++;
            executeRequest(pid,command);
            running--;
            _exit(0);
          }
        } else {
          // enqueue request
        }
      }
      close(fd);
    }
    return 0;
}
