OBJECTS = so-666.o
LIBRARY = libso-666.so
CC = gcc
CFLAGS = -Wall -O3 -lm `pkg-config --cflags --libs lv2core`
INSTALLDIR = $(DESTDIR)/usr/lib/lv2/
INSTALLNAME = so-666.lv2/
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
	mkdir -p $(INSTALLDIR)
	install $(LIBRARY) $(INSTALLDIR)$(INSTALLNAME)
	install manifest.ttl $(INSTALLDIR)$(INSTALLNAME)
	install so-666.ttl $(INSTALLDIR)$(INSTALLNAME)
uninstall :
	rm -rf $(INSTALLDIR)$(INSTALLNAME)
