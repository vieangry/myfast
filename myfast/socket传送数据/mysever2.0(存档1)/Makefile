ob=main.o net.o

se:main.o net.o
	g++ -o sever $(ob)
main.o:main.cpp net.h
	g++ -c main.cpp
net.o:net.cpp net.h
	g++ -c net.cpp
clean:
	rm se $(ob)
