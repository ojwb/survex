## Process this file with automake to produce Makefile.in

EXTRA_DIST = HACKING BUGS alien.pl
SUBDIRS = hto src lib doc tests

extra_bin = lib/*.msg lib/pfont.bit
extra_txt = lib/*.svx lib/print.ini
extra_files = $(extra_bin) $(extra_txt)
extra_text = BUGS AUTHORS COPYING NEWS TODO ChangeLog HACKING

base_progs = 3dtodxf cavern printdm printps printpcl printhpgl diffpos extend survex

dostxt :
	$(RM) -rf dostxt_tmp	
	mkdir dostxt_tmp
	for a in $(extra_text) ; do ln -s ../$$a dostxt_tmp/$$a.txt ; done
	# rename to 8.3
	mv dostxt_tmp/ChangeLog.txt dostxt_tmp/Changes.txt

djgpp : all djgpp_zip

djgpp_zip_name = survex-dos-@VERSION@.zip

djgpp_zip : dostxt
	test -d djgpp_exes || mkdir djgpp_exes
	for a in $(base_progs) caverot ; do cp src/$$a.exe djgpp_exes ; done
	for a in hto2svx svx2hto ; do cp hto/$$a.exe djgpp_exes ; done
	$(RM) $(djgpp_zip_name)
	zip -j $(djgpp_zip_name) djgpp_exes/*.exe $(extra_bin)\
	 djgpp_extras/cwsdpmi.exe djgpp_extras/emu387.dxe
	# -l means convert LF to CR+LF
	zip -j -l $(djgpp_zip_name) djgpp_extras/*.txt dostxt_tmp/*.txt $(extra_txt)
	$(RM) -rf dostxt_tmp

# ick - need to build djgpp before mingw to get caverot.exe, but there's
# no easy fix I can think of...

mingw : all mingw_zip

mingw_zip_name = survex-win32-@VERSION@.zip

mingw_zip : dostxt
	echo "Warning: You must build djgpp before mingw for caverot.exe"
	test -d mingw_exes || mkdir mingw_exes
	for f in $(base_progs) ; do cp src/$$f.exe mingw_exes ; done
	for a in hto2svx svx2hto ; do cp hto/$$a.exe mingw_exes ; done
	$(RM) $(mingw_zip_name)
	zip -j $(mingw_zip_name) mingw_exes/*.exe djgpp_exes/caverot.exe\
	 $(extra_bin)
	# -l means convert LF to CR+LF
	zip -j -l $(mingw_zip_name) mingw_extras/*.txt dostxt_tmp/*.txt\
	 $(extra_txt)
	$(RM) -rf dostxt_tmp

bc_zip_name = survex-dos286-@VERSION@.zip

borlandc_zip : dostxt
	$(RM) $(bc_zip_name)
	zip -j $(bc_zip_name) bc_exes/*.exe $(extra_bin) bc_extras/*.bgi
	# -l means convert LF to CR+LF
	zip -j -l $(bc_zip_name) bc_extras/*.txt dostxt_tmp/*.txt $(extra_txt)
	$(RM) -rf dostxt_tmp

riscos_zip_name = survex-riscod-@VERSION@.zip

riscos_zip :
	$(RM) $(riscos_zip_name)
	# needs a zip with Darren Salt's patches, which'll hopefully be merged
	# into the mainstream distribution in future
	$(RM) -rf riscos_tmp
	mkdir riscos_tmp
	for f in $(base_progs) caverot ; do\
	 ln -s ../riscos_exes/$$f riscos_tmp/$$f,ff8 ; done
	for f in $(extra_files) $(extra_text) ; do\
	 b=`echo "$$f" | sed 's!.*/!!'` ;\
	 type=fff ;\
	 case "$$f" in\
	  *.bit) type=ffd ;;\
	  *.msg) type=ffd ;;\
         esac ;\
	 ln -s ../$$f riscos_tmp/$$b,$$type ; done
	./zip-magic -N -j $(riscos_zip_name) riscos_tmp/*\
	 riscos_extras/*,[0-9a-f][0-9a-f][0-9a-f]
	$(RM) -rf riscos_tmp

# source code for transfer to the RISC OS/DOS machines for compilation
alien_src_zip_name = src$(shell echo "@VERSION@"|sed 's/\./_/g').zip

alien_src_zip :
	$(RM) $(alien_src_zip_name)
	$(RM) -rf alien_tmp
	mkdir alien_tmp
	mkdir alien_tmp/c
	mkdir alien_tmp/h
	mkdir alien_tmp/s
	mkdir alien_tmp/o
	perl alien.pl src/*.c src/*.h hto/*.c hto/*.h > alien_tmp/ObeyMe,feb
	ln -s ../src/riscos/stdh alien_tmp
	ln -s ../src/borlandc/makefile alien_tmp/makefile.mak
	ln -s ../../src/riscos/config.h alien_tmp/h/config
	ln -s ../../src/riscos/armrot.s alien_tmp/s/armrot
	./zip-magic -N -j $(alien_src_zip_name) alien_tmp src/*.c src/*.h\
	 hto/*.c hto/*.h src/riscos/makefile src/borlandc/config.h
	cd alien_tmp && ../zip-magic -N -r ../$(alien_src_zip_name) *
	$(RM) -rf alien_tmp

# FIXME: this is almost duplicated in doc/Makefile.am
SVXDOCS = svxdocs svxdocs/*.htm svxdocs.rtf svxdocs.txt svxdocs.ps

dos_doc_zip_name = svxdoc-doswin-@VERSION@.zip

dos_doc_zip :
	$(RM) $(dos_doc_zip_name)
	# -l means convert LF to CR+LF
	cd doc && zip -l ../$(dos_doc_zip_name) $(SVXDOCS)

riscos_doc_zip_name = svxdoc-riscos-@VERSION@.zip

riscos_doc_zip :
	$(RM) $(riscos_doc_zip_name)
	$(RM) -rf riscos_doc_tmp
	mkdir riscos_doc_tmp
	cd doc && for f in $(SVXDOCS) ; do\
	 if test -d $$f ; then\
	  mkdir ../riscos_doc_tmp/$$f ;\
	 else\
	  case "$$f" in\
	   */*.htm|*/*.html) ln -s ../../doc/$$f ../riscos_doc_tmp/$$f,faf ;;\
	   *.txt) ln -s ../doc/$$f ../riscos_doc_tmp/$$f,fff ;;\
	   *.rtf) ln -s ../doc/$$f ../riscos_doc_tmp/$$f,c32 ;;\
	   *) ln -s ../doc/$$f ../riscos_doc_tmp/$$f,ffd ;;\
	  esac;\
	 fi\
	done
	cd riscos_doc_tmp && ../zip-magic -N -r ../$(riscos_doc_zip_name) *
	$(RM) -rf riscos_doc_tmp
