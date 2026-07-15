CC := vc +aos68k
#INCLUDES := -Inetinclude:
INCLUDES := -I$(NDK32_INC) -I$(NET_INC) -I$(AMISSL_INC) 
CFLAGS := -c99 -O3 $(INCLUDES)
LIBS := -lamiga
OBJDIR := objs_os3
VPATH := ..

OBJS := avalanche.o arexx.o config.o deark.o glyph.o http.o libs.o locale.o misc.o module.o mod_lha.o mod_zip_os3.o new.o progress.o req.o tab.o update.o win.o xad.o xfd.o xvs.o
DEPS := avalanche_rev.h avalanche.h arexx.h config.h deark.h glyph.h http.h libs.h locale.h locale_strings.h misc.h module.h new.h progress.h req.h tab.h update.h win.h xad.h xfd.h xvs.h

all: Avalanche

%.o: %.c $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $<

Avalanche: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

clean:
	delete #?.o
	delete Avalanche
