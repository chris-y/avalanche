CC := vc +aos68k
CFLAGS := -c99
LIBS := -lamiga

OBJS := avalanche.o libs.o xad.o
DEPS := avalanche_rev.h avalanche.h libs.h xad.h

all: Avalanche AvalanchePPC install

%.o: %.c $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $<

Avalanche: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

AvalanchePPC: $(OBJS)
	make -i -f Makefile.OS4 clean
	make -f Makefile.OS4

install: Avalanche
	copy Avalanche /

distrib: install
	makedir RAM:Avalanche
	copy //Avalanche.info RAM:
	copy /Avalanche#? RAM:Avalanche
	copy /avalanche.readme#? RAM:Avalanche
	copy /Install#? RAM:Avalanche
	copy /LICENSE RAM:Avalanche
	lha a ram:avalanche.lha ram:Avalanche ram:Avalanche.info

clean:
	delete #?.o
	delete Avalanche

version:
	bumprev 1 Avalanche
