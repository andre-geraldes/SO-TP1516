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
int backupSteps(int it, char *fsha, char *token);
int restoreSteps(int it, char *fsha, char *token);

// steps of backup
int backupSteps(int it, char fsha[], char * token) {
  int errno;

  switch(it) {
    case 0 : {
      errno = execlp("gzip","gzip","-f","-k",token,NULL);
      return(errno);
      break;
    }

    case 1 : {
      char buffer[SIZE];
      int fd[2],status;
      pipe(fd);

      if(fork() == 0) {
        dup2(fd[1],1); // redirects STDOUT to pipe
        errno = execlp("sha1sum","sha1sum",token,NULL);
        close(fd[0]);
        close(fd[1]);
        _exit(errno);
      } else {
        wait(&status);
        if(WEXITSTATUS(status) == 0) {
          read(fd[0],buffer,SIZE); // reads from the pipe
          *fsha = strtok(buffer," ");
          //fsha = strtok(buffer," "); // gets the digest
          close(fd[1]);
          close(fd[0]);
        }
        return WEXITSTATUS(status);
      }
      break;
    }

    case 2 : {
      char compressed[SIZE],*target;
      sprintf(compressed,"%s%s",token,".gz");
      puts(getenv("HOME"));
      //errno = execlp("mv","mv",compressed,target,NULL);
      //return(errno);
      break;
    }

    case 3 : {
      char *dir = strcat(getenv("HOME"),"/.Backup");
      dir = strcat(dir,"/metadata");
      errno = execlp("cd","cd",dir,NULL);
      return(errno);
      break;
    }

    case 4 : {
      char link[SIZE];
      sprintf(link,"../data/%s",fsha);
      errno = execlp("ln","ln","-s","-f",link,token, NULL);
      break;
    }
  }
  return 0;
}

// backup operation
void backup(int pid, char *command) {
  int status,errno,i,error;
  char *token, *delimiters = " ",fsha[512];

  // token == backup
  token = strtok(command, delimiters);
  token = strtok(NULL,delimiters); // gets the next token
  // tokenize all files
  while(token != NULL) {
    error = 0;
    for(i=0;i<3 && !error;i++) {
      if(fork() == 0) {
        errno = backupSteps(i,fsha,token);
        _exit(errno);
      } else {
        wait(&status);
        // error
        if(WEXITSTATUS(status) != 0) {
          kill(pid,SIGUSR2);
          error = 1;
        }
      }
    }
    // all went good
    if(!error) {
      kill(pid,SIGUSR1);
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
    fifo = strcat(getenv("HOME"),"/.Backup/fifo"); //here
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
