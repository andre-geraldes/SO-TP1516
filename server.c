#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/wait.h>
#include<sys/stat.h>
#include<sys/types.h>
#include<fcntl.h>

#define SIZE 512
#define BACKUP 0
#define RESTORE 1

int running = 0; // number of requests running
char fifo[SIZE]; // path to the named pipe

// queue structure

// functions' headers
void processRequest(int pid, char *command);
void executeRequest(int pid, char *command, int op);
int backupSteps(int it, char *fsha, char *token);
int restoreSteps(int it, char *fsha, char *token);

// steps of restore


// steps of backup
int backupSteps(int it, char *fsha, char *token) {
  int errno,status;

  switch(it) {
    case 0 : {
      if(fork() == 0) {
          errno = execlp("gzip","gzip","-f","-k",token,NULL);
          _exit(errno);
      }
      wait(&status);
      return WEXITSTATUS(status);
      break;
    }

    case 1 : {
      char buffer[SIZE];
      int fd[2],status;

      pipe(fd);
      // calculates file's sha1sum
      if(fork() == 0) {
        dup2(fd[1],1); // redirects STDOUT to pipe
        errno = execlp("sha1sum","sha1sum",token,NULL);
        _exit(errno);
      }
      wait(&status);
      if(WEXITSTATUS(status) == 0) {
        close(fd[1]);
        read(fd[0],buffer,SIZE); // reads from the pipe
        strcpy(fsha,strtok(buffer," "));
        close(fd[0]);
      }
      return WEXITSTATUS(status);
      break;
    }

    case 2 : {
      char compressed[SIZE],target[SIZE];
      sprintf(compressed,"%s%s",token,".gz");
      sprintf(target,"%s%s%s%s",getenv("HOME"),"/.Backup","/data/",fsha);
      if(fork() == 0) {
        // move compressed file to the designated directory
        errno = execlp("mv","mv",compressed,target,NULL);
        _exit(errno);
      }
      wait(&status);
      return WEXITSTATUS(status);
      break;
    }

    case 3 : {
      char dir[SIZE];
      sprintf(dir,"%s%s%s",getenv("HOME"),"/.Backup","/metadata");
      // change directory to /metadata
      errno = chdir(dir);
      return errno;
      break;
    }

    case 4 : {
      char link[SIZE];
      sprintf(link,"../data/%s",fsha);
      if(fork() == 0) {
        // creates linker from metadata to data
        errno = execlp("ln","ln","-s","-f",link,token, NULL);
        _exit(errno);
      }
      wait(&status);
      return WEXITSTATUS(status);
      break;
    }
  }
  return 0;
}

// execute request by operation
void executeRequest(int pid, char *command, int op) {
  int errno,i,error;
  char *token, *delimiters = " ",fsha[SIZE],cot[SIZE];
  // token == backup || token == restore
  token = strtok(command, delimiters);
  token = strtok(NULL,delimiters); // gets the next token
  // tokenize all files
  while(token != NULL) {
    strcpy(cot,token);
    error = 0;
    for(i=0;(i<5 && !error);i++) {
      switch(op) {
        case BACKUP : {
          errno = backupSteps(i,fsha,cot);
          break;
        }
        case RESTORE : {
          errno = backupSteps(i,fsha,cot);
          break;
        }
      }
      // checks if there was an error
      if(errno != 0) {
        printf("error %d\n",i);
        kill(pid,SIGUSR2);
        error = 1;
      }
    }
    // all went good
    if(!error) {
      kill(pid,SIGUSR1);
    }
    // guarantees that many signals arrive to the client
    sleep(1);
    // get the next token
    token = strtok(NULL," ");
  }
}

// execute request function
void processRequest(int pid, char *command) {
  char *token,copy[512],*delimiters = " ";
  // gets a copy of the commands so it doesnt delete the original command
  strcpy(copy,command);
  // get the first token
  token = strtok(copy,delimiters);
  if(strcmp(token,"backup") == 0) {
    // backup
    executeRequest(pid,command,BACKUP);
  } else if(strcmp(token,"restore")){
    // restore
    executeRequest(pid,command,RESTORE);
  }
}

// server main
// int main(int argc, char const *argv[])
int main() {
    // set fifo location
    sprintf(fifo,"%s%s",getenv("HOME"),"/.Backup/fifo");
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
            processRequest(pid,command);
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
