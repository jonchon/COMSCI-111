#NAME: Jonathan Chon
#EMAIL: jonchon@gmail.com
#ID: 104780881

CC = gcc
FLAGS = -Wall -Wextra -lmcrypt

default: client server

client: lab1b-client.c
	@$(CC) $(FLAGS) lab1b-client.c -o lab1b-client -g

server: lab1b-server.c
	@$(CC) $(FLAGS) lab1b-server.c -o lab1b-server -g

clean:
	@rm -f lab1b-client lab1b-server *.tar.gz

dist: default
	@tar -czf lab1b-104780881.tar.gz Makefile README lab1b-client.c lab1b-server.c my.key
