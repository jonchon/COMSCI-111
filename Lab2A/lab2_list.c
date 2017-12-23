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
int op_num = 2;
int opt_yield = 0;
char opt_sync;
char return_value[16] = "list-";
pthread_mutex_t mutex;
int yield_args[3] = {0,0,0};
int spin = 0;
SortedList_t * list;
SortedListElement_t * element;
int temp_errno;

void signal_handler()
{
  fprintf(stderr, "Segmentation Fault\n");
  exit(2);
}

char * getTag()
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

void random_keys(int num)
{
  srand(time(NULL));
  
  int i;
  for (i = 0; i < num; i++)
    {
      int key_len = rand() % 25 + 1;
      char * rand_key = malloc((key_len + 1) * sizeof(char));
      int k;
      int rand_char;
      for (k = 0; k < key_len; k++)
	{
	  rand_char = rand() % 26;
	  rand_key[k] = 'a' + rand_char;
	}
      rand_key[key_len] = '\0';
      (element + i) ->key = rand_key;
      free (rand_key);
    }
}

void check_length()
{
  if (SortedList_length(list) == -1)
    {
      fprintf(stderr, "Error: Couldn't get list's length\n");
      exit(2);
    }
}

void lookup_and_delete(SortedListElement_t *element)
{
  SortedListElement_t * loc = malloc(sizeof(SortedListElement_t));
  if ((loc = SortedList_lookup(list, element->key)) == NULL)
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

void * list_ops(void* index)
{
  int i;
  //inserts into list
  for (i = *(int *)index; i < op_num; i += thread_num)
    {
      if (opt_sync == 'm')
	{
	  pthread_mutex_lock(&mutex);
	  SortedList_insert(list, &element[i]);
	  pthread_mutex_unlock(&mutex);	  
	}
      else if (opt_sync == 's')
	{
	  while (__sync_lock_test_and_set(&spin, 1));
	  SortedList_insert(list, &element[i]);
	  __sync_lock_release(&spin);
	}
      else
	{
	  SortedList_insert(list, &element[i]);
	}
    } 
  //Checks length
  if (opt_sync == 'm')
    {
      pthread_mutex_lock(&mutex);
      check_length();
      pthread_mutex_unlock(&mutex);
    }
  else if (opt_sync == 's')
    {
      while (__sync_lock_test_and_set(&spin, 1));
      check_length();
      __sync_lock_release(&spin);
    }
  else
    {
      check_length();
    }
  //looksup and deletes
  for (i = *(int*)index; i < op_num; i += thread_num)
    {
      if (opt_sync == 'm')
	{
	  pthread_mutex_lock(&mutex);
	  lookup_and_delete(element + i);
	  pthread_mutex_unlock(&mutex);
	}
      else if (opt_sync == 's')
	{
	  while(__sync_lock_test_and_set(&spin, 1));
	  lookup_and_delete(element + i);
	  __sync_lock_release(&spin);
	}
      else
	{
       	  lookup_and_delete(element + i);
	}
    }
  return NULL;
}

int main (int argc, char ** argv)
{
  int opt;
  int i;
  struct timespec begin, finish;

  static struct option options[] =
    {
      {"threads", required_argument, 0, 't'},
      {"iterations", required_argument, 0, 'i'},
      {"yield", required_argument, 0, 'y'},
      {"sync", required_argument, 0, 's'},
      {0,0,0,0}
    };
  while ((opt = getopt_long(argc, argv, "t:i:y:s:", options, NULL)) != -1)
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
	  for (i = 0; i < (int) strlen(optarg); i++)
	    {
	      if (optarg[i] == 'i')
		{
		  opt_yield |= INSERT_YIELD;
		  yield_args[0] = 1;
		}
	      else if (optarg[i] == 'd')
		{
		  opt_yield |= DELETE_YIELD;
		  yield_args[1] = 1;
		}
	      else if (optarg[i] == 'l')
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
	      if (opt_sync == 'm')
		{
		  pthread_mutex_init(&mutex, NULL);
		}
	    }
	  else
	    {
	      fprintf(stderr, "Incorrect arguments for sync. Valid options: [m, s]");
	      exit(1);
	    }
	  break;
	default:
	  fprintf(stderr, "Usage: ./lab2_add [--threads=] [--yield] [--sync=] [--iterations=]");
	  exit(1);
	}
    }
  
  op_num = iter_num * thread_num;

  signal(SIGSEGV, signal_handler);
  
  list = malloc(sizeof(SortedList_t));
  element = malloc((op_num) * sizeof(SortedListElement_t));

  list->key = NULL;
  list->next = list;
  list->prev = list;

  random_keys(op_num);

  pthread_t * threads = malloc(thread_num * sizeof(pthread_t));
  int * threadID = malloc(thread_num * sizeof(int));
  
  if (clock_gettime(CLOCK_MONOTONIC, &begin) == -1)
    {
      temp_errno = errno;
      fprintf (stderr, "%s", strerror(temp_errno));
      exit(1);
    }

  for (i = 0; i < thread_num; i++)
    {
      threadID[i] = i;
      if (pthread_create(threads + i, NULL, list_ops, &threadID[i]))
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
  
  free(list);
  free(element);
  free(threads);
  free(threadID);
  
  int operations = op_num * 3;
  long time = (finish.tv_sec - begin.tv_sec)*1000000000 + (finish.tv_nsec - begin.tv_nsec);
  int time_per_op = time/operations;
  char * tag = getTag();

  printf ("%s,%d,%d,1,%d,%ld,%d\n", tag, thread_num, iter_num, operations, time, time_per_op);

  exit(0);
}
