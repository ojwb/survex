/* cavern.c
 * SURVEX Cave surveying software: data reduction main and related functions
 * Copyright (C) 1991-2003 Olly Betts
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <limits.h>
#include <time.h>

#include "cavern.h"
#include "cmdline.h"
#include "commands.h"
#include "datain.h"
#include "debug.h"
#include "message.h"
#include "filename.h"
#include "filelist.h"
#include "img.h"
#include "listpos.h"
#include "netbits.h"
#include "netskel.h"
#include "osdepend.h"
#include "out.h"
#include "str.h"
#include "validate.h"

#ifdef CHASM3DX
# if OS != RISCOS
/* include header for getcwd() */
#  if OS == MSDOS && defined(__TURBOC__)
#   include <dir.h>
#  elif OS == WIN32 && defined(__VISUALC__)
#   include <direct.h>
#   include <conio.h>
#  else
#   include <unistd.h>
#  endif
#endif

#ifndef MAXPATHLEN
#define MAXPATHLEN 1024
#endif
#endif

/* For funcs which want to be immune from messing around with different
 * calling conventions */
#ifndef CDECL
# define CDECL
#endif

/* Globals */
node *stnlist = NULL;
settings *pcs;
prefix *root;
long cLegs, cStns;
long cComponents;
bool fExportUsed = fFalse;

FILE *fhErrStat = NULL;
img *pimg = NULL;
#ifndef NO_PERCENTAGE
bool fPercent = fFalse;
#endif
bool fQuiet = fFalse; /* just show brief summary + errors */
bool fMute = fFalse; /* just show errors */
bool fSuppress = fFalse; /* only output 3d(3dx) file */
static bool fLog = fFalse; /* stdout to .log file */
static bool f_warnings_are_errors = fFalse; /* turn warnings into errors */

nosurveylink *nosurveyhead;

real totadj, total, totplan, totvert;
real min[3], max[3];
prefix *pfxHi[3], *pfxLo[3];

char *survey_title = NULL;
int survey_title_len;

bool fExplicitTitle = fFalse;

char *fnm_output_base = NULL;
int fnm_output_base_is_dir = 0;

static void do_stats(void);

static const struct option long_opts[] = {
   /* const char *name; int has_arg (0 no_argument, 1 required_*, 2 optional_*); int *flag; int val; */
#ifdef NO_PERCENTAGE
   {"percentage", no_argument, 0, 0},
   {"no-percentage", no_argument, 0, 0},
#else
   {"percentage", no_argument, 0, 'p'},
   {"no-percentage", no_argument, 0, 3},
#endif
   {"output", required_argument, 0, 'o'},
   {"quiet", no_argument, 0, 'q'},
   {"no-auxiliary-files", no_argument, 0, 's'},
   {"warnings-are-errors", no_argument, 0, 'w'},
   {"log", no_argument, 0, 1},
#ifdef CHASM3DX
   {"chasm-format", no_argument, 0, 'x'},
#endif
#if (OS==WIN32)
   {"pause", no_argument, 0, 2},
#endif
   {"help", no_argument, 0, HLP_HELP},
   {"version", no_argument, 0, HLP_VERSION},
   {0, 0, 0, 0}
};

#ifdef CHASM3DX
#define short_opts "pxao:qswz:"
#else
#define short_opts "pao:qswz:"
#endif

/* TRANSLATE extract help messages to message file */
static struct help_msg help[] = {
/*				<-- */
   {HLP_ENCODELONG(0),          "display percentage progress"},
   {HLP_ENCODELONG(2),          "set location for output files"},
   {HLP_ENCODELONG(3),          "only show brief summary (-qq for errors only)"},
   {HLP_ENCODELONG(4),          "do not create .err file"},
   {HLP_ENCODELONG(5),          "turn warnings into errors"},
   {HLP_ENCODELONG(6),          "log output to .log file"},
#ifdef CHASM3DX
   {HLP_ENCODELONG(7),          "output data in chasm's 3dx format"},
#endif
 /*{'z',                        "set optimizations for network reduction"},*/
   {0, 0}
};

/* atexit functions */
static void
delete_output_on_error(void)
{
   if (msg_errors || (f_warnings_are_errors && msg_warnings))
      filename_delete_output();
}

#if (OS==WIN32)
static void
pause_on_exit(void)
{
   while (_kbhit()) _getch();
   _getch();
}
#endif

