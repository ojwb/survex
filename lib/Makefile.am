## Process this file with automake to produce Makefile.in

# Makefile for survex support files

msg_files = ca.msg de.msg en-us.msg en.msg es.msg fr.msg it.msg

EXTRA_DIST = bcra3.svx bcra5.svx messages.txt pfont.txt print.ini\
 named-entities.txt makefont.pl makemsgs.pl $(msg_files) pfont.bit\
 aven-about.png

MAINTAINERCLEANFILES = $(msgs_files) pfont.bit

pkgdata_DATA = pfont.bit print.ini bcra3.svx bcra5.svx $(msg_files)\
 aven-about.png

ca.msg de.msg en-us.msg en.msg es.msg fr.msg it.msg: messages.txt makemsgs.pl
	perl makemsgs.pl messages.txt

pfont.bit: pfont.txt makefont.pl
	perl makefont.pl pfont.txt pfont.bit
