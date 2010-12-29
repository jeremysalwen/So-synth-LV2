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

.PHONY : clean install uninstall

clean :
	rm -f $(LIBRARY) $(OBJECTS)

install :
	mkdir -p /usr/lib/lv2/so-666.lv2/
	install $(LIBRARY) /usr/lib/lv2/so-666.lv2/
	install manifest.ttl /usr/lib/lv2/so-666.lv2/
	install so-666.ttl /usr/lib/lv2/so-666.lv2/