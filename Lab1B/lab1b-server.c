#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <netinet/in.h>
#include <string.h>
#include <getopt.h>
#include <termios.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <mcrypt.h>

int socketFD, socketFD2, port_num, encryptFD;
socklen_t client_len;
int encrypt_flag = 0;
int temp_errno;
int process_id;
int tchild[2], fchild[2];
char * buffer;
char * encrypt_file = NULL;
char * log_file = NULL;
struct sockaddr_in server_address, client_address;
struct pollfd pfd[2];
MCRYPT encrypt_descriptor, decrypt_descriptor;

void init_socket()
{
  memset((char *) &server_address, 0, sizeof(server_address));
  server_address.sin_family = AF_INET;
  server_address.sin_addr.s_addr = INADDR_ANY;
  server_address.sin_port = htons(port_num);
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

void exit_code()
{
  close (tchild[0]);
  close (tchild[1]);
  close (fchild[0]);
  close (fchild[1]);

  close(socketFD2);
  if (encrypt_flag)
    {
      mcrypt_generic_deinit(encrypt_descriptor);
      mcrypt_module_close(encrypt_descriptor);
    }

  int status;
  if (waitpid(process_id, &status, 0) == -1)
    {
      temp_errno = errno;
      fprintf(stderr, "%s\n", strerror(temp_errno));
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

void signal_handler (int signal_num)
{
  if (signal_num == SIGINT)
    {
      kill(process_id, SIGINT);
      exit_code();
    }
  if (signal_num == SIGPIPE)
    {
      exit_code();
      exit(1);
    }
}

void read_write()
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
          read(socketFD2, buffer, 1);
	  if (encrypt_flag)
	    {
	      mdecrypt_generic(decrypt_descriptor, buffer, 1);
	    }
	  if (*buffer == '\003') //^C                           
            {
              kill(process_id, SIGINT);
              exit(0);
            }
          else if (*buffer == '\004') //^D
            {
	      close(tchild[1]);
            }
          if (*buffer == '\r' || *buffer == '\n')
            {
              char pipe_buffer[1] = {'\n'};
	      write (tchild[1], &pipe_buffer, 1);
            }
          else
            {
              write(tchild[1], buffer, 1);
            }
        }
      if (pfd[1].revents & POLLIN)
        {
          read(fchild[0], buffer, 1);
	  if (encrypt_flag)
	    {
	      if (mcrypt_generic(encrypt_descriptor, buffer, 1) != 0)
		{
		  temp_errno = errno;
		  fprintf(stderr, "%s\n", strerror(temp_errno));
		  exit(1);
		}
	    }
          write(socketFD2, buffer, 1);
	}
      if (pfd[1].revents & (POLLERR | POLLHUP))
	{
	  exit(0);
	}
    }
}

void call_poll()
{
  process_id = fork();

  pfd[0].fd = socketFD2;
  pfd[1].fd = fchild[0];
  pfd[0].events = POLLIN | POLLHUP | POLLERR;
  pfd[1].events = POLLIN | POLLHUP | POLLERR;

  if (process_id > 0) //parent process                                          
    {
      close(tchild[0]);
      close(fchild[1]);

      read_write();
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

void call_encryption()
{
  char * key;
  char * init_vector;
  int i = 0;
  struct stat key_file;
  int key_length;
  int IV_size;
  if ((encryptFD = open(encrypt_file, O_RDONLY)) < 0)
    {
      temp_errno = errno;
      fprintf(stderr, "%s\n", strerror(temp_errno));
      exit(1);
    }
  
  if (fstat(encryptFD, &key_file) < 0)
    {
      temp_errno = errno;
      fprintf(stderr, "%s\n", strerror(temp_errno));
      exit(1);
    }
  key_length = key_file.st_size;
  key = (char *) malloc (key_length);
  if (read(encryptFD, key, key_length) < 0)
    {
      temp_errno = errno;
      fprintf(stderr, "%s\n", strerror(temp_errno));
      exit(1);
    }
  close(encryptFD);
  if ((encrypt_descriptor = mcrypt_module_open("blowfish", NULL, "cfb", NULL)) == MCRYPT_FAILED)
    {
      temp_errno = errno;
      fprintf(stderr, "%s\n", strerror(temp_errno));
      exit(1);
    }
  if ((decrypt_descriptor = mcrypt_module_open("blowfish", NULL, "cfb", NULL)) == MCRYPT_FAILED)
    {
      temp_errno = errno;
      fprintf(stderr, "%s\n", strerror(temp_errno));
      exit(1);
    }
  IV_size = mcrypt_enc_get_iv_size(encrypt_descriptor);
  init_vector = malloc(IV_size);
  for (i = 0; i < IV_size; i++)
    {
      init_vector[i] = 1;
    }
  if (mcrypt_generic_init(encrypt_descriptor, key, key_length, init_vector) < 0)
    {
      temp_errno = errno;
      fprintf (stderr, "%s\n", strerror(temp_errno));
    }
  if (mcrypt_generic_init(decrypt_descriptor, key, key_length, init_vector) < 0)
    {
      temp_errno = errno;
      fprintf (stderr, "%s\n", strerror(temp_errno));
      exit(1);
    }
}

int main(int argc, char * argv[])
{
  int opt;
  buffer = (char*) malloc (256 * sizeof(char*));
  static struct option options[] = 
    {
      {"port", required_argument, 0, 'p'},
      {"encrypt", required_argument, 0, 'e'},
      {0, 0, 0, 0}
    };
  
  while ((opt = getopt_long(argc, argv, "p:e", options, 0)) != -1)
    {
      switch (opt)
	{
	case 'p':
	  port_num = atoi(optarg);
	  break;
	case 'e':
	  encrypt_file = optarg;
	  encrypt_flag = 1;
	  break;
	case '?':
	  temp_errno = errno;
	  fprintf(stderr, "%s,\n", strerror(temp_errno));
	  exit(1);
	}
    }  
  
  atexit(exit_code);

  if (encrypt_flag)
    {
      call_encryption();
    }

  signal(SIGINT, signal_handler);
  signal(SIGPIPE, signal_handler);

  socketFD = socket(AF_INET, SOCK_STREAM,0);
  if (socketFD < 0)
    {
      temp_errno = errno;
      fprintf(stderr, "%s\n", strerror(temp_errno));
      exit(1);
    }

  init_socket();
  
  if (bind(socketFD, (struct sockaddr *) &server_address, sizeof(server_address)) < 0)
    {
      temp_errno = errno;
      fprintf(stderr, "%s\n", strerror(temp_errno));
      exit(1);
    }

  listen(socketFD, 5);
  client_len = sizeof(client_address);
  
  socketFD2 = accept(socketFD, (struct sockaddr *) &client_address, &client_len);
  if (socketFD2 < 0)
    {
      temp_errno = errno;
      fprintf(stderr, "%s\n", strerror(temp_errno));
      exit(1);
    }
  
  create_pipe(tchild);
  create_pipe(fchild);

  call_poll();
  free(buffer);
  return 0;
}
