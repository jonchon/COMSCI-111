/*
NAME: Jonathan Chon
EMAIL: jonchon@gmail.com
ID: 104780881
 */

#include <mraa.h>
#include <mraa/aio.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <math.h>
#include <errno.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <poll.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

int period = 1;
char scale = 'F';
int temp_errno;
char * log_file = NULL;
int logFD;
int log_flag = 0;
int continue_flag = 1;
int buff_length = 0;
int stop_flag = 0;

int port;
int id;
int socketFD;
struct sockaddr_in server_address;
struct hostent *server;
char * host;

SSL_CTX *context;
SSL *ssl;

float get_temp(int voltage)
{
  int B = 4275;
  float r0 = 100000.0;
  float r = 1023.0/((float)voltage) - 1.0;
  r = r0 * r;
  float celsius = 1.0/(log(r/r0)/B+1/298.15) - 273.15;
  if (scale == 'F')
  {
    return celsius * 9.0/5.0 + 32.0;
  }
  return celsius;
}

void get_time(char * temp_time)
{
  time_t t1;
  time(&t1);
  struct tm * tm1;
  if (!(tm1 = localtime(&t1)))
  {
    fprintf(stderr, "Error getting local time");
    exit(1);
  }
  strftime(temp_time, 9, "%H:%M:%S", tm1);
}

void start_shutdown()
{
  char end_time[9];
  get_time(end_time);
  
  char shutdown_buff[50];
  sprintf(shutdown_buff, "%s SHUTDOWN\n", end_time);
  SSL_write(ssl, shutdown_buff, strlen(shutdown_buff));
  if (log_flag)
  {
    dprintf(logFD, "%s SHUTDOWN\n", end_time);
  }
  exit(0);
}

void process_input(char * buff)
{
    if (strcmp(buff, "SCALE=F") == 0)
    {
      scale = 'F';
      if (log_flag)
      {
        dprintf(logFD, "%s\n", buff);
      }
    }
    else if (strcmp(buff, "SCALE=C") == 0)
    {
      scale = 'C';
      if (log_flag)
      {
        dprintf(logFD, "%s\n", buff);
      }
    }
    else if (strncmp(buff, "PERIOD=", 7) == 0)
    {
      int j = 0;
      char temp_buff[10];
      while (buff[j+7] != '\0')
      {
        temp_buff[j] = buff[j+7];
        j++;
      }
      temp_buff[j+1] = '\0';
      period = atoi(temp_buff);
      if (log_flag)
      {
        dprintf(logFD, "%s\n", buff);
      }
    }
    else if (strncmp(buff, "LOG", 3) == 0)
    {
      if (log_flag)
      {
        dprintf(logFD, "%s\n", buff);
      }
    }
    else if (strcmp(buff, "STOP") == 0)
    {
      stop_flag = 1;
      if (log_flag)
      {
        dprintf(logFD, "%s\n", buff);
      }
    }
    else if (strcmp(buff, "START") == 0)
    {
      stop_flag = 0;
      if (log_flag)
      {
        dprintf(logFD, "%s\n", buff);
      }
    }
    else if (strcmp(buff, "OFF") == 0)
    {
      if (log_flag)
      {
        dprintf(logFD, "%s\n", buff);
      }
      start_shutdown();
    }
}

