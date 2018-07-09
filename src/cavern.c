/* cavern.c
 * SURVEX Cave surveying software: data reduction main and related functions
 * Copyright (C) 1991-2003,2004,2005,2010,2011,2013,2014,2015,2016,2017 Olly Betts
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define MSG_SETUP_PROJ_SEARCH_PATH 1

#include <limits.h>
#include <stdlib.h>
#include <time.h>

#include "cavern.h"
#include "cmdline.h"
#include "commands.h"
#include "date.h"
#include "datain.h"
#include "debug.h"
#include "message.h"
#include "filename.h"
#include "filelist.h"
#include "img_hosted.h"
#include "listpos.h"
#include "netbits.h"
#include "netskel.h"
#include "osdepend.h"
#include "out.h"
#include "str.h"
#include "validate.h"
#include "whichos.h"

#if OS_WIN32
# include <conio.h> /* for _kbhit() and _getch() */
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
prefix *anon_list = NULL;
long cLegs, cStns;
long cComponents;
bool fExportUsed = fFalse;
projPJ proj_out = NULL;
char * proj_str_out = NULL;

FILE *fhErrStat = NULL;
img *pimg = NULL;
bool fQuiet = fFalse; /* just show brief summary + errors */
bool fMute = fFalse; /* just show errors */
bool fSuppress = fFalse; /* only output 3d file */
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

lrudlist * model = NULL;
lrud ** next_lrud = NULL;

static void do_stats(void);

static const struct option long_opts[] = {
   /* const char *name; int has_arg (0 no_argument, 1 required_*, 2 optional_*); int *flag; int val; */
   {"percentage", no_argument, 0, 'p'},
   /* Ignore for compatibility with older versions. */
   {"no-percentage", no_argument, 0, 0},
   {"output", required_argument, 0, 'o'},
   {"quiet", no_argument, 0, 'q'},
   {"no-auxiliary-files", no_argument, 0, 's'},
   {"warnings-are-errors", no_argument, 0, 'w'},
   {"log", no_argument, 0, 1},
   {"3d-version", required_argument, 0, 'v'},
#if OS_WIN32
   {"pause", no_argument, 0, 2},
#endif
   {"help", no_argument, 0, HLP_HELP},
   {"version", no_argument, 0, HLP_VERSION},
   {0, 0, 0, 0}
};

#define short_opts "pao:qsv:wz:"

static struct help_msg help[] = {
/*				<-- */
   /* TRANSLATORS: --help output for cavern --output option */
   {HLP_ENCODELONG(2),	      /*set location for output files*/162, 0},
   /* TRANSLATORS: --help output for cavern --quiet option */
   {HLP_ENCODELONG(3),	      /*only show brief summary (-qq for errors only)*/163, 0},
   /* TRANSLATORS: --help output for cavern --no-auxiliary-files option */
   {HLP_ENCODELONG(4),	      /*do not create .err file*/164, 0},
   /* TRANSLATORS: --help output for cavern --warnings-are-errors option */
   {HLP_ENCODELONG(5),	      /*turn warnings into errors*/165, 0},
   /* TRANSLATORS: --help output for cavern --log option */
   {HLP_ENCODELONG(6),	      /*log output to .log file*/170, 0},
   /* TRANSLATORS: --help output for cavern --3d-version option */
   {HLP_ENCODELONG(7),	      /*specify the 3d file format version to output*/171, 0},
 /*{'z',			"set optimizations for network reduction"},*/
   {0, 0, 0}
};

/* atexit functions */
static void
delete_output_on_error(void)
{
   if (msg_errors || (f_warnings_are_errors && msg_warnings))
      filename_delete_output();
}

#if OS_WIN32
static void
pause_on_exit(void)
{
   while (_kbhit()) _getch();
   _getch();
}
#endif

int current_days_since_1900;

