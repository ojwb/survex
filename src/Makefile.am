## Process this file with automake to produce Makefile.in

noinst_HEADERS = armrot.h cavern.h caverot.h commands.h cmdline.h\
 cvrotgfx.h cvrotimg.h datain.h debug.h dosrot.h filelist.h filename.h\
 getopt.h hash.h img.h ini.h labels.h listpos.h matrix.h message.h namecmp.h\
 netartic.h netbits.h netskel.h network.h osalloc.h osdepend.h ostypes.h out.h\
 prbitmap.h prcore.h prio.h readval.h rotplot.h str.h uni2dos.h useful.h\
 validate.h whichos.h xrot.h xcvrtbut.h new3dout.h 3ddefs.h \
 aboutdlg.h aven.h gfxcore.h mainfrm.h matrix4.h quaternion.h vector3.h wx.h \
 aventypes.h

LDADD = @LIBOBJS@

INCLUDES = -DIMG_HOSTED

PRINT = printdm printps printpcl printhpgl

EXTRA_PROGRAMS = caverot xcaverot printxbm printwin aven
bin_PROGRAMS = cavern $(PRINT) diffpos extend survex cad3d sorterr \
 3dtopos @CAVEROT@ @AVEN@ @PRINTWIN@

COMMONSRC = cmdline.c message.c filename.c osdepend.c z_getopt.c getopt1.c

cavern_SOURCES = cavern.c listpos.c commands.c datain.c netskel.c network.c \
 readval.c str.c matrix.c img.c netbits.c useful.c validate.c netartic.c \
 new3dout.c $(COMMONSRC)

survex_SOURCES = wrap.c osdepend.c

aven_SOURCES = aven.cc img.c message.c filename.c osdepend.c \
 gfxcore.cc mainfrm.cc matrix4.cc quaternion.cc vector3.cc aboutdlg.cc \
 useful.c

if WIN32
aven_LDADD = avenrc.o @LIBOBJS@ @WX_LIBS@

# FIXME: should use wxconfig or similar to get includes
avenrc.o: aven.rc
	i586-mingw32msvc-windres --include-dir /opt/wxmsw/include -o $@ $<

else
aven_LDADD = @LIBOBJS@ @WX_LIBS@
endif

aven_CFLAGS = @WX_CFLAGS@ -DAVEN
aven_CXXFLAGS = @WX_CFLAGS@ -DAVEN

PRSRC = prcore.c prio.c useful.c img.c $(COMMONSRC)
 
printps_SOURCES = printps.c $(PRSRC) ini.c hash.c
printdm_SOURCES = printdm.c $(PRSRC) ini.c hash.c prbitmap.c
printpcl_SOURCES = printpcl.c $(PRSRC) ini.c hash.c prbitmap.c
printxbm_SOURCES = printxbm.c $(PRSRC) ini.c hash.c prbitmap.c
printhpgl_SOURCES = prhpgl.c $(PRSRC) ini.c hash.c
printwin_SOURCES = printwin.c $(PRSRC)
printwin_LDFLAGS = -mwindows

ROTSRC = cvrotimg.c img.c useful.c $(COMMONSRC)

caverot_SOURCES = caverot.c $(ROTSRC) rotplot.c labels.c cvrotgfx.c
EXTRA_caverot_SOURCES = dosrot.c
caverot_LDADD = @LIBOBJS@ @CROBJX@ @CRLIB@
caverot_DEPENDENCIES = @CROBJX@

xcaverot_SOURCES = xcaverot.c xcvrtbut.c $(ROTSRC)
xcaverot_CFLAGS = @X_CFLAGS@ -DXCAVEROT
xcaverot_LDADD = @LIBOBJS@ @X_LIBS@ @X_PRE_LIBS@ -lX11 @X_EXTRA_LIBS@

diffpos_SOURCES = diffpos.c namecmp.c img.c useful.c hash.c $(COMMONSRC)
sorterr_SOURCES = sorterr.c $(COMMONSRC)
3dtopos_SOURCES = 3dtopos.c namecmp.c img.c useful.c $(COMMONSRC)
extend_SOURCES = extend.c $(COMMONSRC) img.c useful.c hash.c

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
 riscos/stdh/regnames riscos/stdh/swinames miniam.pl aven.rc aven.ico\
 getopt.c gettexttomsg.pl

BUILT_SOURCES = z_getopt.c

z_getopt.c: getopt.c
	./gettexttomsg.pl `test -f getopt.c || echo '$(srcdir)/'`getopt.c > z_getopt.c~ && mv z_getopt.c~ z_getopt.c

MAINTAINERCLEANFILES = borlandc/makefile riscos/makefile

borlandc/config.h borlandc/makefile: Makefile.am $(top_srcdir)/configure.in $(top_srcdir)/hto/Makefile.am miniam.pl
	cat $(srcdir)/Makefile.am $(top_srcdir)/hto/Makefile.am | $(srcdir)/miniam.pl borlandc $(top_srcdir)/configure.in borlandc/config.h > borlandc/makefile

riscos/config.h riscos/makefile: Makefile.am $(top_srcdir)/configure.in $(top_srcdir)/hto/Makefile.am miniam.pl
	cat $(srcdir)/Makefile.am $(top_srcdir)/hto/Makefile.am | $(srcdir)/miniam.pl riscos $(top_srcdir)/configure.in riscos/config.h > riscos/makefile
