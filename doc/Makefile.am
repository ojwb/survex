## Process this file with automake to produce Makefile.in

pkgdocdir = $(prefix)/doc/@PACKAGE@

doclist = @DOCS_OTHER@

other_mans = cad3d.1 cavern.1 diffpos.1 extend.1 hto2svx.1 \
 printdm.1 printhpgl.1 printpcl.1 printps.1 svx2hto.1 @EXTRA_MANS@

#can produce man page syntax summary from --help something like
#SURVEXHOME=lib src/xcaverot --help | grep -i ^syntax |
#sed 's!^[^ ]* \(.*/\)\?!!;s!^!.B !;s! !
# !;s!\([A-Za-z0-9][A-Za-z0-9_]*\)!\\fI\1\\fR!g;s!\\fI!!;s!\\fR!!'

# People can build from CVS without sgmltools v2 or v3 installed but they
# won't be able to "make dist", which is arguably a good thing
EXTRA_DIST = svxdocs $(doclist) svxdocs.sgml custom.dsl survex.1\
 docindex.htm.in

MAINTAINERCLEANFILES = $(doclist)
CLEANFILES = $(other_mans)

pkgdoc_DATA = $(doclist)

man_MANS = survex.1 $(other_mans)

$(other_mans):
	ln -s $(srcdir)/survex.1 $@

install-data-local:
	if test -d "$(srcdir)/svxdocs" ; then \
	  for p in `find "$(srcdir)/svxdocs" -name *.htm -type f -print` ; do \
	    f="`echo $$p | sed -e 's|^.*/||'`"; \
	    echo " $(INSTALL_DATA) $$p $(DESTDIR)$(pkgdocdir)/$$f"; \
	    $(INSTALL_DATA) $$p $(DESTDIR)$(pkgdocdir)/$$f; \
	  done \
	fi
	if test -d svxdocs ; then \
	  for p in `find svxdocs -name *.htm -type f -print` ; do \
	    f="`echo $$p | sed -e 's|^.*/||'`"; \
	    echo " $(INSTALL_DATA) $$p $(DESTDIR)$(pkgdocdir)/$$f"; \
	    $(INSTALL_DATA) $$p $(DESTDIR)$(pkgdocdir)/$$f; \
	  done \
	fi

uninstall-local:
	if test -d "$(srcdir)/svxdocs" ; then \
	  for p in `find "$(srcdir)/svxdocs" -name *.htm -type f -print` ; do \
	    f="`echo $$p | sed -e 's|^.*/||'`"; \
	    echo " rm -f $(DESTDIR)$(pkgdocdir)/$$f"; \
	    rm -f $(DESTDIR)$(pkgdocdir)/$$f; \
	  done \
	fi
	if test -d svxdocs ; then \
	  for p in `find svxdocs -name *.htm -type f -print` ; do \
	    f="`echo $$p | sed -e 's|^.*/||'`"; \
	    echo " rm -f $(DESTDIR)$(pkgdocdir)/$$f"; \
	    rm -f $(DESTDIR)$(pkgdocdir)/$$f; \
	  done \
	fi

svxdocs: svxdocs.sgml custom.dsl
	@sgmltools@ --backend=html --dsssl-spec=custom.dsl#html svxdocs.sgml

svxdocs.htm: svxdocs.sgml custom.dsl
	@sgmltools@ --backend=onehtml --dsssl-spec=custom.dsl#onehtml svxdocs.sgml
	mv -f svxdocs.html svxdocs.htm

svxdocs.rtf: svxdocs.sgml
	@sgmltools@ --backend=rtf svxdocs.sgml

svxdocs.txt: svxdocs.sgml
	@sgmltools@ --backend=txt svxdocs.sgml

svxdocs.ps: svxdocs.sgml
	@sgmltools@ --backend=ps svxdocs.sgml
