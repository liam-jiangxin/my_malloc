#include <bits/stdc++.h>
#include <assert.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

struct block_meta {
  size_t size;
  struct block_meta *next;
  int free;
  int magic; // debug
};

#define META_SIZE sizeof(struct block_meta)

void *global_base = NULL;

struct block_meta *find_free_block(struct block_meta **last, size_t size) { // TODO: optimize the finding strategy
  struct block_meta *current = (block_meta *)global_base;
  while (current && !(current->free && current->size >= size)) {
    *last = current;
    current = current->next;
  }
  return current;
}

struct block_meta *request_space(struct block_meta* last, size_t size) {
  struct block_meta *block;
  block = (struct block_meta *)sbrk(0);
  void *request = sbrk(size + META_SIZE);
  assert((void*)block == request);
  if (request == (void*) -1) {
    return NULL; // sbrk failed.
  }

  if (last) { // NULL on first request.
    last->next = block;
  }

  block->next = NULL;
  block->size = size;
  block->free = 0;
  block->magic = 0x12345678;

  return block;
}

void *my_malloc(size_t size) {
  struct block_meta *block;

  if (size <= 0) {
    return NULL;
  }

  if (!global_base) { // First call.
    block = request_space(NULL, size);
    if (!block) {
      return NULL;
    }
    global_base = block;
  } else {
    struct block_meta *last = (block_meta *)global_base;
    block = find_free_block(&last, size);
    if (!block) { // Failed to find free block.
      block = request_space(last, size);
      if (!block) {
        return NULL;
      }
    } else {      // Found free block
      // TODO: consider splitting block here.
      block->free = 0;
      block->magic = 0x77777777;
    }
  }

  return(block+1);
}

struct block_meta *get_block_ptr(void *ptr) {
  return (struct block_meta*)ptr - 1;
}

void my_free(void *ptr) {
  if (!ptr) {
    return;
  }

  // TODO: consider merging blocks once splitting blocks is implemented.
  struct block_meta* block_ptr = get_block_ptr(ptr);
  assert(block_ptr->free == 0);
  assert(block_ptr->magic == 0x77777777 || block_ptr->magic == 0x12345678);
  block_ptr->free = 1;
  block_ptr->magic = 0x55555555;
}

int main(){
    int *a;
    a=(int*)my_malloc(1000);
    for(int i=0;i<3;i++){
       a[i]=i;
       printf("a[%d]=%d\n",i,a[i]);
   }
   my_free(a);
   printf("a[1]=%d\n",a[1]);
}
