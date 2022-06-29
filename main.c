#include <stdio.h>
#include <stdlib.h>
#include "add.h"

int main(int argc, char *argv[])
{
	int num1, num2;	

	if(argc != 3){
		printf("the parmer number is three, please check your input!\n");
                return -1;
	}
	
	num1 = atoi(argv[1]);
	num2 = atoi(argv[2]);

	printf("-----------------------------------------\n");
	printf(" %d + %d = %d\n", num1, num2, add(num1, num2));

	return 0;

}
