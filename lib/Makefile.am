## Process this file with automake to produce Makefile.in

# Makefile for survex support files

check-local:
	srcdir=$(srcdir) perl $(srcdir)/checkshortcut.pl $(msg_files)

SUBDIRS = icons

msg_files = ca.msg de.msg de_CH.msg de_DE.msg en.msg en_US.msg\
 es.msg fr.msg it.msg pt.msg pt_BR.msg

EXTRA_DIST = bcra3.svx bcra5.svx messages.txt pfont.txt print.ini\
 named-entities.txt makefont.pl makemsgs.pl checkshortcut.pl\
 $(msg_files) pfont.bit\
 ca.h de.h de_CH.h de_DE.h en.h en_US.h es.h fr.h it.h pt.h

MAINTAINERCLEANFILES = $(msg_files) pfont.bit

pkgdata_DATA = pfont.bit print.ini bcra3.svx bcra5.svx $(msg_files)

BUILT_SOURCES = defaultlang.h

# does $(msg_files) here work?
ca.msg de.msg de_CH.msg de_DE.msg en.msg en_US.msg es.msg fr.msg it.msg pt.msg pt_BR.msg\
 ca.h de.h de_CH.h de_DE.h en.h en_US.h es.h fr.h it.h pt.h pt_BR.h : messages.txt makemsgs.pl
	perl $(srcdir)/makemsgs.pl $(srcdir)/messages.txt

defaultlang.h:
	-rm -f defaultlang.h
	ln -s $(DEFAULTLANG).h defaultlang.h

pfont.bit: pfont.txt makefont.pl
	perl makefont.pl pfont.txt pfont.bit
