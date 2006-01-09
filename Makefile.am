## Process this file with automake to produce Makefile.in

# We aim to be GNU compliant, but some required files are generated
AUTOMAKE_OPTIONS = 1.5 foreign

# These files are installed automatically in --gnu mode
GNU_FILES = INSTALL NEWS README COPYING AUTHORS ChangeLog

EXTRA_DIST = $(GNU_FILES) TODO survex.iss.in OLDNEWS\
 desc.txt desc-aven.txt survex.spec.in survex.spec\
 desc-svxedit.txt alien.pl\
 buildmacosx.sh vim/syntax/survex.vim vim/ftdetect/survex.vim

SUBDIRS = . lib src doc tests debian

# seem to need this for "make distclean" to recurse?!?
DIST_SUBDIRS = $(SUBDIRS)

extra_bin = lib/*.msg\
 lib/default.bit lib/bold.bit
extra_txt = lib/*.svx lib/print.ini
extra_files = $(extra_bin) $(extra_txt)

# FIXME: need to keep in step with src/Makefile.am
base_progs = cad3d cavern diffpos extend sorterr 3dtopos\
 printdm printps printpcl printhpgl

AUTHORS: doc/AUTHORS.htm
	w3m -dump doc/AUTHORS.htm > AUTHORS

HACKING: doc/HACKING.htm
	w3m -dump doc/HACKING.htm > HACKING

TODO: doc/TODO.htm
	w3m -dump doc/TODO.htm > TODO

rpm : dist
	: # spec file is produced using AC_OUTPUT
	echo %_topdir `pwd` > rpmmacros
	echo %_rpmdir `pwd` >> rpmmacros
	echo %_sourcedir `pwd` >> rpmmacros
	echo %_specdir `pwd` >> rpmmacros
	echo %_srcrpmdir `pwd` >> rpmmacros
	echo %_rpmfilename %%{NAME}-%%{VERSION}-%%{RELEASE}.%%{ARCH}.rpm >> rpmmacros
	: #
	echo macrofiles : `rpm --showrc | sed 's/^macrofiles\> *: //p;d'`:`pwd`/rpmmacros > rpmrc
	: #
	rm -rf BUILD
	mkdir BUILD
	: #
	rpmbuild -bs --clean --rcfile /usr/lib/rpm/rpmrc:`pwd`/rpmrc survex.spec

debian : dist
	: # overkill maybe - we could probably just use this tree
	: # but then the .deb-s go in the parent directory, plus we get
	: # lintian moaning about the directory name not having the
	: # version number
	-rm -rf deb
	mkdir deb
	cd deb && tar zxvf ../${PACKAGE}-${VERSION}.tar.gz
	: # -us and -uc mean don't sign packages
	cd deb/${PACKAGE}-${VERSION} && debuild -us -uc
	mv deb/*.deb deb/*.dsc deb/*.changes deb/*.gz .
	rm -rf deb

dostxt :
	$(RM) -rf dostxt_tmp
	mkdir dostxt_tmp
	for a in @EXTRA_TEXT@ ; do ln -s ../$$a dostxt_tmp/$$a.txt ; done
	: # rename to 8.3
	mv dostxt_tmp/ChangeLog.txt dostxt_tmp/Changes.txt

djgpp : all djgpp_zip

djgpp_zip_name = survex-dos-@VERSION@.zip
djgpp_exe_name = survex-dos-@VERSION@.exe

djgpp_exe : djgpp_zip
	$(RM) $(djgpp_exe_name)~
	cat dos_extras/unzipsfx.exe $(djgpp_zip_name) > $(djgpp_exe_name)~
	zip -A $(djgpp_exe_name)~
	mv $(djgpp_exe_name)~ $(djgpp_exe_name)

djgpp_zip : dostxt
	test -d djgpp_exes || mkdir djgpp_exes
	for a in $(base_progs) caverot ; do cp src/$$a.exe djgpp_exes ; done
	$(RM) $(djgpp_zip_name)
	zip -9 -j $(djgpp_zip_name) djgpp_exes/*.exe $(extra_bin)\
	 djgpp_extras/cwsdpmi.exe djgpp_extras/emu387.dxe
	ln -s ../INSTALL.DOS dostxt_tmp/INSTALL.txt
	ln -s ../README dostxt_tmp/README.txt
	ln -s ../src/svxedit dostxt_tmp/svxedit.tcl
	: # -l means convert LF to CR+LF
	: # FIXME: really include dostxt too on DOS?
	zip -9 -j -l $(djgpp_zip_name) dostxt_tmp/*.txt dostxt_tmp/svxedit.tcl\
	 $(extra_txt)
	cd doc;zip -9 -l ../$(djgpp_zip_name) manual/*.htm @HTMLFILES@
	cd doc;zip -9 ../$(djgpp_zip_name) manual/*.png
	$(RM) -rf dostxt_tmp

mingw : all mingw_iss

mingw_iss : survex.iss
	$(RM) -rf iss_tmp
	mkdir iss_tmp
	cp src/svxedit iss_tmp/svxedit.tcl
	cp $(extra_txt) survex.iss iss_tmp
	: # so the installer can display the license
	cp COPYING iss_tmp/COPYING.txt
	cd doc;cp @HTMLFILES@ ../iss_tmp
	: # convert LF to CR+LF
	perl -p -i -e 's/\n/\r\n/' iss_tmp/*
	for f in $(base_progs) aven printwin editwrap ; do cp src/$$f.exe iss_tmp ; done
	cp lib/icons/*.ico lib/icons/*.png iss_tmp
	: # don't ship pngs for which we ship the ico version
	for a in iss_tmp/*.ico ; do rm `echo $$a|sed 's/\.ico/.png/'` ; done
	: # but we do want aven.png for the About dialog
	cp lib/icons/aven.png iss_tmp
	: # not needed if we build wx without threads: gzip -dc /usr/share/doc/mingw32-runtime/mingwm10.dll.gz > iss_tmp/mingwm10.dll
	mkdir iss_tmp/manual
	cp doc/manual/*.htm iss_tmp/manual
	: # convert LF to CR+LF
	perl -p -i -e 's/\n/\r\n/' iss_tmp/manual/*
	cp doc/manual/*.png iss_tmp/manual
	cp $(extra_bin) iss_tmp
	: # NB keep next line in step with survex.iss.in
	for a in cs de es fr it; do\
	 mkdir iss_tmp/$$a;\
	 cp /usr/share/locale/$$a/LC_MESSAGES/wxstd.mo iss_tmp/$$a;\
	done
	: # Also need to reduce colour depth it appears:
	: # src/png2bmp lib/icons/aven_about.png iss_tmp/logo.bmp
	echo 'wine "c:/Program Files/Inno Setup 4/ISCC.exe" iss_tmp/survex.iss' > iss_tmp/runissinwine
	chmod 755 iss_tmp/runissinwine
	./xvfb-run -a --server-args="-screen 0 64x32x8" iss_tmp/runissinwine
	mv iss_tmp/Output/*.exe .
	$(RM) -rf iss_tmp

bc_zip_name = survex-dos286-@VERSION@.zip
bc_exe_name = survex-dos286-@VERSION@.exe

borlandc_exe : borlandc_zip
	$(RM) $(bc_exe_name)~
	cat dos_extras/unzipsfx.exe $(bc_zip_name) > $(bc_exe_name)~
	zip -A $(bc_exe_name)~
	mv $(bc_exe_name)~ $(bc_exe_name)

borlandc_zip : dostxt
	$(RM) $(bc_zip_name)
	zip -9 -j $(bc_zip_name) bc_exes/*.exe $(extra_bin) bc_extras/*.bgi
	ln -s ../INSTALL.DOS dostxt_tmp/INSTALL.txt
	ln -s ../README dostxt_tmp/README.txt
	: # -l means convert LF to CR+LF
	: # FIXME: really include dostxt too on DOS?
	zip -9 -j -l $(bc_zip_name) dostxt_tmp/*.txt $(extra_txt)
	cd doc;zip -9 -l ../$(bc_zip_name) manual/*.htm @HTMLFILES@
	cd doc;zip -9 ../$(bc_zip_name) manual/*.png
	$(RM) -rf dostxt_tmp

riscos_zip_name = survex-riscos-@VERSION@.zip

riscos_zip :
	$(RM) $(riscos_zip_name)
	: # needs a zip with Darren Salt's patches, which'll hopefully be merged
	: # into the mainstream distribution in future
	$(RM) -rf riscos_tmp
	mkdir riscos_tmp
	ln -s ../README.ros riscos_tmp/\!ReadMe,fff
	: # "cp -r riscos_extras riscos_tmp" but avoid CVS directories
	cd riscos_extras; tar --exclude CVS -cf - . | (cd ../riscos_tmp ; tar xf -)
	cp -r riscos_extras/\!* riscos_tmp
	for f in $(base_progs) caverot ; do\
	 ln -s ../../riscos_exes/$$f riscos_tmp/\!Cavern/$$f,ff8 ; done
	for f in @EXTRA_TEXT@ ; do\
	 b=`echo "$$f" | sed 's!.*/!!'` ;\
	 type=fff ;\
	 case "$$f" in\
	  *.bit) type=ffd ;;\
	  *.msg) type=ffd ;;\
         esac ;\
	 ln -s ../$$f riscos_tmp/$$b,$$type ; done
	for f in $(extra_files) ; do\
	 b=`echo "$$f" | sed 's!.*/!!'` ;\
	 type=fff ;\
	 case "$$f" in\
	  *.bit) type=ffd ;;\
	  *.msg) type=ffd ;;\
         esac ;\
	 ln -s ../../$$f riscos_tmp/\!Cavern/$$b,$$type ; done
	cd doc && for f in @HTMLFILES@ ; do\
	 ln -s ../doc/$$f ../riscos_tmp/$$f,faf ; done
	mkdir riscos_tmp/manual
	cd doc && for f in manual/*.htm ; do\
	 ln -s ../../doc/$$f ../riscos_tmp/$$f,faf ; done
	cd doc && for f in manual/*.png ; do\
	 ln -s ../../doc/$$f ../riscos_tmp/$$f,b60 ; done
	: # -R doesn't seem to work here, so just glob it all in
	cd riscos_tmp && ../zip-magic -N ../$(riscos_zip_name) * */* */*/*
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
	mkdir alien_tmp/stdh
	perl alien.pl src/*.c src/*.h > alien_tmp/\!ObeyMe,feb
	echo 'RMEnsure DDEUtils 0 System:modules.ddeutils' > alien_tmp/\!MakeMe,feb
	echo 'Dir <Obey$$Dir>' >> alien_tmp/\!MakeMe,feb
	echo amu >> alien_tmp/\!MakeMe,feb
	echo Back >> alien_tmp/\!MakeMe,feb
	ln -s ../src/riscos/stdh/[a-z]* alien_tmp
	ln -s ../src/borlandc/makefile alien_tmp/makefile.mak
	ln -s ../src/borlandc/make.bat alien_tmp
	ln -s ../../src/riscos/config.h alien_tmp/h/config
	ln -s ../../src/riscos/armrot.s alien_tmp/s/armrot
	ln -s ../../src/riscos/stdh/regnames alien_tmp/stdh
	ln -s ../../src/riscos/stdh/swinames alien_tmp/stdh
	./zip-magic -N -j $(alien_src_zip_name) alien_tmp src/*.c src/*.h\
	 src/riscos/makefile src/borlandc/config.h
	cd alien_tmp && ../zip-magic -N -r ../$(alien_src_zip_name) *
	$(RM) -rf alien_tmp

dos_doc_zip_name = svxdoc-doswin-@VERSION@.zip
dos_doc_exe_name = svxdoc-doswin-@VERSION@.exe

dos_doc_exe : dos_doc_zip
	$(RM) $(dos_doc_exe_name)~
	cat dos_extras/unzipsfx.exe $(dos_doc_zip_name) > $(dos_doc_exe_name)~
	zip -A $(dos_doc_exe_name)~
	mv $(dos_doc_exe_name)~ $(dos_doc_exe_name)

dos_doc_zip :
	$(RM) $(dos_doc_zip_name)
	: # -l means convert LF to CR+LF
	cd doc && zip -9 -l ../$(dos_doc_zip_name) @DOCS_OTHER@

riscos_doc_zip_name = svxdoc-riscos-@VERSION@.zip

riscos_doc_zip :
	$(RM) $(riscos_doc_zip_name)
	$(RM) -rf riscos_doc_tmp
	mkdir riscos_doc_tmp
	cd doc && for f in @DOCS_OTHER@ ; do\
	 if test -d $$f ; then\
	  mkdir ../riscos_doc_tmp/$$f ;\
	 else\
	  case "$$f" in\
	   */*.htm|*/*.html) ln -s ../../doc/$$f ../riscos_doc_tmp/$$f,faf ;;\
	   *.htm|*.html) ln -s ../doc/$$f ../riscos_doc_tmp/$$f,faf ;;\
	   *.txt) ln -s ../doc/$$f ../riscos_doc_tmp/$$f,fff ;;\
	   *.rtf) ln -s ../doc/$$f ../riscos_doc_tmp/$$f,c32 ;;\
	   *.ps) ln -s ../doc/$$f ../riscos_doc_tmp/$$f,ff5 ;;\
	   *) ln -s ../doc/$$f ../riscos_doc_tmp/$$f,ffd ;;\
	  esac;\
	 fi\
	done
	cd riscos_doc_tmp && ../zip-magic -N -r ../$(riscos_doc_zip_name) *
	$(RM) -rf riscos_doc_tmp
