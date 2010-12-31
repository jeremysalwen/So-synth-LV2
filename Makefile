OBJECTS = so-666.o so-kl5.o
LIBRARY = libsosynth.so
CC = gcc
CFLAGS += -Wall -O3 -lm `pkg-config --cflags --libs lv2core` -fPIC
INSTALLDIR = $(DESTDIR)/usr/lib/lv2/
INSTALLNAME = so-synth.lv2/
$(LIBRARY) : $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -shared -o $@

.SUFFIXES : .c .o

.c.o:
	$(CC) $(CFLAGS) -c $<

so-666.o : so-666.c

so-kl5.o : so-kl5.c

.PHONY : clean install uninstall

clean :
	rm -f $(LIBRARY) $(OBJECTS)

install :
	mkdir -p $(INSTALLDIR)$(INSTALLNAME)
	install $(LIBRARY) $(INSTALLDIR)$(INSTALLNAME)
	install manifest.ttl $(INSTALLDIR)$(INSTALLNAME)
	install so-666.ttl $(INSTALLDIR)$(INSTALLNAME)
	install so-kl5.ttl $(INSTALLDIR)$(INSTALLNAME)
	
uninstall :
	rm -rf $(INSTALLDIR)$(INSTALLNAME)
