test: mergesort.cpp lib
	g++ mergesort.cpp -o test -I. -L. -lgoodmalloc

lib: goodmalloc.o
	ar rcs libgoodmalloc.a goodmalloc.o

goodmalloc.o: goodmalloc.cpp goodmalloc.h
	g++ -c goodmalloc.cpp

clean:
	rm -f *.o *.a test