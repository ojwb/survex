## Process this file with automake to produce Makefile.in

EXTRA_DIST = BUGS
SUBDIRS = hto src lib doc

extra_files = lib/*.msg lib/*.svx lib/pfont.bit lib/print.ini\
 BUGS AUTHORS COPYING NEWS TODO ChangeLog

djgpp_zip : all
	$(RM) svx-dos-@VERSION@.zip
	zip -j svx-dos-@VERSION@.zip src/*.exe $(extra_files)\
	djgpp_extras/cwsdpmi.exe djgpp_extras/emu387.dxe djgpp_extras/*.TXT

mingw32_zip : all
	$(RM) svx-nt-@VERSION@.zip
	zip -j svx-nt-@VERSION@.zip src/*.exe
	#$(extra_files) mingw32_extras/*.TXT

borlandc_zip :
	$(RM) svx-bcdos-@VERSION@.zip
	zip -j svx-bcdos-@VERSION@.zip bc_exes/*.exe $(extra_files)\
	bc_extras/*.bgi bc_extras/*.TXT

riscos_zip :
	$(RM) svx-riscos-@VERSION@.zip
	zip -j svx-riscos-@VERSION@.zip riscos_exes/* $(extra_files)\
	riscos_extras/ReadMe

# source code for transfer to the RISC OS/DOS machines for compilation
alien_src = src$(shell echo "@VERSION@"|sed 's/\./_/g').zip

alien_src_zip :
	$(RM) $(alien_src)
	$(RM) -rf alien_tmp
	mkdir alien_tmp
	mkdir alien_tmp/c
	mkdir alien_tmp/h
	mkdir alien_tmp/s
	mkdir alien_tmp/o
	perl alien.pl src/*.c src/*.h > alien_tmp/ObeyMe
	ln -s ../src/riscos/stdh alien_tmp
	ln -s ../src/borlandc/makefile alien_tmp/makefile.mak
	ln -s ../../src/riscos/config.h alien_tmp/h/config
	ln -s ../../src/riscos/armrot.s alien_tmp/s/armrot
	zip -j $(alien_src) alien_tmp src/*.c src/*.h src/riscos/makefile\
	 src/borlandc/config.h
	cd alien_tmp && zip -r ../$(alien_src) *
