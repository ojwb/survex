## Process this file with automake to produce Makefile.in

noinst_HEADERS = armrot.h cavern.h caverot.h commands.h cmdline.h\
 cvrotgfx.h cvrotimg.h datain.h debug.h dosrot.h filelist.h filename.h\
 getopt.h img.h ini.h labels.h listpos.h matrix.h message.h netartic.h\
 netbits.h netskel.h network.h osalloc.h osdepend.h ostypes.h out.h\
 prbitmap.h prcore.h prio.h readval.h rotplot.h str.h uni2dos.h useful.h\
 validate.h whichos.h xrot.h

LDADD = @LIBOBJS@

PRINT = printdm printps printpcl printhpgl

EXTRA_PROGRAMS = caverot xcaverot printxbm aven
bin_PROGRAMS = cavern $(PRINT) diffpos extend survex 3dtodxf @CAVEROT@ \
 @AVEN@

cavern_SOURCES = cavern.c message.c filename.c listpos.c commands.c\
 cmdline.c getopt.c getopt1.c datain.c netskel.c network.c readval.c str.c\
 matrix.c img.c osdepend.c netbits.c useful.c validate.c\
 netartic.c

survex_SOURCES = wrap.c osdepend.c

aven_SOURCES = aven.cc cvrotimg.c img.c useful.c osdepend.c filename.c\
 message.c
aven_LDADD = @LIBOBJS@ @GTKMM_LIBS@ -lgnomemm
aven_CFLAGS = @GTKMM_CFLAGS@
aven_CXXFLAGS = @GTKMM_CFLAGS@

PRSRC = prcore.c prio.c message.c filename.c useful.c img.c osdepend.c\
 ini.c getopt.c getopt1.c cmdline.c
 
printps_SOURCES = printps.c $(PRSRC)
printdm_SOURCES = printdm.c $(PRSRC) prbitmap.c
printpcl_SOURCES = printpcl.c $(PRSRC) prbitmap.c
printxbm_SOURCES = printxbm.c $(PRSRC) prbitmap.c
printhpgl_SOURCES = prhpgl.c $(PRSRC)

ROTSRC = cvrotimg.c img.c useful.c osdepend.c filename.c message.c

caverot_SOURCES = caverot.c cmdline.c $(ROTSRC) rotplot.c labels.c cvrotgfx.c
EXTRA_caverot_SOURCES = dosrot.c
caverot_LDADD = @LIBOBJS@ @CROBJX@ @CRLIB@
caverot_DEPENDENCIES = @CROBJX@

xcaverot_SOURCES = xcaverot.c cmdline.c $(ROTSRC)
xcaverot_CFLAGS = @X_CFLAGS@
xcaverot_LDADD = @LIBOBJS@ @X_LIBS@ @X_PRE_LIBS@ -lX11 @X_EXTRA_LIBS@

# next line is actually superfluous to automake (but riscos/bc hack needs it)
diffpos_SOURCES = diffpos.c cmdline.c
extend_SOURCES = extend.c message.c filename.c useful.c img.c osdepend.c\
 cmdline.c

3dtodxf_SOURCES = 3dtodxf.c message.c getopt.c getopt1.c img.c filename.c\
 useful.c osdepend.c cmdline.c

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

###########################################################################
# stuff below here is probably out-of-date and should probably be ignored #
###########################################################################

### Define customising flags here (eg -DNO_MOUSE_SUPPORT, -DISO8859_1)
## at start of command line
#UFLG = -DDEBUG_INVALID
## at end of command line when linking
#ULFLG =
##ULFLG = C:Asylum.o.AsylumLib

# There are a number of macros of the form NO_xxxx or HAVE_xxxx which allow
# for compilers or OS shortcomings.  Prefixes indicate:
#
#  * not really much use (eg not supported everywhere, or for debugging)
#  D option is ignored unless compiling for MS-DOS
#  - not currently used
#
# The current list is:
#
#    HAVE_DIFFTIME         - difftime() func, else just subtract times
#    HAVE_PERROR           - perror() available
#    HAVE_POPEN            - popen(), etc available
#    HAVE_SETJMP           - use setjmp
#    HAVE_SIGNAL           - compiler supports signals
#    HAVE_STRERROR         - strerror() available
#    HAVE_STRFTIME         - strftime() available
#
#  * NO_CLIP               - turns off clipping in printps (debug)
#    NO_FUNC_PTRS          - don't use fn ptrs (only used by caverot)
#                            [use this if fn ptrs won't fit in a coord]
#    NO_MOUSE_SUPPORT      - no mouse library available (ignored for RISC OS)
#    NO_PERCENTAGE         - permanently disable %age displays
# D -NO_STDPRN             - DOS compiler without stdprn
