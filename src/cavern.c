/* cavern.c
 * SURVEX Cave surveying software: data reduction main and related functions
 * Copyright (C) 1991-2025 Olly Betts
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

#include <config.h>

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
#include "img_for_survex.h"
#include "listpos.h"
#include "netbits.h"
#include "netskel.h"
#include "osalloc.h"
#include "out.h"
#include "str.h"
#include "validate.h"

#ifdef _WIN32
# include <conio.h> /* for _kbhit() and _getch() */
#endif

/* Globals */
node *fixedlist = NULL; // Fixed points
node *stnlist = NULL; // Unfixed stations
settings *pcs;
prefix *root;
prefix *anon_list = NULL;
long cLegs = 0, cStns = 0;
long cComponents = 0;
long cSolves = 0;
bool fExportUsed = false;
char * proj_str_out = NULL;
PJ * pj_cached = NULL;

FILE *fhErrStat = NULL;
img *pimg = NULL;
int quiet = 0; // 1 to turn off progress messages; >=2 turns off summary too.
bool fSuppress = false; /* only output 3d file */
static bool fLog = false; /* stdout to .log file */
static bool f_warnings_are_errors = false; /* turn warnings into errors */

nosurveylink *nosurveyhead;

real totadj, total, totplan, totvert;
real min[9], max[9];
prefix *pfxHi[9], *pfxLo[9];

string survey_title = S_INIT;

bool fExplicitTitle = false;

char *fnm_output_base = NULL;
bool fnm_output_base_is_dir = false;

lrudlist * model = NULL;
lrud ** next_lrud = NULL;

char output_separator = '.';

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
#ifdef _WIN32
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
   {HLP_ENCODELONG(2),	      /*set location for output files*/162, 0, 0},
   /* TRANSLATORS: --help output for cavern --quiet option */
   {HLP_ENCODELONG(3),	      /*fewer messages (-qq for even fewer)*/163, 0, 0},
   /* TRANSLATORS: --help output for cavern --no-auxiliary-files option */
   {HLP_ENCODELONG(4),	      /*do not create .err file*/164, 0, 0},
   /* TRANSLATORS: --help output for cavern --warnings-are-errors option */
   {HLP_ENCODELONG(5),	      /*turn warnings into errors*/165, 0, 0},
   /* TRANSLATORS: --help output for cavern --log option */
   {HLP_ENCODELONG(6),	      /*log output to .log file*/170, 0, 0},
   /* TRANSLATORS: --help output for cavern --3d-version option */
   {HLP_ENCODELONG(7),	      /*specify the 3d file format version to output*/171, 0, 0},
 /*{'z',			"set optimizations for network reduction"},*/
   {0, 0, 0, 0}
};

/* atexit functions */
static void
delete_output_on_error(void)
{
   if (msg_errors || (f_warnings_are_errors && msg_warnings))
      filename_delete_output();
}

#ifdef _WIN32
static void
pause_on_exit(void)
{
   while (_kbhit()) _getch();
   _getch();
}
#endif

int current_days_since_1900;
unsigned current_year;

static void discarding_proj_logger(void *ctx, int level, const char *message) {
    (void)ctx;
    (void)level;
    (void)message;
}

