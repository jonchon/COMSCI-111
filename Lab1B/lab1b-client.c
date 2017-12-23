#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <netinet/in.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <sys/poll.h>
#include <getopt.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <mcrypt.h>

int socketFD, port_num, logFD, encryptFD;
static int log_flag = 0, encrypt_flag = 0;
int temp_errno;
int ind;
char log_buffer[1024];
char * buffer;
char * log_file = NULL;
char * encrypt_file = NULL;
struct sockaddr_in server_address;
struct hostent * server;
struct termios original_terminal_attributes;
struct pollfd pfd[2];
MCRYPT encrypt_descriptor, decrypt_descriptor;

void reset_input_terminal()
{
  if (log_flag && ind)
    {
      write (logFD, "RECEIVED ", 9);
      dprintf(logFD, "%d", ind);
      write (logFD, " bytes: ", 8);
      write (logFD, buffer, ind);
      write (logFD, "\n", 1);
      ind = 0;
    }
  tcsetattr(STDIN_FILENO, TCSANOW, &original_terminal_attributes);
  
  if (encrypt_flag)
    {
      mcrypt_generic_deinit(encrypt_descriptor);
      mcrypt_module_close(encrypt_descriptor);
      mcrypt_generic_deinit(decrypt_descriptor);
      mcrypt_module_close(decrypt_descriptor);
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

void create_socket()
{
  if ((socketFD = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
      temp_errno = errno;
      fprintf(stderr, "%s\n", strerror(temp_errno));
      exit(1);
    }
  if ((server = gethostbyname("127.0.0.1")) == NULL)
    {
      temp_errno = errno;
      fprintf(stderr, "%s\n", strerror(temp_errno));
      exit(1);
    }

  memset ((char *) &server_address, 0, sizeof(server_address));
  server_address.sin_family = AF_INET;
  memcpy ((char *) &server_address.sin_addr.s_addr, (char *) server->h_addr, server->h_length);
  server_address.sin_port = htons(port_num);
}

void set_poll()
{
  pfd[0].fd = STDIN_FILENO;
  pfd[1].fd = socketFD;
  pfd[0].events = POLLIN | POLLHUP | POLLERR;
  pfd[1].events = POLLIN | POLLHUP | POLLERR;
}

void call_encryption()
{
  char * key;
  char * init_vector;
  int i = 0;
  struct stat key_file;
  int key_length;
  int IV_size;
  int init;

  if ((encryptFD = open(encrypt_file, O_RDONLY)) < 0)
    {
      temp_errno = errno;
      fprintf (stderr, "%s\n", strerror(temp_errno));
      exit(1);
    }
  
  if (fstat(encryptFD, &key_file) < 0)
    {
      temp_errno = errno;
      fprintf (stderr, "%s\n", strerror(temp_errno));
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
  if ((init = mcrypt_generic_init(encrypt_descriptor, key, key_length, init_vector)) < 0)
    {
      temp_errno = errno;
      fprintf (stderr, "%s\n", strerror(temp_errno));
      exit(1);
    }
  if ((init = mcrypt_generic_init(decrypt_descriptor, key, key_length, init_vector)) < 0)
    {
      temp_errno = errno;
      fprintf (stderr, "%s\n", strerror(temp_errno));
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
	  if (log_flag && ind)
	    {
	      write (logFD, "RECEIVED ", 9);
	      dprintf(logFD, "%d", ind);
	      write(logFD, " bytes: ", 8);
	      write(logFD, buffer, ind);
	      write(logFD, "\n", 1);
	      ind = 0;
	    }
          read(STDIN_FILENO, buffer, 1);
	  
          if (*buffer == '\r' || *buffer == '\n')
            {
     	      char special_buffer[2] = {'\r', '\n'};
	      write (STDOUT_FILENO, &special_buffer, 2);
	      if (encrypt_flag)
		{
		  char temp = '\n';
		  mcrypt_generic(encrypt_descriptor, &temp, 1);
		  write (socketFD, &temp, 1);
		}
	      if (log_flag)
		{
		  write (logFD, "SENT 1 bytes: ", 14);
		  write (logFD, "\n\n", 2); //One cause of persons input and one cause of spec
		}
	      if (!encrypt_flag)
		{
		  write (socketFD, buffer, 1);
		}
            }	  
          else
	    {
              write(STDOUT_FILENO, buffer, 1);
	      if (encrypt_flag)
		{
		  mcrypt_generic(encrypt_descriptor, buffer, 1);
		}
	      if (log_flag)
		{
		  write (logFD, "SENT 1 bytes: ", 14);
		  write (logFD, buffer, 1);
		  write (logFD, "\n" , 1);
		}
	      write (socketFD, buffer, 1);
	    }
        }
      if (pfd[1].revents & POLLIN)
        {
          read(socketFD, buffer, 1);
	  if (log_flag && encrypt_flag)
	    {
	      log_buffer[ind] = *buffer;
	      ind += 1;
	    }
	  if (encrypt_flag)
	    {
	      mdecrypt_generic(decrypt_descriptor, buffer, 1);
	    }
          if (*buffer == '\n' || *buffer == '\r')
	    {
	      char pipe_buffer[2] = {'\r', '\n'};
	      write (STDOUT_FILENO, &pipe_buffer, 2);
	      if (log_flag)
		{
		  log_buffer[ind] = *buffer;
		  write (logFD, "RECEIVED ", 9);
		  dprintf(logFD, "%d", ind);
		  write(logFD, " bytes: ", 8);
		  write(logFD, &log_buffer, ind);
		  write(logFD, "\n", 1);
		  ind = 0;
		}
	    }
	  else
	    {
	      if (log_flag && !encrypt_flag)
		{
		  log_buffer[ind] = *buffer;
		  ind+=1;
		}
	      write(STDOUT_FILENO, buffer, 1);
	    }
	  if (*buffer == '\004')
	    {
	      close (socketFD);
	      exit(0);
	    }
	}
      if (pfd[1].revents & (POLLERR | POLLHUP))
	{
	  exit(0);
	}
    }
}

int main(int argc, char *argv[])
{
  int opt;

  buffer = (char *) malloc (sizeof(char*));
  static struct option options[] = 
    {
      {"port", required_argument, 0, 'p'},
      {"log", required_argument, 0, 'l'},
      {"encrypt", required_argument, 0, 'e'},
      {0, 0, 0, 0}
    };
  
  while ((opt = getopt_long(argc, argv, "p:le", options, 0)) != -1)
    {
      switch (opt)
	{
	case 'p':
	  port_num = atoi(optarg);
	  break;
	case 'l':
	  log_file = optarg;
	  if ((logFD = creat(log_file, S_IRWXU)) == -1)
	    {
	      temp_errno = errno;
	      fprintf(stderr, "%s\n", strerror(temp_errno));
	      exit(1);
	    }
	  log_flag = 1;
	  break;
	case 'e':
	  encrypt_flag = 1;
	  encrypt_file = optarg;
	  break;
	case '?':
	  temp_errno = errno;
	  fprintf(stderr, "%s\n", strerror(temp_errno));
	  exit(1);
	}
    }
  set_input_terminal();
  if (encrypt_flag)
    {
      call_encryption();
    }

  create_socket();

  if (connect(socketFD, (struct sockaddr*)&server_address, sizeof(server_address)) < 0)
    {
      temp_errno = errno;
      fprintf(stderr, "%s\n", strerror(temp_errno));
      exit(1);
    }
  
  set_poll();
  read_write();
  close(socketFD);
  free(buffer);
  return 0;
}
