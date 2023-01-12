CC := vc +aos68k
CFLAGS := -c99 -O3
LIBS := -lamiga
OBJDIR := objs_os3
VPATH := ..

OBJS := avalanche.o arexx.o config.o libs.o locale.o module.o mod_lha.o mod_zip.o req.o win.o xad.o xfd.o xvs.o
DEPS := avalanche_rev.h avalanche.h arexx.h config.h libs.h locale.h locale_strings.h module.h req.h win.h xad.h xfd.h xvs.h

all: Avalanche

%.o: %.c $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $<

Avalanche: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

clean:
	delete #?.o
	delete Avalanche
