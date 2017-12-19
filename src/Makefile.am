## Process this file with automake to produce Makefile.in

noinst_HEADERS = cavern.h commands.h cmdline.h date.h datain.h debug.h\
 filelist.h filename.h getopt.h hash.h img.c img.h img_hosted.h kml.h\
 labelinfo.h listpos.h matrix.h message.h namecmp.h namecompare.h netartic.h\
 netbits.h netskel.h network.h osalloc.h\
 osdepend.h ostypes.h out.h readval.h str.h useful.h validate.h whichos.h\
 glbitmapfont.h guicontrol.h gla.h gpx.h moviemaker.h exportfilter.h hpgl.h\
 cavernlog.h aboutdlg.h aven.h avenpal.h gfxcore.h json.h log.h mainfrm.h\
 pos.h vector3.h wx.h aventypes.h aventreectrl.h export.h printing.h\
 avenprcore.h img2aven.h thgeomag.h thgeomagdata.h\
 moviemaker-legacy.cc

LDADD = $(LIBOBJS)

# dump3d is a test program
#
# findentrances is mostly superceded by aven's GPX export, so is disabled
# by default, and will be removed entirely at some point.  To build and
# install it, pass "FINDENTRANCES=findentrances" as an argument to make
# like so:
#     make FINDENTRANCES=findentrances
#     sudo make install FINDENTRANCES=findentrances
FINDENTRANCES =
EXTRA_PROGRAMS =\
	aven dump3d findentrances
# FIXME: base_progs in top level Makefile.am needs updating if this is
bin_PROGRAMS = cad3d cavern diffpos extend sorterr 3dtopos dump3d \
 aven $(OSPROGS) $(FINDENTRANCES)

check_PROGRAMS = imgtest

COMMONSRC = cmdline.c message.c str.c filename.c osdepend.c z_getopt.c getopt1.c

cavern_SOURCES = cavern.c date.c listpos.c commands.c datain.c netskel.c \
 network.c readval.c matrix.c img_hosted.c netbits.c useful.c \
 validate.c netartic.c thgeomag.c \
 $(COMMONSRC)
cavern_LDADD = $(PROJ_LIBS)

aven_SOURCES = aven.cc gfxcore.cc mainfrm.cc vector3.cc aboutdlg.cc \
 namecompare.cc aventreectrl.cc export.cc guicontrol.cc gla-gl.cc \
 glbitmapfont.cc gpx.cc json.cc kml.cc log.cc moviemaker.cc hpgl.cc \
 cavernlog.cc avenprcore.cc printing.cc buttontaghandler.cc pos.cc \
 date.c img_hosted.c useful.c hash.c \
 brotatemask.xbm brotate.xbm handmask.xbm hand.xbm \
 rotatemask.xbm rotate.xbm vrotatemask.xbm vrotate.xbm \
 rotatezoom.xbm rotatezoommask.xbm \
 $(COMMONSRC)

dump3d_SOURCES = dump3d.c date.c img_hosted.c useful.c \
 $(COMMONSRC)

findentrances_SOURCES = findentrances.cc img_hosted.c useful.c \
 $(COMMONSRC)
findentrances_LDADD = $(PROJ_LIBS)

if WIN32
aven_LDADD = avenrc.o $(LIBOBJS) $(LIBAV_LIBS) $(WX_LIBS) $(PROJ_LIBS)

avenrc.o: $(srcdir)/aven.rc ../lib/icons/aven.ico
	pwd=`pwd` && cd $(srcdir) && `$(WX_CONFIG) --rescomp` --include-dir "$$pwd/../lib/icons" -o "$$pwd/avenrc.o" aven.rc

else
aven_LDADD = $(LIBOBJS) $(AVEN_LIBS) $(WX_LIBS) $(PROJ_LIBS) $(LIBAV_LIBS)
endif

AM_CFLAGS += $(PROJ_CFLAGS)

aven_CFLAGS = $(AM_CFLAGS) $(WX_CFLAGS) -DAVEN
aven_CXXFLAGS = $(AM_CXXFLAGS) $(PROJ_CFLAGS) $(LIBAV_CFLAGS) $(WX_CXXFLAGS)
aven_LDFLAGS =

if MACOSX
# FIXME: It looks like modern wx-config should give us this...
aven_LDFLAGS += -framework OpenGL
endif

diffpos_SOURCES = diffpos.c namecmp.c img_hosted.c useful.c hash.c \
 $(COMMONSRC)
sorterr_SOURCES = sorterr.c $(COMMONSRC)
3dtopos_SOURCES = 3dtopos.c namecmp.c img_hosted.c useful.c \
 $(COMMONSRC)
extend_SOURCES = extend.c img_hosted.c useful.c hash.c \
 $(COMMONSRC)

cad3d_SOURCES = cad3d.c useful.c img_hosted.c hash.c \
 $(COMMONSRC)

#testerr_SOURCES = testerr.c message.c filename.c useful.c osdepend.c

imgtest_SOURCES = imgtest.c img.c

all_sources = \
	$(noinst_HEADERS) \
	$(COMMONSRC) \
	$(cavern_SOURCES) \
	$(aven_SOURCES) \
	$(dump3d_SOURCES) \
	$(diffpos_SOURCES) \
	$(sorterr_SOURCES) \
	$(3dtopos_SOURCES) \
	$(extend_SOURCES) \
	$(cad3d_SOURCES) \
	$(findentrances_SOURCES)

../lib/survex.pox: $(all_sources) ../lib/extract-msgs.pl
	cd $(srcdir)/../lib && ./extract-msgs.pl `echo $(all_sources)|perl -ne 'print "../src/$$_\n" for sort split'|uniq` > survex.pox

EXTRA_DIST = \
 aven.rc getopt.c gettexttomsg.pl gradient.pov gdtconvert gen_img2aven \
 igrf12coeffs.txt igrf2c.py

BUILT_SOURCES = z_getopt.c avenpal.h img2aven.h thgeomagdata.h

z_getopt.c: getopt.c gettexttomsg.pl
	./gettexttomsg.pl `test -f getopt.c || echo '$(srcdir)/'`getopt.c > z_getopt.c~ && mv z_getopt.c~ z_getopt.c

avenpal.h: $(srcdir)/gdtconvert $(srcdir)/gradient.pov
	$(srcdir)/gdtconvert < $(srcdir)/gradient.pov > tmp-avenpal.h && mv tmp-avenpal.h avenpal.h

img2aven.h: $(srcdir)/gen_img2aven
	$(srcdir)/gen_img2aven > tmp-img2aven.h && mv tmp-img2aven.h img2aven.h

thgeomagdata.h: $(srcdir)/igrf2c.py $(srcdir)/igrf12coeffs.txt
	python $(srcdir)/igrf2c.py $(srcdir)/igrf12coeffs.txt
	mv ../thgeomagdata.h .

MAINTAINERCLEANFILES =\
 $(BUILT_SOURCES)
