## Process this file with automake to produce Makefile.in

INCLUDES = -DSURVEXHOME=\"$(pkgdatadir)\"

noinst_HEADERS = armrot.h cavern.h caverot.h commands.h cmdline.h\
 cvrotgfx.h cvrotimg.h datain.h debug.h dosrot.h filelist.h filename.h\
 getopt.h img.h ini.h labels.h listpos.h matrix.h message.h netbits.h\
 network.h osalloc.h osdepend.h ostypes.h out.h prbitmap.h prcore.h\
 prio.h readval.h rotplot.h str.h useful.h validate.h whichos.h xrot.h\
 uni2dos.h

PRINT = printdm printps printpcl printhpgl

EXTRA_PROGRAMS = caverot xcaverot printxbm
bin_PROGRAMS = cavern $(PRINT) diffpos extend @CAVEROT@ survex

cavern_SOURCES = cavern.c message.c filename.c listpos.c commands.c\
 cmdline.c getopt.c getopt1.c datain.c network.c readval.c str.c\
 matrix.c img.c osdepend.c svxmacro.c netbits.c useful.c validate.c

survex_SOURCES = wrap.c osdepend.c

PRSRC = prcore.c prio.c message.c filename.c useful.c img.c osdepend.c\
 ini.c getopt.c getopt1.c cmdline.c
 
printps_SOURCES = printps.c $(PRSRC)
printdm_SOURCES = printdm.c $(PRSRC) prbitmap.c
printpcl_SOURCES = printpcl.c $(PRSRC) prbitmap.c
printxbm_SOURCES = printxbm.c $(PRSRC) prbitmap.c
printhpgl_SOURCES = prhpgl.c $(PRSRC)
printps_LDADD = @LIBOBJS@
printdm_LDADD = @LIBOBJS@
printpcl_LDADD = @LIBOBJS@
printxbm_LDADD = @LIBOBJS@
printhpgl_LDADD = @LIBOBJS@

#printmd0_SOURCES = printmd0.c prbitmap.c prcore.c message.c filename.c\
# useful.c img.c osdepend.c

ROTSRC = cvrotimg.c img.c useful.c osdepend.c filename.c message.c

caverot_SOURCES = caverot.c $(ROTSRC) rotplot.c labels.c cvrotgfx.c
EXTRA_caverot_SOURCES = dosrot.c
caverot_LDADD = @CROBJX@ @CRLIB@
caverot_DEPENDENCIES = @CROBJX@

xcaverot_SOURCES = xcaverot.c $(ROTSRC)
xcaverot_LDADD = @X_LIBS@ @X_PRE_LIBS@ -lX11 @X_EXTRA_LIBS@

# next line is actually superfluous to automake (but riscos/bc hack needs it)
diffpos_SOURCES = diffpos.c
extend_SOURCES = extend.c message.c filename.c useful.c img.c osdepend.c

#3dtodxf_SOURCES = 3dtodxf.c
#3dtodxf_LDADD = imgalone.o

#testerr_SOURCES = testerr.c message.c filename.c useful.c osdepend.c
#imgtest_SOURCES = imgtest.c imgalone.c

# this doesn't work if compiler doesn't support -c and -o, or with
# deANSIfication
# Use imgalone.c containing '#define STANDALONE/#include "img.c"'?
#imgalone.o: img.c
#        $(COMPILE) -DSTANDALONE -o imgalone.o -c img.c

VERSION = @VERSION@

### Define customising flags here (eg -DNO_MOUSE_SUPPORT, -DISO8859_1)
## at start of command line
#UFLG = -DDEBUG_INVALID -DNO_COVARIANCES
## at end of command line when linking
#ULFLG =
##ULFLG = C:Asylum.o.AsylumLib

############################################################################
# In an ideal world, you shouldn't need to change anything below here      #
# Let me know if you do - Olly                                             #
############################################################################

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
#    HAVE_STRCASECMP       - strcasecmp() available
#    HAVE_STRERROR         - strerror() available
#    HAVE_STRFTIME         - strftime() available
#
#  * NO_CLIP               - turns off clipping in printps (debug)
#    NO_EXTENSIONS         - no filename extensions (output from survex)
#  *-NO_FGETPOS            - fgetpos()/fsetpos() not available
# D  NO_FLAT_DOS           - we've gotta fight DOS's idea of a memory map
#    NO_FUNC_PTRS          - don't use fn ptrs (only used by caverot)
#                            [use this if fn ptrs won't fit in a coord]
#    NO_MOUSE_SUPPORT      - no mouse library available (ignored for RISC OS)
#    NO_NICEHELLO          - turn off "Good afternoon" and big SURVEX
#    NO_PERCENTAGE         - permanently disable %age displays
# D -NO_STDPRN             - DOS compiler without stdprn
#    NO_TEX                - don't use TeX style accented chars in messages
#
# If you're porting, don't define any to start with, and define them as
# needed, using the compiler and linker errors for guidance.

#BUILT_SOURCES = makefile.bc makefile.ro

noinst_DATA = borlandc/makefile riscos/makefile
 
EXTRA_DIST = borlandc/makefile borlandc/config.h\
 riscos/makefile riscos/config.h riscos/armrot.s\
 riscos/stdh/regnames riscos/stdh/swinames

borlandc/makefile: Makefile.am miniam.pl
	./miniam.pl borlandc Makefile.am > borlandc/makefile

riscos/makefile: Makefile.am miniam.pl
	./miniam.pl riscos Makefile.am > riscos/makefile
