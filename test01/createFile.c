#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>

int main(int argc, char *argv[])
{
	int fd;

	if(argc != 2){
		printf("the parmer number is two, please check your input!\n");
		return -1;
	}

	fd = open( argv[1], O_CREAT);
	if(fd == -1){
		perror("open failed:");
		return -2;
	}

	close(fd);
	return 0;
}
