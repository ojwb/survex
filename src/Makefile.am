## Process this file with automake to produce Makefile.in

noinst_HEADERS = armrot.h cavern.h caverot.h commands.h cmdline.h\
 cvrotgfx.h cvrotimg.h datain.h debug.h dosrot.h filelist.h filename.h\
 getopt.h img.h ini.h labels.h listpos.h matrix.h message.h netartic.h\
 netbits.h netskel.h network.h osalloc.h osdepend.h ostypes.h out.h\
 prbitmap.h prcore.h prio.h readval.h rotplot.h str.h uni2dos.h useful.h\
 validate.h whichos.h xrot.h new3dout.h 3ddefs.h \
 aven.h gfxcore.h mainfrm.h matrix4.h quaternion.h vector3.h wx.h

LDADD = @LIBOBJS@

PRINT = printdm printps printpcl printhpgl

EXTRA_PROGRAMS = caverot xcaverot printxbm aven
bin_PROGRAMS = cavern $(PRINT) diffpos extend survex cad3d @CAVEROT@ \
 @AVEN@

COMMONSRC = cmdline.c message.c filename.c osdepend.c getopt.c getopt1.c

cavern_SOURCES = cavern.c listpos.c commands.c datain.c netskel.c network.c \
 readval.c str.c matrix.c img.c netbits.c useful.c validate.c netartic.c \
 new3dout.c $(COMMONSRC)

survex_SOURCES = wrap.c osdepend.c

aven_SOURCES = aven.cc img.c \
 gfxcore.cc mainfrm.cc matrix4.cc quaternion.cc vector3.cc

aven_LDADD = @LIBOBJS@ @WX_LIBS@
aven_CFLAGS = -DSTANDALONE @WX_CFLAGS@
aven_CXXFLAGS = @WX_CFLAGS@

PRSRC = prcore.c prio.c useful.c img.c ini.c $(COMMONSRC)
 
printps_SOURCES = printps.c $(PRSRC)
printdm_SOURCES = printdm.c $(PRSRC) prbitmap.c
printpcl_SOURCES = printpcl.c $(PRSRC) prbitmap.c
printxbm_SOURCES = printxbm.c $(PRSRC) prbitmap.c
printhpgl_SOURCES = prhpgl.c $(PRSRC)

ROTSRC = cvrotimg.c img.c useful.c $(COMMONSRC)

caverot_SOURCES = caverot.c $(ROTSRC) rotplot.c labels.c cvrotgfx.c
EXTRA_caverot_SOURCES = dosrot.c
caverot_LDADD = @LIBOBJS@ @CROBJX@ @CRLIB@
caverot_DEPENDENCIES = @CROBJX@

xcaverot_SOURCES = xcaverot.c $(ROTSRC)
xcaverot_CFLAGS = @X_CFLAGS@
xcaverot_LDADD = @LIBOBJS@ @X_LIBS@ @X_PRE_LIBS@ -lX11 @X_EXTRA_LIBS@

# next line is actually superfluous to automake (but riscos/bc hack needs it)
diffpos_SOURCES = diffpos.c $(COMMONSRC)
extend_SOURCES = extend.c $(COMMONSRC) useful.c img.c

cad3d_SOURCES = 3dtodxf.c $(COMMONSRC) useful.c img.c

#testerr_SOURCES = testerr.c message.c filename.c useful.c osdepend.c
#imgtest_SOURCES = imgtest.c imgalone.c

# this doesn't work if compiler doesn't support -c and -o, or with
# deANSIfication
# Use imgalone.c containing '#define STANDALONE/#include "img.c"'?
#imgalone.o: img.c
#        $(COMPILE) -DSTANDALONE -o imgalone.o -c img.c

noinst_DATA = borlandc/makefile riscos/makefile
 
EXTRA_DIST = borlandc/makefile borlandc/config.h borlandc/config.h.in\
 riscos/makefile riscos/config.h riscos/config.h.in riscos/armrot.s\
 riscos/stdh/regnames riscos/stdh/swinames miniam.pl

MAINTAINERCLEANFILES = borlandc/makefile riscos/makefile

borlandc/config.h borlandc/makefile: Makefile.am $(top_srcdir)/configure.in $(top_srcdir)/hto/Makefile.am miniam.pl
	cat $(srcdir)/Makefile.am $(top_srcdir)/hto/Makefile.am | $(srcdir)/miniam.pl borlandc $(top_srcdir)/configure.in borlandc/config.h > borlandc/makefile

riscos/config.h riscos/makefile: Makefile.am $(top_srcdir)/configure.in $(top_srcdir)/hto/Makefile.am miniam.pl
	cat $(srcdir)/Makefile.am $(top_srcdir)/hto/Makefile.am | $(srcdir)/miniam.pl riscos $(top_srcdir)/configure.in riscos/config.h > riscos/makefile
