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
