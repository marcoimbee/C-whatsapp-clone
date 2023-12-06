all: dev serv

#make rule device (dev)
dev: dev.o
	gcc -Wall dev.o -o dev

#make rule server (serv)
serv: serv.o
	gcc -Wall serv.o -o serv

#pulizia file compilazione (make clean)
clean:
	rm *o dev serv