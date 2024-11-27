#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>

#define BUFFER_SIZE 50
char buffer[BUFFER_SIZE + 1];

void forked(int *fd)
{
	close(fd[1]);
	read(fd[0], buffer, BUFFER_SIZE);
	printf("[FORK %d]: %s\n", getpid(), buffer);
}

int main()
{
	buffer[BUFFER_SIZE] = '\0';
	int fd[2], fd2[2];
	if (pipe(fd) || pipe(fd2))
	{
		printf("Errore gravissimo");
		exit(0);
	}

	int pid;

	if (!fork())
		forked(fd);
	else if (!fork())
		forked(fd);
	else if (!fork())
		forked(fd2);
	else
	{
		close(fd[0]);
		sleep(1);
		strcpy(buffer, "Hello World!");
		write(fd[1], buffer, strlen(buffer));
		write(fd[1], buffer, strlen(buffer));
		write(fd2[1], buffer, strlen(buffer));
		sleep(1);
		write(fd[1], buffer, strlen(buffer));
	}
}
