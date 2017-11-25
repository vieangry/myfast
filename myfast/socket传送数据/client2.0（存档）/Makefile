object=main.o net.o

client:main.o net.o
	g++ -o client $(object)
main.o:main.cpp net.h
	g++ -c  main.cpp
net.o:net.cpp net.h
	g++ -c  net.cpp

clean:
	rm client $(object)
