## Process this file with automake to produce Makefile.in

ACLOCAL_AMFLAGS = -I m4

SUBDIRS = . lib src doc tests

# We never want to implicitly recurse into the vim subdirectory, but we still
# want to distribute the files there.
DIST_SUBDIRS = $(SUBDIRS) vim

EXTRA_DIST = TODO OLDNEWS\
 desc.txt desc-aven.txt survex.spec

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

RUN_EXE = DISPLAY= wine

PROJ_PREFIX = /usr/i686-w64-mingw32

mingw_iss : survex.iss
	$(RM) -rf iss_tmp
	mkdir iss_tmp
	$(MAKE) install-strip DESTDIR=$(PWD)/iss_tmp
	:
	cp survex.iss iss_tmp
	: # so the installer can display the license
	cp COPYING iss_tmp/COPYING.txt
	mv iss_tmp$(prefix)/share/doc/survex/*.htm iss_tmp
	:
	: # This is identical to survexport.exe - we install under two names
	: # via the installer so don't need to ship both.
	rm -f iss_tmp$(prefix)/bin/3dtopos.exe
	mv iss_tmp$(prefix)/bin/* iss_tmp
	mv iss_tmp$(prefix)/share/survex/images/* iss_tmp
	rmdir iss_tmp$(prefix)/share/survex/images
	mv iss_tmp$(prefix)/share/survex/* iss_tmp
	:
	cp lib/icons/*.ico iss_tmp
	: # not needed if we build wx without threads: gzip -dc /usr/share/doc/mingw32-runtime/mingwm10.dll.gz > iss_tmp/mingwm10.dll
	mv iss_tmp$(prefix)/share/doc/survex/manual iss_tmp
	rm -rf iss_tmp/usr
	p=$(PWD) ; cd "`$(WX_CONFIG) --prefix`/share/locale" && for d in * ; do \
	  f=$$d/LC_MESSAGES/wxstd.mo ; \
	  if [ -f "$$f" ] ; then \
	    mkdir "$$p/$$d" ; \
	    cp "$$f" "$$p/$$d" ; \
	    printf 'Source: "%s\\wxstd.mo"; DestDir: "{app}\\%s"\n' "$$d" "$$d" ; \
	  fi ; \
	done > $$p/iss_tmp/i18nfiles.iss
	cp lib/*.isl iss_tmp
	: # Also need to reduce colour depth it appears:
	: # src/png2bmp lib/icons/aven_about.png iss_tmp/logo.bmp
	cp -a $(PROJ_PREFIX)/share/proj iss_tmp/proj
	: # Include required non-system DLLs.
	[ -z "$(NTLDD)" ] || cp -a $$($(NTLDD) --recursive src/aven.exe 2>/dev/null|sed -n 's!\\!/!g;s!.*msys64\(/ucrt64/bin/[^ ]*\).*!\1!p') iss_tmp
	: # libpodofo triggers openssl to dynamically load its legacy.dll at startup.
	: # Looks like this may get fixed in libpodofo 1.0
	cp /ucrt64/lib/ossl-modules/legacy.dll iss_tmp
	$(STRIP) iss_tmp/*.dll
	$(RUN_EXE) "c:/Program Files (x86)/Inno Setup 6/ISCC.exe" iss_tmp/survex.iss
	mv iss_tmp/Output/*.exe .
	$(RM) -rf iss_tmp

.PHONY: mingw mingw_iss
