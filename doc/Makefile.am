## Process this file with automake to produce Makefile.in

pkgdocdir = @PKGDOCDIR@

man_sources = \
    aven.rst \
    cavern.rst \
    diffpos.rst \
    dump3d.rst \
    extend.rst \
    sorterr.rst \
    survexport.rst \
    survex.rst

manual_sources = \
    index.rst \
    intro.rst \
    getstart.rst \
    cmdline.rst \
    $(man_sources) \
    datafile.rst \
    svxhowto.rst \
    genhowto.rst \
    compass.rst \
    maillist.rst \
    walls.rst \
    cmap.rst \
    future.rst

manual_alt = manual.pdf

HTMLFILES= AUTHORS.htm HACKING.htm NEWS.htm OLDNEWS.htm TODO.htm index.htm \
 3dformat.htm 3dformat-old.htm

NEWS.htm: ../NEWS news2html.pl
	./news2html.pl '@PRETTYPACKAGE@ @VERSION@' < ../NEWS > NEWS.htm

OLDNEWS.htm: ../OLDNEWS news2html.pl
	./news2html.pl < ../OLDNEWS > OLDNEWS.htm

MAINTAINERCLEANFILES = $(manual_alt) NEWS.htm OLDNEWS.htm

pkgdoc_DATA = $(HTMLFILES) $(manual_alt)

man_MANS = \
    aven.1 \
    cavern.1 \
    diffpos.1 \
    dump3d.1 \
    extend.1 \
    sorterr.1 \
    survexport.1 \
    survex.7

EXTRA_DIST = $(HTMLFILES) manual $(manual_alt) \
 $(manual_sources) conf.py \
 $(man_MANS) \
 news2html.pl

# Export these as an environment variables for use by conf.py
export PKGDOCDIR_EXPANDED
export VERSION

# Recover from the removal of $@.  A full explanation of this is in the
# automake manual under the heading "Multiple Outputs".
$(man_MANS): man.stamp
	@if test -f $@; then :; else \
	  trap 'rm -rf man.lock data.stamp' 1 2 13 15; \
	  if mkdir man.lock 2>/dev/null; then \
	    rm -f man.stamp; \
	    $(MAKE) $(AM_MAKEFLAGS) man.stamp; \
	    result=$$?; rm -rf man.lock; exit $$result; \
	  else \
	    while test -d man.lock; do sleep 1; done; \
	    test -f man.stamp; \
	  fi; \
	fi

man.stamp: $(man_sources) $(srcdir)/../configure.ac
	@rm -f $@-t
	@touch $@-t
	$(SPHINX_BUILD) -b man $(srcdir) man.tmp $<
	mv man.tmp/*.1 man.tmp/*.7 .
	rm -rf man.tmp
	@mv -f $@-t $@

install-data-local: manual
	$(mkinstalldirs) $(DESTDIR)$(pkgdocdir)/manual/_static
	test ! -d manual -a -d "$(srcdir)/manual" && cd $(srcdir) ; \
	for p in manual/*.htm manual/*.js manual/_static/*.js manual/_static/*.css ; do \
	  echo " $(INSTALL_DATA) $$p $(DESTDIR)$(pkgdocdir)/$$p"; \
	  $(INSTALL_DATA) $$p $(DESTDIR)$(pkgdocdir)/$$p; \
	done

uninstall-local:
	test ! -d manual -a -d "$(srcdir)/manual" && cd $(srcdir) ; \
	for p in manual/*.htm manual/*.js manual/_static/*.js manual/_static/*.css ; do \
	  echo " rm -f $(DESTDIR)$(pkgdocdir)/$$p"; \
	  rm -f $(DESTDIR)$(pkgdocdir)/$$p; \
	done
	-rmdir $(DESTDIR)$(pkgdocdir)/manual/_static
	-rmdir $(DESTDIR)$(pkgdocdir)/manual

maintainer-clean-local:
	rm -rf manual "$(srcdir)/manual"

manual: manual/stampfile ../configure.ac
manual/stampfile: $(manual_sources)
	rm -rf manual
	$(SPHINX_BUILD) -b html . manual
	touch manual/stampfile

manual.pdf: $(manual_sources) ../configure.ac
	$(SPHINX_BUILD) -M latexpdf $(srcdir) manual.latex
	mv manual.latex/latex/survex.pdf manual.pdf
	rm -rf manual.latex

# The rules below aren't used by the build system - they're for "manual" use
# (ho ho)

manual.html: $(manual_sources) ../configure.ac
	$(SPHINX_BUILD) -b singlehtml . tmpx
