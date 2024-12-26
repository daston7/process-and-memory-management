CC = gcc
CFLAGS = -Wall -O2
EXEC = allocate
OBJ = allocate.o Process.o Queue.o ContiguousMemory.o PagedMemory.o

all: $(EXEC)

$(EXEC): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

allocate.o: allocate.c Process.h Queue.h ContiguousMemory.h PagedMemory.h
Process.o: Process.c Process.h
Queue.o: Queue.c Queue.h Process.h
ContiguousMemory.o: ContiguousMemory.c ContiguousMemory.h
PagedMemory.o: PagedMemory.c PagedMemory.h Process.h

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(EXEC)

.PHONY: all clean