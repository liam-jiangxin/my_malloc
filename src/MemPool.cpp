#include "MemPool.h"

#define MP_CHUNKHEADER sizeof(struct _mp_chunk)
#define MP_CHUNKEND sizeof(struct _mp_chunk*)

#define MP_LOCK(lockobj) pthread_mutex_lock(&lockobj->lock); 
#define MP_UNLOCK(lockobj) pthread_mutex_unlock(&lockobj->lock);

// 对齐，使_n变成sizeof(int)的倍数
#define MP_ALIGN_SIZE(_n) (_n + sizeof(long) - ((sizeof(long) - 1) & _n))

// 初始化内存池数据结构
#define MP_INIT_MEMORY_STRUCT(mm, mem_sz)       \
    do {                                        \
        mm->mem_pool_size = mem_sz;             \
        mm->alloc_mem = 0;                      \
        mm->alloc_prog_mem = 0;                 \
        mm->free_list = (_MP_Chunk*) mm->start; \
        mm->free_list->is_free = 1;             \
        mm->free_list->alloc_mem = mem_sz;      \
        mm->free_list->prev = NULL;             \
        mm->free_list->next = NULL;             \
        mm->alloc_list = NULL;                  \
    } while (0)

//把x插到head之前
#define MP_DLINKLIST_INS_FRT(head, x) \
    do {                              \
        x->prev = NULL;               \
        x->next = head;               \
        if (head) head->prev = x;     \
        head = x;                     \
    } while (0)

//删除节点x
#define MP_DLINKLIST_DEL(head, x)                 \
    do {                                          \
        if (!x->prev) {                           \
            head = x->next;                       \
            if (x->next) x->next->prev = NULL;    \
        } else {                                  \
            x->prev->next = x->next;              \
            if (x->next) x->next->prev = x->prev; \
        }                                         \
    } while (0)

//功能：得到MemoryPool的pool的数量
void get_memory_list_count(MemoryPool* mp, mem_size_t* mlist_len) {
#ifdef MULTI_THREAD
    MP_LOCK(mp);
#endif
    mem_size_t mlist_l = 0;
    _MP_Memory* mm = mp->mlist;
    while (mm) {
        mlist_l++;
        mm = mm->next;
    }
    *mlist_len = mlist_l;
#ifdef MULTI_THREAD
    MP_UNLOCK(mp);
#endif
}

//功能：得到MemoryPool的某一个pool的空闲块数量和已分配块的数量
void get_memory_info(MemoryPool* mp,
                     _MP_Memory* mm,
                     mem_size_t* free_list_len,
                     mem_size_t* alloc_list_len) {
#ifdef MULTI_THREAD
    MP_LOCK(mp);
#endif
    mem_size_t free_l = 0, alloc_l = 0;
    _MP_Chunk* p = mm->free_list;
    while (p) {
        free_l++;
        p = p->next;
    }

    p = mm->alloc_list;
    while (p) {
        alloc_l++;
        p = p->next;
    }
    *free_list_len = free_l;
    *alloc_list_len = alloc_l;
#ifdef MULTI_THREAD
    MP_UNLOCK(mp);
#endif
}

int get_memory_id(_MP_Memory* mm) {
    return mm->id;
}

//功能：申请一个pool
static _MP_Memory* extend_memory_list(MemoryPool* mp, mem_size_t new_mem_sz) {
    char* s = (char*) malloc(sizeof(_MP_Memory) + new_mem_sz * sizeof(char));
    if (!s) return NULL;

    _MP_Memory* mm = (_MP_Memory*) s;
    mm->start = s + sizeof(_MP_Memory);

    MP_INIT_MEMORY_STRUCT(mm, new_mem_sz);
    mm->id = mp->last_id++;
    mm->next = mp->mlist;
    mp->mlist = mm;
    return mm;
}

//功能：找到需要的p对应的内存地址所在的pool
static _MP_Memory* find_memory_list(MemoryPool* mp, void* p) {
    _MP_Memory* tmp = mp->mlist;
    while (tmp) {
        if (tmp->start <= (char*) p &&
            tmp->start + mp->mem_pool_size > (char*) p)
            break;
        tmp = tmp->next;
    }

    return tmp;
}

static int merge_free_chunk(MemoryPool* mp, _MP_Memory* mm, _MP_Chunk* c) {
    _MP_Chunk *p0 = c, *p1 = c;
    while (p0->is_free) {
        p1 = p0;
        if ((char*) p0 - MP_CHUNKEND - MP_CHUNKHEADER <= mm->start) break;
        p0 = *(_MP_Chunk**) ((char*) p0 - MP_CHUNKEND);
    }

    p0 = (_MP_Chunk*) ((char*) p1 + p1->alloc_mem);
    while ((char*) p0 < mm->start + mp->mem_pool_size && p0->is_free) {
        MP_DLINKLIST_DEL(mm->free_list, p0);
        p1->alloc_mem += p0->alloc_mem;
        p0 = (_MP_Chunk*) ((char*) p0 + p0->alloc_mem);
    }

    *(_MP_Chunk**) ((char*) p1 + p1->alloc_mem - MP_CHUNKEND) = p1;
#ifdef MULTI_THREAD
    MP_UNLOCK(mp);
#endif
    return 0;
}

// 初始化内存池库，规定最大内存容量和每个内存池大小
MemoryPool* MemoryPoolInit(mem_size_t maxmempoolsize, mem_size_t mempoolsize) {
    if (mempoolsize > maxmempoolsize) {
        // printf("[MemoryPool_Init] MemPool Init ERROR! Mempoolsize is too big!
        // \n");
        return NULL;
    }

    MemoryPool* mp = (MemoryPool*) malloc(sizeof(MemoryPool));
    if (!mp) return NULL;

    mp->last_id = 0;
    if (mempoolsize < maxmempoolsize) mp->auto_extend = 1;
    mp->max_mem_pool_size = maxmempoolsize;
    mp->total_mem_pool_size = mp->mem_pool_size = mempoolsize;

#ifdef MULTI_THREAD
    pthread_mutex_init(&mp->lock, NULL);
#endif

    char* s = (char*) malloc(sizeof(_MP_Memory) +
                             sizeof(char) * mp->mem_pool_size);
    if (!s) return NULL;

    mp->mlist = (_MP_Memory*) s;
    mp->mlist->start = s + sizeof(_MP_Memory);
    MP_INIT_MEMORY_STRUCT(mp->mlist, mp->mem_pool_size);
    mp->mlist->next = NULL;
    mp->mlist->id = mp->last_id++;

    return mp;
}

//功能：my_malloc(), 用内存池实现内存分配
void* MemoryPoolAlloc(MemoryPool* mp, mem_size_t wantsize) {
    if (wantsize <= 0) return NULL;
    mem_size_t total_needed_size =
            MP_ALIGN_SIZE(wantsize + MP_CHUNKHEADER + MP_CHUNKEND);
    if (total_needed_size > mp->mem_pool_size) return NULL;

    _MP_Memory *mm = NULL, *mm1 = NULL;
    _MP_Chunk *_free = NULL, *_not_free = NULL;
#ifdef MULTI_THREAD
    MP_LOCK(mp);
#endif
FIND_FREE_CHUNK:
    mm = mp->mlist;
    while (mm) {
        if (mp->mem_pool_size - mm->alloc_mem < total_needed_size) {
            mm = mm->next;
            continue;
        }

        _free = mm->free_list;
        _not_free = NULL;

        while (_free) {
            if (_free->alloc_mem >= total_needed_size) {
                // 如果free块分割后剩余内存足够大 则进行分割
                if (_free->alloc_mem - total_needed_size >
                    MP_CHUNKHEADER + MP_CHUNKEND) {
                    // 从free块头开始分割出alloc块
                    _not_free = _free;

                    _free = (_MP_Chunk*) ((char*) _not_free +
                                          total_needed_size);
                    *_free = *_not_free;
                    _free->alloc_mem -= total_needed_size;
                    *(_MP_Chunk**) ((char*) _free + _free->alloc_mem -
                                    MP_CHUNKEND) = _free;

                    // update free_list
                    if (!_free->prev) {
                        mm->free_list = _free;
                    } else {
                        _free->prev->next = _free;
                    }
                    if (_free->next) _free->next->prev = _free;

                    _not_free->is_free = 0;
                    _not_free->alloc_mem = total_needed_size;

                    *(_MP_Chunk**) ((char*) _not_free + total_needed_size -
                                    MP_CHUNKEND) = _not_free;
                }
                // 不够 则整块分配为alloc
                else {
                    _not_free = _free;
                    MP_DLINKLIST_DEL(mm->free_list, _not_free);
                    _not_free->is_free = 0;
                }
                MP_DLINKLIST_INS_FRT(mm->alloc_list, _not_free);

                mm->alloc_mem += _not_free->alloc_mem;
                mm->alloc_prog_mem +=
                        (_not_free->alloc_mem - MP_CHUNKHEADER - MP_CHUNKEND);
#ifdef MULTI_THREAD
                MP_UNLOCK(mp);
#endif
                return (void*) ((char*) _not_free + MP_CHUNKHEADER);
            }
            _free = _free->next;
        }

        mm = mm->next;
    }

    if (mp->auto_extend) {
        // 超过总内存限制
        if (mp->total_mem_pool_size >= mp->max_mem_pool_size) {
            goto err_out;
        }
        mem_size_t add_mem_sz = mp->max_mem_pool_size - mp->mem_pool_size;
        add_mem_sz = add_mem_sz >= mp->mem_pool_size ? mp->mem_pool_size
                                                     : add_mem_sz;
        mm1 = extend_memory_list(mp, add_mem_sz);
        if (!mm1) {
            goto err_out;
        }
        mp->total_mem_pool_size += add_mem_sz;

        goto FIND_FREE_CHUNK;
    }

err_out:
// printf("[MemoryPool_Alloc] No enough memory! \n");
#ifdef MULTI_THREAD
    MP_UNLOCK(mp);
#endif
    return NULL;
}

