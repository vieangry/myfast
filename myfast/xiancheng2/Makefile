object=main.o one.o two.o

main:main.o one.o two.o
	gcc -o main $(object) -lpthread 
main.o:main.c one.h 
	gcc -c  main.c
one.o:one.c one.h two.h 
	gcc -c  one.c 
two.o:two.c two.h 
	gcc -c  two.c -lpthread
clean:
	rm main $(object)
