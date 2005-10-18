## Process this file with automake to produce Makefile.in

check-local dist-hook:
	if grep '@.*@' borlandc/config.h riscos/config.h ; then \
	  echo 'Unsubstituted token in src/*/config.h - something is amiss with configure.in'; \
	  exit 1; \
	fi

noinst_HEADERS = cavern.h commands.h cmdline.h datain.h debug.h filelist.h\
 filename.h getopt.h hash.h img.h ini.h listpos.h matrix.h message.h namecmp.h\
 netartic.h netbits.h netskel.h network.h osalloc.h osdepend.h ostypes.h out.h\
 readval.h str.h useful.h validate.h whichos.h\
 prbitmap.h prcore.h prio.h new3dout.h 3ddefs.h \
 armrot.h caverot.h cvrotgfx.h cvrotimg.h dosrot.h labels.h rotplot.h xrot.h\
 xcvrtbut.h\
 aboutdlg.h aven.h avenpal.h gfxcore.h mainfrm.h matrix4.h quaternion.h\
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

INCLUDES = -DIMG_HOSTED

PRINT = printdm printps printpcl printhpgl

# printxbm and dump3d are test programs
EXTRA_PROGRAMS =\
	caverot xcaverot\
	printwin aven dump3d printxbm editwrap
# FIXME: base_progs in top level Makefile.am needs updating if this is
bin_PROGRAMS = cavern $(PRINT) diffpos extend cad3d sorterr \
 3dtopos $(CAVEROT) $(AVEN) $(WINPROGS) $(TESTPROGS)
bin_SCRIPTS = svxedit

COMMONSRC = cmdline.c message.c filename.c osdepend.c z_getopt.c getopt1.c

cavern_SOURCES = cavern.c listpos.c commands.c datain.c netskel.c network.c \
 readval.c str.c matrix.c img.c netbits.c useful.c validate.c netartic.c \
 new3dout.c $(COMMONSRC)

aven_SOURCES = aven.cc img.c message.c filename.c osdepend.c \
 gfxcore.cc mainfrm.cc matrix4.cc quaternion.cc vector3.cc aboutdlg.cc \
 useful.c cmdline.c z_getopt.c getopt1.c namecmp.c aventreectrl.cc export.cc \
 avenprcore.cc printwx.cc ini.c hash.c

dump3d_SOURCES = dump3d.c img.c useful.c $(COMMONSRC)

editwrap_SOURCES = editwrap.c
editwrap_LDFLAGS = -mwindows

if WIN32
aven_LDADD = avenrc.o $(LIBOBJS) $(WX_LIBS)

svxeditrc.o: $(srcdir)/svxedit.rc svxedit.ico
	pwd=`pwd` && cd $(srcdir) && i586-mingw32msvc-windres -o "$$pwd/svxeditrc.o" svxedit.rc

svxedit.ico: ../lib/icons/svxedit.ico
	cp ../lib/icons/svxedit.ico .

editwrap_LDADD = svxeditrc.o

# FIXME: use path on WXCONFIG to get --include-dir?
avenrc.o: $(srcdir)/aven.rc aven.ico
	pwd=`pwd` && cd $(srcdir) && i586-mingw32msvc-windres --include-dir /opt/wxmsw/include --include-dir /usr/i586-mingw32msvc/include -o "$$pwd/avenrc.o" aven.rc

aven.ico: ../lib/icons/aven.ico
	cp ../lib/icons/aven.ico .

CLEANFILES = aven.ico svxedit.ico
else
aven_LDADD = $(LIBOBJS) $(WX_LIBS)
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
	`$(WXCONFIG) --rezflags` aven
	touch aven-rez

install-exec-hook:
	`$(WXCONFIG) --rezflags` $(bindir)/aven
else !MACOSX
AVEN_REZ =

# We need an empty target as automake will try to make it because it's in the
# other branch.
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

aven_CFLAGS = $(WX_CFLAGS) $(AM_CFLAGS) -DAVEN
aven_CXXFLAGS = $(WX_CXXFLAGS) $(AM_CXXFLAGS)
aven_LDFLAGS = $(WX_LDFLAGS)

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

EXTRA_DIST = \
 miniam.pl borlandc/makefile borlandc/config.h borlandc/config.h.in\
 riscos/makefile riscos/config.h riscos/config.h.in riscos/armrot.s\
 riscos/stdh/regnames riscos/stdh/swinames msvc/config.h msvc/config.h.in\
 aven.rc svxedit.rc getopt.c gettexttomsg.pl gradient.pov

BUILT_SOURCES = z_getopt.c avenpal.h

z_getopt.c: getopt.c gettexttomsg.pl
	./gettexttomsg.pl `test -f getopt.c || echo '$(srcdir)/'`getopt.c > z_getopt.c~ && mv z_getopt.c~ z_getopt.c

avenpal.h: $(srcdir)/gradient.pov
	$(srcdir)/gdtconvert < $(srcdir)/gradient.pov > tmp-avenpal.h && mv tmp-avenpal.h avenpal.h

MAINTAINERCLEANFILES =\
 borlandc/makefile riscos/makefile\
 $(BUILT_SOURCES)

borlandc/makefile: $(srcdir)/Makefile.am miniam.pl
	$(srcdir)/miniam.pl borlandc < $(srcdir)/Makefile.am > borlandc/makefile

riscos/makefile: $(srcdir)/Makefile.am miniam.pl
	$(srcdir)/miniam.pl riscos < $(srcdir)/Makefile.am > riscos/tmp-makefile && mv riscos/tmp-makefile riscos/makefile