extern int
main(int argc, char **argv)
{
   int d;
   time_t tmUserStart = time(NULL);
   clock_t tmCPUStart = clock();
   {
       // Convert the current date in the local timezone to the number of days
       // since 1900 which we use to warn if a `*date` command specifies a date
       // in the future.
       struct tm * t = localtime(&tmUserStart);
       int y = t->tm_year + 1900;
       current_year = (unsigned)y;
       current_days_since_1900 = days_since_1900(y, t->tm_mon + 1, t->tm_mday);
   }

   /* Always buffer by line for aven's benefit. */
   setvbuf(stdout, NULL, _IOLBF, 0);

   /* Prevent stderr spew from PROJ. */
   proj_log_func(PJ_DEFAULT_CTX, NULL, discarding_proj_logger);

   msg_init(argv);

   pcs = osnew(settings);
   pcs->next = NULL;
   pcs->from_equals_to_is_only_a_warning = false;
   pcs->Translate = ((short*) osmalloc(sizeof(short) * 257)) + 1;
   pcs->meta = NULL;
   pcs->proj_str = NULL;
   pcs->declination = HUGE_REAL;
   pcs->convergence = HUGE_REAL;
   pcs->input_convergence = HUGE_REAL;
   pcs->dec_filename = NULL;
   pcs->dec_line = 0;
   pcs->dec_context = NULL;
   pcs->dec_lat = HUGE_VAL;
   pcs->dec_lon = HUGE_VAL;
   pcs->dec_alt = HUGE_VAL;
   pcs->min_declination = HUGE_VAL;
   pcs->max_declination = -HUGE_VAL;
   pcs->cartesian_north = TRUE_NORTH;
   pcs->cartesian_rotation = 0.0;

   /* Set up root of prefix hierarchy */
   root = osnew(prefix);
   root->up = root->right = root->down = NULL;
   root->stn = NULL;
   root->pos = NULL;
   root->ident.p = NULL;
   root->min_export = root->max_export = 0;
   root->sflags = BIT(SFLAGS_SURVEY);
   root->filename = NULL;

   nosurveyhead = NULL;

   fixedlist = NULL;
   stnlist = NULL;
   totadj = total = totplan = totvert = 0.0;

   for (d = 0; d < 9; d++) {
      min[d] = HUGE_REAL;
      max[d] = -HUGE_REAL;
      pfxHi[d] = pfxLo[d] = NULL;
   }

   // TRANSLATORS: Here "survey" is a "cave map" rather than list of questions
   // - it should be translated to the terminology that cavers using the
   // language would use.
   //
   // Part of cavern --help
   cmdline_set_syntax_message(/*[SURVEY_DATA_FILE]*/507, 0, NULL);
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
	 free(fnm_output_base); /* in case of multiple -o options */
	 /* can be a directory (in which case use basename of leaf input)
	  * or a file (in which case just trim the extension off) */
	 if (fDirectory(optarg)) {
	    /* this is a little tricky - we need to note the path here,
	     * and then add the leaf later on (in datain.c) */
	    fnm_output_base = base_from_fnm(optarg);
	    fnm_output_base_is_dir = true;
	 } else {
	    fnm_output_base = base_from_fnm(optarg);
	 }
	 break;
       }
       case 'q':
	 ++quiet;
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
	 static bool seen_opt_z = false;
	 char c;
	 if (!seen_opt_z) {
	    optimize = 0;
	    seen_opt_z = true;
	 }
	 /* Lollipops, Parallel legs, Iterate mx, Delta* */
	 while ((c = *optarg++) != '\0')
	    if (islower((unsigned char)c)) optimize |= BITA(c);
	 break;
       case 1:
	 fLog = true;
	 break;
#ifdef _WIN32
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
	 free(p);
      } else if (fnm_output_base_is_dir) {
	 char *p;
	 fnm = baseleaf_from_fnm(argv[optind]);
	 p = use_path(fnm_output_base, fnm);
	 free(fnm);
	 fnm = add_ext(p, EXT_LOG);
	 free(p);
      } else {
	 fnm = add_ext(fnm_output_base, EXT_LOG);
      }

      if (!freopen(fnm, "w", stdout))
	 fatalerror(/*Failed to open output file “%s”*/3, fnm);

      free(fnm);
   }

   if (quiet < 2) {
      const char *p = COPYRIGHT_MSG;
      puts(PRETTYPACKAGE" "VERSION);
      while (1) {
	  const char *q = p;
	  p = strstr(p, "(C)");
	  if (p == NULL) {
	      puts(q);
	      break;
	  }
	  FWRITE_(q, 1, p - q, stdout);
	  fputs(msg(/*©*/0), stdout);
	  p += 3;
      }
   }

   atexit(delete_output_on_error);

   /* end of options, now process data files */
   while (argv[optind]) {
      const char *fnm = argv[optind];

      if (!fExplicitTitle) {
	  char *lf = baseleaf_from_fnm(fnm);
	  if (s_empty(&survey_title)) {
	      s_donate(&survey_title, lf);
	  } else {
	      s_appendch(&survey_title, ' ');
	      s_append(&survey_title, lf);
	      free(lf);
	  }
      }

      /* Select defaults settings */
      default_all(pcs);
      data_file(NULL, fnm); /* first argument is current path */

      optind++;
   }

   validate();

   report_declination(pcs);

   solve_network(); /* Find coordinates of all points */
   validate();

   check_for_unused_fixed_points();

   /* close .3d file */
   if (!img_close(pimg)) {
      char *fnm = add_ext(fnm_output_base, EXT_SVX_3D);
      fatalerror(img_error2msg(img_error()), fnm);
   }
   if (fhErrStat) safe_fclose(fhErrStat);

   out_current_action(msg(/*Calculating statistics*/120));
   if (quiet < 2) do_stats();
   if (!quiet) {
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
      putnl();
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
   if (d < 3) {
      /* If the bound including anonymous stations is at an anonymous station
       * but the bound only considering named stations is the same, use the
       * named station for the anonymous bound too.
       */
      if (TSTBIT(pfxHi[d]->sflags, SFLAGS_ANON) && max[d] == max[d + 3]) {
	 pfxHi[d] = pfxHi[d + 3];
      }
      if (TSTBIT(pfxLo[d]->sflags, SFLAGS_ANON) && min[d] == min[d + 3]) {
	 pfxLo[d] = pfxLo[d + 3];
      }
   }

   /* sprint_prefix uses a single buffer, so to report two stations in one
    * message we need to make a temporary copy of the string for one of them.
    */
   char * pfx_hi = osstrdup(sprint_prefix(pfxHi[d]));
   char * pfx_lo = sprint_prefix(pfxLo[d]);
   real hi = max[d] * length_factor;
   real lo = min[d] * length_factor;
   printf(msg(msgno), hi - lo, units, pfx_hi, hi, units, pfx_lo, lo, units);
   free(pfx_hi);
   putnl();

   /* Range without anonymous stations at offset 3. */
   if (d < 3 && (pfxHi[d] != pfxHi[d + 3] || pfxLo[d] != pfxLo[d + 3])) {
      do_range(d + 3, msgno, length_factor, units);
   }
}

