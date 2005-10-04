## Process this file with automake to produce Makefile.in

# Makefile for survex support files

check-local:
	srcdir=$(srcdir) LANG=C perl $(srcdir)/checkshortcut.pl $(msg_files)

SUBDIRS = icons

msg_files = ca.msg de.msg de_CH.msg de_DE.msg en.msg en_US.msg\
 es.msg fr.msg it.msg pt.msg pt_BR.msg sk.msg ro.msg zh.msg

hdr_files = ca.h de.h de_CH.h de_DE.h en.h en_US.h\
 es.h fr.h it.h pt.h pt_BR.h sk.h ro.h zh.h

EXTRA_DIST = bcra3.svx bcra5.svx messages.txt default.txt bold.txt print.ini\
 named-entities.txt makefont.pl makemsgs.pl checkshortcut.pl\
 $(msg_files) $(hdr_files) survex.mime survex.keys.in\
 aven.txf gentexfont.c TexFont.h\
 INSTALL.OSX

MAINTAINERCLEANFILES = $(msg_files) $(hdr_files) *.todo aven.txf

pkgdata_DATA = print.ini bcra3.svx bcra5.svx $(msg_files) aven.txf

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
mime_DATA = survex.mime survex.keys

aven.txf:
	$(MAKE) gentexfont$(EXEEXT)
	./gentexfont$(EXEEXT) -w 128 -h 128 -glist `perl -e 'for (33..126,160..255) {print chr $$_}'` -fn -misc-fixed-medium-r-normal--13-100-100-100-c-80-iso8859-1 -file aven.txf

gentexfont$(EXEEXT): gentexfont.c TexFont.h
	$(CC) -o gentexfont$(EXEEXT) gentexfont.c @X_LIBS@ @X_PRE_LIBS@ -lX11 @X_EXTRA_LIBS@
