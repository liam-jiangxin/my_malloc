#include <bits/stdc++.h>
#include <assert.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

struct block_meta {
  size_t size;
  struct block_meta *next;
  int free;
};

#define META_SIZE sizeof(struct block_meta)


void *my_malloc(size_t size);
void my_free(void *ptr);
void *my_realloc(void *ptr, size_t size);