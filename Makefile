all: ufs

ufs: UFS.o main.o disque.o
	gcc main.o UFS.o disque.o -o ufs

UFS.o: UFS.c
	gcc -c -g UFS.c

main.o: main.c
	gcc -c -g main.c

disque.o: disque.c
	gcc -c -g disque.c

clean: 
	rm main.o UFS.o disque.o ufs
