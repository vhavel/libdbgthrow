CFLAGS=-g

libdbgthrow.so: cxa_throw_filter.cpp backtrace_print.cpp
	$(CXX) $(CFLAGS) -c -fPIC -o cxa_throw_filter.o cxa_throw_filter.cpp
	$(CXX) $(CFLAGS) -c -fPIC -o backtrace_print.o backtrace_print.cpp
	$(CXX) $(CFLAGS) -shared -rdynamic -o libdbgthrow.so\
		cxa_throw_filter.o backtrace_print.o -ldl

clean:
	-rm *.so
	-rm *.o

test1: test/test1.cpp libdbgthrow.so
	$(CXX) -rdynamic test/test1.cpp -o test1

runtests: test1
	sh test/test1.sh

.PHONY: runtests

