CC = gcc
CPP = g++
MTFLAG = -Wall -lpthread
SOURCES = MemPool.cpp
MAIN_SOURCES = multi_test.cpp
MAIN_OUTPUT = multi_test
EXAMPLE_SOURCES = example.cpp
EXAMPLE_OUTPUT = example
THREAD_SAFE = -D MULTI_THREAD

# 多线程
run_multi_test:
	$(CPP) $(MAIN_SOURCES) $(SOURCES) $(THREAD_SAFE) -o $(MAIN_OUTPUT).out $(MTFLAG)
	./$(MAIN_OUTPUT).out

# 单线程
run_example:
	$(CPP) $(EXAMPLE_SOURCES) $(SOURCES) -o $(EXAMPLE_OUTPUT).out
	./$(EXAMPLE_OUTPUT).out

.PHONY: clean
clean:
	rm *.out