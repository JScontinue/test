#add:main.o add.o
#	gcc main.o add.o -o add
#main.o:main.c
#	gcc -c main.c -o main.o
#add.o:add.c
#	gcc -c add.c -o add.o
#clean:
#	rm -rf main.o add.o
#
obj 	= main.o add.o
cc 	= gcc
target	= add
$(target):$(obj)
	$(cc) $^ -o $@
%.o:%.c
	$(cc) -c $<
clean:
	rm -f *.o $@
