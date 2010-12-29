OBJECTS = so-666.o
LIBRARY = libso-666.so
CC = gcc
CFLAGS = -Wall -O3 -lm `pkg-config --cflags --libs lv2core`

$(LIBRARY) : $(OBJECTS)
	$(CC) $(CFLAGS) -shared -o $@

.SUFFIXES : .c .o

.c.o:
	$(CC) $(CFLAGS) -c $<

so-666.o : so-666.c

.PHONY : clean

clean :
	rm -f $(LIBRARY) $(OBJECTS)

