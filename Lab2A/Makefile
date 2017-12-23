#NAME: Jonathan Chon
#EMAIL: jonchon@gmail.com
#ID: 104780881

CC = gcc
FLAGS = -Wall -Wextra -lpthread
PNG = lab2_add-1.png lab2_add-2.png lab2_add-3.png lab2_add-4.png lab2_add-5.png lab2_list-1.png lab2_list-2.png lab2_list-3.png lab2_list-4.png
SOURCE = lab2_add.c SortedList.h SortedList.c lab2_list.c
CSV = lab2_add.csv lab2_list.csv
SCRIPT = addscript.sh listscript.sh

build: add list

add:
	@$(CC) $(FLAGS) -o lab2_add -g lab2_add.c

list:
	@$(CC) $(FLAGS) -o lab2_list -g SortedList.c lab2_list.c

tests: clean build
	@chmod +x addscript.sh
	@./addscript.sh
	@chmod +x listscript.sh
	@./listscript.sh

graphs: clean tests
	@/usr/local/cs/bin/gnuplot ./lab2_add.gp
	@/usr/local/cs/bin/gnuplot ./lab2_list.gp

dist: graphs
	@tar -czf lab2a-104780881.tar.gz $(SOURCE) $(PNG) $(CSV) $(SCRIPT) README Makefile 

clean:
	@rm -rf *.tar.gz $(PNG) $(CSV) lab2_add lab2_list 
