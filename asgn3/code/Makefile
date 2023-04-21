all: a.out prod cons del_shm

a.out : main.cpp
	g++ main.cpp

prod: producer.cpp
	g++ -o prod producer.cpp

cons: consumer.cpp
	g++ -o cons consumer.cpp

del_shm:
	g++ -o del_shm del_shm.cpp

clean:
	./del_shm
	rm prod cons del_shm a.out
