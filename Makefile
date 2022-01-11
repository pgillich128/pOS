CC=gcc -g 

all : interpreter.o shell.o shellmemory.o pcb.o cpu.o kernel.o ram.o memorymanager.o
	$(CC) interpreter.o shell.o shellmemory.o pcb.o cpu.o kernel.o ram.o memorymanager.o -o mykernel

interpreter.o : interpreter.c interpreter.h
	$(CC) -c interpreter.c 

shell.o : shell.c shell.h
	$(CC) -c shell.c 

shellmemory.o : shellmemory.c shellmemory.h
	$(CC) -c shellmemory.c 

pcb.o : pcb.c pcb.h
	$(CC) -c pcb.c

ram.o: ram.c ram.h
	$(CC) -c ram.c

cpu.o : cpu.c cpu.h
	$(CC) -c cpu.c

kernel.o : kernel.c kernel.h
	$(CC) -c kernel.c

memorymanager.o : memorymanager.c memorymanager.h
	$(CC) -c memorymanager.c

.PHONY:
clean:
	rm -f interpreter.o shell.o shellmemory.o
	rm -f mysh
	rm cpu.o
	rm ram.o
	rm pcb.o
	rm kernel.o
	rm memorymanager.o