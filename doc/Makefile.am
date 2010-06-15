## Process this file with automake to produce Makefile.in

pkgdocdir = @PKGDOCDIR@

other_mans = cad3d.1 cavern.1 diffpos.1 extend.1 sorterr.1 3dtopos.1 \
 aven.1 \
 print.ini.5 svxedit.1

manual_sources = manual.sgml cad3d.sgml cavern.sgml\
 diffpos.sgml extend.sgml sorterr.sgml 3dtopos.sgml aven.sgml\
 print.ini.sgml svxedit.sgml

manual_alt = manual.txt manual.pdf

# People can build from CVS without sgmltools v2 or v3 installed but they
# won't be able to "make dist", which is arguably a good thing - we don't
# want tarballs accidentally lacking the documentation.
EXTRA_DIST = manual $(HTMLFILES) $(manual_alt)\
 survex.1.in index.htm.in\
 custom.dsl survex.ent\
 caution.png note.png\
 $(manual_sources) $(other_mans)\
 man_3dtopos.sgml man_aven.sgml man_cad3d.sgml man_cavern.sgml\
 man_diffpos.sgml man_extend.sgml man_sorterr.sgml\
 man_print.ini.sgml man_svxedit.sgml news2html.pl\
 version.ent version.ent.in

NEWS.htm: ../NEWS news2html.pl
	./news2html.pl '@PRETTYPACKAGE@ @VERSION@' < ../NEWS > NEWS.htm

OLDNEWS.htm: ../OLDNEWS news2html.pl
	./news2html.pl < ../OLDNEWS > OLDNEWS.htm

MAINTAINERCLEANFILES = $(manual_alt) $(other_mans) NEWS.htm OLDNEWS.htm

pkgdoc_DATA = @HTMLFILES@ $(manual_alt)

man_MANS = survex.1 $(other_mans)

aven.1: $(srcdir)/aven.sgml $(srcdir)/man_aven.sgml $(srcdir)/survex.ent
	$(DOCBOOK_TO_MAN) $(srcdir)/man_aven.sgml > aven.1

cad3d.1: $(srcdir)/cad3d.sgml $(srcdir)/man_cad3d.sgml $(srcdir)/survex.ent
	$(DOCBOOK_TO_MAN) $(srcdir)/man_cad3d.sgml > cad3d.1

cavern.1: $(srcdir)/cavern.sgml $(srcdir)/man_cavern.sgml $(srcdir)/survex.ent
	$(DOCBOOK_TO_MAN) $(srcdir)/man_cavern.sgml > cavern.1

diffpos.1: $(srcdir)/diffpos.sgml $(srcdir)/man_diffpos.sgml $(srcdir)/survex.ent
	$(DOCBOOK_TO_MAN) $(srcdir)/man_diffpos.sgml > diffpos.1

extend.1: $(srcdir)/extend.sgml $(srcdir)/man_extend.sgml $(srcdir)/survex.ent
	$(DOCBOOK_TO_MAN) $(srcdir)/man_extend.sgml > extend.1

sorterr.1: $(srcdir)/sorterr.sgml $(srcdir)/man_sorterr.sgml $(srcdir)/survex.ent
	$(DOCBOOK_TO_MAN) $(srcdir)/man_sorterr.sgml > sorterr.1

3dtopos.1: $(srcdir)/3dtopos.sgml $(srcdir)/man_3dtopos.sgml $(srcdir)/survex.ent
	$(DOCBOOK_TO_MAN) $(srcdir)/man_3dtopos.sgml > 3dtopos.1

svxedit.1: $(srcdir)/svxedit.sgml $(srcdir)/man_svxedit.sgml $(srcdir)/survex.ent
	$(DOCBOOK_TO_MAN) $(srcdir)/man_svxedit.sgml > svxedit.1

print.ini.5: $(srcdir)/print.ini.sgml $(srcdir)/man_print.ini.sgml $(srcdir)/survex.ent
	$(DOCBOOK_TO_MAN) $(srcdir)/man_print.ini.sgml > print.ini.5

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

# manual, etc depend on configure.in so they get rebuilt when the
# version number is bumped

manual: $(manual_sources) custom.dsl survex.ent ../configure.in
	$(SGMLTOOLS) --backend=html --dsssl-spec='custom.dsl#html' manual.sgml
	cp caution.png note.png manual

manual.txt: $(manual_sources) survex.ent ../configure.in
	$(SGMLTOOLS) --backend=txt manual.sgml

manual.pdf: $(manual_sources) survex.ent ../configure.in
	$(SGMLTOOLS) --backend=ps manual.sgml
	ps2pdf manual.ps manual.pdf
	rm manual.ps

# The rules below aren't used by the build system - they're for "manual" use
# (ho ho)

manual.html: $(manual_sources) custom.dsl survex.ent ../configure.in
	$(SGMLTOOLS) --backend=onehtml \
		--dsssl-spec='custom.dsl#onehtml' manual.sgml

manual.rtf: $(manual_sources) survex.ent ../configure.in
	$(SGMLTOOLS) --backend=rtf manual.sgml

manual.ps: $(manual_sources) survex.ent ../configure.in
	$(SGMLTOOLS) --backend=ps manual.sgml

CLEANFILES = usr
