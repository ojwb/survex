/* > commline.c
 * Command line processing for Survex data reduction program
 * Copyright (C) 1991-1997 Olly Betts
 */

/* support some common old style command line options */
#define COMPAT 1

/* we now bundled GNU getopt so we always have this */
#define HAVE_GETOPT_LONG 1

/*
1992.07.20 Fiddled with filename handling bits
1992.10.25 Minor tidying
1993.01.20ish Corrected fopen() mode to "rb" from "rt" (was retimestamping)
1993.01.25 PC Compatibility OKed
1993.02.24 pthData added
1993.02.28 changed to -!X to turn option X off ( & -!C, -CU, -CL )
           indentation prettied up
1993.04.05 (W) added pthData hack
1993.04.06 added case COMMAND: to switch statements to suppress GCC warning
1993.04.07 added catch for no command line arguments
1993.04.08 de-octaled numbers
1993.04.22 out of memory -> error # 0
1993.04.23 added path & extension gubbins & removed extern sz pthData
1993.04.26 hacked on survey pseudo-title code (nasty)
1993.05.03 settings.Ascii -> swttings.fAscii
1993.05.20-ish (W) added #include "filelist.h"
1993.05.23 now use path from .svc file for output if no other path found yet
1993.05.27 fixed above so it'll compile
1993.06.04 FALSE -> fFalse
1993.06.11 fixed -U option so that multiple digit numbers should work
1993.06.16 malloc() -> osmalloc(); syserr() -> fatal(); free() -> osfree()
1993.06.28 changed (!pthOutput) to (pthOutput==NULL)
           command_file() now osfree()s strings returned by PthFromFnm()
1993.08.06 changed 'c.CommLine' -> commline.c in error 'where' strings
1993.08.13 fettled header
1993.08.16 fixed code dealing with paths of included files (.svx & .svc)
           added fnmInput
           command line files read in text mode (no null-padding problems)
1993.08.21 fixed minor bug (does nested stuff work now?)
1993.09.23 (W) replaced errors 25 & 26 with identical 1 & 2
1993.10.24 changed OptionChars to more sensible SWITCH_SYMBOLS
           fettled a little
1993.11.03 changed error routines
1993.11.29 added @ <fname> to mean the same as -f <fname>
           errors 1-3 -> 18-20; 23->26 ; 24->23
1993.11.30 sorted out error vs fatal
1994.01.05 added workaround for msc (probable) bug in toupper(*(sz++))
1994.01.17 toupper(*(sz++)) bug is because msc's ctype.h defines it as a
           macro if not in ANSI mode - recoded to be friendly to non-ANSI ccs
1994.03.14 altered fopenWithPthAndExt to give filename actually used
1994.03.15 moved typedef ... Mode; here from survex.h
           added -D and -T switches
           copes with directory extensions now
1994.03.16 extracted UsingDataFile()
1994.03.19 error output all to stderr
1994.04.06 removed unused fQuote
1994.04.16 added -O command line switch
1994.04.17 fixed bug in -O command line switch; added -OI
1994.04.18 created commline.h
1994.05.12 adjusted -O switches; added -P to control percentages
1994.06.20 reordered function to suppress compiler warning
1994.06.21 added int argument to warning, error and fatal
1994.06.26 cs -> pcs; pcs->next used instead of braces_stack
1994.06.27 split off push() and pop() so we can use them from BEGIN and END
1994.08.27 removed all explicit calls to printf, puts, etc
1994.09.13 a change so small even diff might miss it
1994.10.08 added osnew
1994.11.13 added settings.fOrderingFreeable
           rearranged command line options code to make it almost readable
1994.11.14 mode is now static, external to any functions
1994.12.03 kludged fix for multi-word titles
1994.12.11 -f "Name with spaces" should work on command line now
1995.01.21 fixed bug Tim Long reported with -T not absorbing titles
1995.01.31 removed unused message note and moved comment
1996.03.24 changes for Translate table ; buffer -> cmdbuf
1997.08.24 removed limit on survey station name length
1998.03.21 fixed up to compile cleanly on Linux
1998.06.09 fettled layout in preparation for an overhaul
*/

