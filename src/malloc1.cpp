#include <assert.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>


void *malloc(size_t size) {
  void *p = sbrk(0); // sbrk(size): OS使堆增长size, 并返回堆顶。 heap.top
  void *request = sbrk(size);
  if (request == (void*) -1) {
    return NULL; // sbrk failed.
  } else {
    assert(p == request);
    return p;
  }
}

free(p)