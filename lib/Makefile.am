## Process this file with automake to produce Makefile.in

# Makefile for survex support files

SUBDIRS = icons

msg_files = ca.msg de.msg de_CH.msg de_DE.msg en.msg en_US.msg\
 es.msg fr.msg it.msg pt.msg

EXTRA_DIST = bcra3.svx bcra5.svx messages.txt pfont.txt print.ini\
 named-entities.txt makefont.pl makemsgs.pl $(msg_files) pfont.bit\
 aven-about.png ca.h de.h de_CH.h de_DE.h en.h en_US.h es.h fr.h\
 it.h pt.h

MAINTAINERCLEANFILES = $(msgs_files) pfont.bit

pkgdata_DATA = pfont.bit print.ini bcra3.svx bcra5.svx $(msg_files)

# does $(msgs_files) here work?
ca.msg de.msg de_CH.msg de_DE.msg en.msg en_US.msg es.msg fr.msg it.msg pt.msg\
 ca.h de.h de_CH.h de_DE.h en.h en_US.h es.h fr.h it.h pt.h : messages.txt makemsgs.pl
	perl makemsgs.pl messages.txt

pfont.bit: pfont.txt makefont.pl
	perl makefont.pl pfont.txt pfont.bit
