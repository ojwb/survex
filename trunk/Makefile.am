## Process this file with automake to produce Makefile.in

# We aim to be GNU compliant, but some required files are generated
AUTOMAKE_OPTIONS = 1.5 foreign

# These files are installed automatically in --gnu mode
GNU_FILES = INSTALL NEWS README COPYING AUTHORS ChangeLog

SUBDIRS = . lib src doc tests

# We never want to implicitly recurse into the vim subdirectory, but we still
# want to distribute the files there.
DIST_SUBDIRS = $(SUBDIRS) vim

EXTRA_DIST = $(GNU_FILES) TODO OLDNEWS\
 desc.txt desc-aven.txt survex.spec\
 buildmacosx.sh

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
	for f in /usr/share/locale/*/LC_MESSAGES/wxstd.mo ; do \
	 a=`echo "$$f"|sed 's!^/usr/share/locale/\([^/]*\).*!\1!'`; \
	 mkdir iss_tmp/$$a;\
	 cp "$$f" iss_tmp/$$a;\
	 echo 'Source: "'"$$a"'\wxstd.mo"; DestDir: "{app}\'"$$a"'"' >> iss_tmp/i18nfiles.iss;\
	done
	cp lib/*.isl iss_tmp
	: # Also need to reduce colour depth it appears:
	: # src/png2bmp lib/icons/aven_about.png iss_tmp/logo.bmp
	DISPLAY= wine "c:/Program Files/Inno Setup 5/ISCC.exe" iss_tmp/survex.iss
	mv iss_tmp/Output/*.exe .
	$(RM) -rf iss_tmp

.PHONY: mingw mingw_iss
