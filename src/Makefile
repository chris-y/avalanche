CC := vc +aos68k
CFLAGS := -c99 -g
LIBS := -lamiga

OBJS := avalanche.o arexx.o libs.o locale.o req.o win.o xad.o xfd.o xvs.o
DEPS := avalanche_rev.h avalanche.h arexx.h libs.h locale.h locale_strings.h req.h win.h xad.h xfd.h xvs.h

all: install

%.o: %.c $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $<

Avalanche: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

AvalanchePPC: $(OBJS)
	make -i -f Makefile.OS4 clean
	make -f Makefile.OS4

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
	delete locale_strings.h
	delete Avalanche

version:
	bumprev 1 Avalanche
