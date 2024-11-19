# Makefile

all: client server

client: ./src/client.c
	gcc -g -Wall ./src/client.c -o ./bin/client

server: ./src/server.c
	gcc -g -Wall ./src/server.c -o ./bin/server

clean:
	#find . -maxdepth 1 -type f -executable -delete
	#find . -iname "*.o"
	rm -f ./bin/server ./bin/client
	
test:

