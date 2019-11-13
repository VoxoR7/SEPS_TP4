GCC = gcc
CFLAGS = -g -Wall

EXEC = TP4

$(EXEC): clean $(EXEC).o messages.o
	$(GCC) $(EXEC).o messages.o -o $(EXEC)

$(EXEC).o: $(EXEC).c
	$(GCC) $(CFLAGS) -c $(EXEC).c -o $(EXEC).o

messages.o: messages.c messages.h
	$(GCC) $(CFLAGS) -c messages.c -o messages.o

clean:
	rm -f *.o

cleanall: clean
	rm -f $(EXEC)