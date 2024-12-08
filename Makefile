# Makefile

all: server client

server: ./src/server/server.c
	gcc -g -Wall ./src/server/server.c -o ./bin/server

client: ./src/client/client.c
	gcc -g -Wall ./src/client/client.c -o ./bin/client

clean:
	#find . -maxdepth 1 -type f -executable -delete
	#find . -iname "*.o"
	rm -f ./bin/server ./bin/client
	rm -f ./bin/server.exe ./bin/client.exe ./bin/cygwin1.dll

windows: ./src/client/client.c ./src/server/server.c
	x86_64-pc-cygwin-gcc ./src/server/server.c -o ./bin/server.exe
	x86_64-pc-cygwin-gcc ./src/client/client.c -o ./bin/client.exe
	cp "/usr/x86_64-pc-cygwin/sys-root/usr/bin/cygwin1.dll"	./bin/

test:

