# My malloc
## Logs
- 创建`malloc1.cpp`: 用linux的内核函数sbrk()代替malloc()，不维护申请过的内存空间，不支持free()操作 - **LJX**
- 创建`malloc2.cpp`: 在malloc1.cpp的基础上增加结点头信息，配合链表结构维护申请过的内存空间，支持free()操作（TODO: Split & Merge链表结点；增加realloc()）- **LJX**
- 改进`malloc2.cpp`: 增加my_realloc()函数 - **PS**
- 改进`malloc2.cpp`: 增加链表结点申请块split & merge功能 - **MZR**
- 创建`base_test`: 测试malloc+realloc+free - **LJX**
- 创建`MemPool.cpp` + 创建`example.cpp`: 增加内存池功能，预先分配一大块进行管理，用完可自动扩展 + 测试 - **LZY**
- 改进`MemPool.cpp` + 创建`multi_test.cpp`: 增加内存池的空闲块合并与利用锁保证多线程安全 + 多线程测试 - **LJX**
- 创建`Makefile` - **LJX**
- 维护`README.md` - **LJX**
## 编译运行

Linux下：
- `make base_test`: malloc + realloc + free 基础测试
- `make run_example`: 内存池测试（单线程）
- `make run_multi_test`: 内存池测试（多线程）

## 运行结果

- malloc + realloc + free 基础测试

![Base Test](./base_test.jpg)

- 内存池测试（单线程）

![Single Thread](./single_thread.jpg)

- 内存池测试（多线程）

![Multiple Thread](./multi_thread.jpg)

......

(完整结果):
```
./multi_test.out
Memory Pool:
============ Thread No. : 140206329300736 ============
-> Alloc Before:
->> Memory Usage: 0.0000
->> Memory Usage(prog): 0.0000
->> [memorypool_list_count] mlist(1)
->>> id: 0 [list_count] free_list(1)  alloc_list(0)
============ Thread No. : 140206329300736 ============

============ Thread No. : 140206295680768 ============
-> Alloc Before:
->> Memory Usage: 0.0009
->> Memory Usage(prog): 0.0009
->> [memorypool_list_count] mlist(1)
->>> id: 0 [list_count] free_list(1)  alloc_list(0)
============ Thread No. : 140206295680768 ============

============ Thread No. : 140206262060800 ============
-> Alloc Before:
->> Memory Usage: 0.0071
->> Memory Usage(prog): 0.0070
->> [memorypool_list_count] mlist(1)
->>> id: 0 [list_count] free_list(1)  alloc_list(0)
============ Thread No. : 140206262060800 ============

============ Thread No. : 140206295680768 ============
-> Free Before:
->> Memory Usage: 0.9829
->> Memory Usage(prog): 0.9781
->> [memorypool_list_count] mlist(3)
->>> id: 2 [list_count] free_list(1)  alloc_list(0)
->>> id: 1 [list_count] free_list(1)  alloc_list(0)
->>> id: 0 [list_count] free_list(1)  alloc_list(0)
============ Thread No. : 140206295680768 ============

============ Thread No. : 140206329300736 ============
-> Free Before:
->> Memory Usage: 0.9125
->> Memory Usage(prog): 0.9082
->> [memorypool_list_count] mlist(4)
->>> id: 3 [list_count] free_list(1)  alloc_list(0)
->>> id: 2 [list_count] free_list(1)  alloc_list(0)
->>> id: 1 [list_count] free_list(1)  alloc_list(0)
->>> id: 0 [list_count] free_list(1)  alloc_list(0)
============ Thread No. : 140206329300736 ============

============ Thread No. : 140206262060800 ============
-> Free Before:
->> Memory Usage: 0.7405
->> Memory Usage(prog): 0.7372
->> [memorypool_list_count] mlist(5)
->>> id: 4 [list_count] free_list(1)  alloc_list(0)
->>> id: 3 [list_count] free_list(1)  alloc_list(0)
->>> id: 2 [list_count] free_list(1)  alloc_list(0)
->>> id: 1 [list_count] free_list(1)  alloc_list(0)
->>> id: 0 [list_count] free_list(1)  alloc_list(0)
============ Thread No. : 140206262060800 ============


============ Thread No. : 140206295680768 ============
-> Free After:
->> Memory Usage: 0.1979
->> Memory Usage(prog): 0.1973
->> [memorypool_list_count] mlist(5)
->>> id: 4 [list_count] free_list(1)  alloc_list(0)
->>> id: 3 [list_count] free_list(1)  alloc_list(0)
->>> id: 2 [list_count] free_list(1)  alloc_list(0)
->>> id: 1 [list_count] free_list(1)  alloc_list(0)
->>> id: 0 [list_count] free_list(1)  alloc_list(0)
============ Thread No. : 140206295680768 ============

Memory Pool Size: 1536.0000 MB
Total Usage Size: 391.7815 MB

============ Thread No. : 140206329300736 ============
-> Free After:
->> Memory Usage: 0.0998
->> Memory Usage(prog): 0.0995
->> [memorypool_list_count] mlist(5)
->>> id: 4 [list_count] free_list(1)  alloc_list(0)
->>> id: 3 [list_count] free_list(1)  alloc_list(0)
->>> id: 2 [list_count] free_list(1)  alloc_list(0)
->>> id: 1 [list_count] free_list(1)  alloc_list(0)
->>> id: 0 [list_count] free_list(1)  alloc_list(0)
============ Thread No. : 140206329300736 ============

Memory Pool Size: 1536.0000 MB
Total Usage Size: 782.1762 MB

============ Thread No. : 140206262060800 ============
-> Free After:
->> Memory Usage: 0.0000
->> Memory Usage(prog): 0.0000
->> [memorypool_list_count] mlist(5)
->>> id: 4 [list_count] free_list(1)  alloc_list(0)
->>> id: 3 [list_count] free_list(1)  alloc_list(0)
->>> id: 2 [list_count] free_list(1)  alloc_list(0)
->>> id: 1 [list_count] free_list(1)  alloc_list(0)
->>> id: 0 [list_count] free_list(1)  alloc_list(0)
============ Thread No. : 140206262060800 ============

Memory Pool Size: 1536.0000 MB
Total Usage Size: 1172.9580 MB

Total time: 2.625000 seconds.
```
