ob=defence_c.o net.o 

defence_c:defence_c.o net.o
	g++ -o defence_cver $(ob)
defence_c.o:defence_c.cpp net.h
	g++ -c defence_c.cpp
net.o:net.cpp net.h
	g++ -c net.cpp
clean:
	rm defence_c $(ob)