extern CDECL int
main(int argc, char **argv)
{
   int d;
   time_t tmUserStart = time(NULL);
   clock_t tmCPUStart = clock();
   {
       /* FIXME: localtime? */
       struct tm * t = localtime(&tmUserStart);
       int y = t->tm_year + 1900;
       current_days_since_1900 = days_since_1900(y, t->tm_mon + 1, t->tm_mday);
   }

   /* Always buffer by line for aven's benefit. */
   setvbuf(stdout, NULL, _IOLBF, 0);

   msg_init(argv);

   pcs = osnew(settings);
   pcs->next = NULL;
   pcs->Translate = ((short*) osmalloc(ossizeof(short) * 257)) + 1;
   pcs->meta = NULL;
   pcs->proj = NULL;
   pcs->declination = HUGE_REAL;
   pcs->convergence = 0.0;

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
	 /* Ignore for compatibility with older versions. */
	 break;
       case 'o': {
	 osfree(fnm_output_base); /* in case of multiple -o options */
	 /* can be a directory (in which case use basename of leaf input)
	  * or a file (in which case just trim the extension off) */
	 if (fDirectory(optarg)) {
	    /* this is a little tricky - we need to note the path here,
	     * and then add the leaf later on (in datain.c) */
	    fnm_output_base = base_from_fnm(optarg);
	    fnm_output_base_is_dir = 1;
	 } else {
	    fnm_output_base = base_from_fnm(optarg);
	 }
	 break;
       }
       case 'q':
	 if (fQuiet) fMute = 1;
	 fQuiet = 1;
	 break;
       case 's':
	 fSuppress = 1;
	 break;
       case 'v': {
	 int v = atoi(optarg);
	 if (v < IMG_VERSION_MIN || v > IMG_VERSION_MAX)
	    fatalerror(/*3d file format versions %d to %d supported*/88,
		       IMG_VERSION_MIN, IMG_VERSION_MAX);
	 img_output_version = v;
	 break;
       }
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
	    if (islower((unsigned char)c)) optimize |= BITA(c);
	 break;
       case 1:
	 fLog = fTrue;
	 break;
#if OS_WIN32
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
	 fatalerror(/*Failed to open output file “%s”*/47, fnm);

      osfree(fnm);
   }

   if (!fMute) {
      const char *p = COPYRIGHT_MSG;
      puts(PRETTYPACKAGE" "VERSION);
      while (1) {
	  const char *q = p;
	  p = strstr(p, "(C)");
	  if (p == NULL) {
	      puts(q);
	      break;
	  }
	  fwrite(q, 1, p - q, stdout);
	  fputs(msg(/*©*/0), stdout);
	  p += 3;
      }
   }

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
      data_file(NULL, fnm); /* first argument is current path */

      optind++;
   }

   validate();

   solve_network(/*stnlist*/); /* Find coordinates of all points */
   validate();

   /* close .3d file */
   if (!img_close(pimg)) {
      char *fnm = add_ext(fnm_output_base, EXT_SVX_3D);
      fatalerror(img_error2msg(img_error()), fnm);
   }
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
   }
   if (msg_warnings || msg_errors) {
      if (msg_errors || (f_warnings_are_errors && msg_warnings)) {
	 printf(msg(/*There were %d warning(s) and %d error(s) - no output files produced.*/113),
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
do_range(int d, int msgno, real length_factor, const char * units)
{
   /* sprint_prefix uses a single buffer, so to report two stations in one
    * message we need to make a temporary copy of the string for one of them.
    */
   char * pfx_hi = osstrdup(sprint_prefix(pfxHi[d]));
   char * pfx_lo = sprint_prefix(pfxLo[d]);
   real hi = max[d] * length_factor;
   real lo = min[d] * length_factor;
   printf(msg(msgno), hi - lo, units, pfx_hi, hi, units, pfx_lo, lo, units);
   osfree(pfx_hi);
   putnl();
}

static void
do_stats(void)
{
   long cLoops = cComponents + cLegs - cStns;
   int length_units = get_length_units(Q_LENGTH);
   const char * units = get_units_string(length_units);
   real length_factor = 1.0 / get_units_factor(length_units);

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
      /* TRANSLATORS: "Connected component" in the graph theory sense - it
       * means there are %ld bits of survey with no connections between them.
       * This message is only used if there are more than 1. */
      printf(msg(/*Survey has %ld connected components.*/178), cComponents);
      putnl();
   }

   printf(msg(/*Total length of survey legs = %7.2f%s (%7.2f%s adjusted)*/132),
	  total * length_factor, units, totadj * length_factor, units);
   putnl();
   printf(msg(/*Total plan length of survey legs = %7.2f%s*/133),
	  totplan * length_factor, units);
   putnl();
   printf(msg(/*Total vertical length of survey legs = %7.2f%s*/134),
	  totvert * length_factor, units);
   putnl();

   /* If there's no underground survey, we've no ranges */
   if (pfxHi[0]) {
      /* TRANSLATORS: numbers are altitudes of highest and lowest stations */
      do_range(2, /*Vertical range = %4.2f%s (from %s at %4.2f%s to %s at %4.2f%s)*/135,
	       length_factor, units);
      /* TRANSLATORS: c.f. previous message */
      do_range(1, /*North-South range = %4.2f%s (from %s at %4.2f%s to %s at %4.2f%s)*/136,
	       length_factor, units);
      /* TRANSLATORS: c.f. previous two messages */
      do_range(0, /*East-West range = %4.2f%s (from %s at %4.2f%s to %s at %4.2f%s)*/137,
	       length_factor, units);
   }

   print_node_stats();
   /* Also, could give:
    *  # nodes stations (ie have other than two references or are fixed)
    *  # fixed stations (list of?)
    */
}
