## Process this file with automake to produce Makefile.in

pkgdocdir = @PKGDOCDIR@

doclist = @DOCS_OTHER@

other_mans = cad3d.1 cavern.1 diffpos.1 extend.1 sorterr.1 3dtopos.1 \
 printdm.1 printhpgl.1 printpcl.1 printps.1 print.ini.5 @EXTRA_MANS@

manual_sources = manual.sgml cad3d.sgml cavern.sgml caverot.sgml\
 diffpos.sgml extend.sgml sorterr.sgml 3dtopos.sgml aven.sgml xcaverot.sgml\
 printdm.sgml printhpgl.sgml printpcl.sgml printps.sgml printwin.sgml\
 print.ini.sgml

# People can build from CVS without sgmltools v2 or v3 installed but they
# won't be able to "make dist", which is arguably a good thing - we don't
# want tarballs accidentally lacking the documentation.
EXTRA_DIST = manual $(doclist) survex.1.in index.htm.in\
 index.htm GPL.htm TODO.htm ChngeLog.htm NEWS.htm OLDNEWS.htm\
 AUTHORS.htm HACKING.htm\
 custom.dsl survex.ent\
 caution.png note.png\
 $(manual_sources) $(other_mans)\
 man_3dtopos.sgml man_aven.sgml man_cad3d.sgml man_cavern.sgml\
 man_diffpos.sgml man_extend.sgml man_sorterr.sgml man_xcaverot.sgml\
 man_printdm.sgml man_printhpgl.sgml man_printpcl.sgml man_printps.sgml\
 man_print.ini.sgml cl2html.pl news2html.pl version.ent version.ent.in

ChngeLog.htm: ../ChangeLog
	./cl2html.pl '@PRETTYPACKAGE@ @VERSION@' < ../ChangeLog > ChngeLog.htm

NEWS.htm: ../NEWS
	./news2html.pl '@PRETTYPACKAGE@ @VERSION@' < ../NEWS > NEWS.htm

OLDNEWS.htm: ../OLDNEWS
	./news2html.pl < ../OLDNEWS > OLDNEWS.htm

MAINTAINERCLEANFILES = $(doclist) $(other_mans)\
 ChngeLog.htm NEWS.htm OLDNEWS.htm

pkgdoc_DATA = index.htm GPL.htm TODO.htm ChngeLog.htm\
 NEWS.htm OLDNEWS.htm AUTHORS.htm HACKING.htm $(doclist)

man_MANS = survex.1 $(other_mans)

aven.1: $(srcdir)/aven.sgml $(srcdir)/man_aven.sgml $(srcdir)/survex.ent
	docbook-to-man $(srcdir)/man_aven.sgml > aven.1

cad3d.1: $(srcdir)/cad3d.sgml $(srcdir)/man_cad3d.sgml $(srcdir)/survex.ent
	docbook-to-man $(srcdir)/man_cad3d.sgml > cad3d.1

cavern.1: $(srcdir)/cavern.sgml $(srcdir)/man_cavern.sgml $(srcdir)/survex.ent
	docbook-to-man $(srcdir)/man_cavern.sgml > cavern.1

caverot.1: $(srcdir)/caverot.sgml $(srcdir)/man_caverot.sgml $(srcdir)/survex.ent
	docbook-to-man $(srcdir)/man_caverot.sgml > caverot.1

diffpos.1: $(srcdir)/diffpos.sgml $(srcdir)/man_diffpos.sgml $(srcdir)/survex.ent
	docbook-to-man $(srcdir)/man_diffpos.sgml > diffpos.1

extend.1: $(srcdir)/extend.sgml $(srcdir)/man_extend.sgml $(srcdir)/survex.ent
	docbook-to-man $(srcdir)/man_extend.sgml > extend.1

printdm.1: $(srcdir)/printdm.sgml $(srcdir)/man_printdm.sgml $(srcdir)/survex.ent
	docbook-to-man $(srcdir)/man_printdm.sgml > printdm.1

printhpgl.1: $(srcdir)/printhpgl.sgml $(srcdir)/man_printhpgl.sgml $(srcdir)/survex.ent
	docbook-to-man $(srcdir)/man_printhpgl.sgml > printhpgl.1

printpcl.1: $(srcdir)/printpcl.sgml $(srcdir)/man_printpcl.sgml $(srcdir)/survex.ent
	docbook-to-man $(srcdir)/man_printpcl.sgml > printpcl.1

printps.1: $(srcdir)/printps.sgml $(srcdir)/man_printps.sgml $(srcdir)/survex.ent
	docbook-to-man $(srcdir)/man_printps.sgml > printps.1

sorterr.1: $(srcdir)/sorterr.sgml $(srcdir)/man_sorterr.sgml $(srcdir)/survex.ent
	docbook-to-man $(srcdir)/man_sorterr.sgml > sorterr.1

3dtopos.1: $(srcdir)/3dtopos.sgml $(srcdir)/man_3dtopos.sgml $(srcdir)/survex.ent
	docbook-to-man $(srcdir)/man_3dtopos.sgml > 3dtopos.1

xcaverot.1: $(srcdir)/xcaverot.sgml $(srcdir)/man_xcaverot.sgml $(srcdir)/survex.ent
	docbook-to-man $(srcdir)/man_xcaverot.sgml > xcaverot.1

print.ini.5: $(srcdir)/print.ini.sgml $(srcdir)/man_print.ini.sgml $(srcdir)/survex.ent
	docbook-to-man $(srcdir)/man_print.ini.sgml > print.ini.5

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

distclean-local:
	-test . != "$(srcdir)" && rm -f manual.sgml

# manual depends on config.h so it gets rebuilt when the version number is
# bumped

manual: $(manual_sources) custom.dsl survex.ent ../config.h
	-test . != "$(srcdir)" && ln -s $(srcdir)/manual.sgml manual.sgml
	@sgmltools@ --backend=html --dsssl-spec='$(srcdir)/custom.dsl#html' \
		manual.sgml
	cp $(srcdir)/caution.png $(srcdir)/note.png manual

# this rule is not used by the build system - it's for "manual" use (ho ho)
manual.html: $(manual_sources) custom.dsl survex.ent ../config.h
	-test . != "$(srcdir)" && ln -s $(srcdir)/manual.sgml manual.sgml
	@sgmltools@ --backend=onehtml \
	   --dsssl-spec='$(srcdir)/custom.dsl#onehtml' manual.sgml

manual.rtf: $(manual_sources) survex.ent ../config.h
	-test . != "$(srcdir)" && ln -s $(srcdir)/manual.sgml manual.sgml
	@sgmltools@ --backend=rtf manual.sgml

manual.txt: $(manual_sources) survex.ent ../config.h
	-test . != "$(srcdir)" && ln -s $(srcdir)/manual.sgml manual.sgml
	@sgmltools@ --backend=txt manual.sgml

manual.ps: $(manual_sources) survex.ent ../config.h
	-test . != "$(srcdir)" && ln -s $(srcdir)/manual.sgml manual.sgml
	@sgmltools@ --backend=ps manual.sgml
