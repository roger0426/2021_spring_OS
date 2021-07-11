SRC=src

all: server.out client.out
server.out: server.o
	gcc -pthread -o server.out server.o
client.out: client.o 
	gcc -pthread -o client.out client.o 
server.o: $(SRC)/server.c
	gcc -c $(SRC)/server.c
client.o: $(SRC)/client.c
	gcc -c $(SRC)/client.c

.PHONY: clean
clean:
	rm -f *.o *.out