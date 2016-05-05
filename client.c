#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/wait.h>
#include<sys/stat.h>
#include<sys/types.h>
#include<fcntl.h>

#define SIZE 512

// client main
int main(int argc, char const *argv[]) {
    // set fifo location
    char *fifo = strcat(getenv("HOME"),"/.Backup/fifo");
    char command[512];
    int fd,i;

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

    // checks operation
    if(strcmp(argv[1],"backup") != 0 && strcmp(argv[1],"restore") != 0) {
      printf("Syntax Error: Try sobucli [ backup | restore ]\n");
      exit(1);
    }

    // concatenates argvs into one command
    strcpy(command,argv[1]);
    for(i=2;i<argc;i++) {
      strcat(command," ");
      strcat(command,argv[i]);
    }
    command[strlen(command)] = '\n';
    command[strlen(command)] = '\0';

    // send command
    write(fd,command,strlen(command)-1);

    // wait for signal

    // close pipe descriptor
    close(fd);

    return 0;
}
