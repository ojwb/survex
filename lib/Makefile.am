## Process this file with automake to produce Makefile.in

# Makefile for survex support files

check-local:
	srcdir=$(srcdir) LANG=C perl $(srcdir)/checkshortcut.pl $(msg_files)

SUBDIRS = icons

msg_files = ca.msg de.msg de_CH.msg de_DE.msg en.msg en_US.msg\
 es.msg fr.msg it.msg pt.msg pt_BR.msg sk.msg ro.msg zh.msg

hdr_files = ca.h de.h de_CH.h de_DE.h en.h en_US.h\
 es.h fr.h it.h pt.h pt_BR.h sk.h ro.h zh.h

EXTRA_DIST = messages.txt\
 named-entities.txt makemsgs.pl checkshortcut.pl\
 $(hdr_files) survex.keys.in\
 gentexfont.c TexFont.h\
 INSTALL.OSX

MAINTAINERCLEANFILES = $(msg_files) $(hdr_files) *.todo aven.txf

dist_pkgdata_DATA = print.ini bcra3.svx bcra5.svx $(msg_files) aven.txf

appsdir = $(datadir)/applications
dist_apps_DATA = survex-aven.desktop survex-svxedit.desktop

BUILT_SOURCES = defaultlang.h

# BUILT_SOURCES are automatically removed by maintainer-clean, but we want it
# removed by distclean
DISTCLEANFILES = defaultlang.h

$(msg_files) $(hdr_files): messages.txt makemsgs.pl
	srcdir=$(srcdir) LANG=C perl $(srcdir)/makemsgs.pl $(srcdir)/messages.txt

defaultlang.h:
	-rm -f defaultlang.h
	ln -s $(srcdir)/$(DEFAULTLANG).h defaultlang.h

mimedir = $(datadir)/mime-info
dist_mime_DATA = survex.mime
mime_DATA = survex.keys

aven.txf:
	$(MAKE) gentexfont$(EXEEXT)
	./gentexfont$(EXEEXT) -w 128 -h 128 -glist `perl -e 'for (33..126,160..255) {print chr $$_}'` -fn -*-helvetica-*-r-*-*-12-*-*-*-*-67-iso8859-1 -file aven.txf

gentexfont$(EXEEXT): gentexfont.c TexFont.h
	$(CC) -o gentexfont$(EXEEXT) gentexfont.c @X_LIBS@ @X_PRE_LIBS@ -lX11 @X_EXTRA_LIBS@
