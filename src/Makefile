export prefix=/usr/local
export distro=$(shell uname -o)

CFLAGS = -Wall -pedantic
PIC =
ARCH = $(shell uname -m)
LIBDIR = $(prefix)/lib


ifeq "$(ARCH)" "x86_64"
PIC = -fPIC

#ifneq "$(distro)" "debian"
#LIBDIR = $(prefix)/lib64
#ifneq "$(shell if [ -d $(prefix)/lib64 ]; then echo yes; fi)" "yes"
#LIBDIR = $(prefix)/lib
#endif
#endif
endif

all:	libpoldit.so poldit poldit.pc

poldit_data.h: units.in cmap.in mkdata.py
	python3 mkdata.py

poldit.o: poldit.c poldit.h poldit_data.h outpush.h
	gcc -c -o poldit.o poldit.c $(CFLAGS) $(PIC)

main.o: main.c poldit.h
	gcc -c -o main.o -I. main.c $(CFLAGS) $(PIC)

libpoldit.so: poldit.o
	gcc -o libpoldit.so -shared -rdynamic poldit.o

poldit: libpoldit.so main.o
	gcc -o poldit main.o -I. -L. -lpoldit

poldit.pc:	poldit.pc.in
	sed -e s\\{PREFIX}\\$(prefix)\\ \
		-e s\\{LIBDIR}\\$(LIBDIR)\\ < poldit.pc.in > poldit.pc

clean:
	rm -f libpoldit.so poldit.o main.o poldit poldit.pc

install:	libpoldit.so poldit
	install -m 0755 poldit $(DESTDIR)$(prefix)/bin
	install -m 0755 libpoldit.so $(DESTDIR)$(LIBDIR)
	install -m 0644 poldit.h $(DESTDIR)$(prefix)/include/
	if [ -d $(LIBDIR)/pkgconfig ] ; then install -m 0644 poldit.pc $(DESTDIR)$(LIBDIR)/pkgconfig; fi
	ldconfig

uninstall:
	rm -f $(DESTDIR)$(prefix)/bin/poldit
	rm -f $(DESTDIR)$(LIBDIR)/libpoldit.so
	rm -f $(DESTDIR)$(prefix)/include/poldit.h
	rm -f $(DESTDIR)$(LIBDIR)/pkgconfig/poldit.pc
	ldconfig
