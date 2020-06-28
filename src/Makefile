CC = gcc
CPP = g++
MTFLAG = -Wall -lpthread
UTILITY = malloc2.cpp
SOURCES = MemPool.cpp
BASE_SOURCES = malloc2_test.cpp
BASE_OUTPUT = malloc2_test
MAIN_SOURCES = multi_test.cpp
MAIN_OUTPUT = multi_test
EXAMPLE_SOURCES = single_test.cpp
EXAMPLE_OUTPUT = single_test
THREAD_SAFE = -D MULTI_THREAD
# 基础代码测试
base_test:
	$(CPP) $(BASE_SOURCES) $(UTILITY) -o $(BASE_OUTPUT).out
	./$(BASE_OUTPUT).out
	
# 多线程内存池测试
run_multi_test:
	$(CPP) $(MAIN_SOURCES) $(SOURCES) $(THREAD_SAFE) -o $(MAIN_OUTPUT).out $(MTFLAG)
	./$(MAIN_OUTPUT).out

# 单线程内存池测试
run_single_test:
	$(CPP) $(EXAMPLE_SOURCES) $(SOURCES) -o $(EXAMPLE_OUTPUT).out
	./$(EXAMPLE_OUTPUT).out

.PHONY: clean
clean:
	rm *.out