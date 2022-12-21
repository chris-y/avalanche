CC := vc +newlib
CFLAGS := -static -O3 -D__USE_INLINE__ -ISDK:local/common/include
LIBS := -lamiga
VPATH := ..

OBJS := avalanche.o arexx.o config.o libs.o locale.o module.o req.o win.o xad.o xfd.o xvs.o
DEPS := avalanche_rev.h avalanche.h arexx.h config.h libs.h locale.h locale_strings.h module.h req.h win.h xad.h xfd.h xvs.h

all: AvalanchePPC

%.o: %.c $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $<

AvalanchePPC: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

clean:
	delete #?.o
	delete AvalanchePPC
