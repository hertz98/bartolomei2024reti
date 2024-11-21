# Makefile

all: server client

server: ./src/server.c
	gcc -g -Wall ./src/server.c -o ./bin/server

client: ./src/client.c
	gcc -g -Wall ./src/client.c -o ./bin/client

clean:
	#find . -maxdepth 1 -type f -executable -delete
	#find . -iname "*.o"
	rm -f ./bin/server ./bin/client
	rm -f ./bin/server.exe ./bin/client.exe ./bin/cygwin1.dll

windows: ./src/client.c ./src/server.c
	x86_64-pc-cygwin-gcc ./src/server.c -o ./bin/server.exe
	x86_64-pc-cygwin-gcc ./src/client.c -o ./bin/client.exe
	cp "/usr/x86_64-pc-cygwin/sys-root/usr/bin/cygwin1.dll"	./bin/

test:

