## Process this file with automake to produce Makefile.in

pkgdocdir = $(prefix)/doc/@PACKAGE@

# FIXME: need to sort out an "all in one" HTML version
# FIXME: this is almost duplicated in ../Makefile.am
doclist = svxdocs.rtf svxdocs.txt svxdocs.ps # svxdocs.html

other_mans = 3dtodxf.1 cavern.1 diffpos.1 extend.1 hto2svx.1 \
 printdm.1 printhpgl.1 printpcl.1 printps.1 svx2hto.1

# People can build from CVS without sgmltools v2 or v3 installed but they
# won't be able to "make dist", which is arguably a good thing
EXTRA_DIST = svxdocs $(doclist) svxdocs.sgml custom.dsl survex.1

MAINTAINERCLEANFILES = $(doclist)
CLEANFILES = $(other_mans)

pkgdoc_DATA = $(doclist)

man_MANS = survex.1 $(other_mans)

$(other_mans):
	ln -s $(srcdir)/survex.1 $@

install-data-local:
	if test -d "$$(srcdir)/svxdocs" ; then \
	  for p in `find "$$(srcdir)/svxdocs" -name *.htm -type f -print` ; do \
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
	if test -d "$$(srcdir)/svxdocs" ; then \
	  for p in `find "$$(srcdir)/svxdocs" -name *.htm -type f -print` ; do \
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

svxdocs.html: svxdocs.sgml custom.dsl
	@sgmltools@ --backend=html --dsssl-spec=custom.dsl#onepage svxdocs.sgml

svxdocs.rtf: svxdocs.sgml
	@sgmltools@ --backend=rtf svxdocs.sgml

svxdocs.txt: svxdocs.sgml
	@sgmltools@ --backend=txt svxdocs.sgml

svxdocs.ps: svxdocs.sgml
	@sgmltools@ --backend=ps svxdocs.sgml
