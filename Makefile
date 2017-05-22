all:	nand.c
	gcc -Wall -pthread -o nand nand.c -lpigpio -lrt
