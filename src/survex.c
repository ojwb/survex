/* > survex.c
 * SURVEX Cave surveying software: main() and related functions
 * Copyright (C) 1991-1997 Olly Betts
 */

/*
1993.01.19 Recoded slightly to take advantage of indexing of delta, etc
           putw() renamed putword() to avoid clash with PC libraries
1993.01.20 source filenames <= 8 chars for PC compatibility
1993.01.25 PC Compatibility OKed
1993.01.26 DipProj.c -> DiplProj.c
           Timing code added
           FPE checked for (RISCOS)
           ELEV_ANG added
1993.01.27 cured non-ANSI #include warning on kernel.h (RISCOS version)
           minx,miny,minz & maxx,maxy,maxz -> min[3] & max[3]
1993.02.17 changed #ifdef RISCOS to #if OS=...
1993.02.17 added #define PRINT_STN_POS_LIST option (off)
1993.02.18 identing fiddled with a bit
           added NODESTAT
           removed list_pos() and NODESTAT to c.ListPos
1993.02.19 added #include Banner.c to print SURVEX big
           changed to #include Banner3D.c - 3D version of banner
           altered leg->to & leg->reverse to leg->l.to & leg->l.reverse
1993.02.23 now do exit(EXIT_FAILURE) or exit(EXIT_SUCCESS)
           fettled indentation
           added guessed UNIX code
           added #define NICE_HELLO
1993.02.24 pthData added
           PC init_screen() works
1993.02.26 ELEV_ANG and minw, maxw removed to SeanIMG.c
           separated off rotdata.c
           added message for when calculation finished
1993.02.28 more silly stats
1993.03.06 added filename for err stats (fnmErrs)
1993.03.10 moved filenames from seanimg.c
1993.03.12 recoded for new error.c
1993.03.17 moved some extern function prototypes into main()
1993.03.19 change from Sean image file to 3D image file
1993.04.05 de-extern-ed U2NT[] and NST[]
1993.04.06 changed included files to *.h for GCC's benefit
           removed #include "rotdata.h"
1993.04.07 Image file now leaf name - put in same dir as first data file
           Same with Error Stats file
           #error Don't ... -> #error Do not ... for GCC's happiness
1993.04.22 corrected error number -1 to 5 for data structure invalid
           out of memory -> error # 0
1993.04.30 moved expiry code to expire.h
1993.05.02 removed U2NT[] and NST[] defns to "svxmacro.c"
           removed putword() to useful.c & renamed put32()
1993.05.03 cmnd line options -A / -!A now produce ASCII / BINARY caverot file
           removed #define print_prefix ... to survex.h
1993.05.05 cosmetic
1993.05.13 moved some #defines here from image3d.h
           printf recoded as puts where appropriate
           'Depth' -> 'Vertical range'
1993.05.19 (W) added filelist.h to provide consistent filenames
1993.05.22 (W) changed pthOutput to try and be data dir (BODGED)
1993.05.23 unhacked the pthOutput bodge
1993.05.27 tidied a little
1993.06.04 removed OS specific stuff to common.h
1993.06.05 changed name from main.c to survex.c
1993.06.14 added stats on how many loops
1993.06.16 malloc() -> osmalloc()
1993.06.28 (MF) fettled fprint_prefix() to placate purify
           tried to correct # of loops statistic - okay if all connected
1993.07.19 "common.h" -> "osdepend.h"
1993.08.02 added #include "img.h"
1993.08.08 slight fettle
1993.08.09 altered data structure (OLD_NODES compiles previous version)
           tidied
1993.08.10 fiddling to get new version working
           turned of debug code
1993.08.10 'Mission completed' -> 'Done'
           image3d.h now produces binary .3d files too, so rotdata.h removed
           debugging flags moved to debug.h
           added more debug code
1993.08.11 NEW_NODES & POS(stn,d)
           FOR_EACH_STN( stn ) does for loop with stn looping thru' all nodes
1993.08.12 validate() is now a null macro if turned off
           added USED(station,leg) macro
1993.08.16 send survey statistics to file as well
           added message in pause that Wooks said was too long
           added EXT_SVX_STAT
1993.11.03 changed error routines
1993.11.06 removed DATEFORMAT, lenDateFormat and NAFFSVILLE
1993.11.08 moved ReadErrorFile call earlier
           improved plurals in messages
           only NEW_NODES code left
1993.11.18 check for output file being directory
1993.11.29 error 5 -> 11
1993.11.30 sorted out error vs fatal
1993.12.01 return error_summary(); instead of EXIT_SUCCESS
           cLegs and cStns are now longs
1993.12.05 removed all diplproj.h stuff; removed fBinary
1993.12.17 added sprint_prefix
1994.01.05 updated copyright to 1994
1994.03.19 moved # to first column
           fettled errors
1994.04.07 removed DRAW_CROSSES
1994.04.08 merged in image3d.h - changes:
>>1993.03.19 First attempt (based on 'SeanIMG.c')
>>1993.04.07 Added UsePth() call, so image file is put where first data file is
>>           #error Don't ... -> #error Do not ... for GCC's happiness
>>1993.04.26 Now output better title for survey
>>1993.05.03 corrected fopen() mode from "wt" to "w"
>>           fp -> fh
>>1993.05.13 moved some #defines to main.c
>>1993.07.07 (WP) added code to generate `name' entries
>>1993.08.02 altered to use img_* stuff
>>           recoded WP 'name' code to work with img_* stuff
>>1993.08.09 altered data structure (OLD_NODES compiles previous version)
>>1993.08.10 fettled to produce binary files if fBinary==fTrue
>>1993.08.11 NEW_NODES & POS(stn,d)
>>           FOR_EACH_STN( stn ) does for loop with stn looping thru' all nodes
>>1993.08.12 added USED(station,leg) macro
>>1993.08.15 standardised header
>>1993.08.16 output files can now be done with extensions
>>           added EXT_SVX_3D
>>1993.11.03 changed error routines
>>1993.11.30 missed error 0 - changed to error 10 for now - eliminate later
>>1993.12.05 fBinary -> !cs.fAscii
>>1993.12.17 altered to use sprint_prefix
>>1994.03.15 error 10 -> 38
>>1994.03.20 added img_error
>>1994.04.07 removed DRAW_CROSSES and only use img_LABEL now
1994.04.16 moved [sf]printf_prefix to netbits.c
1994.04.18 merged in banner3d.h
>>1993.02.19 Written; recoded as a single printf()
>>1993.04.25 prettified X a little
1994.04.18 added defaults.h commline.h network.h listpos.h
1994.04.27 fixed a few -fah -fussy warnings; lf<various> gone
1994.05.05 added a few global definitions
1994.05.08 now use fZero to check for equating legs
1994.05.12 added fPercent
1994.06.09 added SURVEXHOME
1994.06.21 added int argument to warning, error and fatal
1994.06.26 cs -> pcs
1994.08.13 commented out pthMe as it's not used here any more
1994.08.14 added #include "validate.h"
1994.08.16 fhErrStat made global so we can solve, add more data and resolve
1994.08.27 split off code into display_banner() and do_stats()
           reworked code to use out module
1994.08.29 added missing }
1994.08.31 fputnl()
1994.09.12 now output legs in traverses contiguously in .3d file
1994.09.14 added RISCOS_MALLOC_DISPLAY stuff
1994.09.20 global temporary buffer szOut created
           now give N-S and E-W ranges
1994.09.21 turned off traverse code, except when (OS==RISCOS); fputsnl()
1994.09.27 added RISCOS_LEAKFIND stuff
1994.09.28 internally defined NICE_HELLO -> makefile defined NO_NICEHELLO
1994.10.01 stnlist now passed to solve_network
           pimgOut added
1994.10.04 E-W and N-S ranges were swapped
1994.10.05 added language arg to ReadErrorFile
1994.10.08 added osnew()
1994.10.09 free stations and legs after solution
1994.10.14 made net stats global
1994.10.18 linked in my string.h debugging code
1994.10.27 stnHi, stnLo -> pfxHi, pfxLo ; made fhPosList global
1994.10.28 added cComponents
1994.12.03 fhPosList no longer global
           added component count if not one
1994.12.21 North-South range was using East-West range message
1995.01.15 removed comment suggesting component count be added (it has been)
1995.01.31 fixed range message code to allow different prepositions (German)
1995.03.25 fixed copyright dates; fettled; rejigged greetings code
1996.03.24 Translate now malloc-ed
1996.05.05 added CDECL
1996.05.12 recoded to remove ceil()
1997.02.01 moved fp_check_ok() call much earlier ; fettled spacing a bit
1997.06.03 converted DOS eols
1997.08.24 removed limit on survey station name length
1998.03.21 fixed up to compile cleanly on Linux
           expiry code turned off
*/

