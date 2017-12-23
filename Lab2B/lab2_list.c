/*
NAME: Jonathan Chon
EMAIL: jonchon@gmail.com
ID: 104780881
*/

#include "SortedList.h"
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <pthread.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include <signal.h>

int thread_num = 1;
int iter_num = 1;
int list_num = 1;
int op_num = 2;
int opt_yield = 0;
char opt_sync;
char return_value[16] = "list-";
pthread_mutex_t * mutex; //changed
int yield_args[3] = {0,0,0};
int * spin; //changed
int * list_ind;
long * lock_time;
SortedList_t * list;
SortedListElement_t * element;
int temp_errno;
pthread_mutex_t global_mutex;
int global_spin;

void signal_handler()  //FINISHED
{
  fprintf(stderr, "Error: Segmentation Fault\n");
  exit(2);
}

char * getTag()  //FINISHED
{
  if (yield_args[0] == 0 && yield_args[1] == 0 && yield_args[2] == 0)
    strcat(return_value, "none");
  else
    {
      if (yield_args[0] == 1)
	strcat(return_value, "i");
      if (yield_args[1] == 1)
	strcat(return_value, "d");
      if (yield_args[2] == 1)
	strcat(return_value, "l");
    }

  switch (opt_sync)
    {
    case 'm':
      strcat(return_value, "-m");
      break;
    case 's':
      strcat(return_value, "-s");
      break;
    default:
      strcat(return_value, "-none");
    }
  return return_value;
}

void random_keys(int num)  //FINISHED
{
  srand(time(NULL));
  
  int i;
  for (i = 0; i < num; i++)
    {
      int key_len = rand() % 25 + 1;
      char * rand_key = (char *)malloc((key_len + 1) * sizeof(char));
      int k;
      int rand_char;
      for (k = 0; k < key_len; k++)
	{
	  rand_char = rand() % 26;
	  rand_key[k] = 'a' + rand_char;
	}
      rand_key[key_len] = '\0';
      (element + i)->key = rand_key;
    }
}

int check_length(SortedList_t * list_element)
{
  int length = 0;
  if ((length = SortedList_length(list_element)) == -1)
    {
      fprintf(stderr, "Error: Couldn't get list's length\n");
      exit(2);
    }
  return length;
}

void lookup_and_delete(SortedList_t * list_element, SortedListElement_t *element)  //Change
{
  SortedListElement_t * loc = (SortedListElement_t *)malloc(sizeof(SortedListElement_t));
  if ((loc = SortedList_lookup(list_element, element->key)) == NULL)
    {
      fprintf(stderr, "Failed to find element\n");
      exit(2);
    }
  if (SortedList_delete(loc))
    {
      fprintf(stderr, "Failed to delete element\n");
      exit(2);
    }
}

void check_clock(struct timespec * time)
{
  if (clock_gettime(CLOCK_MONOTONIC, time) == -1)
    {
      temp_errno = errno;
      fprintf(stderr, "%s", strerror(temp_errno));
      exit(1);
    }  
}

long clock_difference(struct timespec * start, struct timespec * end)
{
  return 1000000000 * (end->tv_sec - start->tv_sec) + (end->tv_nsec - start->tv_nsec);
}

