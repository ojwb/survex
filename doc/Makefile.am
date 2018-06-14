## Process this file with automake to produce Makefile.in

pkgdocdir = @PKGDOCDIR@

other_mans = cavern.1 diffpos.1 dump3d.1 extend.1 sorterr.1 survexport.1 \
 aven.1

manual_sources = manual.sgml cavern.sgml\
 diffpos.sgml dump3d.sgml extend.sgml sorterr.sgml survexport.sgml aven.sgml

manual_alt = manual.txt manual.pdf

# People can build from CVS without sgmltools v2 or v3 installed but they
# won't be able to "make dist", which is arguably a good thing - we don't
# want tarballs accidentally lacking the documentation.
EXTRA_DIST = $(HTMLFILES) manual $(manual_alt)\
 custom.dsl survex.ent\
 caution.png note.png\
 $(manual_sources) $(other_mans)\
 man_aven.sgml man_cavern.sgml man_diffpos.sgml man_dump3d.sgml \
 man_extend.sgml man_sorterr.sgml man_survexport.sgml\
 news2html.pl

NEWS.htm: ../NEWS news2html.pl
	./news2html.pl '@PRETTYPACKAGE@ @VERSION@' < ../NEWS > NEWS.htm

OLDNEWS.htm: ../OLDNEWS news2html.pl
	./news2html.pl < ../OLDNEWS > OLDNEWS.htm

MAINTAINERCLEANFILES = $(manual_alt) $(other_mans) NEWS.htm OLDNEWS.htm

pkgdoc_DATA = @HTMLFILES@ $(manual_alt)

man_MANS = survex.7 $(other_mans)

aven.1: $(srcdir)/aven.sgml $(srcdir)/man_aven.sgml $(srcdir)/survex.ent
	$(DOCBOOK2MAN) $(srcdir)/man_aven.sgml
	rm -f manpage.links manpage.refs

survexport.1: $(srcdir)/survexport.sgml $(srcdir)/man_survexport.sgml $(srcdir)/survex.ent
	$(DOCBOOK2MAN) $(srcdir)/man_survexport.sgml
	rm -f manpage.links manpage.refs

cavern.1: $(srcdir)/cavern.sgml $(srcdir)/man_cavern.sgml $(srcdir)/survex.ent
	$(DOCBOOK2MAN) $(srcdir)/man_cavern.sgml
	rm -f manpage.links manpage.refs

diffpos.1: $(srcdir)/diffpos.sgml $(srcdir)/man_diffpos.sgml $(srcdir)/survex.ent
	$(DOCBOOK2MAN) $(srcdir)/man_diffpos.sgml
	rm -f manpage.links manpage.refs

dump3d.1: $(srcdir)/dump3d.sgml $(srcdir)/man_dump3d.sgml $(srcdir)/survex.ent
	$(DOCBOOK2MAN) $(srcdir)/man_dump3d.sgml
	rm -f manpage.links manpage.refs

extend.1: $(srcdir)/extend.sgml $(srcdir)/man_extend.sgml $(srcdir)/survex.ent
	$(DOCBOOK2MAN) $(srcdir)/man_extend.sgml
	rm -f manpage.links manpage.refs

sorterr.1: $(srcdir)/sorterr.sgml $(srcdir)/man_sorterr.sgml $(srcdir)/survex.ent
	$(DOCBOOK2MAN) $(srcdir)/man_sorterr.sgml
	rm -f manpage.links manpage.refs

install-data-local: manual
	$(mkinstalldirs) $(DESTDIR)$(pkgdocdir)/manual
	test ! -d manual -a -d "$(srcdir)/manual" && cd $(srcdir) ; \
	for p in manual/*.htm manual/*.png ; do \
	  echo " $(INSTALL_DATA) $$p $(DESTDIR)$(pkgdocdir)/$$p"; \
	  $(INSTALL_DATA) $$p $(DESTDIR)$(pkgdocdir)/$$p; \
	done

uninstall-local:
	test ! -d manual -a -d "$(srcdir)/manual" && cd $(srcdir) ; \
	for p in manual/*.htm manual/*.png ; do \
	  echo " rm -f $(DESTDIR)$(pkgdocdir)/$$p"; \
	  rm -f $(DESTDIR)$(pkgdocdir)/$$p; \
	done
	-rmdir $(DESTDIR)$(pkgdocdir)/manual

maintainer-clean-local:
	rm -rf manual "$(srcdir)/manual"

# manual, etc depend on configure.ac so they get rebuilt when the
# version number is bumped

manual: manual/stampfile
manual/stampfile: $(manual_sources) custom.dsl survex.ent ../configure.ac
	$(SGMLTOOLS) --backend=html --dsssl-spec='custom.dsl#html' manual.sgml
	cp caution.png note.png manual
	touch manual/stampfile

manual.txt: $(manual_sources) survex.ent ../configure.ac
	$(SGMLTOOLS) --backend=txt manual.sgml

manual.pdf: $(manual_sources) survex.ent ../configure.ac
	$(SGMLTOOLS) --backend=ps manual.sgml
	ps2pdf manual.ps manual.pdf
	rm manual.ps

# The rules below aren't used by the build system - they're for "manual" use
# (ho ho)

manual.html: $(manual_sources) custom.dsl survex.ent ../configure.ac
	$(SGMLTOOLS) --backend=onehtml \
		--dsssl-spec='custom.dsl#onehtml' manual.sgml

manual.rtf: $(manual_sources) survex.ent ../configure.ac
	$(SGMLTOOLS) --backend=rtf manual.sgml

manual.ps: $(manual_sources) survex.ent ../configure.ac
	$(SGMLTOOLS) --backend=ps manual.sgml

CLEANFILES = usr

check-local:
	grep -F '<!--VERSION-->$(VERSION)' '$(srcdir)/manual.sgml'
