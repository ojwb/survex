## Process this file with automake to produce Makefile.in

EXTRA_DIST = OLDNEWS HACKING BUGS alien.pl ZOMBIES survex.iss.in

SUBDIRS = hto src lib doc tests debian

# seem to need this for "make distclean" to recurse?!?
DIST_SUBDIRS = $(SUBDIRS)

extra_bin = lib/*.msg lib/pfont.bit
extra_txt = lib/*.svx lib/print.ini
extra_files = $(extra_bin) $(extra_txt)
extra_text = BUGS AUTHORS COPYING NEWS TODO ChangeLog HACKING OLDNEWS ZOMBIES

base_progs = cad3d cavern printdm printps printpcl printhpgl diffpos extend survex

spec = ${PACKAGE}-${VERSION}.spec

RELEASE = 1

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
	rpm -ba --clean --rcfile /usr/lib/rpm/rpmrc:`pwd`/rpmrc ${spec}

debcl=deb/${PACKAGE}-${VERSION}/debian/changelog

debian : dist
	-rm -rf deb
	mkdir deb
	cd deb && tar zxvf ../${PACKAGE}-${VERSION}.tar.gz
	cp -a debian deb/${PACKAGE}-${VERSION}
	echo "${PACKAGE} (${VERSION}.${RELEASE}) unstable; urgency=low" > ${debcl}
	echo >> ${debcl}
	echo "  * Dummy changelog - see ChangeLog and NEWS." >> ${debcl}
	echo >> ${debcl}
	echo " -- Olly Betts <olly@survex.com>  "`date "+%a, %d %b %Y %X %z"` >> ${debcl}
	echo Source: survex > debian/control
	echo Section: math >> debian/control
	echo Priority: optional >> debian/control
	echo "Maintainer: Wookey <wookey@survex.com>" >> debian/control
	echo Standards-Version: 3.0.1 >> debian/control
	echo >> debian/control
	echo Package: ${PACKAGE} >> debian/control
	echo Architecture: any >> debian/control
	echo 'Depends: ${shlibs:Depends}' >> debian/control
	echo Description: Cave survey software >> debian/control
	sed 's/^/ /' < desc.txt >> debian/control
	echo >> debian/control
	echo Package: survex-aven >> debian/control
	echo Architecture: any >> debian/control
	echo 'Depends: ${shlibs:Depends}' >> debian/control
	echo Description: Aven viewer for Survex >> debian/control
	sed 's/^/ /' < desc-aven.txt >> debian/control	
	cd deb/${PACKAGE}-${VERSION} && debuild
	mv deb/*.deb .
	rm -rf deb
	rm debian/control

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

mingw : all mingw_zip

mingw_zip_name = survex-win32-@VERSION@.zip

mingw_zip : dostxt
	test -d mingw_exes || mkdir mingw_exes
	for f in $(base_progs) ; do cp src/$$f.exe mingw_exes ; done
	for a in hto2svx svx2hto ; do cp hto/$$a.exe mingw_exes ; done
	$(RM) $(mingw_zip_name)
	zip -j $(mingw_zip_name) mingw_exes/*.exe $(extra_bin)
	# -l means convert LF to CR+LF
	zip -j -l $(mingw_zip_name) mingw_extras/*.txt dostxt_tmp/*.txt $(extra_txt)
	$(RM) -rf dostxt_tmp

bc_zip_name = survex-dos286-@VERSION@.zip

borlandc_zip : dostxt
	$(RM) $(bc_zip_name)
	zip -j $(bc_zip_name) bc_exes/*.exe $(extra_bin) bc_extras/*.bgi
	# -l means convert LF to CR+LF
	zip -j -l $(bc_zip_name) bc_extras/*.txt dostxt_tmp/*.txt $(extra_txt)
	$(RM) -rf dostxt_tmp

riscos_zip_name = survex-riscos-@VERSION@.zip

riscos_zip :
	$(RM) $(riscos_zip_name)
	# needs a zip with Darren Salt's patches, which'll hopefully be merged
	# into the mainstream distribution in future
	$(RM) -rf riscos_tmp
	mkdir riscos_tmp
	cp -r riscos_extras/* riscos_tmp
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
	perl alien.pl src/*.c src/*.h hto/*.c hto/*.h > alien_tmp/\!ObeyMe,feb
	echo 'Dir <Obey$$Dir>' > alien_tmp/\!MakeMe,feb
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
	   *.ps) ln -s ../doc/$$f ../riscos_doc_tmp/$$f,ff5 ;;\
	   *) ln -s ../doc/$$f ../riscos_doc_tmp/$$f,ffd ;;\
	  esac;\
	 fi\
	done
	cd riscos_doc_tmp && ../zip-magic -N -r ../$(riscos_doc_zip_name) *
	$(RM) -rf riscos_doc_tmp
