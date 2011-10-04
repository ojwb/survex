## Process this file with automake to produce Makefile.in

noinst_HEADERS = cavern.h commands.h cmdline.h date.h datain.h debug.h\
 filelist.h filename.h getopt.h hash.h img.h ini.h listpos.h matrix.h\
 message.h namecmp.h netartic.h netbits.h netskel.h network.h osalloc.h\
 osdepend.h ostypes.h out.h readval.h str.h useful.h validate.h whichos.h\
 fnt.h guicontrol.h gla.h moviemaker.h exportfilter.h hpgl.h cavernlog.h\
 aboutdlg.h aven.h avenpal.h gfxcore.h log.h mainfrm.h\
 vector3.h wx.h aventypes.h aventreectrl.h export.h printwx.h avenprcore.h

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

LDADD = $(LIBOBJS)

INCLUDES = -DIMG_HOSTED -DIMG_API_VERSION=1

# dump3d is a test program
EXTRA_PROGRAMS =\
	aven dump3d editwrap
# FIXME: base_progs in top level Makefile.am needs updating if this is
bin_PROGRAMS = cavern diffpos extend cad3d sorterr \
 3dtopos aven $(WINPROGS) $(TESTPROGS)
bin_SCRIPTS = svxedit

COMMONSRC = cmdline.c message.c filename.c osdepend.c z_getopt.c getopt1.c

cavern_SOURCES = cavern.c date.c listpos.c commands.c datain.c netskel.c \
 network.c readval.c str.c matrix.c img.c netbits.c useful.c validate.c \
 netartic.c \
 $(COMMONSRC)

aven_SOURCES = aven.cc date.c img.c message.c filename.c osdepend.c \
 gfxcore.cc mainfrm.cc vector3.cc aboutdlg.cc \
 useful.c cmdline.c z_getopt.c getopt1.c namecmp.c aventreectrl.cc export.cc \
 guicontrol.cc gla-gl.cc fnt.cc log.cc moviemaker.cc hpgl.cc cavernlog.cc \
 brotatemask.xbm brotate.xbm handmask.xbm hand.xbm \
 rotatemask.xbm rotate.xbm vrotatemask.xbm vrotate.xbm \
 rotatezoom.xbm rotatezoommask.xbm \
 avenprcore.cc printwx.cc ini.c hash.c \
 buttontaghandler.cc

dump3d_SOURCES = dump3d.c img.c useful.c $(COMMONSRC)

editwrap_SOURCES = editwrap.c
editwrap_LDFLAGS = -mwindows

if WIN32
aven_LDADD = avenrc.o $(LIBOBJS) $(MOVIE_LIBS) $(WX_LIBS)

svxeditrc.o: $(srcdir)/svxedit.rc svxedit.ico
	pwd=`pwd` && cd $(srcdir) && `$(WX_CONFIG) --rescomp` -o "$$pwd/svxeditrc.o" svxedit.rc

svxedit.ico: ../lib/icons/svxedit.ico
	cp ../lib/icons/svxedit.ico .

editwrap_LDADD = svxeditrc.o

avenrc.o: $(srcdir)/aven.rc aven.ico
	pwd=`pwd` && cd $(srcdir) && `$(WX_CONFIG) --rescomp` -o "$$pwd/avenrc.o" aven.rc

aven.ico: ../lib/icons/aven.ico
	cp ../lib/icons/aven.ico .

CLEANFILES = aven.ico svxedit.ico
else
aven_LDADD = $(LIBOBJS) $(MOVIE_LIBS) $(WX_LIBS)
endif

if MACOSX
# Aven must always be built into an application bundle to work correctly.
aven_CFLAGS = $(WX_CFLAGS) $(AM_CFLAGS) -DAVEN -DMACOSX_BUNDLE
aven_CXXFLAGS = $(WX_CXXFLAGS) $(AM_CXXFLAGS) -DMACOSX_BUNDLE
aven_LDFLAGS = $(WX_LDFLAGS) -framework OpenGL
else
aven_CFLAGS = $(WX_CFLAGS) $(AM_CFLAGS) -DAVEN
aven_CXXFLAGS = $(WX_CXXFLAGS) $(AM_CXXFLAGS)
aven_LDFLAGS = $(WX_LDFLAGS)
endif

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

EXTRA_DIST = \
 aven.rc svxedit.rc getopt.c gettexttomsg.pl gradient.pov

BUILT_SOURCES = z_getopt.c avenpal.h

z_getopt.c: getopt.c gettexttomsg.pl
	./gettexttomsg.pl `test -f getopt.c || echo '$(srcdir)/'`getopt.c > z_getopt.c~ && mv z_getopt.c~ z_getopt.c

avenpal.h: $(srcdir)/gradient.pov
	$(srcdir)/gdtconvert < $(srcdir)/gradient.pov > tmp-avenpal.h && mv tmp-avenpal.h avenpal.h

MAINTAINERCLEANFILES =\
 $(BUILT_SOURCES)
