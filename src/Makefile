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
	makedir RAM:Avalanche/misc
	copy /misc/#? RAM:Avalanche/misc
	lha a ram:avalanche.lha ram:Avalanche ram:Avalanche.info ram:AutoInstall

clean:
	make -i -C os3 clean
	make -i -C os4 clean
	delete locale_strings.h

version:
	bumprev 3 Avalanche
