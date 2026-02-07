
all: final
final: main.o hyalloc.o
	gcc $(CFLAGS) main.o hyalloc.o -o final

main.o: main.c
	gcc $(CFLAGS) -c main.c  

hyalloc.o: hyalloc.c 
	gcc $(CFLAGS) -c hyalloc.c 

clean: 
	rm main.o hyalloc.o final