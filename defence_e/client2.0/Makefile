object=defence_e.o net.o

defence_e:defence_e.o net.o
	g++ -o defence_e $(object)
defence_e.o:defence_e.cpp net.h
	g++ -c  defence_e.cpp
net.o:net.cpp net.h
	g++ -c  net.cpp

clean:
	rm defence_e $(object)