void * list_ops(void* index)  //CHANGE
{
  int i;
  struct timespec lock_start, lock_end;
  int list_len = 0;
  long temp_time = 0;

  //inserts into list
  for (i = *(int *)index; i < op_num; i += thread_num)
    {
      if (opt_sync == 'm')
	{
       	  check_clock(&lock_start);
	  pthread_mutex_lock(&mutex[list_ind[i]]);
       	  check_clock(&lock_end);
	  temp_time += clock_difference(&lock_start, &lock_end);
	  SortedList_insert(&list[list_ind[i]], &element[i]);
	  pthread_mutex_unlock(&mutex[list_ind[i]]);	  
	}
      else if (opt_sync == 's')
	{
       	  check_clock(&lock_start);
	  while (__sync_lock_test_and_set(&spin[list_ind[i]], 1));
	  check_clock(&lock_end);
      	  temp_time += clock_difference(&lock_start, &lock_end);
	  SortedList_insert(&list[list_ind[i]], &element[i]);
	  __sync_lock_release(&spin[list_ind[i]]);
	}
      else
	{
	  SortedList_insert(&list[list_ind[i]], &element[i]);
	}
    } 

  //Checks length
  if (opt_sync == 'm')
    {
      for (i = 0; i < list_num; i++)
	{
	  int temp_len = 0;
	  //local structure
	  check_clock(&lock_start);
	  pthread_mutex_lock(&mutex[i]);
      	  check_clock(&lock_end);
      	  temp_time += clock_difference(&lock_start, &lock_end);
	  temp_len = check_length(&list[i]);
	  pthread_mutex_unlock(&mutex[i]);
	  
	  //global structure
          check_clock(&lock_start);
	  pthread_mutex_lock(&global_mutex);
          check_clock(&lock_end);
       	  temp_time += clock_difference(&lock_start, &lock_end);
	  list_len += temp_len;
	  pthread_mutex_unlock(&global_mutex);
	}
    }
  else if (opt_sync == 's')
    {
      for (i = 0; i < list_num; i++)
	{
	  //local structure
	  int temp_len = 0;
       	  check_clock(&lock_start);
	  while (__sync_lock_test_and_set(&spin[i], 1));
      	  check_clock(&lock_end);
      	  temp_time += clock_difference(&lock_start, &lock_end);
	  temp_len = check_length(&list[i]);
	  __sync_lock_release(&spin[i]);
	  
	  //global structure
          check_clock(&lock_start);
	  while (__sync_lock_test_and_set(&global_spin, 1));
	  check_clock(&lock_end);
	  temp_time += clock_difference(&lock_start, &lock_end);
	  list_len += temp_len;
	  __sync_lock_release(&global_spin);
	}
    }
  else
    {
      for (i = 0; i < list_num; i++)
	{
	  int temp_len = 0;
	  temp_len = check_length(&list[i]);
	  list_len += temp_len;
	}
    }

  //looksup and deletes
  for (i = *(int*)index; i < op_num; i += thread_num)
    {
      if (opt_sync == 'm')
	{
 	  check_clock(&lock_start);
	  pthread_mutex_lock(&mutex[list_ind[i]]);
       	  check_clock(&lock_end);
	  temp_time += clock_difference(&lock_start, &lock_end);
	  lookup_and_delete(&list[list_ind[i]], element + i);
	  pthread_mutex_unlock(&mutex[list_ind[i]]);
	}
      else if (opt_sync == 's')
	{
       	  check_clock(&lock_start);
	  while(__sync_lock_test_and_set(&spin[list_ind[i]], 1));
      	  check_clock(&lock_end);
	  temp_time += clock_difference(&lock_start, &lock_end);
	  lookup_and_delete(&list[list_ind[i]], element + i);
	  __sync_lock_release(&spin[list_ind[i]]);
	}
      else
	{
       	  lookup_and_delete(&list[list_ind[i]], element + i);
	}
    }  
  return (void *) temp_time;
}

