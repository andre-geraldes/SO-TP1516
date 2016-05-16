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
char *local; // path of the local directory

// queue structure

// functions' headers
void processRequest(int pid, char *command);
void executeRequest(int pid, char *command, int op);
int backupSteps(int it, char *fsha, char *token);
int restoreSteps(char *token);

// steps of restore
int restoreSteps(char *token) {
  int errno,status,fd,fd1[2],fd2[2],fd3[2],bytes;
  char *metadata = strdup(getenv("HOME")),*comp=NULL;
  char buffer[SIZE];

  metadata = strcat(metadata,"/.Backup/metadata");
  errno = chdir(metadata);

  // set path to the file in local directory for overwrite
  char *path = strdup(local);
  sprintf(path,"%s/%s",path,token);
  // creates fd to the file
  fd = open(path,O_CREAT | O_WRONLY,0666);
  // checks if created a file descriptor
  if(fd == -1) {
    printf("Error: Could not open a file descriptor!");
    _exit(1);
  }
  // creates the first pipe
  pipe(fd1);
  if(fork() == 0) {
    pipe(fd2);
    if(fork() == 0) {
      pipe(fd3);
      if(fork() == 0) {
        dup2(fd3[1],1);
        errno = execlp("ls","ls","-l",NULL);
        _exit(errno);
      } else {
        wait(&status);
        if(WEXITSTATUS(status) != 0) {
          puts("Error: Could not execute ls!");
        }
        dup2(fd3[0],0);
        dup2(fd2[1],1);
        close(fd3[1]);
        close(fd2[1]);
        errno = execlp("grep","grep","-w",token,NULL);
        _exit(errno);
      }
    } else {
      wait(&status);
      if(WEXITSTATUS(status) != 0) {
        puts("Error: Could not execute grep!");
        exit(1);
      }
      dup2(fd2[0],0);
      dup2(fd1[1],1);
      close(fd2[1]);
      close(fd1[1]);
      errno = execlp("awk","awk","{print $11}",NULL);
      _exit(errno);
    }
  } else {
    wait(&status);
    if(WEXITSTATUS(status) != 0) {
      puts("Error: Could not execute awk!");
    }
    // redirects STDIN to the pipe
    close(fd1[1]);
    dup2(fd1[0],0);
    // gets the path of the compressed file
    bytes = read(fd1[0],buffer,SIZE);
    comp = strndup(buffer,bytes-1);

    wait(0);
    // test rm local file because of double WR
    if(fork() == 0) {
      dup2(fd,1);
      errno = execlp("gunzip","gunzip","-c",comp,NULL);
      _exit(errno);
    }
    wait(0);

  }
  close(fd);
  return 0;
}

// steps of backup
int backupSteps(int it, char *fsha, char *token) {
  int errno,status;

  switch(it) {
    case 0 : {
      chdir(local);
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
        close(fd[0]);
        dup2(fd[1],1); // redirects STDOUT to pipe
        errno = execlp("sha1sum","sha1sum",token,NULL);
        close(fd[1]);
        _exit(errno);
      }
      wait(&status);
      if(WEXITSTATUS(status) == 0) {
        close(fd[1]);
        read(fd[0],buffer,SIZE); // reads from the pipe
        fsha = strcpy(fsha,strtok(buffer," "));
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
  char *token, *delimiters = " ",fsha[SIZE];
  fsha[0]='\0';
  // token == backup || token == restore
  token = strsep(&command, delimiters);
  token = strdup(strsep(&command,delimiters)); // gets the next token
  // tokenize all files
  while(token != NULL) {
    error = 0;
    switch(op) {
      case BACKUP : {
        for(i=0;(i<5 && !error);i++) {
          errno = backupSteps(i,fsha,token);
          // checks if there was an error
          if(errno != 0) {
            kill(pid,SIGUSR2);
            error = 1;
          }
        }
        break;
      }

      case RESTORE : {
        errno = restoreSteps(token);
        // checks if there was an error
        if(errno != 0) {
          kill(pid,SIGUSR2);
          error = 1;
        }
        break;
      }
    }
    // all went good
    if(!error) {
      kill(pid,SIGUSR1);
    }
    // guarantees that many signals arrive to the client
    sleep(1);
    // get the next token
    token = strdup(strsep(&command,delimiters));
    // resets fsha
    fsha[0] = '\0';
  }
}

// execute request function
void processRequest(int pid, char *command) {
  char *token,*copy,*delimiters = " ";
  // gets a copy of the commands so it doesnt delete the original command
  copy = strdup(command);
  // get the first token
  token = strtok(copy,delimiters);
  if(strcmp(token,"backup") == 0) {
    // backup
    executeRequest(pid,command,BACKUP);
  } else if(strcmp(token,"restore") == 0){
    // restore
    executeRequest(pid,command,RESTORE);
  }
}

// server main
int main() {
  int r,fd,pid;
  char buffer[SIZE],command[SIZE];
  // set fifo location
  sprintf(fifo,"%s%s",getenv("HOME"),"/.Backup/fifo");
  // set local directory
  local = strdup(getenv("PWD"));
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
        // creates a child process to process request
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
