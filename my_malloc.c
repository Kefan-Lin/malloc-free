#include<stdio.h>
#include<unistd.h>
#include<sys/types.h>
#include"my_malloc.h"

void * begin = NULL;
void * end = NULL;

Metadata_t * head=NULL;
__thread Metadata_t * head_nolock=NULL;

//Memory alignment
size_t align_memory(size_t data_size){
  size_t temp=(data_size>>3)<<3;
  if(data_size==temp){
    return data_size;
  }
  return((data_size>>3)+1)<<3;
}

//lock version malloc
void * ts_malloc_lock(size_t size){
  pthread_mutex_lock(&lock);
  void * ans = bf_malloc(size,0);
  pthread_mutex_unlock(&lock);
  return ans;
}

//lock version free
void ts_free_lock(void * ptr){
  pthread_mutex_lock(&lock);
  bf_free(ptr,0);
  pthread_mutex_unlock(&lock);
}

//no lock version malloc
void * ts_malloc_nolock(size_t size){
  return bf_malloc(size,1);
}

//no lock version free
void ts_free_nolock(void * ptr){
  bf_free(ptr,1);
}

// the actual malloc function.
// It takes the flag argument "is_nolock", which
// indicates the version
void * bf_malloc(size_t data_size,int is_nolock){
  data_size=align_memory(data_size);
  void * ans = find_best_fit(data_size,is_nolock);
  if(!ans){
    ans = increase_heap(data_size,is_nolock);
  }
  return ans;
}

//the actual free function
void bf_free(void * ptr,int is_nolock){
  ptr-=METASIZE;
  Metadata_t * p = (Metadata_t *)ptr;
  add_freelist(p,is_nolock);
  coalesce_blocks(p,is_nolock);
}

//if possible, split the block into 2 blocks
void split_block(Metadata_t * p, size_t data_size,int is_nolock){
  void * curr = (void*)p + data_size + METASIZE; 
  Metadata_t * cur=(Metadata_t *)curr;
  if(is_nolock==1){
    head_nolock = remove_freelist(p,is_nolock);
  }
  else{
    head = remove_freelist(p,is_nolock);
  }
  cur->size=p->size-data_size-METASIZE;
  p->size=data_size;
  add_freelist(cur,is_nolock);
}

//if needed, increase the heap
void * increase_heap(size_t data_size,int is_nolock){
  size_t size = data_size + METASIZE;
  void * ans =NULL;
  if(is_nolock==0){
    ans = sbrk(size);
  }
  else{
    pthread_mutex_lock(&lock);
    ans = sbrk(size);
    pthread_mutex_unlock(&lock);
  }
  Metadata_t * p = (Metadata_t *) ans;
  p->size = data_size;
  p->next = NULL;
  ans += METASIZE;
  return ans;
}

//find the best fit block
void * find_best_fit(size_t data_size,int is_nolock){
  Metadata_t * actual_head = NULL;
  if(is_nolock==0){
    actual_head = head;
  }
  else{
    actual_head = head_nolock;
  }
  size_t best_diff=__SIZE_MAX__;
  // size_t size = data_size + METASIZE;
  void * ans = NULL;
  Metadata_t * curr = actual_head;
  while(curr){
    if(curr->size>=data_size){
      size_t temp_diff = curr->size - data_size;
      if(temp_diff==0){
       ans=(void*)curr;
       break;
     }
     if(temp_diff<best_diff){
       best_diff = temp_diff;
       ans=(void *)curr;
     }
   }
   curr=curr->next;
 }
 if(ans){
  Metadata_t * ans_meta = (Metadata_t * )ans;
  if(ans_meta->size>=data_size+METASIZE+8){
    split_block(ans_meta,data_size,is_nolock);
  }
  else{
    actual_head = remove_freelist(ans_meta,is_nolock);
    if(is_nolock==0){
      head=actual_head;
    }
    else{
      head_nolock=actual_head;
    }
  }
  ans+=METASIZE;
}
return ans;
}

//coalesce adjacent blocks if possible
void coalesce_blocks(Metadata_t * p,int is_nolock){
  Metadata_t * actual_head = NULL;
  if(is_nolock==0){
    actual_head = head;
  }
  else{
    actual_head = head_nolock;
  }
  if(p==actual_head){
    if(p->next==NULL){
      return;
    }
    void * temp_next=(void*)p->next;
    void * p_void = (void * )p;
    if(p_void+p->size+METASIZE==temp_next){
      p->size=p->size+METASIZE+p->next->size;
      p->next=p->next->next;
    }
    return;
  }
  //p is not head
  void * p_void = (void *) p;
  Metadata_t * curr=actual_head;
  int count=0;
  while(curr){
    void * cur= (void *)curr;
    void * temp_next=(void *)curr->next;
    if(cur+curr->size+METASIZE==p_void){
      curr->size=curr->size+METASIZE+p->size;
      curr->next=p->next;
      p=p->next;
      p_void=(void *)p;
      count++;
      if(count==2){
       break;
     }
     continue;
   }
   curr=curr->next;
 }
}

//add a block into free list
void add_freelist(Metadata_t * p,int is_nolock){
  Metadata_t * actual_head = NULL;
  if(is_nolock==0){
    actual_head = head;
  }
  else{
    actual_head = head_nolock;
  }
  if(actual_head==NULL){
    actual_head=p;
    if(is_nolock==0){
      head=p;
    }
    else{
      head_nolock=p;
    }
    p->next=NULL;
    return;
  }
  // p is ahead of head
  if(actual_head>p){
    p->next=actual_head;
    actual_head=p;
    if(is_nolock==0){
      head=p;
    }
    else{
      head_nolock=p;
    }
    return;
  }
  // p is normal 
  Metadata_t * curr=actual_head;
  while(curr&&curr->next){
    if(curr<p&&curr->next>p){
      Metadata_t * temp_next=curr->next;
      curr->next=p;
      p->next=temp_next;
      return;
    }
    curr=curr->next;
  }
  // p is behind the tail
  p->next=curr->next;
  curr->next=p;
}

//remove a block from free list
Metadata_t * remove_freelist(Metadata_t * p,int is_nolock){
  Metadata_t * actual_head = NULL;
  if(is_nolock==0){
    actual_head = head;
  }
  else{
    actual_head = head_nolock;
  }
  Metadata_t dummyNode;
  dummyNode.next=actual_head;
  dummyNode.size=0;
  Metadata_t * dummy=&dummyNode;
  Metadata_t * curr=dummy;
  while(curr){
    if(curr->next==p){
      curr->next=p->next;
      break;
    }
    curr=curr->next;
  }
  return dummy->next;
}


