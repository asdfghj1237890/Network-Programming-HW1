all:
	gcc -o HW1_103062261_Ser server.c
	gcc -o HW1_103062261_Cli client.c
clean:
	rm -f HW1_103062261_Ser HW1_103062261_Cli
