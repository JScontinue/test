#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>

int main(int argc, char *argv[])
{
	int fd, size;

	if(argc != 3){
		printf("the parmer number is three, please check your input!\n");
		return -1;
	}

	fd = open( argv[1], O_CREAT|O_RDWR);
	if(fd == -1){
		perror("open failed");
		return -2;
	}
	
	//printf("the number of write to file is %d.\n", sizeof(argv[2]));	

	size = write( fd, argv[2], sizeof(argv[2]));
	if(size == -1){
		perror("write file failed");
	}
	
	close(fd);
	return 0;
}
