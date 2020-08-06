#include<unistd.h>
#include<pthread.h>
#include<stdio.h>
#include<sys/types.h>
#define METASIZE 16

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

typedef struct Metadata_tag Metadata_t;

struct Metadata_tag{
  size_t size;	//8
  Metadata_t * next; //8
};

void* bf_malloc(size_t size,int is_nolock);
void bf_free(void *ptr,int is_nolock);
size_t align_memory(size_t size);
void coalesce_blocks(Metadata_t * p,int is_nolock);
void split_block(Metadata_t * p,size_t size,int is_nolock);
void* increase_heap(size_t size,int is_nolock);
void* find_best_fit(size_t size,int is_nolock);

//locking version
void * ts_malloc_lock(size_t size);
void ts_free_lock(void * ptr);

//non-locking version
void * ts_malloc_nolock(size_t size);
void ts_free_nolock(void * ptr);

void add_freelist(Metadata_t * p,int is_nolock);
Metadata_t * remove_freelist(Metadata_t * p,int is_nolock);
