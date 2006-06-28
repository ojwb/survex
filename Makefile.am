## Process this file with automake to produce Makefile.in

# We aim to be GNU compliant, but some required files are generated
AUTOMAKE_OPTIONS = 1.5 foreign

# These files are installed automatically in --gnu mode
GNU_FILES = INSTALL NEWS README COPYING AUTHORS ChangeLog

EXTRA_DIST = $(GNU_FILES) TODO survex.iss.in OLDNEWS\
 desc.txt desc-aven.txt survex.spec.in survex.spec\
 buildmacosx.sh vim/syntax/survex.vim vim/ftdetect/survex.vim

SUBDIRS = . lib src doc tests debian

# seem to need this for "make distclean" to recurse?!?
DIST_SUBDIRS = $(SUBDIRS)

extra_bin = lib/*.msg\
 lib/aven.txf
extra_txt = lib/*.svx lib/print.ini
extra_files = $(extra_bin) $(extra_txt)

# FIXME: need to keep in step with src/Makefile.am
base_progs = cad3d cavern diffpos extend sorterr 3dtopos

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
	echo macrofiles : `rpmbuild --showrc | sed 's/^macrofiles\> *: //p;d'`:`pwd`/rpmmacros > rpmrc
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
	cd deb/${PACKAGE}-${VERSION} && DEBUILD_TGZ_CHECK=no debuild -us -uc
	mv deb/*.deb deb/*.dsc deb/*.changes deb/*.gz .
	rm -rf deb

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
	for f in $(base_progs) aven editwrap ; do cp src/$$f.exe iss_tmp ; done
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
	: # NB check for updates for each new wx version
	: # also currently available are:
	: # af da el eu fi hi hu id ja lv nb nl pl ru sl sv tr uk zh_CN zh_TW
	for a in ca cs de es fr it pt_BR ; do\
	 mkdir iss_tmp/$$a;\
	 cp /usr/i586-mingw32msvc/share/locale/$$a/LC_MESSAGES/wxstd.mo iss_tmp/$$a;\
	 echo 'Source: "'"$$a"'\wxstd.mo"; DestDir: "{app}\'"$$a"'"' >> iss_tmp/i18nfiles.iss;\
	done
	: # Also need to reduce colour depth it appears:
	: # src/png2bmp lib/icons/aven_about.png iss_tmp/logo.bmp
	echo 'wine "c:/Program Files/Inno Setup 5/ISCC.exe" iss_tmp/survex.iss' > iss_tmp/runissinwine
	chmod 755 iss_tmp/runissinwine
	./xvfb-run -a --server-args="-screen 0 64x32x8" iss_tmp/runissinwine
	mv iss_tmp/Output/*.exe .
	$(RM) -rf iss_tmp

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
