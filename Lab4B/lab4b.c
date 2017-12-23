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

int period = 1;
char scale = 'F';
int temp_errno;
char * log_file = NULL;
int logFD;
int log_flag = 0;
int continue_flag = 1;
int buff_length = 0;
int stop_flag = 0;

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

void shutdown()
{
  char end_time[9];
  get_time(end_time);
  if (log_flag)
    {
    
    }
  fprintf(stdout, "%s SHUTDOWN\n", end_time);
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
      //i+= 8;
    }
    else if (strcmp(buff, "SCALE=C") == 0)
    {
      scale = 'C';
      if (log_flag)
      {
        dprintf(logFD, "%s\n", buff);
      }
      //i+= 8;
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
      //i+= 8;
    }
    else if (strcmp(buff, "STOP") == 0)
    {
      stop_flag = 1;
      if (log_flag)
      {
        dprintf(logFD, "%s\n", buff);
      }
      //i+=5;
    }
    else if (strcmp(buff, "START") == 0)
    {
      stop_flag = 0;
      if (log_flag)
      {
        dprintf(logFD, "%s\n", buff);
      }
      //i+=6;
    }
    else if (strcmp(buff, "OFF") == 0)
    {
      if (log_flag)
      {
        dprintf(logFD, "%s\n", buff);
      }
      shutdown();
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
    {"log", required_argument, 0, 'l'}
  };

  while ((opt = getopt_long(argc, argv, "p:s:l:", options, NULL)) != -1)
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
      default:
	      fprintf(stderr, "Usage: ./lab4 [--period=] [--scale=] [--log=]");
	      exit(1);
      }
  }

  //initialize poll
  struct pollfd poll1[1];
  poll1[0].fd = STDIN_FILENO;
  poll1[0].events = POLLIN | POLLHUP | POLLERR;
  
  //initialize temperature sensor

  mraa_aio_context temp_sensor;
  temp_sensor = mraa_aio_init(1);

  //initialize button
  mraa_gpio_context button;
  button = mraa_gpio_init(62);
  mraa_gpio_dir(button, MRAA_GPIO_IN);
  mraa_gpio_isr(button, MRAA_GPIO_EDGE_RISING,&shutdown,NULL);
  
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
      fprintf(stdout, "%s %.1f\n", curr_time, temp_value);
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
	      char buff[25];
        if((buff_length = scanf("%s", buff)) != -1)
        {
          process_input(buff);
        }
        //Gets rid of \n and replaces with \0
	    }

      if (mraa_gpio_read(button))
	    {
	      shutdown();
	    }
         
      //handle the input and button shit
      if (!stop_flag)
        time(&period_end);
    }
  }
  mraa_aio_close(temp_sensor);
  mraa_gpio_close(button);
  exit(0);
}
