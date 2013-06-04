## Process this file with automake to produce Makefile.in

noinst_HEADERS = cavern.h commands.h cmdline.h date.h datain.h debug.h\
 filelist.h filename.h getopt.h hash.h img.h ini.h listpos.h matrix.h\
 message.h namecmp.h namecompare.h netartic.h netbits.h netskel.h network.h\
 osalloc.h\
 osdepend.h ostypes.h out.h readval.h str.h useful.h validate.h whichos.h\
 glbitmapfont.h guicontrol.h gla.h moviemaker.h exportfilter.h hpgl.h\
 cavernlog.h aboutdlg.h aven.h avenpal.h gfxcore.h log.h mainfrm.h\
 vector3.h wx.h aventypes.h aventreectrl.h export.h printwx.h avenprcore.h

LDADD = $(LIBOBJS)

AM_CPPFLAGS = -DIMG_HOSTED -DIMG_API_VERSION=1

# dump3d is a test program
#
# findentrances needs libproj so is disabled by default for the moment -
# to enable it install the development stuff for libproj and build survex
# with:
#     make FINDENTRANCES=findentrances
#     make install FINDENTRANCES=findentrances
FINDENTRANCES =
EXTRA_PROGRAMS =\
	aven dump3d editwrap findentrances
# FIXME: base_progs in top level Makefile.am needs updating if this is
bin_PROGRAMS = cad3d cavern diffpos extend sorterr 3dtopos \
 aven $(WINPROGS) $(TESTPROGS) $(FINDENTRANCES)
bin_SCRIPTS = svxedit

COMMONSRC = cmdline.c message.c filename.c osdepend.c z_getopt.c getopt1.c

cavern_SOURCES = cavern.c date.c listpos.c commands.c datain.c netskel.c \
 network.c readval.c str.c matrix.c img.c netbits.c useful.c validate.c \
 netartic.c \
 $(COMMONSRC)

aven_SOURCES = aven.cc date.c img.c message.c filename.c osdepend.c \
 gfxcore.cc mainfrm.cc vector3.cc aboutdlg.cc useful.c cmdline.c z_getopt.c \
 getopt1.c namecompare.cc aventreectrl.cc export.cc guicontrol.cc gla-gl.cc \
 glbitmapfont.cc log.cc moviemaker.cc hpgl.cc cavernlog.cc \
 brotatemask.xbm brotate.xbm handmask.xbm hand.xbm \
 rotatemask.xbm rotate.xbm vrotatemask.xbm vrotate.xbm \
 rotatezoom.xbm rotatezoommask.xbm \
 avenprcore.cc printwx.cc ini.c hash.c \
 buttontaghandler.cc

dump3d_SOURCES = dump3d.c date.c img.c useful.c $(COMMONSRC)

findentrances_SOURCES = findentrances.cc img.c useful.c $(COMMONSRC)
findentrances_LDADD = -lproj

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

all_sources = \
	$(noinst_HEADERS) \
	$(COMMONSRC) \
	$(cavern_SOURCES) \
	$(aven_SOURCES) \
	$(dump3d_SOURCES) \
	$(editwrap_SOURCES) \
	$(diffpos_SOURCES) \
	$(sorterr_SOURCES) \
	$(3dtopos_SOURCES) \
	$(extend_SOURCES) \
	$(cad3d_SOURCES) \
	$(findentrances_SOURCES)

../lib/survex.pox: $(all_sources) ../lib/extract-msgs.pl
	cd $(srcdir)/../lib && ./extract-msgs.pl `echo $(all_sources)|perl -ne 'print "../src/$$_\n" for sort split'|uniq` > survex.pox

EXTRA_DIST = \
 aven.rc svxedit.rc getopt.c gettexttomsg.pl gradient.pov

BUILT_SOURCES = z_getopt.c avenpal.h

z_getopt.c: getopt.c gettexttomsg.pl
	./gettexttomsg.pl `test -f getopt.c || echo '$(srcdir)/'`getopt.c > z_getopt.c~ && mv z_getopt.c~ z_getopt.c

avenpal.h: $(srcdir)/gdtconvert $(srcdir)/gradient.pov
	$(srcdir)/gdtconvert < $(srcdir)/gradient.pov > tmp-avenpal.h && mv tmp-avenpal.h avenpal.h

MAINTAINERCLEANFILES =\
 $(BUILT_SOURCES)
