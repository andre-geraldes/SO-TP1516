#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/wait.h>
#include<sys/stat.h>
#include<sys/types.h>
#include<fcntl.h>

char *fifo = "/home/barbosa/.Backup/fifo";

// client main
int main(int argc, char const *argv[]) {
    /* code */
    char b;
    int fd;

    // at least, it must have the executable and the operation
    if(argc < 2) {
      printf("Syntax Error: Try sobucli [ backup | restore ]\n");
      exit(1);
    }

    // tries to open the pipe
    if((fd = open(fifo,O_WRONLY)) == -1) {
      printf("Error: Could not open the pipe!\n")
      exit(1);
    }

    // checks operation
    if(strcmp(argv[1],"backup") != 0 && strcmp(argv[1],"restore") != 0) {
      printf("Syntax Error: Try sobucli [ backup | restore ]\n");
      exit(1);
    }

    close(fd);

    return 0;
}
