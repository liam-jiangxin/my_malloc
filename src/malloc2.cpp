// #include "malloc2.h"
// void *global_base = NULL;

// struct block_meta *find_free_block(struct block_meta **last, size_t size) 
// { // TODO: optimize the finding strategy
//   struct block_meta *current = (block_meta *)global_base;
//   while (current && !(current->free && current->size >= size)) 
//   {
//     *last = current;
//     current = current->next;
//   }
//   return current;
// }

// struct block_meta *request_space(struct block_meta* last, size_t size) 
// {
//   struct block_meta *block;
//   block = (struct block_meta *)sbrk(0);
//   void *request = sbrk(size + META_SIZE);
//   assert((void*)block == request);
//   if (request == (void*) -1) 
//   {
//     return NULL; // sbrk failed.
//   }

//   if (last) { // NULL on first request.
//     last->next = block;
//   }

//   block->next = NULL;
//   block->size = size;
//   block->free = 0;
//   block->magic = 0x12345678;

//   return block;
// }

// void *my_malloc(size_t size) 
// {
//   struct block_meta *block;

//   if (size <= 0) 
//   {
//     return NULL;
//   }

//   if (!global_base) 
//   { // First call.
//     block = request_space(NULL, size);
//     if (!block) 
//     {
//       return NULL;
//     }
//     global_base = block;
//   } 
//   else 
//   {
//     struct block_meta *last = (block_meta *)global_base;
//     block = find_free_block(&last, size);
//     if (!block) 
//     { // Failed to find free block.
//       block = request_space(last, size);
//       if (!block) 
//       {
//         return NULL;
//       }
//     } 
//     else 
//     { // Found free block
//       // TODO: consider splitting block here.
//       struct block_meta * next_block;
//       if((block->size-size)>META_SIZE)
//       {
//         /*split*/
//         next_block = (struct block_meta*)((char*)block + META_SIZE + size);
//         next_block -> free = 1;
//         next_block -> next = block -> next;
//         next_block -> size = (block -> size) - size - META_SIZE;
//         next_block -> magic = 0x12345678;
//       }
//       block->size = size;
//       block->next = next_block;
//       block->free = 0;
//       block->magic = 0x55555555;
//     }
//   }

//   return(block+1);
// }

// struct block_meta *get_block_ptr(void *ptr) 
// {
//   return (struct block_meta*)ptr - 1;
// }

// void my_free(void *ptr) {
//   if (!ptr) {
//     return;
//   }

//   // TODO: consider merging blocks once splitting blocks is implemented.
//   struct block_meta* block_ptr = get_block_ptr(ptr);
//   struct block_meta* next_ptr = block_ptr;

//   assert(block_ptr->free == 0);
//   assert(block_ptr->magic == 0x77777777 || block_ptr->magic == 0x12345678);

//   /*MERGE*/
//   int add_size = 0;
//   while(next_ptr&&next_ptr->free==1)
//   {
//     add_size += (META_SIZE+next_ptr->size);
//   }
//   block_ptr->free = 1;
//   block_ptr->next = next_ptr;
//   block_ptr->size = block_ptr->size + add_size;
//   block_ptr->magic = 0x55555555;
// }

// void *my_realloc(void *ptr, size_t size) {
//   if (!ptr) { 
//     // NULL ptr. realloc should act like malloc.
//     return my_malloc(size);
//   }

//   struct block_meta* block_ptr = get_block_ptr(ptr);
//   if (block_ptr->size >= size) {
//     // We have enough space. Could free some once we implement split.
//     return ptr;
//   }

//   // Need to really realloc. Malloc new space and free old space.
//   // Then copy old data to new space.
//   void *new_ptr;
//   new_ptr = my_malloc(size);
//   if (!new_ptr) {
//     return NULL; // TODO: set errno on failure.
//   }
//   memcpy(new_ptr, ptr, block_ptr->size);
//   my_free(ptr);  
//   return new_ptr;
// }

#include "malloc2.h"
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

void *my_realloc(void *ptr, size_t size) {
  if (!ptr) { 
    // NULL ptr. realloc should act like malloc.
    return my_malloc(size);
  }

  struct block_meta* block_ptr = get_block_ptr(ptr);
  if (block_ptr->size >= size) {
    // We have enough space. Could free some once we implement split.
    return ptr;
  }

  // Need to really realloc. Malloc new space and free old space.
  // Then copy old data to new space.
  void *new_ptr;
  new_ptr = my_malloc(size);
  if (!new_ptr) {
    return NULL; // TODO: set errno on failure.
  }
  memcpy(new_ptr, ptr, block_ptr->size);
  my_free(ptr);  
  return new_ptr;
}
