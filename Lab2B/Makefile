#NAME: Jonathan Chon
#EMAIL: jonchon@gmail.com
#ID: 104780881

CC = gcc
FLAGS = -lrt -Wall -Wextra -pthread -g
FILES = lab2b_1.png lab2b_2.png lab2b_3.png lab2b_4.png lab2b_5.png SortedList.h SortedList.c lab2_list.c lab2b_list.csv listscript.sh README Makefile profile.out lab2_list.gp

build:
	@$(CC) $(FLAGS) -lrt -o lab2_list SortedList.c lab2_list.c

tests: clean build
	@chmod +x listscript.sh
	@./listscript.sh

graphs: tests
	@/usr/local/cs/bin/gnuplot ./lab2_list.gp

profile: clean build
	@rm -f ./raw.gperf
	@LD_PRELOAD=/u/cs/ugrad/chon/lib/libprofiler.so.0 CPUPROFILE=./raw.gperf ./lab2_list --threads=12 --iterations=1000 --sync=s
	@pprof --text ./lab2_list ./raw.gperf > profile.out
	@pprof --list=list_ops ./lab2_list ./raw.gperf >> profile.out
	@rm -f ./raw.gperf


dist: clean build graphs profile
	@tar -czf lab2b-104780881.tar.gz $(FILES)

clean:
	@rm -rf lab2_list lab2b-104780881.tar.gz
