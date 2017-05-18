libdbgthrow.so: cxa_throw_filter.cpp
	g++ -c -fPIC -o cxa_throw_filter.o cxa_throw_filter.cpp
	g++ -shared -rdynamic -o libdbgthrow.so cxa_throw_filter.o -ldl

clean:
	-rm *.so
	-rm *.o

