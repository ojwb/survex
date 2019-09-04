## Process this file with automake to produce Makefile.in

ACLOCAL_AMFLAGS = -I m4

SUBDIRS = . lib src doc tests

# We never want to implicitly recurse into the vim subdirectory, but we still
# want to distribute the files there.
DIST_SUBDIRS = $(SUBDIRS) vim

EXTRA_DIST = TODO OLDNEWS\
 desc.txt desc-aven.txt survex.spec

extra_bin = lib/*.msg\
 lib/unifont.pixelfont
extra_txt = lib/*.svx
extra_files = $(extra_bin) $(extra_txt)

# FIXME: need to keep in step with bin_PROGRAMS in src/Makefile.am
mingw_progs = cavern diffpos dump3d extend sorterr survexport aven

AUTHORS: doc/AUTHORS.htm
	w3m -dump doc/AUTHORS.htm > AUTHORS

HACKING: doc/HACKING.htm
	w3m -dump doc/HACKING.htm > HACKING

TODO: doc/TODO.htm
	w3m -dump doc/TODO.htm > TODO

# Create Aven.app for macOS - run as e.g.:
# make create-aven-app APP_PATH=Aven.app
create-aven-app:
	mkdir -p '$(APP_PATH)/Contents/MacOS' '$(APP_PATH)/Contents/Resources'
	cp lib/Info.plist '$(APP_PATH)/Contents'
	printf APPLAVEN > '$(APP_PATH)/Contents/PkgInfo'
	for zip in lib/icons/*.iconset.zip ; do \
	  set -e; \
	  unzip -d '$(APP_PATH)/Contents/Resources' "$$zip"; \
	  i=`echo "$$zip"|sed 's!.*/\(.*\)\.zip$$!\1!'`; \
	  iconutil --convert icns '$(APP_PATH)/Contents/Resources/'"$$i"; \
	  rm -rf '$(APP_PATH)/Contents/Resources/'"$$i"; \
	done

mingw : all mingw_iss

mingw_iss : survex.iss
	$(RM) -rf iss_tmp
	mkdir iss_tmp
	cp $(extra_txt) survex.iss iss_tmp
	: # so the installer can display the license
	cp COPYING iss_tmp/COPYING.txt
	cd doc && cp @HTMLFILES@ ../iss_tmp
	: # convert LF to CR+LF
	perl -p -i -e 's/\n/\r\n/' iss_tmp/*
	for f in $(mingw_progs) ; do cp src/$$f.exe iss_tmp ; done
	cp lib/icons/*.ico lib/images/*.png iss_tmp
	: # not needed if we build wx without threads: gzip -dc /usr/share/doc/mingw32-runtime/mingwm10.dll.gz > iss_tmp/mingwm10.dll
	mkdir iss_tmp/manual
	cp doc/manual/*.htm iss_tmp/manual
	: # convert LF to CR+LF
	perl -p -i -e 's/\n/\r\n/' iss_tmp/manual/*
	cp doc/manual/*.png iss_tmp/manual
	cp $(extra_bin) iss_tmp
	for f in "`$(WX_CONFIG) --prefix`"/share/locale/*/LC_MESSAGES/wxstd.mo ; do \
	  a=`echo "$$f"|sed 's!^.*/\([^/]*\)/LC_MESSAGES/wxstd\.mo$$!\1!'`; \
	  mkdir iss_tmp/$$a; \
	  cp "$$f" iss_tmp/$$a; \
	  echo 'Source: "'"$$a"'\wxstd.mo"; DestDir: "{app}\'"$$a"'"' ; \
	done > iss_tmp/i18nfiles.iss
	cp lib/*.isl iss_tmp
	mkdir iss_tmp/proj
	cp -p /usr/${host}/share/proj/epsg lib/proj/esri iss_tmp/proj
	: # Also need to reduce colour depth it appears:
	: # src/png2bmp lib/icons/aven_about.png iss_tmp/logo.bmp
	DISPLAY= wine "c:/Program Files/Inno Setup 5/ISCC.exe" iss_tmp/survex.iss
	mv iss_tmp/Output/*.exe .
	$(RM) -rf iss_tmp

.PHONY: mingw mingw_iss