int main (int argc, char ** argv)
{
  int opt;
  int i;
  unsigned int k;
  struct timespec begin, finish;
  long total_lock_time = 0;
  static struct option options[] =
    {
      {"threads", required_argument, 0, 't'},
      {"iterations", required_argument, 0, 'i'},
      {"yield", required_argument, 0, 'y'},
      {"sync", required_argument, 0, 's'},
      {"lists", required_argument, 0, 'l'},
      {0,0,0,0}
    };
  while ((opt = getopt_long(argc, argv, "t:i:y:s:l:", options, NULL)) != -1)
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
	  for (k = 0; k < strlen(optarg); k++)
	    {
	      if (optarg[k] == 'i')
		{
		  opt_yield |= INSERT_YIELD;
		  yield_args[0] = 1;
		}
	      else if (optarg[k] == 'd')
		{
		  opt_yield |= DELETE_YIELD;
		  yield_args[1] = 1;
		}
	      else if (optarg[k] == 'l')
		{
		  opt_yield |= LOOKUP_YIELD;
		  yield_args[2] = 1;
		}
	      else
		{
		  fprintf(stderr, "Invalid yield option. Choose from [i,d,l]\n");
		  exit(1);
		}
	    }
	  break;      
	case 's':
	  if (strlen(optarg) == 1 && (optarg[0] == 'm' || optarg[0] == 's'))
	    {
	      opt_sync = optarg[0];
	    }
	  else
	    {
	      fprintf(stderr, "Incorrect arguments for sync. Valid options: [m, s]");
	      exit(1);
	    }
	  break;
	case 'l':
	  list_num = atoi(optarg);
	  break;
	default:
	  fprintf(stderr, "Usage: ./lab2_add [--threads=] [--yield] [--sync=] [--iterations=] [lists=]");
	  exit(1);
	}
    }
  
  signal(SIGSEGV, signal_handler);
  op_num = iter_num * thread_num;
  long * thread_time = (long *) malloc(thread_num * sizeof(long));
  element = (SortedList_t *) malloc(op_num * sizeof(SortedListElement_t));
  list = (SortedList_t *) malloc(list_num * sizeof(SortedList_t));
  for (i = 0; i < list_num; i++)
    {
      list[i].key = NULL;
      list[i].next = &list[i];
      list[i].prev = &list[i];
    }
  
  if (opt_sync == 'm')
    {
      mutex = (pthread_mutex_t *)malloc(list_num * sizeof(pthread_mutex_t));
      for (i = 0; i < list_num; i++)
	{
	  pthread_mutex_init(&mutex[i], NULL);
	}
    }
  else if (opt_sync == 's')
    {
      spin = (int *)malloc(list_num * sizeof(int));
      for (i = 0; i < list_num; i++)
	{
	  spin[i] = 0;
	}
    }

  random_keys(op_num);

  list_ind = (int *)malloc(op_num * sizeof(int));
  for (i = 0; i < op_num; i++)
    {
      list_ind[i] = *(element[i].key) % list_num;
    }
  
  /*
  lock_time = malloc(list_num * sizeof(long long));
  for (i = 0; i < list_num; i++)
    {
      lock_time[i] = 0;
    }
  */

  pthread_t * threads = (pthread_t *)malloc(thread_num * sizeof(pthread_t));
  int * threadID = (int *)malloc(thread_num * sizeof(int));
 
  check_clock(&begin);
  
  for (i = 0; i < thread_num; i++)
    {
      threadID[i] = i;
    }

  for (i = 0; i < thread_num; i++)
    {
      if (pthread_create(threads + i, NULL, list_ops, &threadID[i]))
	{
	  temp_errno = errno;
	  fprintf(stderr, "%s", strerror(temp_errno));
	  exit(1);
	}
    }
  for (i = 0; i < thread_num; i++)
    {
      if (pthread_join(*(threads + i), (void **)(thread_time + i)))
	{
	  temp_errno = errno;
	  fprintf(stderr, "%s", strerror(temp_errno));
	  exit(1);
	}
    }

  check_clock(&finish);
  
  for (i = 0; i < thread_num; i++)
    {
      total_lock_time += thread_time[i];
    }

  /*
  if (clock_gettime(CLOCK_MONOTONIC, &finish) == -1)
    {
      temp_errno = errno;
      fprintf(stderr, "%s", strerror(temp_errno));
      exit(1);
    }
  */

  //  free(list);
  //free(element);
  //free(mutex);
  //free(spin);
  //free(threads);

  for (i = 0; i < list_num; i++)
    {
      if (SortedList_length(&list[i]))
	{
	  fprintf (stderr, "Error: list length is not 0");
	  exit(2);
	}
    }

  int operations = op_num * 3;
  long time = clock_difference(&begin, &finish);
  int time_per_op = time/operations;
  long average_lock_time = total_lock_time/operations;
  char * tag = getTag();

  printf ("%s,%d,%d,%d,%d,%ld,%d,%ld\n", tag, thread_num, iter_num, list_num,  operations, time, time_per_op, average_lock_time);

  exit(0);
}
