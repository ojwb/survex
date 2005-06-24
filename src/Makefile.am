## Process this file with automake to produce Makefile.in

noinst_HEADERS = cavern.h commands.h cmdline.h\
 datain.h debug.h filelist.h filename.h\
 getopt.h hash.h img.h ini.h listpos.h matrix.h message.h namecmp.h\
 netartic.h netbits.h netskel.h network.h osalloc.h osdepend.h ostypes.h out.h\
 prbitmap.h prcore.h prio.h readval.h str.h useful.h\
 validate.h whichos.h new3dout.h 3ddefs.h \
 aboutdlg.h aven.h gfxcore.h mainfrm.h matrix4.h quaternion.h vector3.h wx.h \
 aventypes.h aventreectrl.h guicontrol.h gla.h survey.h

# splash.h paneldlg.h paneldlgpage.h \
# tubeprefs.h prefsdlg.h stnprefs.h legprefs.h gridprefs.h unitsprefs.h \
# ctlprefs.h indicatorprefs.h winprefs.h

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
EXTRA_PROGRAMS = printwin aven dump3d printxbm editwrap
# FIXME: base_progs in top level Makefile.am needs updating if this is
bin_PROGRAMS = cavern $(PRINT) diffpos extend cad3d sorterr \
 3dtopos @AVEN@ @WINPROGS@ @TESTPROGS@
bin_SCRIPTS = svxedit

COMMONSRC = cmdline.c message.c filename.c osdepend.c z_getopt.c getopt1.c

cavern_SOURCES = cavern.c listpos.c commands.c datain.c netskel.c network.c \
 readval.c str.c matrix.c img.c netbits.c useful.c validate.c netartic.c \
 new3dout.c $(COMMONSRC)

aven_SOURCES = aven.cc img.c message.c filename.c osdepend.c \
 gfxcore.cc mainfrm.cc matrix4.cc quaternion.cc vector3.cc aboutdlg.cc \
 useful.c cmdline.c z_getopt.c getopt1.c namecmp.c aventreectrl.cc \
 guicontrol.cc gla-gl.cc avenpal.h fnt.cc fnt.h moviemaker.cc moviemaker.h

# splash.cc paneldlg.cc paneldlgpage.cc prefsdlg.cc \
# tubeprefs.cc stnprefs.cc legprefs.cc ctlprefs.cc gridprefs.cc unitsprefs.cc \
# indicatorprefs.cc winprefs.cc

dump3d_SOURCES = dump3d.c img.c useful.c $(COMMONSRC)

editwrap_SOURCES = editwrap.c
editwrap_LDFLAGS = -mwindows

if WIN32
#aven_LDADD = avenrc.o @LIBOBJS@ @WX_LIBS@
aven_LDADD = avenrc.o @LIBOBJS@ @WXGL_LIBS@ @MOVIE_LIBS@

svxeditrc.o: $(srcdir)/svxedit.rc
	pwd=`pwd` && cd $(srcdir) && i586-mingw32msvc-windres -o $$pwd/svxeditrc.o svxedit.rc

editwrap_LDADD = svxeditrc.o

# FIXME: use path on WXCONFIG to get --include-dir?
avenrc.o: $(srcdir)/aven.rc
	pwd=`pwd` && cd $(srcdir) && i586-mingw32msvc-windres --include-dir /opt/wxmsw/include --include-dir /usr/i586-mingw32msvc/include -o $$pwd/avenrc.o aven.rc
else
#aven_LDADD = @LIBOBJS@ @WX_LIBS@
aven_LDADD = @LIBOBJS@ @WXGL_LIBS@ @MOVIE_LIBS@
endif

# If we're building aven on MacOS X, we need to bind resources to aven after
# we build it.  Use aven-rez as a timestamp file to track whether we've
# bound resources.
# The MacOS X install program doesn't copy the resource fork (clever eh?) so
# we reapply the resources after installing too...
if AVEN
if MACOSX
AVEN_REZ = aven-rez

aven-rez: aven
	`@WXCONFIG@ --rezflags` aven
	touch aven-rez

install-exec-hook:
	`@WXCONFIG@ --rezflags` $(bindir)/aven
else !MACOSX
AVEN_REZ =

# need empty target as automake will try to make it because it's in the other
# branch
install-exec-hook:

endif !MACOSX
else !AVEN
AVEN_REZ =

# need empty target as automake will try to make it because it's in the other
# branch
install-exec-hook:

endif !AVEN

noinst_SCRIPTS = $(AVEN_REZ)
MOSTLYCLEANFILES = $(AVEN_REZ)

#aven_CFLAGS = @WX_CFLAGS@ -DAVEN
#aven_CXXFLAGS = @WX_CXXFLAGS@
#aven_LDFLAGS = @WX_LDFLAGS@

aven_CFLAGS = @WXGL_CFLAGS@ $(AM_CFLAGS) -DAVEN
aven_CXXFLAGS = @WXGL_CXXFLAGS@ $(AM_CXXFLAGS)
aven_LDFLAGS = @WX_LDFLAGS@

PRSRC = prcore.c prio.c useful.c img.c ini.c hash.c $(COMMONSRC)

printps_SOURCES = printps.c $(PRSRC)
printdm_SOURCES = printdm.c $(PRSRC) prbitmap.c
printpcl_SOURCES = printpcl.c $(PRSRC) prbitmap.c
printxbm_SOURCES = printxbm.c $(PRSRC) prbitmap.c
printhpgl_SOURCES = prhpgl.c $(PRSRC)
printwin_SOURCES = printwin.c $(PRSRC)
printwin_LDFLAGS = -mconsole
printwin_LDADD = -lgdi32 -lcomdlg32

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

EXTRA_DIST = aven.rc getopt.c gettexttomsg.pl gradient.pov

BUILT_SOURCES = z_getopt.c avenpal.h

z_getopt.c: getopt.c gettexttomsg.pl
	./gettexttomsg.pl `test -f getopt.c || echo '$(srcdir)/'`getopt.c > z_getopt.c~ && mv z_getopt.c~ z_getopt.c

avenpal.h: $(srcdir)/gradient.pov
	$(srcdir)/gdtconvert < $(srcdir)/gradient.pov > tmp-avenpal.h && mv tmp-avenpal.h avenpal.h

MAINTAINERCLEANFILES = $(BUILT_SOURCES)