#include <time.h>

#include "debug.h"
#include "validate.h"
#include "survex.h"
#include "error.h"
#include "version.h"
#include "filelist.h"
#include "osdepend.h"
#include "img.h"
#include "netbits.h"
#include "defaults.h"
#include "commline.h"
#include "network.h"
#include "listpos.h"
#include "out.h"

/* For funcs which want to be immune from messing around with different
 * calling conventions */
#ifndef CDECL
#define CDECL
#endif

/* Globals */
node     *stnlist = NULL;
settings *pcs;
prefix   *root;
sz       pthOutput;
long     cLegs, cStns;
long     cComponents;
sz       fnmInput;
FILE     *fhErrStat = NULL;
img      *pimgOut = NULL;
#ifndef NO_PERCENTAGE
bool     fPercent = fTrue;
#endif
char     szOut[256];
real     totadj, total, totplan, totvert;
real     min[3], max[3];
prefix   *pfxHi[3], *pfxLo[3];

static clock_t  tmCPUStart;
static time_t   tmUserStart;
static double   tmCPU, tmUser;

static void display_banner(void);
static void do_stats( void );

/* Send malloc heap info to display program */
#ifdef RISCOS_MALLOC_DISPLAY
# include "HeapGraph.HeapGraph.h"
#endif

