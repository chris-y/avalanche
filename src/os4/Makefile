CC := vc +newlib
CFLAGS := -static -O3 -D__USE_INLINE__ -ISDK:local/common/include -ISDK:include/netinclude
LIBS := 
VPATH := ..

OBJS := avalanche.o arexx.o config.o deark.o http.o libs.o locale.o misc.o module.o mod_lha.o mod_zip.o new.o req.o update.o win.o xad.o xfd.o xvs.o
DEPS := avalanche_rev.h avalanche.h arexx.h config.h deark.h http.h libs.h locale.h locale_strings.h misc.h module.h new.h req.h update.h win.h xad.h xfd.h xvs.h

all: AvalanchePPC

%.o: %.c $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $<

AvalanchePPC: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

clean:
	delete #?.o
	delete AvalanchePPC
