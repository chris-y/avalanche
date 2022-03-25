CC := vc +aos68k
CFLAGS := -c99
LIBS := -lamiga

OBJS := avalanche.o libs.o req.o xad.o xfd.o xvs.o
DEPS := avalanche_rev.h avalanche.h libs.h req.h xad.h xfd.h xvs.h

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
	lha a ram:avalanche.lha ram:Avalanche ram:Avalanche.info ram:AutoInstall

clean:
	delete #?.o
	delete Avalanche

version:
	bumprev 1 Avalanche
