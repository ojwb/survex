## Process this file with automake to produce Makefile.in

check-local dist-hook:
	if grep '@.*@' borlandc/config.h riscos/config.h ; then \
	  echo 'Unsubstituted token in src/*/config.h - something is amiss with configure.in'; \
	  exit 1; \
	fi

SUFFIXES = .ico .png

.png.ico:
	$(SHELL) $(srcdir)/png2winicon $< $@

noinst_HEADERS = armrot.h cavern.h caverot.h commands.h cmdline.h\
 cvrotgfx.h cvrotimg.h datain.h debug.h dosrot.h filelist.h filename.h\
 getopt.h hash.h img.h ini.h labels.h listpos.h matrix.h message.h namecmp.h\
 netartic.h netbits.h netskel.h network.h osalloc.h osdepend.h ostypes.h out.h\
 prbitmap.h prcore.h prio.h readval.h rotplot.h str.h useful.h\
 validate.h whichos.h xrot.h xcvrtbut.h new3dout.h 3ddefs.h \
 aboutdlg.h aven.h gfxcore.h mainfrm.h matrix4.h quaternion.h vector3.h wx.h \
 aventypes.h aventreectrl.h

# GCC 2.95 frequently dies on gfxcore.cc with an internal error, so if the
# compile fails, rerun it with -O instead of -O2 - FIXME this probably
# assumes things about automake internals...
# Argh - the Apple's cc for MacOS X (based on GCC 2.95) won't compile
# gfxcore.cc with *any* level of optimisation...
h = $(CXX) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS)
t = -c -o aven-gfxcore.o `test -f gfxcore.cc || echo '$(srcdir)/'`gfxcore.cc

aven-gfxcore.o: gfxcore.cc
	source='gfxcore.cc' object='aven-gfxcore.o' libtool=no \
	depfile='$(DEPDIR)/aven-gfxcore.Po' \
	tmpdepfile='$(DEPDIR)/aven-gfxcore.TPo' \
	$(CXXDEPMODE) $(depcomp) \
	$h $(aven_CXXFLAGS) $(CXXFLAGS) $t \
	 || $h `echo $(aven_CXXFLAGS) $(CXXFLAGS)|sed 's/-O2/-O/g'` $t \
	 || $h `echo $(aven_CXXFLAGS) $(CXXFLAGS)|sed 's/-O2*//g'` $t
 
LDADD = @LIBOBJS@

INCLUDES = -DIMG_HOSTED

PRINT = printdm printps printpcl printhpgl

# developer tools, testharnesses, etc
TESTPROGS = @TESTPROGS@

# printxbm and dump3d are test programs
EXTRA_PROGRAMS = caverot xcaverot printwin aven avengl dump3d printxbm \
 editwrap
# FIXME: base_progs in top level Makefile.am needs updating if this is
bin_PROGRAMS = cavern $(PRINT) diffpos extend cad3d sorterr \
 3dtopos @CAVEROT@ @AVEN@ @WINPROGS@ @TESTPROGS@
bin_SCRIPTS = svxedit

COMMONSRC = cmdline.c message.c filename.c osdepend.c z_getopt.c getopt1.c

cavern_SOURCES = cavern.c listpos.c commands.c datain.c netskel.c network.c \
 readval.c str.c matrix.c img.c netbits.c useful.c validate.c netartic.c \
 new3dout.c $(COMMONSRC)

aven_SOURCES = aven.cc img.c message.c filename.c osdepend.c \
 gfxcore.cc mainfrm.cc matrix4.cc quaternion.cc vector3.cc aboutdlg.cc \
 useful.c cmdline.c z_getopt.c getopt1.c namecmp.c aventreectrl.cc

avengl_SOURCES = $(aven_SOURCES)

dump3d_SOURCES = dump3d.c img.c useful.c $(COMMONSRC)

editwrap_SOURCES = editwrap.c
editwrap_LDFLAGS = -mwindows

if WIN32
aven_LDADD = avenrc.o @LIBOBJS@ @WX_LIBS@ @LIBRX@
avengl_LDADD = avenrc.o @LIBOBJS@ @WXGL_LIBS@ @LIBRX@

svxeditrc.o: $(srcdir)/svxedit.rc
	pwd=`pwd` && cd $(srcdir) && i586-mingw32msvc-windres -o $$pwd/svxeditrc.o svxedit.rc

editwrap_LDADD = svxeditrc.o

# FIXME: should use wxconfig or similar to get includes
avenrc.o: $(srcdir)/aven.rc
	pwd=`pwd` && cd $(srcdir) && i586-mingw32msvc-windres --include-dir /opt/wxmsw/include -o $$pwd/avenrc.o aven.rc
else
aven_LDADD = @LIBOBJS@ @WX_LIBS@ @LIBRX@
avengl_LDADD = @LIBOBJS@ @WXGL_LIBS@ @LIBRX@
endif

if MACOSX
noinst_SCRIPTS = aven-rez

aven-rez: aven
	`@WXCONFIG@ --rezflags` aven
	touch aven-rez
endif

aven_CFLAGS = @WX_CFLAGS@ -DAVEN
aven_CXXFLAGS = @WX_CXXFLAGS@ -DAVEN

avengl_CFLAGS = @WXGL_CFLAGS@ -DAVEN
avengl_CXXFLAGS = @WXGL_CXXFLAGS@ -DAVEN
avengl_LDFLAGS = @LIBRX@

PRSRC = prcore.c prio.c useful.c img.c ini.c hash.c $(COMMONSRC)
 
printps_SOURCES = printps.c $(PRSRC)
printdm_SOURCES = printdm.c $(PRSRC) prbitmap.c
printpcl_SOURCES = printpcl.c $(PRSRC) prbitmap.c
printxbm_SOURCES = printxbm.c $(PRSRC) prbitmap.c
printhpgl_SOURCES = prhpgl.c $(PRSRC)
printwin_SOURCES = printwin.c $(PRSRC)
printwin_LDFLAGS = -mconsole
printwin_LDADD = -lgdi32 -lcomdlg32

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

cad3d_SOURCES = cad3d.c $(COMMONSRC) useful.c img.c hash.c

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
 riscos/stdh/regnames riscos/stdh/swinames miniam.pl aven.rc aven.png aven.ico\
 svxedit.png svxedit.ico getopt.c png2winicon gettexttomsg.pl

BUILT_SOURCES = z_getopt.c

z_getopt.c: getopt.c gettexttomsg.pl
	./gettexttomsg.pl `test -f getopt.c || echo '$(srcdir)/'`getopt.c > z_getopt.c~ && mv z_getopt.c~ z_getopt.c

MAINTAINERCLEANFILES = borlandc/makefile riscos/makefile

borlandc/makefile: $(srcdir)/Makefile.am miniam.pl
	$(srcdir)/miniam.pl borlandc < $(srcdir)/Makefile.am > borlandc/makefile

riscos/makefile: $(srcdir)/Makefile.am miniam.pl
	$(srcdir)/miniam.pl riscos < $(srcdir)/Makefile.am > riscos/makefile