#ifdef RISCOS_LEAKFIND
# include "Mnemosyne.MnemoStubs.h"
#endif

extern CDECL int main(int argc, sz argv[]) {
   int d;

   check_fp_ok(); /* check very early on */

   tmUserStart = time( NULL );
   tmCPUStart = clock();
   init_screen();

   pthOutput = NULL;

   ReadErrorFile( "Survex", "SURVEXHOME", "SURVEXLANG", argv[0],
                  MESSAGE_FILE );

   display_banner();

/*#include "expire.h"*/

   pcs = osnew(settings);
   pcs->next = NULL;
   pcs->Translate = ((short*)osmalloc(ossizeof(short)*257))+1;

   /* Set up root of prefix hierarchy */
   root = osnew(prefix);
   root->up = root->right = root->down = NULL;
   root->stn = NULL;
   root->pos = NULL;
   root->ident = "\\";

   stnlist = NULL;
   cLegs = 0;
   cStns = 0;
   cComponents = 0;
   totadj = total = totplan = totvert = 0.0;

   for ( d=0 ; d<=2 ; d++ ) {
      min[d] = REAL_BIG;
      max[d] = -REAL_BIG;
      pfxHi[d] = pfxLo[d] = NULL;
   }

   /* Select defaults settings */
   default_truncate(pcs);
   default_ascii(pcs);
   default_90up(pcs);
   default_case(pcs);
   default_style(pcs);
   default_prefix(pcs);
   default_translate(pcs);
   default_grade(pcs);
   default_units(pcs);
   default_calib(pcs);

   process_command_line( argc, argv ); /* Read files and build network */
   validate();

   solve_network(/*stnlist*/); /* Find coordinates of all points */
   validate();
   img_close(pimgOut); /* close .3d file */
   fclose(fhErrStat);

   list_pos(root); /* produce .pos file */

   out_current_action(msg(120));
   do_stats();

   tmCPU = (clock_t)(clock() - tmCPUStart) / (double)CLOCKS_PER_SEC;
   tmUser = difftime(time( NULL ),tmUserStart);

#if 0
   /* use ceil because tmUser is integer no of seconds */
   if (ceil(tmCPU) >= tmUser) {
#else
   /* equivalent test, I think */
   if (tmCPU+1 > tmUser) {
#endif
      sprintf(szOut,msg(140),tmCPU);
      out_info(szOut);
   } else if (tmCPU == 0) {
      if (tmUser == 0.0) {
         sprintf( szOut, msg(141), tmUser );
         out_info(szOut);
      } else {
         out_info(msg(142));
      }
   } else {
      sprintf( szOut, msg(143), tmUser, tmCPU );
      out_info(szOut);
   }

   out_info(msg(144));
   return error_summary();
}

/* Display startup banner */
static void display_banner(void) {
   int msgGreet = 112;
   const char *szWelcomeTo;
#ifndef NO_NICEHELLO
   if (tmUserStart != (time_t)-1) {
      int hr;
      static uchar greetings[24] = {
         109,109,109,109, 109,109,109,109, 109,109,109,109,
         110,110,110,110, 110,110,111,111, 111,111,112,112 };
      hr = (localtime(&tmUserStart))->tm_hour;
      msgGreet = (int)greetings[hr];
   }
#endif
   szWelcomeTo = msgPerm(113);
   sprintf( szOut, "%s%s", msg(msgGreet), szWelcomeTo );
   out_info(szOut);
   msgFree(szWelcomeTo);
#ifndef NO_NICEHELLO
   out_info("  ______   __   __   ______    __   __   _______  ___   ___");
   out_info(" / ____|| | || | || |  __ \\\\  | || | || |  ____|| \\ \\\\ / //");
   out_info("( ((___   | || | || | ||_) )) | || | || | ||__     \\ \\/ //");
   out_info(" \\___ \\\\  | || | || |  _  //   \\ \\/ //  |  __||     >  <<");
   out_info(" ____) )) | ||_| || | ||\\ \\\\    \\  //   | ||____   / /\\ \\\\");
   out_info("|_____//   \\____//  |_|| \\_||    \\//    |______|| /_// \\_\\\\");
#else
   out_info("SURVEX");
#endif
   out_info("");
   sprintf( szOut, "%s "VERSION, msg(114) );
   out_info(szOut);
   out_info( "Copyright (C) 1991-"THIS_YEAR" Olly Betts" );
}

static void do_range( FILE *fh, int d, int msg1, int msg2, int msg3 ) {
   sprintf(szOut,msg(msg1),max[d]-min[d]);
   strcat(szOut,sprint_prefix(pfxHi[d]));
   sprintf(szOut+strlen(szOut),msg(msg2),max[d]);
   strcat(szOut,sprint_prefix(pfxLo[d]));
   sprintf(szOut+strlen(szOut),msg(msg3),min[d]);
   out_info(szOut);
   fputsnl(szOut,fh);
}

static void do_stats( void ) {
   sz fnm;
   FILE *fh;
   long cLoops = cComponents + cLegs - cStns;

#ifdef NO_EXTENSIONS
   fnm = UsePth( pthOutput, STATS_FILE );
#else
   fnm = AddExt( fnmInput, EXT_SVX_STAT );
#endif
   if (fDirectory(fnm))
      fatal(44,wr,fnm,0);
   if ((fh=fopen(fnm,"w")) == 0)
      fatal(47,wr,fnm,0);
   osfree(fnm);

   out_info("");

   if (cStns == 1)
      sprintf(szOut,msg(172));
   else
      sprintf(szOut,msg(173),cStns);
   if (cLegs == 1)
      sprintf(szOut+strlen(szOut),msg(174));
   else
      sprintf(szOut+strlen(szOut),msg(175),cLegs);
   out_info(szOut); fputsnl(szOut,fh);

   if (cLoops == 1)
      sprintf(szOut,msg(138));
   else
      sprintf(szOut,msg(139),cLoops);
   out_info(szOut); fputsnl(szOut,fh);

   if (cComponents != 1) {
      sprintf(szOut,msg(178),cComponents);
      out_info(szOut); fputsnl(szOut,fh);
   }

   sprintf(szOut,msg(132),total,totadj);
   out_info(szOut); fputsnl(szOut,fh);

   sprintf(szOut,msg(133),totplan);
   out_info(szOut); fputsnl(szOut,fh);

   sprintf(szOut,msg(134),totvert);
   out_info(szOut); fputsnl(szOut,fh);

   do_range(fh,2,135,136,137);
   do_range(fh,1,148,196,197);
   do_range(fh,0,149,196,197);

   print_node_stats(fh);
   /* Also, could give:
    *  # nodes stations (ie have other than two references or are fixed)
    *  # fixed stations (list of?)
    */
   fclose(fh);
}
