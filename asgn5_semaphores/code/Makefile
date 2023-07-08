main: main.o guest.o cleaner.o
	g++ -o main main.o guest.o cleaner.o -lpthread

main.o : main.cpp main.hpp
	g++ -c main.cpp

guest.o: guest.cpp main.hpp
	g++ -c guest.cpp

cleaner.o: cleaner.cpp main.hpp
	g++ -c cleaner.cpp

clean:
	rm -f main.o guest.o cleaner.o main