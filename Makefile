CC := vc +aos68k
CFLAGS := -c99
LIBS := -lamiga -lauto -lreaction

OBJS := avalanche.o xad.o
DEPS := avalanche_rev.h xad.h

all: Avalanche

%.o: %.c $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $<

Avalanche: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)
