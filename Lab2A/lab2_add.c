/*
NAME: Jonathan Chon
EMAIL: jonchon@gmail.com
ID: 104780881
*/

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <pthread.h>
#include <time.h>
#include <errno.h>
#include <string.h>

long long counter = 0;
int thread_num = 1;
int iter_num = 1;
int temp_errno;
int yield_flag = 0;
char opt_sync;
char return_value[16] = "add";
pthread_mutex_t mutex;
int spin = 0;

void add(long long *pointer, long long value)
{
  long long sum = *pointer + value;
  if (yield_flag)
  {
    sched_yield();
  }
  *pointer = sum;
}

void add_compare_and_swap(long long *pointer, long long value)
{
  long long sum, old;
  do
  {
    old = *pointer;
    sum = old + value;
    if (yield_flag)
    {
      sched_yield();
    }   
  }while (__sync_val_compare_and_swap (pointer, old, sum) != old);
}

char * getTag()
{
  if (yield_flag)
  {
    strcat(return_value, "-yield");
  }
  switch (opt_sync)
  {
    case 'm':
      strcat(return_value, "-m");
      break;
    case 's':
      strcat(return_value, "-s");
      break;
    case 'c':
      strcat(return_value, "-c");
      break;
    default:
      strcat(return_value, "-none");
      break;
   }
  return return_value;
}

void *add_subtract(void * counter)
{
  counter = (long long *) counter;
  int i;
  for (i = 0; i < iter_num; i++)
  {
    if (opt_sync == 'm')
    {
      pthread_mutex_lock(&mutex);
      add(counter,1);
      pthread_mutex_unlock(&mutex);
    }
    else if (opt_sync == 's')
    {
      while(__sync_lock_test_and_set(&spin, 1));
      add(counter, 1);
      __sync_lock_release(&spin);
    }
    else if (opt_sync == 'c')
    {
      add_compare_and_swap(counter, 1);
    }
    else
    {
      add(counter, 1);
    }
  }
  for (i = 0; i < iter_num; i++)
  {
    if (opt_sync == 'm')
    {
      pthread_mutex_lock(&mutex);
      add(counter, -1);
      pthread_mutex_unlock(&mutex);
    }
    else if (opt_sync == 's')
    {
      while(__sync_lock_test_and_set(&spin, 1));
      add(counter, -1);
      __sync_lock_release(&spin);
    }
    else if (opt_sync == 'c')
    {
      add_compare_and_swap(counter, -1);
    }
    else
    {
      add(counter, -1);
    }
  }
  return NULL;
}

int main (int argc, char ** argv)
{
  int opt;
  struct timespec begin, finish;
  
  static struct option options[] = 
  {
    {"threads", required_argument, 0, 't'},
    {"iterations", required_argument, 0, 'i'},
    {"yield", no_argument, 0, 'y'},
    {"sync", required_argument, 0, 's'},
    {0,0,0,0}
  };
  while ((opt = getopt_long(argc, argv, "t:i:ys:", options, NULL)) != -1)
  {
    switch (opt)
    {
      case 't':
        thread_num = atoi(optarg);
        break;
      case 'i':
        iter_num = atoi(optarg);
        break;
      case 'y':
        yield_flag = 1;
        break;
      case 's':
        if (strlen(optarg) == 1 && (optarg[0] == 'm' || optarg[0] == 's' || optarg[0] == 'c'))
        {
          opt_sync = optarg[0];
	  if (opt_sync == 'm')
	    {
	      pthread_mutex_init(&mutex, NULL);
	    }
        }
        else
        {
          fprintf(stderr, "Incorrect arguments for sync. Valid options: m, s, c");
          exit(1);
        }
	break;  
      default:
	fprintf(stderr, "Usage: ./lab2_add [--threads=] [--yield] [--sync=] [--iterations=]");
	exit(1);
    }
  }
  
  if (clock_gettime(CLOCK_MONOTONIC, &begin) == -1)
  {
    temp_errno = errno;
    fprintf (stderr, "%s", strerror(temp_errno));
    exit(1);
  }
  
  pthread_t *threads = malloc(thread_num * sizeof(pthread_t));
  int i;
  for (i = 0; i < thread_num; i++)
  {
    if (pthread_create(threads + i, NULL, add_subtract, &counter))
    {
      temp_errno = errno;
      fprintf(stderr, "%s", strerror(temp_errno));
      exit(1);
    }
  }
  for (i = 0; i < thread_num; i++)
  {
    if (pthread_join(*(threads + i), NULL))
    {
      temp_errno = errno;
      fprintf(stderr, "%s", strerror(temp_errno));
      exit(1);
    }
  }
  
  if (clock_gettime(CLOCK_MONOTONIC, &finish) == -1)
  {
    temp_errno = errno;
    fprintf(stderr, "%s", strerror(temp_errno));
    exit(1);
  }

  free(threads);
  
  int operations = thread_num * iter_num * 2;
  long time = (finish.tv_sec - begin.tv_sec)*1000000000 + (finish.tv_nsec - begin.tv_nsec);
  int time_per_op = time/operations; 
  char * tag = getTag();
  
  printf ("%s,%d,%d,%d,%ld,%d,%lld\n", tag, thread_num, iter_num, operations, time, time_per_op, counter);
  exit(0);
}
