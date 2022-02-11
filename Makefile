PREFIX ?= /usr
LV2_DIR ?= /lib/lv2
OBJECTS = so-666.o so-kl5.o so-404.o sosynth.o
LIBRARY = libsosynth.so
TTLS = so-666.ttl so-kl5.ttl so-404.ttl manifest.ttl
CC = gcc
CFLAGS += -Wall -O3 -ffast-math -lm `pkg-config --cflags --libs lv2` -fPIC
INSTALLDIR = $(DESTDIR)$(PREFIX)$(LV2_DIR)/
INSTALLNAME = so-synth.lv2/
$(LIBRARY) : $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -shared -o $@

.SUFFIXES : .c .o

.c.o:
	$(CC) $(CFLAGS) -c $<

so-666.o : so-666.c

so-kl5.o : so-kl5.c

so-404.o : so-404.c

.PHONY : clean install uninstall

clean :
	rm -f $(LIBRARY) $(OBJECTS)

install :
	mkdir -p $(INSTALLDIR)$(INSTALLNAME)
	install $(LIBRARY) $(INSTALLDIR)$(INSTALLNAME)
	install -m 644 $(TTLS) $(INSTALLDIR)$(INSTALLNAME)

uninstall :
	rm -rf $(INSTALLDIR)$(INSTALLNAME)
