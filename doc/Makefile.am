## Process this file with automake to produce Makefile.in

pkgdocdir = $(prefix)/doc/@PACKAGE@

doclist = @DOCS_OTHER@

other_mans = cad3d.1 cavern.1 diffpos.1 extend.1 sorterr.1 3dtopos.1 \
 printdm.1 printhpgl.1 printpcl.1 printps.1 @EXTRA_MANS@

#can produce man page syntax summary from --help something like
#SURVEXHOME=lib src/xcaverot --help | grep -i ^syntax |
#sed 's!^[^ ]* \(.*/\)\?!!;s!^!.B !;s! !
# !;s!\([A-Za-z0-9][A-Za-z0-9_]*\)!\\fI\1\\fR!g;s!\\fI!!;s!\\fR!!'

manual_sources = manual.sgml cad3d.sgml cavern.sgml caverot.sgml\
 diffpos.sgml extend.sgml sorterr.sgml 3dtopos.sgml aven.sgml xcaverot.sgml\
 printdm.sgml printhpgl.sgml printpcl.sgml printps.sgml printwin.sgml

# People can build from CVS without sgmltools v2 or v3 installed but they
# won't be able to "make dist", which is arguably a good thing
EXTRA_DIST = manual $(doclist) survex.1.in index.htm.in\
 index.htm GPL.htm TODO.htm ChngeLog.htm NEWS.htm OLDNEWS.htm\
 AUTHORS.htm HACKING.htm\
 custom.dsl survex.ent\
 caution.png note.png\
 $(manual_sources)\
 man_3dtopos.sgml man_aven.sgml man_cad3d.sgml man_cavern.sgml\
 man_diffpos.sgml man_extend.sgml man_sorterr.sgml man_xcaverot.sgml\
 man_printdm.sgml man_printhpgl.sgml man_printpcl.sgml man_printps.sgml

ChngeLog.htm: ../ChangeLog
	./cl2html.pl '@PRETTYPACKAGE@ @VERSION@' < ../ChangeLog > ChngeLog.htm

NEWS.htm: ../NEWS
	./news2html.pl '@PRETTYPACKAGE@ @VERSION@' < ../NEWS > NEWS.htm

OLDNEWS.htm: ../OLDNEWS
	./news2html.pl < ../OLDNEWS > OLDNEWS.htm

MAINTAINERCLEANFILES = $(doclist)
CLEANFILES = $(other_mans)

pkgdoc_DATA = index.htm GPL.htm TODO.htm ChngeLog.htm\
 NEWS.htm OLDNEWS.htm AUTHORS.htm HACKING.htm $(doclist)

man_MANS = survex.1 $(other_mans)

aven.1: aven.sgml man_aven.sgml survex.ent
	docbook-to-man man_aven.sgml > aven.1

cad3d.1: cad3d.sgml man_cad3d.sgml survex.ent
	docbook-to-man man_cad3d.sgml > cad3d.1

cavern.1: cavern.sgml man_cavern.sgml survex.ent
	docbook-to-man man_cavern.sgml > cavern.1

caverot.1: caverot.sgml man_caverot.sgml survex.ent
	docbook-to-man man_caverot.sgml > caverot.1

diffpos.1: diffpos.sgml man_diffpos.sgml survex.ent
	docbook-to-man man_diffpos.sgml > diffpos.1

extend.1: extend.sgml man_extend.sgml survex.ent
	docbook-to-man man_extend.sgml > extend.1

printdm.1: printdm.sgml man_printdm.sgml survex.ent
	docbook-to-man man_printdm.sgml > printdm.1

printhpgl.1: printhpgl.sgml man_printhpgl.sgml survex.ent
	docbook-to-man man_printhpgl.sgml > printhpgl.1

printpcl.1: printpcl.sgml man_printpcl.sgml survex.ent
	docbook-to-man man_printpcl.sgml > printpcl.1

printps.1: printps.sgml man_printps.sgml survex.ent
	docbook-to-man man_printps.sgml > printps.1

sorterr.1: sorterr.sgml man_sorterr.sgml survex.ent
	docbook-to-man man_sorterr.sgml > sorterr.1

3dtopos.1: 3dtopos.sgml man_3dtopos.sgml survex.ent
	docbook-to-man man_3dtopos.sgml > 3dtopos.1

xcaverot.1: xcaverot.sgml man_xcaverot.sgml survex.ent
	docbook-to-man man_xcaverot.sgml > xcaverot.1

install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(pkgdocdir)/manual; \
	if test -d manual ; then \
	  for p in manual/*.htm manual/*.png ; do \
	    f="`echo $$p | sed -e 's|^.*/|manual/|'`"; \
	    echo " $(INSTALL_DATA) $$p $(DESTDIR)$(pkgdocdir)/$$f"; \
	    $(INSTALL_DATA) $$p $(DESTDIR)$(pkgdocdir)/$$f; \
	  done \
	elif test -d "$(srcdir)/manual" ; then \
	  for p in $(srcdir)/manual/*.htm $(srcdir)/manual/*.png ; do \
	    f="`echo $$p | sed -e 's|^.*/|manual/|'`"; \
	    echo " $(INSTALL_DATA) $$p $(DESTDIR)$(pkgdocdir)/$$f"; \
	    $(INSTALL_DATA) $$p $(DESTDIR)$(pkgdocdir)/$$f; \
	  done \
	fi

uninstall-local:
	if test -d manual ; then \
	  for p in manual/*.htm manual/*.png ; do \
	    f="`echo $$p | sed -e 's|^.*/|manual/|'`"; \
	    echo " rm -f $(DESTDIR)$(pkgdocdir)/$$f"; \
	    rm -f $(DESTDIR)$(pkgdocdir)/$$f; \
	  done \
	elif test -d "$(srcdir)/manual" ; then \
	  for p in $(srcdir)/manual/*.htm $(srcdir)/manual/*.png ; do \
	    f="`echo $$p | sed -e 's|^.*/|manual/|'`"; \
	    echo " rm -f $(DESTDIR)$(pkgdocdir)/$$f"; \
	    rm -f $(DESTDIR)$(pkgdocdir)/$$f; \
	  done \
	fi
	-rmdir $(DESTDIR)$(pkgdocdir)/manual

manual: $(manual_sources) custom.dsl survex.ent
	@sgmltools@ --backend=html --dsssl-spec=custom.dsl#html manual.sgml
	cp caution.png note.png manual

manual.html: $(manual_sources) custom.dsl survex.ent
	@sgmltools@ --backend=onehtml --dsssl-spec=custom.dsl#onehtml manual.sgml

manual.rtf: $(manual_sources) survex.ent
	@sgmltools@ --backend=rtf manual.sgml

manual.txt: $(manual_sources) survex.ent
	@sgmltools@ --backend=txt manual.sgml

manual.ps: $(manual_sources) survex.ent
	@sgmltools@ --backend=ps manual.sgml