static void
do_stats(void)
{
   putnl();

   if (proj_str_out) {
       prefix *name_min = NULL;
       prefix *name_max = NULL;
       double convergence_min = HUGE_VAL;
       double convergence_max = -HUGE_VAL;
       prefix **pfx = pfxLo;
       for (int bound = 0; bound < 2; ++bound) {
	   for (int d = 6; d < 9; ++d) {
	       if (pfx[d]) {
		   pos *p = pfx[d]->pos;
		   double convergence = calculate_convergence_xy(proj_str_out,
								 p->p[0],
								 p->p[1],
								 p->p[2]);
		   if (convergence < convergence_min) {
		       convergence_min = convergence;
		       name_min = pfx[d];
		   }
		   if (convergence > convergence_max) {
		       convergence_max = convergence;
		       name_max = pfx[d];
		   }
	       }
	   }
	   pfx = pfxHi;
       }
       if (name_min && name_max) {
	   const char* deg_sign = msg(/*°*/344);
	   /* sprint_prefix uses a single buffer, so to report two stations in
	    * one message we need to make a temporary copy of the string for
	    * one of them.
	    */
	   char *pfx_hi = osstrdup(sprint_prefix(name_max));
	   char *pfx_lo = sprint_prefix(name_min);
	   // TRANSLATORS: Cavern computes the grid convergence at the
	   // representative location(s) specified by the
	   // `*declination auto` command(s).  The convergence values
	   // for the most N, S, E and W survey stations with legs
	   // attached are also computed and the range of these values
	   // is reported in this message.  It's approximate because the
	   // min or max convergence could actually be beyond this range
	   // but it's unlikely to be very wrong.
	   //
	   // Each %.1f%s will be replaced with a convergence angle (e.g.
	   // 0.9°) and the following %s with the station name where that
	   // convergence angle was computed.
	   printf(msg(/*Approximate full range of grid convergence: %.1f%s at %s to %.1f%s at %s\n*/531),
		  deg(convergence_min), deg_sign, pfx_lo,
		  deg(convergence_max), deg_sign, pfx_hi);
	   free(pfx_hi);
       }
   }

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

   if (cSolves == 1) {
       // If *solve is used then cComponents will often be wrong.  Rather than
       // reporting incorrect counts of loops and components we omit these in
       // this case for now.
       long cLoops = cComponents + cLegs - cStns;
       if (cLoops == 1) {
	  fputs(msg(/*There is 1 loop.*/138), stdout);
       } else {
	  printf(msg(/*There are %ld loops.*/139), cLoops);
       }
       putnl();

       if (cComponents != 1) {
	  /* TRANSLATORS: "Connected component" in the graph theory sense - it
	   * means there are %ld bits of survey with no connections between
	   * them.  This message is only used if there are more than 1. */
	  printf(msg(/*Survey has %ld connected components.*/178), cComponents);
	  putnl();
       }
   }

   int length_units = get_length_units(Q_LENGTH);
   const char * units = get_units_string(length_units);
   real length_factor = 1.0 / get_units_factor(length_units);

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

   check_node_stats();
}
