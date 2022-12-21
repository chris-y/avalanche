all: install

Avalanche: locale_strings.h
	make -C os3

AvalanchePPC: locale_strings.h
	make -C os4

locale_strings.h: ../catalogs/avalanche.cd
	catcomp /catalogs/avalanche.cd CFILE locale_strings.h NOCODE NOARRAY

install: Avalanche AvalanchePPC
	copy os3/Avalanche /
	copy os4/AvalanchePPC /

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
	delete locale_strings.h
	make -i -f Makefile.OS3 -C os3 clean
	make -i -f Makefile.OS4 -C os4 clean

version:
	bumprev 1 Avalanche