extern CDECL int
main(int argc, char **argv)
{
   int d;
   time_t tmUserStart = time(NULL);
   clock_t tmCPUStart = clock();
   init_screen();

   msg_init(argv);

   pcs = osnew(settings);
   pcs->next = NULL;
   pcs->Translate = ((short*) osmalloc(ossizeof(short) * 257)) + 1;
   pcs->meta = NULL;

   /* Set up root of prefix hierarchy */
   root = osnew(prefix);
   root->up = root->right = root->down = NULL;
   root->stn = NULL;
   root->pos = NULL;
   root->ident = NULL;
   root->min_export = root->max_export = 0;
   root->sflags = BIT(SFLAGS_SURVEY);
   root->filename = NULL;

   nosurveyhead = NULL;

   stnlist = NULL;
   cLegs = cStns = cComponents = 0;
   totadj = total = totplan = totvert = 0.0;

   for (d = 0; d <= 2; d++) {
      min[d] = HUGE_REAL;
      max[d] = -HUGE_REAL;
      pfxHi[d] = pfxLo[d] = NULL;
   }

   /* at least one argument must be given */
   cmdline_init(argc, argv, short_opts, long_opts, NULL, help, 1, -1);
   while (1) {
      int opt = cmdline_getopt();
      if (opt == EOF) break;
      switch (opt) {
       case 'p':
#ifndef NO_PERCENTAGE
	 fPercent = 1;
#endif
	 break;
#ifndef NO_PERCENTAGE
       case 3:
	 fPercent = 0;
	 break;
#endif
       case 'o': {
	 /* can be a directory (in which case use basename of leaf input)
	  * or a file (in which case just trim the extension off) */
	 if (fDirectory(optarg)) {
	    /* this is a little tricky - we need to note the path here,
	     * and then add the leaf later on (in datain.c) */
	    fnm_output_base = base_from_fnm(optarg);
	    fnm_output_base_is_dir = 1;
	 } else {
	    osfree(fnm_output_base); /* in case of multiple -o options */
	    fnm_output_base = base_from_fnm(optarg);
	 }
	 break;
       }
#ifdef CHASM3DX
       case 'x': {
	 fUseNewFormat = 1;
	 break;
       }
#endif
       case 'q':
	 if (fQuiet) fMute = 1;
	 fQuiet = 1;
	 break;
       case 's':
	 fSuppress = 1;
	 break;
       case 'w':
	 f_warnings_are_errors = 1;
	 break;
       case 'z': {
	 /* Control which network optimisations are used (development tool) */
	 static int first_opt_z = 1;
	 char c;
	 if (first_opt_z) {
	    optimize = 0;
	    first_opt_z = 0;
	 }
	 /* Lollipops, Parallel legs, Iterate mx, Delta* */
	 while ((c = *optarg++) != '\0')
	    if (islower(c)) optimize |= BITA(c);
	 break;
       case 1:
	 fLog = fTrue;
	 break;
#if (OS==WIN32)
       case 2:
	 atexit(pause_on_exit);
	 break;
#endif
       }
      }
   }

   if (fLog) {
      char *fnm;
      if (!fnm_output_base) {
	 char *p;
	 p = baseleaf_from_fnm(argv[optind]);
	 fnm = add_ext(p, EXT_LOG);
	 osfree(p);
      } else if (fnm_output_base_is_dir) {
	 char *p;
	 fnm = baseleaf_from_fnm(argv[optind]);
	 p = use_path(fnm_output_base, fnm);
	 osfree(fnm);
	 fnm = add_ext(p, EXT_LOG);
	 osfree(p);
      } else {
	 fnm = add_ext(fnm_output_base, EXT_LOG);
      }

      if (!freopen(fnm, "w", stdout))
	 fatalerror(/*Failed to open output file `%s'*/47, fnm);

      osfree(fnm);
   }

   if (!fMute)
      printf(PRETTYPACKAGE" "VERSION"\n"COPYRIGHT_MSG"\n", msg(/*&copy;*/0));

   atexit(delete_output_on_error);

   /* end of options, now process data files */
   while (argv[optind]) {
      const char *fnm = argv[optind];

      if (!fExplicitTitle) {
	 char *lf;
	 lf = baseleaf_from_fnm(fnm);
	 if (survey_title) s_catchar(&survey_title, &survey_title_len, ' ');
	 s_cat(&survey_title, &survey_title_len, lf);
	 osfree(lf);
      }

      /* Select defaults settings */
      default_all(pcs);
#ifdef CHASM3DX
      /* we need to get the filename of the first one for our base_source */
      /* and also run_file */
      if (fUseNewFormat) {
	 create_twig(root, fnm);
	 rhizome = root->twig_link;
	 limb = get_twig(root);
	 firstfilename = osstrdup(fnm);
	 startingdir = osmalloc(MAXPATHLEN);
#if (OS==RISCOS)
	 strcpy(startingdir, "@");
#else
	 getcwd(startingdir, MAXPATHLEN);
#endif
      }
#endif
      data_file(NULL, fnm); /* first argument is current path */

      optind++;
   }

   validate();

   solve_network(/*stnlist*/); /* Find coordinates of all points */
   validate();

#ifdef CHASM3DX
   if (fUseNewFormat) {
      /* this actually does all the writing */
      if (!cave_close(pimg)) {
	 char *fnm = add_ext(fnm_output_base, EXT_SVX_3DX);
	 fatalerror_in_file(fnm, 0, /*Error writing to file*/111);
      }
   } else {
#endif
      /* close .3d file */
      if (!img_close(pimg)) {
	 char *fnm = add_ext(fnm_output_base, EXT_SVX_3D);
	 fatalerror(img_error(), fnm);
      }
#ifdef CHASM3DX
   }
#endif
   if (fhErrStat) safe_fclose(fhErrStat);

   out_current_action(msg(/*Calculating statistics*/120));
   if (!fMute) do_stats();
   if (!fQuiet) {
      /* clock() typically wraps after 72 minutes, but there doesn't seem
       * to be a better way.  Still 72 minutes means some cave!
       * We detect if clock() could have wrapped and suppress CPU time
       * printing in this case.
       */
      double tmUser = difftime(time(NULL), tmUserStart);
      double tmCPU;
      clock_t now = clock();
#define CLOCK_T_WRAP \
	(sizeof(clock_t)<sizeof(long)?(1ul << (CHAR_BIT * sizeof(clock_t))):0)
      tmCPU = (now - (unsigned long)tmCPUStart)
	 / (double)CLOCKS_PER_SEC;
      if (now < tmCPUStart)
	 tmCPU += CLOCK_T_WRAP / (double)CLOCKS_PER_SEC;
      if (tmUser >= tmCPU + CLOCK_T_WRAP / (double)CLOCKS_PER_SEC)
	 tmCPU = 0;

      /* tmUser is integer, tmCPU not - equivalent to (ceil(tmCPU) >= tmUser) */
      if (tmCPU + 1 > tmUser) {
	 printf(msg(/*CPU time used %5.2fs*/140), tmCPU);
      } else if (tmCPU == 0) {
	 if (tmUser != 0.0) {
	    printf(msg(/*Time used %5.2fs*/141), tmUser);
	 } else {
	    fputs(msg(/*Time used unavailable*/142), stdout);
	 }
      } else {
	 printf(msg(/*Time used %5.2fs (%5.2fs CPU time)*/143), tmUser, tmCPU);
      }
      putnl();

      puts(msg(/*Done.*/144));
   }
   if (msg_warnings || msg_errors) {
      if (msg_errors || (f_warnings_are_errors && msg_warnings)) {
	 printf(msg(/*There were %d warning(s) and %d non-fatal error(s) - no output files produced.*/113),
		msg_warnings, msg_errors);
	 putnl();
	 return EXIT_FAILURE;
      }
      printf(msg(/*There were %d warning(s).*/16), msg_warnings);
      putnl();
   }
   return EXIT_SUCCESS;
}

static void
do_range(int d, int msg1, int msg2, int msg3)
{
   printf(msg(msg1), max[d] - min[d]);
   fprint_prefix(stdout, pfxHi[d]);
   printf(msg(msg2), max[d]);
   fprint_prefix(stdout, pfxLo[d]);
   printf(msg(msg3), min[d]);
   putnl();
}

static void
do_stats(void)
{
   long cLoops = cComponents + cLegs - cStns;

   putnl();

   if (cStns == 1) {
      fputs(msg(/*Survey contains 1 survey station,*/172), stdout);
   } else {
      printf(msg(/*Survey contains %ld survey stations,*/173), cStns);
   }

   if (cLegs == 1) {
      fputs(msg(/* joined by 1 leg.*/174), stdout);
   } else {
      printf(msg(/* joined by %ld legs.*/175), cLegs);
   }

   putnl();

   if (cLoops == 1) {
      fputs(msg(/*There is 1 loop.*/138), stdout);
   } else {
      printf(msg(/*There are %ld loops.*/139), cLoops);
   }

   putnl();

   if (cComponents != 1) {
      printf(msg(/*Survey has %ld connected components.*/178), cComponents);
      putnl();
   }

   printf(msg(/*Total length of survey legs = %7.2fm (%7.2fm adjusted)*/132),
	  total, totadj);
   putnl();
   printf(msg(/*Total plan length of survey legs = %7.2fm*/133),
	  totplan);
   putnl();
   printf(msg(/*Total vertical length of survey legs = %7.2fm*/134),
	  totvert);
   putnl();

   /* If there's no underground survey, we've no ranges */
   if (pfxHi[0]) {
      do_range(2, /*Vertical range = %4.2fm (from */135,
	       /* at %4.2fm to */136, /* at %4.2fm)*/137);
      do_range(1, /*North-South range = %4.2fm (from */148,
	       /* at %4.2fm to */196, /* at %4.2fm)*/197);
      do_range(0, /*East-West range = %4.2fm (from */149,
	       /* at %4.2fm to */196, /* at %4.2fm)*/197);
   }

   print_node_stats();
   /* Also, could give:
    *  # nodes stations (ie have other than two references or are fixed)
    *  # fixed stations (list of?)
    */
}
