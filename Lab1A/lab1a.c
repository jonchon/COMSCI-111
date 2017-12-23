/*
NAME: Jonathan Chon
EMAIL: jonchon@gmail.com
ID: 104780881
*/

#include <termios.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <sys/poll.h>
#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>

struct termios original_terminal_attributes;
char * buffer;
int size_buffer = sizeof(char) * 256;
static int shell_flag = 0;
int process_id;
int tchild[2], fchild[2];
int temp_errno;
struct pollfd pfd[2];

void reset_input_terminal()
{
  tcsetattr(STDIN_FILENO, TCSANOW, &original_terminal_attributes);
  if (shell_flag)
    {
      
      close (tchild[0]);
      close (tchild[1]);
      close (fchild[0]);
      close (fchild[1]);
      
      int status;
      if (waitpid(process_id, &status, 0) == -1)
	{
	  temp_errno = errno;
	  fprintf(stderr, "%s", strerror(temp_errno));
	  exit(1);
	}
      
      if (WIFEXITED(status))
	{
	  int first = ((status) & 0x7f);
	  int last = (((status) & 0xff00) >> 8);
      	  fprintf(stderr, "SHELL EXIT SIGNAL=%d STATUS=%d\n", first, last);
	  exit(0);
	}
    }
}

void set_input_terminal()
{
  struct termios terminal_attributes;
  if (!isatty (STDIN_FILENO))
    {
      fprintf (stderr, "Not a terminal.\n");
      exit (1);
    }

  tcgetattr (STDIN_FILENO, &original_terminal_attributes);
  atexit (reset_input_terminal);

  tcgetattr (STDIN_FILENO, &terminal_attributes);
  terminal_attributes.c_lflag &= ~(ICANON|ECHO);
  terminal_attributes.c_cc[VMIN] = 1;
  terminal_attributes.c_cc[VTIME] = 0;
  terminal_attributes.c_oflag = 0;
  terminal_attributes.c_lflag = 0;
  terminal_attributes.c_iflag |= ISTRIP;
  tcsetattr (STDIN_FILENO, TCSANOW, &terminal_attributes);
}

void signal_handler (int signal_num)
{
  if (signal_num == SIGINT)
    {
      kill(process_id, SIGINT);
      reset_input_terminal();
    }
  if (signal_num == SIGPIPE)
    {
      reset_input_terminal();
      exit(1);
    }
}

void create_pipe(int temp_pipe[2])
{
  if (pipe(temp_pipe) == -1)
    {
      temp_errno = errno;
      fprintf(stderr, "%s", strerror(errno));
      exit(1);
    }
}

void write_special()
{
  char special_buffer[2] = {'\r', '\n'};
  write (STDOUT_FILENO, &special_buffer, 2);
}

void read_write()
{
  while (1)
    {
      read (STDIN_FILENO, buffer, size_buffer);
      //Check for ^D
      if (*buffer == '\004')
	{
	  write (STDOUT_FILENO, "^D\n", 3);
	  reset_input_terminal();
	  exit(0);
	}
      //<cr> or <lf> to <cr><lf>
      else if (*buffer == '\r' || *buffer == '\n')
	{
	  write_special();
	}
      else
	write (STDOUT_FILENO, buffer, 1);
    }
}

void shell_read_write()
{  
  while (1)
    {
      if (poll(pfd, 2, 0) < 0)
	{
	  temp_errno = errno;
	  fprintf(stderr, "%s", strerror(temp_errno));
	  exit(1);
	}

      if (pfd[0].revents & POLLIN)
        {
          read(STDIN_FILENO, buffer, 1);
          
	  if (*buffer == '\003') //^C
	    {
	      write(STDOUT_FILENO, "^C\n", 3);
	      kill(process_id, SIGINT);
	      exit(0);
	    }
	  else if (*buffer == '\004') //^D
	    {
	      write(STDOUT_FILENO, "^D\n", 3);
	      close(tchild[1]);
	    }
	  if (*buffer == '\r' || *buffer == '\n')
            {
              write_special();
              char pipe_buffer[1] = {'\n'};
              write (tchild[1], &pipe_buffer, 1);
            }
	  else
	    {
	      write(STDOUT_FILENO, buffer, 1);
	      write(tchild[1], buffer, 1); 
	    }
	}
      if (pfd[1].revents & POLLIN)
        {
	  read(fchild[0], buffer, 1);
          if (*buffer == '\n')
     	{
      	  char pipe_buffer[2] = {'\r', '\n'};
       	  write (STDOUT_FILENO, &pipe_buffer, 2);
       	}
      else
       	{
       	  write(STDOUT_FILENO, buffer, 1);
       	}
     }
     if (pfd[1].revents & (POLLERR | POLLHUP))
       {
         exit(0);
       }
    }
}

void call_shell()
{
  create_pipe(tchild);
  create_pipe(fchild);

  process_id = fork();
  
  pfd[0].fd = STDIN_FILENO;
  pfd[1].fd = fchild[0];
  pfd[0].events = POLLIN | POLLHUP | POLLERR;
  pfd[1].events = POLLIN | POLLHUP | POLLERR;

  if (process_id > 0) //parent process
    {
      close(tchild[0]);
      close(fchild[1]);
      
      shell_read_write();
    }
  else if (process_id == 0) //child process
    {
      close(tchild[1]);
      close(fchild[0]);
      dup2(tchild[0], STDIN_FILENO);
      dup2(fchild[1], STDOUT_FILENO);
      dup2(fchild[1], STDERR_FILENO);
      close(tchild[0]);
      close(fchild[1]);

      char * name = "/bin/bash";
      char * arg2[2] = {name, NULL};
      if (execvp(name, arg2) == -1)
	{
	  temp_errno = errno;
	  fprintf(stderr, "%s\n", strerror(temp_errno));
	  exit(1);
	}
    }
  else //fork() failed
    {
      temp_errno = errno;
      fprintf(stderr, "%s", strerror(temp_errno));
    }
}

int main (int argc, char ** argv)
{
  int opt;

  buffer = (char*) malloc (size_buffer);

  static struct option options[] = 
    {
      {"shell", no_argument, &shell_flag, 's'},
      {0,0,0,0}
    };
  set_input_terminal();
  while ((opt = getopt_long(argc, argv, "s", options, 0)) != -1)
    {
      switch (opt)
	{
	case 's':
	  signal(SIGINT, signal_handler);
	  signal(SIGPIPE, signal_handler);
	  break;
	case '?':
	  fprintf(stderr, "Correct usage: ./lab1 [--shell]\n");
	  exit(1);
	}
    }

  if (shell_flag)
    {
      call_shell();
      exit(0);
    }

  read_write();

  exit (0);
}
