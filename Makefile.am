## Process this file with automake to produce Makefile.in

# We aim to be GNU compliant, but some required files are generated
AUTOMAKE_OPTIONS = 1.5 foreign

# These files are installed automatically in --gnu mode
GNU_FILES = INSTALL NEWS README COPYING AUTHORS ChangeLog

EXTRA_DIST = $(GNU_FILES) TODO survex.iss.in OLDNEWS\
 desc.txt desc-aven.txt survex.spec.in survex.spec

SUBDIRS = . lib src doc tests debian

# seem to need this for "make distclean" to recurse?!?
DIST_SUBDIRS = $(SUBDIRS)

extra_bin = lib/*.msg lib/default.bit lib/bold.bit
extra_txt = lib/*.svx lib/print.ini
extra_files = $(extra_bin) $(extra_txt)

# FIXME: need to keep in step with src/Makefile.am
base_progs = cad3d cavern printdm printps printpcl printhpgl diffpos\
 extend sorterr 3dtopos

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
	rpm -bs --clean --rcfile /usr/lib/rpm/rpmrc:`pwd`/rpmrc survex.spec

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

mingw : all mingw_iss

mingw_iss :
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
	cp mingw_extras/librx.dll iss_tmp
	cp mingw_extras/glut32.dll iss_tmp
	gzip -dc /usr/share/doc/mingw32-runtime/mingwm10.dll.gz > iss_tmp/mingwm10.dll
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
	-killall Xvfb
	(Xvfb :9 -screen 0 64x32x8 &)
	DISPLAY=:9 wine "c:/Program Files/Inno Setup 2/ISCC.exe" iss_tmp/survex.iss
	mv iss_tmp/Output/*.exe .
	$(RM) -rf iss_tmp
	killall Xvfb

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