#include <limits.h>

#include "getopt.h"

#include "cavein.h"
#include "filename.h"
#include "message.h"
#include "filelist.h"
#include "datain.h"
#include "commline.h"

static void process_command( const char *string, const char *pth );

#define TITLE_LEN 80
char szSurveyTitle[TITLE_LEN];
bool fExplicitTitle = fFalse;

static void datafile(const char *fnm, const char *pth) {
   char *lf;
   lf = LfFromFnm(fnm);
   if (!fExplicitTitle && strlen(lf)+strlen(szSurveyTitle)+2 <= TITLE_LEN) {
      /* !HACK! should cope better with exceeding title buffer */
      char *p = szSurveyTitle;
      if (*p) {
         /* If there's already a file in the list */
         p += strlen(szSurveyTitle);
	 *p++ = ' ';
      }
      strcpy(p,lf);
   }
   free(lf);
   /* default path is path of command line file (if we're in one) */
   data_file(pth, fnm);
   return;
}

static void display_syntax_message(const char *x, int y) {
   /* avoid warnings about x and y unused */
   x = x;
   y = y;
   printf("%s: %s [-a] [-p] [<.svx file> ...]\n", msg(49), szAppNameCopy);
}

extern void process_command_line(int argc, char **argv) {
#ifdef HAVE_GETOPT_LONG
   static const struct option opts[] = {
      /* const char *name; int has_arg (0 no_argument, 1 required_*, 2 optional_*); int *flag; int val; */
#ifdef NO_PERCENTAGE
      "percentage", no_argument, 0, 0,
      "no-percentage", no_argument, 0, 0,
#else
      "percentage", no_argument, (int*)&fPercent, 1, /* or 0, 'p' */
      "no-percentage", no_argument, (int*)&fPercent, 0,
#endif
      0, 0, 0, 0
   };
#endif

   *szSurveyTitle = '\0';

   /* No arguments given */
   if (argc < 2) fatal(49, display_syntax_message, NULL, 0);

   while (1) {
      extern long optimize; /* defined in network.c */
#ifdef HAVE_GETOPT_LONG
# ifdef COMPAT
      int opt = getopt_long(argc, argv, "!:Ppao:", opts, NULL);
# else
      int opt = getopt_long(argc, argv, "pao:", opts, NULL);
# endif
#else
# ifdef COMPAT
      int opt = getopt(argc, argv, "!:Ppao:");
# else
      int opt = getopt(argc, argv, "pao:");
# endif
#endif
      switch (opt) {
#ifdef COMPAT
       /* for back compat allow "-!P" "-!p" & "-P" and ignore other "-!<X>" */
       case '!':
# ifndef NO_PERCENTAGE
	 if (tolower(optarg[0]) == 'p') fPercent = 0;
# endif
	 break;
#endif
       case EOF:
	 /* end of options, now process data files */
	 while (argv[optind]) {
	    datafile(argv[optind], "");
	    optind++;
	 }
	 return;
       case 0:
	 break; /* long option processed - ignore */
	 /* no params, so param can't be missing (!)       case ':' */
       case '?':
	 /* FIXME unknown option */
	 /* or ambiguous match or extraneous param */
	 fatal(49, display_syntax_message, NULL, 0);
	 break;
       case 'a':
	 pcs->fAscii = fTrue;
	 break;
       case 'p':
#ifdef COMPAT
       case 'P':
#endif
#ifndef NO_PERCENTAGE
	 fPercent = 1;
#endif
	 break;
       case 'o': {/* Optimizations level used to solve network */
	 static int first_opt_o = 0;
	 int ch;
	 if (first_opt_o) {
	    optimize = 0;
	    first_opt_o = 1;
	 }
	 /* Lollipops, Parallel legs, Iterate mx, Delta*, Split stnlist */
	 while ((ch = *optarg++)) if (islower(ch)) optimize |= BIT(ch - 'a');
	 break;
       }
      }
   }
}
