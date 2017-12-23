#NAME: Jonathan Chon
#EMAIL: jonchon@gmail.com
#ID: 104780881

CC = gcc
FLAGS = -Wall -Wextra -lmraa -lm -g

default:
	@$(CC) $(FLAGS) lab4b.c -o lab4b

check: default
	@chmod 777 smoke_check.sh
	@./smoke_check.sh
clean:
	@rm -f lab4b *.txt lab4b-104780881.tar.gz

dist: default
	@tar -czf lab4b-104780881.tar.gz README lab4b.c Makefile smoke_check.sh