//功能:功能同free(),释放p指向的内存空间
int MemoryPoolFree(MemoryPool* mp, void* p) {
    if (p == NULL || mp == NULL) return 1;
#ifdef MULTI_THREAD
    MP_LOCK(mp);
#endif
    _MP_Memory* mm = mp->mlist;
    if (mp->auto_extend) mm = find_memory_list(mp, p);

    _MP_Chunk* ck = (_MP_Chunk*) ((char*) p - MP_CHUNKHEADER);

    MP_DLINKLIST_DEL(mm->alloc_list, ck);
    MP_DLINKLIST_INS_FRT(mm->free_list, ck);
    ck->is_free = 1;

    mm->alloc_mem -= ck->alloc_mem;
    mm->alloc_prog_mem -= (ck->alloc_mem - MP_CHUNKHEADER - MP_CHUNKEND);

    return merge_free_chunk(mp, mm, ck);
}

//功能:清空MemoryPool里面的pool
MemoryPool* MemoryPoolClear(MemoryPool* mp) {
    if (!mp) return NULL;
#ifdef MULTI_THREAD
    MP_LOCK(mp);
#endif
    _MP_Memory* mm = mp->mlist;
    while (mm) {
        MP_INIT_MEMORY_STRUCT(mm, mm->mem_pool_size);
        mm = mm->next;
    }
#ifdef MULTI_THREAD
    MP_UNLOCK(mp);
#endif
    return mp;
}

// 功能:销毁内存池库
int MemoryPoolDestroy(MemoryPool* mp) {
    if (mp == NULL) return 1;
#ifdef MULTI_THREAD
    MP_LOCK(mp);
#endif
    _MP_Memory *mm = mp->mlist, *mm1 = NULL;
    while (mm) {
        mm1 = mm;
        mm = mm->next;
        free(mm1);
    }
#ifdef MULTI_THREAD
    MP_UNLOCK(mp);
    pthread_mutex_destroy(&mp->lock);
#endif
    free(mp);
    return 0;
}

//MemoryPool总内存
mem_size_t GetTotalMemory(MemoryPool* mp) {
    return mp->total_mem_pool_size;
}

// 总使用内存
mem_size_t GetUsedMemory(MemoryPool* mp) {
#ifdef MULTI_THREAD
    MP_LOCK(mp);
#endif
    mem_size_t total_alloc = 0;
    _MP_Memory* mm = mp->mlist;
    while (mm) {
        total_alloc += mm->alloc_mem;
        mm = mm->next;
    }
#ifdef MULTI_THREAD
    MP_UNLOCK(mp);
#endif
    return total_alloc;
}

//遍历pool，获得有效分配内存（没加上chunk头和chunk头指针的）
mem_size_t GetProgMemory(MemoryPool* mp) {
#ifdef MULTI_THREAD
    MP_LOCK(mp);
#endif
    mem_size_t total_alloc_prog = 0;
    _MP_Memory* mm = mp->mlist;
    while (mm) {
        total_alloc_prog += mm->alloc_prog_mem;
        mm = mm->next;
    }
#ifdef MULTI_THREAD
    MP_UNLOCK(mp);
#endif
    return total_alloc_prog;
}
//两个比例
float MemoryPoolGetUsage(MemoryPool* mp) {
    return (float) GetUsedMemory(mp) / GetTotalMemory(mp);
}

float MemoryPoolGetProgUsage(MemoryPool* mp) {
    return (float) GetProgMemory(mp) / GetTotalMemory(mp);
}

#undef MP_CHUNKHEADER
#undef MP_CHUNKEND
#undef MP_LOCK
#undef MP_ALIGN_SIZE
#undef MP_INIT_MEMORY_STRUCT
#undef MP_DLINKLIST_INS_FRT
#undef MP_DLINKLIST_DEL
