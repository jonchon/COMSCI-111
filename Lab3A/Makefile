#NAME: Jonathan Chon, William Tan
#EMAIL: jonchon@gmail.com, willtan510@gmail.com
#ID: 104780881, 104770108

CC=gcc
FLAGS= -Wall -Wextra
default:
	@$(CC) $(FLAGS) lab3a.c -o lab3a

clean:
	@rm -f lab3a *tar.gz

dist: default
	@tar -czf lab3a-104780881.tar.gz lab3a.c README Makefile ext2_fs.h
