#Name: Jonathan Chon
#Email: jonchon@gmail.com
#ID: 104780881

CC = gcc
CFLAGS = -g -Wall -Wextra

all: 
	@$(CC) $(CFLAGS) lab0.c -o lab0

clean:
	@rm -f lab0 *.tar.gz

dist: all
	@tar -czf lab0-104780881.tar.gz Makefile README backtrace.png breakpoint.png lab0.c

check: all compare_test exit_code_test_0 exit_code_test_1 exit_code_test_2_pt1 exit_code_test_2_pt2 exit_code_test_3 exit_code_test_4

compare_test:
#checks whether a file input successfully copies to a file output
	@echo "this tests for the same stuff in input and output" > input.txt; \
	./lab0 --input input.txt --output output.txt &> /dev/null ; \
	cmp input.txt output.txt; \
	if [ $$? -ne 0 ] ; \
	then \
		echo "Error: Copying from input.txt to output.txt failed" ; \
	fi
	@rm -f input.txt output.txt

exit_code_test_0:
#Checks for successful exit code 0 printing to output
#Different from compare test since compare test uses cmp to test what
#was in each file but this just tests exit codes
	@echo "This is a test for exit code 0" > input.txt; \
	./lab0 --input input.txt --output output.txt &>/dev/null; \
	if [ $$? -ne 0 ]; then \
		echo "Error: Didn't exit with exit code 0" ;\
	fi
	@rm -f input.txt output.txt

exit_code_test_1:
#Checks for exit code 1 with unrecognized argument
	@./lab0 --notanoption &>/dev/null; \
	if [ $$? -ne 1 ]; then \
		echo "Error: Didn't exit with exit code 1" ;\
	fi

exit_code_test_2_pt1:
#Checks for exit code 2 with non-existing input file
	@./lab0 --input notafile.txt &>/dev/null; \
	if [ $$? -ne 2 ]; then \
		echo "Error: Didn't exit with exit code 2" ;\
	fi

exit_code_test_2_pt2:
#Checks for exit code 2 with output file that cannot be opened
	@echo "This file cannot be opened" > cantbeopened.txt; \
	chmod -777 cantbeopened.txt; \
	./lab0 --input cantbeopened.txt &>/dev/null; \
	if [ $$? -ne 2 ]; then \
		echo "Error: Didn't exit with exit code 2" ;\
	fi
	@rm -f cantbeopened.txt

exit_code_test_3:
#Checks for exit code 3 with output file that cannot be opened
	@echo "This is the input file" > aninputfile.txt; \
	touch anoutputfile.txt; \
	chmod -w anoutputfile.txt; \
	./lab0 --input aninputfile.txt --output anoutputfile.txt &>/dev/null; \
	if [ $$? -ne 3 ]; then \
		echo "Error: Didn't exit with exit code 3" ;\
	fi
	@rm -f aninputfile.txt anoutputfile.txt 

exit_code_test_4:
#checks for errors with --check  
	@./lab0 --segfault --catch &>/dev/null; \
	if [ $$? -ne 4 ]; \
	then \
		echo "Error: Didn't exit with exit code 4" ; \
	fi
