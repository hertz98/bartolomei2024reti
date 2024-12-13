# Makefile

all: server client

server: ./src/server/server.c
	gcc -g -Wall ./src/server/*.c -o ./bin/server

client: ./src/client/client.c
	gcc -g -Wall ./src/client/*.c -o ./bin/client

clean:
	#find . -maxdepth 1 -type f -executable -delete
	#find . -iname "*.o"
	rm -f ./bin/server ./bin/client
	rm -f ./bin/server.exe ./bin/client.exe ./bin/cygwin1.dll

windows: ./src/client/client.c ./src/server/server.c
	x86_64-pc-cygwin-gcc ./src/server/*.c -o ./bin/server.exe
	x86_64-pc-cygwin-gcc ./src/client/*.c -o ./bin/client.exe
	cp "/usr/x86_64-pc-cygwin/sys-root/usr/bin/cygwin1.dll"	./bin/

run_server: test_server
test_server:
	./bin/server 127.0.0.1 1234

run_client: test_client
test_client:
	./bin/client 127.0.0.1 1234

gdb_server:
	gdb -x ./src/server/gdbinit ./bin/server

gdb_client:
	gdb -x ./src/client/gdbinit ./bin/client