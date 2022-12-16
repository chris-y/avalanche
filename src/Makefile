CC := vc +aos68kpl
CFLAGS := -c99 -O3
LIBS := -lamiga -lposix

OBJS := avalanche.o arexx.o config.o libs.o locale.o mod_zip.o module.o req.o win.o xad.o xfd.o xvs.o lib/zipc/zipc.o
DEPS := avalanche_rev.h avalanche.h arexx.h config.h libs.h locale.h locale_strings.h module.h req.h win.h xad.h xfd.h xvs.h lib/zipc/zipc.h

all: install

%.o: %.c $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $<

Avalanche: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

AvalanchePPC: $(OBJS)
	make -i -f Makefile.OS4 clean
	make -f Makefile.OS4

locale_strings.h: ../catalogs/avalanche.cd
	catcomp /catalogs/avalanche.cd CFILE locale_strings.h NOCODE NOARRAY

install: Avalanche AvalanchePPC
	copy Avalanche /
	copy AvalanchePPC /

distrib: install
	makedir RAM:Avalanche
	copy //Avalanche.info RAM:
	copy /Avalanche#? RAM:Avalanche
	copy /avalanche.readme#? RAM:Avalanche
	copy /Install#? RAM:Avalanche
	copy /LICENSE RAM:Avalanche
	copy /AutoInstall RAM:
	makedir RAM:Avalanche/libs
	copy /libs/#? RAM:Avalanche/libs
	makedir RAM:Avalanche/catalogs
	copy /catalogs/#? RAM:Avalanche/catalogs
	lha a ram:avalanche.lha ram:Avalanche ram:Avalanche.info ram:AutoInstall

clean:
	delete #?.o
	delete lib/zipc/#?.o
	delete Avalanche

version:
	bumprev 1 Avalanche
