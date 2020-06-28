#include <cstdio>
#include "MemPool.h"
#include <iostream>
using namespace std;
#define MP_ALIGN_SIZE(_n) (_n + sizeof(long) - ((sizeof(long) - 1) & _n))
mem_size_t max_mem =  1024 * MB ;
mem_size_t mem_pool_size = 16*MB;

int main() {
    MemoryPool* mp = MemoryPoolInit(max_mem, mem_pool_size);

    // struct TAT* tat = (struct TAT*) MemoryPoolAlloc(mp, sizeof(struct TAT));
    // tat->T_T = 2333;
    // cout<<(tat->T_T)<<endl;
    int sz = sizeof(int)*128*KB;

    int *a = (int*) MemoryPoolAlloc(mp,sz);
    for(int i=0;i<128*KB;i++){
        a[i] = i;
    }

    cout<<"Total memory: "<<GetTotalMemory(mp)<<endl;
    cout<<"Allocated memory: "<<GetUsedMemory(mp)<<endl;
    cout<<"Data Memory: "<<GetProgMemory(mp)<<endl;
    MemoryPoolFree(mp, a);
    MemoryPoolClear(mp);
    MemoryPoolDestroy(mp);
    return 0;
}