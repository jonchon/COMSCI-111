/*
NAME: Jonathan Chon
EMAIL: jonchon@gmail.com
ID: 104780881
 */

#include "SortedList.h"
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

void SortedList_insert(SortedList_t * list, SortedListElement_t * element)
{
  if (list == NULL || element == NULL)
    {
      return;
    }
  
  SortedListElement_t * current = list->next;
  while (current != list)
    {
      if (strcmp(element->key, current->key) <= 0)
	{
	  break;
	}
      current = current->next;
    }

  if(opt_yield & INSERT_YIELD)
    {
      sched_yield();
    }

  element->next = current;
  element->prev = current->prev;
  current->prev->next = element;
  current->prev = element;
}

int SortedList_delete(SortedListElement_t * element)
{
  if (element == NULL)
    {
      return 1;
    }
  if (element->next->prev == element->prev->next)
    {
      if(opt_yield & DELETE_YIELD)
	{
	  sched_yield();
	}
      element->prev->next = element->next;
      element->next->prev = element->prev;
      return 0;
    }
  return 1;
}

SortedListElement_t * SortedList_lookup(SortedList_t * list, const char * key)
{
  if (list == NULL || key == NULL)
    {
      return NULL;
    }
  SortedListElement_t * current = list->next;
  while (current != list)
    {
      if (!strcmp(current->key, key))
	{
	  return current;
	}
      if (opt_yield & LOOKUP_YIELD)
	{
	  sched_yield();
	}
      current = current->next;
    }
  return NULL;
}

int SortedList_length(SortedList_t * list)
{
  if (list == NULL)
    {
      return -1;
    }
  int counter = 0;
  SortedListElement_t * current = list->next;

  while (current != list)
    {
      counter +=1;
      
      if (opt_yield & LOOKUP_YIELD)
	{
	  sched_yield();
	}
      current = current->next;
    }
  return counter;
}

