#NAME: Jonathan Chon
#EMAIL: jonchon@gmail.com
#ID: 104780881

FLAGS=-Wall -Wextra -g -lmraa -lm

default:
	@gcc $(FLAGS) lab4c_tcp.c -o lab4c_tcp
	@gcc $(FLAGS) -lssl -lcrypto lab4c_tls.c -o lab4c_tls

clean:
	@rm -f lab4c_tcp lab4c_tls lab4c-104780881.tar.gz

dist: clean default
	@tar -czf lab4c-104780881.tar.gz README Makefile lab4c_tcp.c lab4c_tls.c
