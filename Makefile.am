## Process this file with automake to produce Makefile.in

# We aim to be GNU compliant, but some required files are generated
AUTOMAKE_OPTIONS = foreign

RELEASE = 1

# These files are installed automatically in --gnu mode
GNU_FILES = INSTALL NEWS README COPYING AUTHORS ChangeLog

EXTRA_DIST = $(GNU_FILES) OLDNEWS HACKING TODO alien.pl ZOMBIES survex.iss.in

SUBDIRS = lib src doc tests debian

# seem to need this for "make distclean" to recurse?!?
DIST_SUBDIRS = $(SUBDIRS)

extra_bin = lib/*.msg lib/pfont.bit
extra_txt = lib/*.svx lib/print.ini
extra_files = $(extra_bin) $(extra_txt)
extra_text = AUTHORS COPYING NEWS TODO ChangeLog HACKING OLDNEWS ZOMBIES

# FIXME: need to keep in step with src/Makefile.am
base_progs = cad3d cavern printdm printps printpcl printhpgl diffpos\
 extend survex sorterr 3dtopos

AUTHORS: doc/AUTHORS.html
	w3m -dump doc/AUTHORS.html > AUTHORS

#NEWS: doc/NEWS.html
#	w3m -dump doc/NEWS.html > NEWS

TODO: doc/TODO.html
	w3m -dump doc/TODO.html > TODO

HACKING: doc/HACKING.html
	w3m -dump doc/HACKING.html > HACKING

spec = ${PACKAGE}-${VERSION}.spec

rpm : dist
	echo Summary: Cave Surveying Software > ${spec}
	echo Name: ${PACKAGE} >> ${spec}
	echo Vendor: The Survex Project >> ${spec}
	echo Version: ${VERSION} >> ${spec}
	echo Release: ${RELEASE} >> ${spec}
	echo Copyright: GPL >> ${spec}
	#echo Group: Applications/Engineering >> ${spec}
	echo Group: Applications/Misc >> ${spec}
	echo Source: http://www.survex.com/software/${PACKAGE}-${VERSION}.tar.gz >> ${spec}
	echo URL: http://www.survex.com/ >> ${spec}
	echo "Packager: Olly Betts <olly@survex.com>" >> ${spec}
	echo BuildRoot: /var/tmp/%{name}-buildroot >> ${spec}
	echo >> ${spec}
	echo %description >> ${spec}
	cat desc.txt >> ${spec}
	echo >> ${spec}
	echo %package aven >> ${spec}
	echo Summary: Aven viewer for Survex >> ${spec}
	echo Group: Applications/Misc >> ${spec}
	echo Requires: survex = ${VERSION} >> ${spec}
	echo >> ${spec}
	echo %description aven >> ${spec}
	cat desc-aven.txt >> ${spec}
	echo >> ${spec}
	echo %prep >> ${spec}
	echo %setup >> ${spec}
	echo >> ${spec}
	echo %build >> ${spec}
	echo ./configure --prefix=/usr >> ${spec}
	echo make >> ${spec}
	echo >> ${spec}
	echo %install >> ${spec}
	echo rm -rf '$$RPM_BUILD_ROOT' >> ${spec}
	echo mkdir '$$RPM_BUILD_ROOT' >> ${spec}
	echo make install-strip 'prefix=$$RPM_BUILD_ROOT/usr' >> ${spec}
	echo >> ${spec}
	echo %clean >> ${spec}
	echo rm -rf '$$RPM_BUILD_ROOT' >> ${spec}
	echo >> ${spec}
	echo %files >> ${spec}
	echo "%defattr(-,root,root)" >> ${spec}
	echo %doc ${extra_text} >> ${spec}
	echo %doc /usr/doc/${PACKAGE} >> ${spec}
	echo %doc /usr/man >> ${spec}
	echo '/usr/bin/[b-zA-Z0-9]*' >> ${spec}
	echo /usr/share/${PACKAGE} >> ${spec}
	echo %files aven >> ${spec}
	echo "%defattr(-,root,root)" >> ${spec}
	echo /usr/bin/aven >> ${spec}
	#
	echo %_topdir `pwd` > rpmmacros
	echo %_rpmdir `pwd` >> rpmmacros
	echo %_sourcedir `pwd` >> rpmmacros
	echo %_specdir `pwd` >> rpmmacros
	echo %_srcrpmdir `pwd` >> rpmmacros
	echo %_rpmfilename %%{NAME}-%%{VERSION}-%%{RELEASE}.%%{ARCH}.rpm >> rpmmacros
	#
	echo macrofiles : `rpm --showrc | sed 's/^macrofiles\> *: //p;d'`:`pwd`/rpmmacros > rpmrc
	#
	rm -rf BUILD
	mkdir BUILD
	#
	if test -f /usr/lib/rpm/rpmrc ; then \
	  rpm -ba --clean --rcfile /usr/lib/rpm/rpmrc:`pwd`/rpmrc ${spec} ; \
	else \
	  rpm -ba --clean --rcfile /opt/crosstools/autotools/lib/rpm/rpmrc:`pwd`/rpmrc ${spec} ; \
	fi

debcl=deb/${PACKAGE}-${VERSION}/debian/changelog
debct=deb/${PACKAGE}-${VERSION}/debian/control

debian : dist
	-rm -rf deb
	mkdir deb
	cd deb && tar zxvf ../${PACKAGE}-${VERSION}.tar.gz
	echo "${PACKAGE} (${VERSION}.${RELEASE}) unstable; urgency=low" > ${debcl}
	echo >> ${debcl}
	echo "  * Dummy changelog - see ChangeLog and NEWS." >> ${debcl}
	echo >> ${debcl}
	echo " -- Olly Betts <olly@survex.com>  "`date "+%a, %d %b %Y %X %z"` >> ${debcl}
	echo Source: survex > ${debct}
	echo Section: math >> ${debct}
	echo Priority: optional >> ${debct}
	echo "Maintainer: Wookey <wookey@survex.com>" >> ${debct}
	echo Standards-Version: 3.0.1 >> ${debct}
	echo Build-Depends: debhelper, libwxgtk2.2-dev, sgmltools-lite >> ${debct}
	echo >> ${debct}
	echo Package: ${PACKAGE} >> ${debct}
	echo Architecture: any >> ${debct}
	echo 'Depends: ${shlibs:Depends}' >> ${debct}
	echo Recommends: survex-aven >> ${debct}
	echo Description: Cave surveying and mapping software >> ${debct}
	sed 's/^/ /' < desc.txt >> ${debct}
	echo >> ${debct}
	echo Package: survex-aven >> ${debct}
	echo Architecture: any >> ${debct}
	echo 'Depends: survex, ${shlibs:Depends}' >> ${debct}
	echo Description: Sophisticated cave survey viewer for Survex >> ${debct}
	sed 's/^/ /' < desc-aven.txt >> ${debct}	
	# -us and -uc mean don't sign packages
	cd deb/${PACKAGE}-${VERSION} && debuild -us -uc
	mv deb/*.deb deb/*.dsc deb/*.changes deb/*.gz .
	rm -rf deb

dostxt :
	$(RM) -rf dostxt_tmp	
	mkdir dostxt_tmp
	for a in $(extra_text) ; do ln -s ../$$a dostxt_tmp/$$a.txt ; done
	# rename to 8.3
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
	zip -j $(djgpp_zip_name) djgpp_exes/*.exe $(extra_bin)\
	 djgpp_extras/cwsdpmi.exe djgpp_extras/emu387.dxe
	ln -s ../INSTALL.DOS dostxt_tmp/INSTALL.txt
	ln -s ../README dostxt_tmp/README.txt
	# -l means convert LF to CR+LF
	zip -j -l $(djgpp_zip_name) dostxt_tmp/*.txt $(extra_txt)
	$(RM) -rf dostxt_tmp

mingw : all mingw_iss

mingwexes :
	test -d mingw_exes || mkdir mingw_exes
	for f in $(base_progs) aven printwin ; do cp src/$$f.exe mingw_exes ; done

mingw_zip_name = survex-win32-@VERSION@.zip

mingw_zip : dostxt mingwexes
	$(RM) $(mingw_zip_name)
	zip -j $(mingw_zip_name) mingw_exes/*.exe $(extra_bin)
	# -l means convert LF to CR+LF
	zip -j -l $(mingw_zip_name) dostxt_tmp/*.txt $(extra_txt) survex.iss
	$(RM) -rf dostxt_tmp

mingw_iss : dostxt mingwexes
	$(RM) -rf iss_tmp
	mkdir iss_tmp
	cp dostxt_tmp/*.txt $(extra_txt) survex.iss doc/docindex.htm iss_tmp
	# convert LF to CR+LF
	perl -p -i -e 's/\n/\r\n/' iss_tmp/*
	cp mingw_extras/*.ico lib/icons/*.png iss_tmp
	cp /opt/rx-win32/bin/librx.dll iss_tmp
	mkdir iss_tmp/manual
	cp doc/manual/*.htm iss_tmp/manual
	# convert LF to CR+LF
	perl -p -i -e 's/\n/\r\n/' iss_tmp/manual/*
	cp mingw_exes/*.exe $(extra_bin) iss_tmp
	$(RM) -rf dostxt_tmp
	-killall Xvfb
	(Xvfb :9 -screen 0 64x32x8 &)
	DISPLAY=:9 wine "c:/Program Files/Inno Setup 2/ISCC.exe" iss_tmp/survex.iss
	mv iss_tmp/Output/*.exe .
	$(RM) -rf iss_tmp	
	killall Xvfb

bc_zip_name = survex-dos286-@VERSION@.zip
bc_exe_name = survex-dos286-@VERSION@.exe

borlandc_exe : borlandc_zip
	$(RM) $(bc_exe_name)~
	cat dos_extras/unzipsfx.exe $(bc_zip_name) > $(bc_exe_name)~
	zip -A $(bc_exe_name)~
	mv $(bc_exe_name)~ $(bc_exe_name)

borlandc_zip : dostxt
	$(RM) $(bc_zip_name)
	zip -j $(bc_zip_name) bc_exes/*.exe $(extra_bin) bc_extras/*.bgi
	ln -s ../INSTALL.DOS dostxt_tmp/INSTALL.txt
	ln -s ../README dostxt_tmp/README.txt
	# -l means convert LF to CR+LF
	zip -j -l $(bc_zip_name) dostxt_tmp/*.txt $(extra_txt)
	$(RM) -rf dostxt_tmp

riscos_zip_name = survex-riscos-@VERSION@.zip

riscos_zip :
	$(RM) $(riscos_zip_name)
	# needs a zip with Darren Salt's patches, which'll hopefully be merged
	# into the mainstream distribution in future
	$(RM) -rf riscos_tmp
	mkdir riscos_tmp
	ln -s ../README.ros riscos_tmp/\!ReadMe,fff
	# "cp -r riscos_extras riscos_tmp" but avoid CVS directories
	cd riscos_extras; tar --exclude CVS -cf - . | (cd ../riscos_tmp ; tar xf -)
	cp -r riscos_extras/\!* riscos_tmp
	for f in $(base_progs) caverot ; do\
	 ln -s ../../riscos_exes/$$f riscos_tmp/\!Cavern/$$f,ff8 ; done
	for f in $(extra_text) ; do\
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
	# -R doesn't seem to work here, so just glob it all in
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

MANUAL = manual manual/*.htm @DOCS_OTHER@

dos_doc_zip_name = svxdoc-doswin-@VERSION@.zip
dos_doc_exe_name = svxdoc-doswin-@VERSION@.exe

dos_doc_exe : dos_doc_zip
	$(RM) $(dos_doc_exe_name)~
	cat dos_extras/unzipsfx.exe $(dos_doc_zip_name) > $(dos_doc_exe_name)~
	zip -A $(dos_doc_exe_name)~
	mv $(dos_doc_exe_name)~ $(dos_doc_exe_name)

dos_doc_zip :
	$(RM) $(dos_doc_zip_name)
	# -l means convert LF to CR+LF
	cd doc && zip -l ../$(dos_doc_zip_name) $(MANUAL) docindex.htm

riscos_doc_zip_name = svxdoc-riscos-@VERSION@.zip

riscos_doc_zip :
	$(RM) $(riscos_doc_zip_name)
	$(RM) -rf riscos_doc_tmp
	mkdir riscos_doc_tmp
	cd doc && for f in $(MANUAL) docindex.htm ; do\
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