int main (int argc, char ** argv)
{
  int opt;
  int temp_voltage = 0;
  time_t period_start, period_end;
  static struct option options[] = 
  {
    {"period", required_argument, 0, 'p'},
    {"scale", required_argument, 0, 's'},
    {"log", required_argument, 0, 'l'},
    {"id", required_argument, 0, 'i'},
    {"host", required_argument, 0, 'h'}
  };

  while ((opt = getopt_long(argc, argv, "pslih", options, NULL)) != -1)
  {
    switch (opt)
      {
      case 'p':
      	period = atoi(optarg);
	      break;
      case 's':
	      if ((optarg[0] == 'C' || optarg[0] == 'F') && strlen(optarg) == 1)
	      {
	        scale = optarg[0];
	      }
	      else
	      {
	        fprintf(stderr, "Error: valid options for scale [C, F]");
	        exit(1);
	      }
	      break;
      case 'l':
	      log_flag = 1;
        log_file = optarg;
        if ((logFD = creat(log_file, S_IRWXU)) < 0)
        {
          temp_errno = errno;
          fprintf(stderr, "%s", strerror(temp_errno));
          exit(1);
        }
	      break;
      case 'i':
        if (strlen(optarg) != 9)
        {
          fprintf(stderr, "ID must be 9 numbers");
          exit(1);
        }
        id = atoi(optarg);
        break;
      case 'h':
        host = optarg;
        break;
      default:
	      fprintf(stderr, "Usage: ./lab4 [--period=] [--scale=] [--log=] [--id=] [--host=] port");
	      exit(1);
      }
  }
  //get the port number
  port = atoi(argv[argc - 1]);
  
  //socket
  if ((socketFD = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  {
    fprintf(stderr, "Error in creating socket\n");
    exit(2);
  }
  
  //server
  if ((server = gethostbyname(host)) == NULL)
  {
    fprintf(stderr, "Error in finding host\n");
    exit(1);
  }
  
  //initialize connection
  server_address.sin_family = AF_INET;
  server_address.sin_port = htons(port);
  memcpy((char *) &server_address.sin_addr.s_addr, (char *) server->h_addr, server->h_length);
  if (connect(socketFD, (struct sockaddr*) &server_address, sizeof(server_address)) < 0)
  {
    fprintf(stderr, "Error in connecting to server");
    exit(1);
  }

  //SSL
  SSL_library_init();
  OpenSSL_add_all_algorithms();
  SSL_load_error_strings();
  context = SSL_CTX_new(SSLv23_client_method());
  if (context == NULL)
  {
    ERR_print_errors_fp(stderr);
    exit(2);
  }
  ssl = SSL_new(context);
  
  SSL_set_fd(ssl, socketFD);
  if (SSL_connect(ssl) < 0)
  {
    temp_errno = errno;
    fprintf(stderr, "%s\n", strerror(temp_errno));
    exit(2);
  }

  //initialize poll
  struct pollfd poll1[1];
  poll1[0].fd = socketFD;
  poll1[0].events = POLLIN | POLLHUP | POLLERR;
  
  //Report the ID
  fcntl(socketFD, F_SETFL, O_NONBLOCK);
  char ID_buff[25];
  sprintf(ID_buff, "ID=%d\n", id);
  SSL_write(ssl, ID_buff, strlen(ID_buff));
  if (log_flag)
  {
    dprintf(logFD, "ID=%d\n", id);
  }
  
  //initialize temperature sensor
  mraa_aio_context temp_sensor;
  temp_sensor = mraa_aio_init(1);
  
  while (continue_flag)
  {
    //Temperature
    temp_voltage = mraa_aio_read(temp_sensor);
    float temp_value = get_temp(temp_voltage);
    //Time
    
    char curr_time[9];
    get_time(curr_time);
  
    //Print
    //If statement added so if period goes from big to smaller when STOP, doesn't output
    if (!stop_flag)
    {
      char temp_buffer[25];
      memset(temp_buffer, 0, 25);
      sprintf(temp_buffer, "%s %.1f\n", curr_time, temp_value);
      SSL_write(ssl, temp_buffer, strlen(temp_buffer));
      if (log_flag)
      {
        dprintf(logFD, "%s %.1f\n", curr_time, temp_value);
      }
    }
    
    //Wait for period to be over
    time(&period_start);
    time(&period_end);
    while (difftime(period_end, period_start) < period)
    {
      if (poll (poll1, 1, 0) < 0)
	    {
	      temp_errno = errno;
	      fprintf(stderr, "%s", strerror(temp_errno));
	      exit(1);
	    }
      if (poll1[0].revents & POLLIN)
	    {
	      char buff[256];
        int ind = 0;
        while (SSL_read(ssl, &buff[ind], 1) > 0)
        {
          if (buff[ind] == '\n')
          {
            buff[ind] = '\0';
            ind = 0;
            break;
          }
          ind++;
        }
        process_input(buff);
	    }
         
      //handle the input
      if (!stop_flag)
        time(&period_end);
    }
  }
  mraa_aio_close(temp_sensor);
  exit(0);
}
