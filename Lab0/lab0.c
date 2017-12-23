/*
NAME: Jonathan Chon
EMAIL: jonchon@gmail.com
ID:104780881
*/

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <getopt.h>
#include <signal.h>
#include <errno.h>
#include <string.h>

void error_usage()
{
  printf ("Correct usage: ./lab0 [--input=file], [--output=file], [--segfault], [--catch]\n");
}

void segmentation_fault()
{
  char * fault = NULL;
  *fault = 'E';
}

void signal_handler(int signum)
{
  if (signum == SIGSEGV)
    {
      fprintf(stderr, "Error: Segmentation Fault found\n");
      exit(4);
    }
}

int main(int argc, char **argv)
{
  char * input = NULL;
  char * output = NULL;
  static int segfault_flag = 0;
  int input_fd = 0, output_fd = 1;
  char * buf;
  ssize_t val;
  int opt;
  int error;
  static struct option options[] = 
  {
    {"input", 1, 0, 'i'},
    {"output", 1, 0, 'o'},
    {"segfault", 0, &segfault_flag, 's'},
    {"catch", 0, 0, 'c'},
    {0, 0, 0, 0}
  };

  while ((opt = getopt_long(argc, argv, "iosc", options, 0)) != -1)
    {
      switch (opt)
	{
	case 'i':
	  input = optarg;
      	  break;
	case 'o':
	  output = optarg;
	  break;
	case 's':
	  break;
	case 'c':
	  signal(SIGSEGV, signal_handler);
	  break;
	case '?':
	  //Error message for flags not defined 
	  error_usage();
	  exit(1);
 	}
    }
  if (input)
    {
      input_fd = open(input, O_RDONLY);
      if (input_fd != -1)
	{
	  close(0);
	  dup(input_fd);
	  close(input_fd);
	}      
      else
	{
	  error = errno;
	  fprintf(stderr, "Error with --input. Failed to open %s. %s\n", input, strerror(error));
	  //fprintf(stderr, "Error: Failure to open %s. Problem with --input\n", input);
	  exit(2);
	}
    }
  if (output)
    {
      output_fd = creat(output, S_IRWXU);
      if (output_fd != -1)
	{
	  close(1);
	  dup(output_fd);
	  close(output_fd);
	}
      else
	{
	  error = errno;
	  fprintf(stderr, "Failure to create file %s. Problem with --output. %s.\n", output, strerror(error));
	  exit(3);
	}
    }
  if (segfault_flag)
    segmentation_fault();
  
  buf = (char *) malloc (sizeof(char));
  while ((val = read(0, buf, 1)) > 0)
    {
      write(1, buf, 1);
    }
  free(buf);
  exit(0);
}
