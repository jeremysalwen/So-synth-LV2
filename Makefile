OBJECTS = main.o
BINARY = so-666
CC = gcc
CFLAGS = -Wall -O3 -lm `pkg-config --cflags --libs jack` `pkg-config --cflags --libs alsa`

$(BINARY) : $(OBJECTS)
	$(CC) $(CFLAGS) -lm $^ -o $@

.SUFFIXES : .c .o

.c.o:
	$(CC) $(CFLAGS) -c $<

main.o : main.c

.PHONY : clean

clean :
	rm -f $(BINARY) $(OBJECTS)

