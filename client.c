#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/wait.h>
#include<sys/stat.h>
#include<sys/types.h>
#include<fcntl.h>

#define SIZE 512

int result = -1; // variable that confirms the operation

// handler function
void hand(int s) {
  if(s == SIGUSR1) {
    result = 0; // successfully completed
  } else if (s == SIGUSR2) {
    result = 1; // something wen't wrong
  }
}

// client main
int main(int argc, char const *argv[]) {
    // set fifo location
    char *fifo = strcat(getenv("HOME"),"/.Backup/fifo");
    char command[SIZE];
    int fd,i,l;

    // at least, it must have the executable and the operation
    if(argc < 2) {
      printf("Syntax Error: Try sobucli [ backup | restore ]\n");
      exit(1);
    }

    // tries to open the pipe
    if((fd = open(fifo,O_WRONLY)) == -1) {
      printf("Error: Could not open the pipe!\n");
      exit(1);
    }

    // checks if the operation is valid
    if(strcmp(argv[1],"backup") != 0 && strcmp(argv[1],"restore") != 0) {
      printf("Syntax Error: Try sobucli [ backup | restore ]\n");
      exit(1);
    }

    // concatenates argvs into one command
    sprintf(command,"%d\t%s",getpid(),argv[1]);
    for(i=2;i<argc;i++) {
      // verifies if the file exists
      sprintf(command,"%s %s",command,argv[i]);
    }
    l = sprintf(command,"%s\n",command);

    // send command
    write(fd,command,l);

    // wait for signal
    signal(SIGUSR1,hand);
    signal(SIGUSR2,hand);
    pause();

    // receives the signal
    if(result == 0) { // successfully done
      puts("Good");
    } else if (result == 1) { // something wen't wrong
      puts("Bad");
    } else { // nothing happened
      puts("Nothing");
    }

    // close pipe descriptor
    close(fd);

    return 0;
}
