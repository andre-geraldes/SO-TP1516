# makefile
CC=gcc
CFLAGS=-Wall -Wextra
DIR=/home/$(USER)/.Backup/

# make options
all: client server dir

client:
	$(CC) $(CFLAGS) -o sobucli client.c

server:
	$(CC) $(CFLAGS) -o sobusrv server.c

dir:
	mkdir $(DIR)
	mkdir $(DIR)data
	mkdir $(DIR)metadata

# clean options
cleanall: clean cleandir

clean:
	rm sobucli sobusrv

cleandir:
	rm -r $(DIR)

cleangz:
	rm *.gz
