CC := vc +aos68k
CFLAGS := -c99
LIBS := -lamiga

OBJS := avalanche.o libs.o xad.o
DEPS := avalanche_rev.h libs.h xad.h

all: Avalanche install

%.o: %.c $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $<

Avalanche: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

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
