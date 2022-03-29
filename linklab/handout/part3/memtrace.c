//------------------------------------------------------------------------------
//
// memtrace
//
// trace calls to the dynamic memory manager
//
#define _GNU_SOURCE

#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <memlog.h>
#include <memlist.h>

//
// function pointers to stdlib's memory management functions
//
static void *(*mallocp)(size_t size) = NULL;
static void (*freep)(void *ptr) = NULL;
static void *(*callocp)(size_t nmemb, size_t size);
static void *(*reallocp)(void *ptr, size_t size);

//
// statistics & other global variables
//
static unsigned long n_malloc  = 0;
static unsigned long n_calloc  = 0;
static unsigned long n_realloc = 0;
static unsigned long n_allocb  = 0;
static unsigned long n_freeb   = 0;
static item *list = NULL;

//
// init - this function is called once when the shared library is loaded
//
__attribute__((constructor))
void init(void)
{
  char *error;

  LOG_START();

  mallocp = dlsym(RTLD_NEXT, "malloc");
  if((error = dlerror()) != NULL)
  {
    fprintf(stderr, "dlsym error: %s \n", error);
    exit(1);
  }

  callocp = dlsym(RTLD_NEXT, "calloc");
  if((error = dlerror()) != NULL)
  {
    fprintf(stderr, "dlsym error: %s \n", error);
    exit(1);
  }

  reallocp = dlsym(RTLD_NEXT, "realloc");
  if((error = dlerror()) != NULL)
  {
    fprintf(stderr, "dlsym error: %s \n", error);
    exit(1);
  }

  freep = dlsym(RTLD_NEXT, "free");
  if((error = dlerror()) != NULL)
  {
    fprintf(stderr, "dlsym error: %s \n", error);
    exit(1);
  }


  // initialize a new list to keep track of all memory (de-)allocations
  // (not needed for part 1)
  list = new_list();

  // ...
}

void *malloc(size_t size)
{
  assert(size != 0);
  void* result_pointer = mallocp(size);
  item *new_malloc_item = alloc(list, result_pointer, size);

  n_allocb += size;
  n_malloc++;

  LOG_MALLOC(size, result_pointer);

  return result_pointer;
}

void *calloc(size_t nmenb, size_t size)
{
  size_t msize = nmenb*size;
  void* result_pointer = callocp(nmenb, size);
  item *new_malloc_item = alloc(list, result_pointer, msize);

  n_allocb += msize;
  n_calloc++;

  LOG_CALLOC(nmenb, size, result_pointer);

  return result_pointer;
}

void *realloc(void *ptr, size_t size)
{
  assert(ptr!=NULL);

  item* prev = (item*)find(list, ptr);
  n_freeb += prev->size;

  dealloc(list, ptr);
  void* result_pointer = reallocp(ptr, size);
  alloc(list, result_pointer, size);

  n_allocb += size;
  n_realloc++;

  LOG_REALLOC(ptr, size, result_pointer);
 
  return result_pointer;
}

void free(void *ptr)
{
  assert(ptr != NULL);
  LOG_FREE(ptr);

  item* prev = (item*)find(list, ptr);

  if(find(list, ptr) == NULL)
  {
    LOG_ILL_FREE();
    return;
  }
  else if(prev->cnt <= 0)
  {
    LOG_DOUBLE_FREE();
  }
  else
  {
    dealloc(list, ptr);
    n_freeb += prev->size;
    freep(ptr);
  }
}

//
// fini - this function is called once when the shared library is unloaded
//
__attribute__((destructor))
void fini(void)
{
  // ...

  unsigned long total_allocated = n_allocb;
  unsigned long average_allocated = n_allocb / (n_malloc+n_calloc+n_realloc);
  item* temp = list;
  temp = temp->next;
  int flag = 0;

  LOG_STATISTICS(total_allocated, average_allocated, n_freeb);

  while(temp != NULL)
  {
    if(temp->cnt > 0) 
    {
      if(flag == 0)
      {
	LOG_NONFREED_START();
      }	      

      LOG_BLOCK(temp->ptr, temp->size, temp->cnt);
      flag++;
    }
    temp = temp->next;
  }
  
  LOG_STOP();

  // free list (not needed for part 1)
  free_list(list);
}

// ...
