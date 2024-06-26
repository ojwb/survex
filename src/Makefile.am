## Process this file with automake to produce Makefile.in

# Allow easy overriding via: make WERROR=
AM_CFLAGS += $(WERROR)
AM_CXXFLAGS += $(WERROR)

noinst_HEADERS = cavern.h commands.h cmdline.h date.h datain.h debug.h\
 filelist.h filename.h getopt.h hash.h img.c img.h img_hosted.h kml.h\
 labelinfo.h listpos.h matrix.h message.h namecmp.h namecompare.h netartic.h\
 netbits.h netskel.h network.h osalloc.h\
 out.h readval.h str.h useful.h validate.h whichos.h\
 glbitmapfont.h gllogerror.h guicontrol.h gla.h gpx.h moviemaker.h\
 export3d.h exportfilter.h hpgl.h cavernlog.h aboutdlg.h aven.h avenpal.h\
 gfxcore.h json.h log.h mainfrm.h pos.h vector3.h wx.h aventypes.h\
 aventreectrl.h export.h model.h printing.h avenprcore.h img2aven.h\
 thgeomag.h thgeomagdata.h moviemaker-legacy.cc

LDADD = $(LIBOBJS)

# FIXME: mingw_progs in top level Makefile.am needs keeping in step with this
bin_PROGRAMS = cavern diffpos dump3d extend sorterr survexport aven

install-exec-hook:
	ln $(DESTDIR)$(bindir)/survexport$(EXEEXT) \
	   $(DESTDIR)$(bindir)/3dtopos$(EXEEXT) || \
	    cp -p $(DESTDIR)$(bindir)/survexport$(EXEEXT) \
		  $(DESTDIR)$(bindir)/3dtopos$(EXEEXT)

uninstall-hook:
	rm -f $(DESTDIR)$(bindir)/3dtopos$(EXEEXT)

check_PROGRAMS = imgtest

COMMONSRC = cmdline.c message.c str.c filename.c z_getopt.c getopt1.c

cavern_SOURCES = cavern.c date.c listpos.c commands.c datain.c netskel.c \
 network.c readval.c matrix.c img_hosted.c netbits.c useful.c \
 validate.c netartic.c thgeomag.c \
 $(COMMONSRC)
cavern_LDADD = $(PROJ_LIBS)

aven_SOURCES = aven.cc gfxcore.cc mainfrm.cc model.cc vector3.cc aboutdlg.cc \
 namecompare.cc aventreectrl.cc export.cc export3d.cc guicontrol.cc gla-gl.cc \
 glbitmapfont.cc gpx.cc json.cc kml.cc log.cc moviemaker.cc hpgl.cc \
 cavernlog.cc avenprcore.cc printing.cc buttontaghandler.cc pos.cc \
 date.c img_hosted.c useful.c hash.c \
 brotatemask.xbm brotate.xbm handmask.xbm hand.xbm \
 rotatemask.xbm rotate.xbm vrotatemask.xbm vrotate.xbm \
 rotatezoom.xbm rotatezoommask.xbm \
 $(COMMONSRC)

dump3d_SOURCES = dump3d.c date.c img_hosted.c useful.c \
 $(COMMONSRC)

aven_LDADD = $(LIBOBJS) $(WX_LIBS) $(GDAL_LIBS) $(PROJ_LIBS) $(FFMPEG_LIBS)

if WIN32
aven_LDADD += avenrc.o

avenrc.o: $(srcdir)/aven.rc ../lib/icons/aven.ico
	cd $(srcdir) && `$(WX_CONFIG) --rescomp` --include-dir '$(abs_top_builddir)/lib/icons' -o '$(abs_builddir)/avenrc.o' aven.rc
endif

AM_CFLAGS += $(PROJ_CFLAGS)

aven_CFLAGS = $(AM_CFLAGS) $(WX_CFLAGS) -DAVEN
aven_CXXFLAGS = $(AM_CXXFLAGS) $(GDAL_CFLAGS) $(PROJ_CFLAGS) $(FFMPEG_CFLAGS) $(WX_CXXFLAGS)
aven_LDFLAGS =

survexport_CXXFLAGS = $(AM_CXXFLAGS) $(PROJ_CFLAGS) $(WX_CXXFLAGS)
survexport_LDFLAGS =
survexport_LDADD = $(LIBOBJS) $(WX_LIBS) $(PROJ_LIBS)

if MACOS
# FIXME: It looks like modern wx-config should give us this...
aven_LDFLAGS += -framework OpenGL
endif

diffpos_SOURCES = diffpos.c namecmp.c img_hosted.c useful.c hash.c \
 $(COMMONSRC)
sorterr_SOURCES = sorterr.c $(COMMONSRC)
extend_SOURCES = extend.c img_hosted.c useful.c hash.c \
 $(COMMONSRC)

survexport_SOURCES = survexport.cc model.cc export.cc export3d.cc \
		namecompare.cc useful.c hash.c img_hosted.c \
		gpx.cc hpgl.cc json.cc kml.cc pos.cc vector3.cc $(COMMONSRC)

#testerr_SOURCES = testerr.c message.c filename.c useful.c

imgtest_SOURCES = imgtest.c img.c

all_sources = \
	$(noinst_HEADERS) \
	$(COMMONSRC) \
	$(cavern_SOURCES) \
	$(aven_SOURCES) \
	$(dump3d_SOURCES) \
	$(diffpos_SOURCES) \
	$(sorterr_SOURCES) \
	$(extend_SOURCES) \
	$(survexport_SOURCES)

../lib/survex.pox: $(all_sources) ../lib/extract-msgs.pl
	cd $(srcdir)/../lib && ./extract-msgs.pl `echo $(all_sources)|perl -ne 'print "../src/$$_\n" for sort split'|uniq` > survex.pox

EXTRA_DIST = \
 aven.rc getopt.c gettexttomsg.pl gradient.pov gdtconvert gen_img2aven \
 igrf13coeffs.txt igrf2c.py

BUILT_SOURCES = z_getopt.c avenpal.h img2aven.h thgeomagdata.h

z_getopt.c: getopt.c gettexttomsg.pl
	./gettexttomsg.pl `test -f getopt.c || echo '$(srcdir)/'`getopt.c > z_getopt.c~ && mv z_getopt.c~ z_getopt.c

avenpal.h: $(srcdir)/gdtconvert $(srcdir)/gradient.pov
	$(srcdir)/gdtconvert < $(srcdir)/gradient.pov > tmp-avenpal.h && mv tmp-avenpal.h avenpal.h

img2aven.h: $(srcdir)/gen_img2aven
	$(srcdir)/gen_img2aven > tmp-img2aven.h && mv tmp-img2aven.h img2aven.h

PYTHON = python3

thgeomagdata.h: $(srcdir)/igrf2c.py $(srcdir)/igrf13coeffs.txt
	$(PYTHON) $(srcdir)/igrf2c.py $(srcdir)/igrf13coeffs.txt
	mv ../thgeomagdata.h .

MAINTAINERCLEANFILES =\
 $(BUILT_SOURCES)
