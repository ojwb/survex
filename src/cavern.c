/* > cavern.c
 * SURVEX Cave surveying software: data reduction main and related functions
 * Copyright (C) 1991-2000 Olly Betts
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

#ifdef NEW3DFORMAT
#include "new3dout.h"
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

FILE *fhErrStat = NULL;
img *pimgOut = NULL;
#ifndef NO_PERCENTAGE
bool fPercent = fFalse;
#endif
bool fAscii = fFalse;
bool fQuiet = fFalse; /* just show brief summary + errors */
bool fMute = fFalse; /* just show errors */
bool fSuppress = fFalse; /* only output 3d(3dx) file */

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
   {"no-percentage", no_argument, (int*)&fPercent, 0},
#endif
   {"output", required_argument, 0, 'o'},
   {"quiet", no_argument, 0, 'q'},
   {"no-auxiliary-files", no_argument, 0, 's'},
#ifdef NEW3DFORMAT
   {"new-format", no_argument, 0, 'x'},
#endif
   {"help", no_argument, 0, HLP_HELP},
   {"version", no_argument, 0, HLP_VERSION},
   {0, 0, 0, 0}
};

#ifdef NEW3DFORMAT
#define short_opts "pxao:qsz:"
#else
#define short_opts "pao:qsz:"
#endif

/* TRANSLATE extract help messages to message file */
static struct help_msg help[] = {
/*				<-- */
   {HLP_ENCODELONG(0),          "display percentage progress"},
   {'a',                        "output ascii variant of .3d file"},
   {HLP_ENCODELONG(2),          "set location for output files"},
   {HLP_ENCODELONG(3),          "only show brief summary (-qq for errors only)"},
   {HLP_ENCODELONG(4),          "do not create .pos, .inf, or .err files"},
#ifdef NEW3DFORMAT
   {HLP_ENCODELONG(5),          "output data in 3dx format"},
#endif
 /*{'z',                        "set optimizations for network reduction"},*/
   {0, 0}
};

extern CDECL int
main(int argc, char **argv)
{
   int d;
   static clock_t tmCPUStart;
   static time_t tmUserStart;
   static double tmCPU, tmUser;

   tmUserStart = time(NULL);
   tmCPUStart = clock();
   init_screen();

   msg_init(argv[0]);

   pcs = osnew(settings);
   pcs->next = NULL;
   pcs->Translate = ((short*) osmalloc(ossizeof(short) * 257)) + 1;

   /* Set up root of prefix hierarchy */
   root = osnew(prefix);
   root->up = root->right = root->down = NULL;
   root->stn = NULL;
   root->pos = NULL;
   root->ident = "\\";
   root->fSuspectTypo = fFalse;

   nosurveyhead = NULL;

   stnlist = NULL;
   cLegs = cStns = cComponents = 0;
   totadj = total = totplan = totvert = 0.0;

   for (d = 0; d <= 2; d++) {
      min[d] = REAL_BIG;
      max[d] = -REAL_BIG;
      pfxHi[d] = pfxLo[d] = NULL;
   }

   cmdline_init(argc, argv, short_opts, long_opts, NULL, help, 1, -1);
   while (1) {
      /* at least one argument must be given */
      int opt = cmdline_getopt();
      if (opt == EOF) break;
      switch (opt) {
       case 'a':
	 fAscii = fTrue;
	 break;
       case 'p':
#ifndef NO_PERCENTAGE
	 fPercent = 1;
#endif
	 break;
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
#ifdef NEW3DFORMAT
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
       case 'z': {
	 /* Control which network optimisations are used (development tool) */
	 static int first_opt_z = 1;
	 char ch;
	 if (first_opt_z) {
	    optimize = 0;
	    first_opt_z = 0;
	 }
	 /* Lollipops, Parallel legs, Iterate mx, Delta* */
	 while ((ch = *optarg++) != '\0')
	     if (islower(ch)) optimize |= BITA(ch);
	 break;
       }
      }
   }

   out_puts(PACKAGE" "VERSION);
   out_puts(COPYRIGHT_MSG);
   putnl();

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
#ifdef NEW3DFORMAT
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
      data_file("", fnm); /* first argument is current path */

      optind++;
   }

   validate();

   solve_network(/*stnlist*/); /* Find coordinates of all points */
   validate();

#ifdef NEW3DFORMAT
   if (fUseNewFormat) {
     cave_close(pimgOut); /* this actually does all the writing */
   } else {
#endif
     img_close(pimgOut); /* close .3d file */
#ifdef NEW3DFORMAT
   }
#endif
   if (fhErrStat) fclose(fhErrStat);

   if (!fSuppress) list_pos(root); /* produce .pos file */

   out_current_action(msg(/*Calculating statistics*/120));
   do_stats();
   if (!fQuiet) {
      tmCPU = (clock_t)(clock() - tmCPUStart) / (double)CLOCKS_PER_SEC;
      tmUser = difftime(time(NULL), tmUserStart);

      /* tmCPU is integer, tmUser not - equivalent to (ceil(tmCPU) >= tmUser) */
      if (tmCPU + 1 > tmUser) {
         out_printf((msg(/*CPU time used %5.2fs*/140), tmCPU));
      } else if (tmCPU == 0) {
         if (tmUser == 0.0) {
            out_printf((msg(/*Time used %5.2fs*/141), tmUser));
	 } else {
            out_puts(msg(/*Time used unavailable*/142));
	 }
      } else {
	 out_printf((msg(/*Time used %5.2fs (%5.2fs CPU time)*/143),
		    tmUser, tmCPU));
      }

      out_puts(msg(/*Done.*/144));
   }
   return error_summary();
}

static void
do_range(FILE *fh, int d, int msg1, int msg2, int msg3)
{
   char buf[1024];
   sprintf(buf, msg(msg1), max[d] - min[d]);
   strcat(buf, sprint_prefix(pfxHi[d]));
   sprintf(buf + strlen(buf), msg(msg2), max[d]);
   strcat(buf, sprint_prefix(pfxLo[d]));
   sprintf(buf + strlen(buf), msg(msg3), min[d]);
   if (!fMute) out_puts(buf);
   if (!fSuppress) fputsnl(buf, fh);
}

static void
do_stats(void)
{
   FILE *fh;
   long cLoops = cComponents + cLegs - cStns;
   char buf[1024];

   if (!fSuppress)
      fh = safe_fopen_with_ext(fnm_output_base, EXT_SVX_STAT, "w");

   out_puts("");

   if (cStns == 1)
      sprintf(buf, msg(/*Survey contains 1 survey station,*/172));
   else
      sprintf(buf,
	      msg(/*Survey contains %ld survey stations,*/173), cStns);

   if (cLegs == 1)
      sprintf(buf + strlen(buf), msg(/* joined by 1 leg.*/174));
   else
      sprintf(buf + strlen(buf),
	      msg(/* joined by %ld legs.*/175), cLegs);

   if (!fMute) out_puts(buf);
   if (!fSuppress) fputsnl(buf, fh);

   if (cLoops == 1)
      sprintf(buf, msg(/*There is 1 loop.*/138));
   else
      sprintf(buf, msg(/*There are %ld loops.*/139), cLoops);

   if (!fMute) out_puts(buf);
   if (!fSuppress) fputsnl(buf, fh);

   if (cComponents != 1) {
      sprintf(buf,
	      msg(/*Survey has %ld connected components.*/178), cComponents);
      if (!fMute) out_puts(buf);
      if (!fSuppress) fputsnl(buf, fh);
   }

   sprintf(buf,
	   msg(/*Total length of survey legs = %7.2fm (%7.2fm adjusted)*/132),
	   total, totadj);
   if (!fMute) out_puts(buf);
   if (!fSuppress) fputsnl(buf, fh);

   sprintf(buf,
	   msg(/*Total plan length of survey legs = %7.2fm*/133), totplan);
   if (!fMute) out_puts(buf);
   if (!fSuppress) fputsnl(buf, fh);

   sprintf(buf, msg(/*Total vertical length of survey legs = %7.2fm*/134),
	   totvert);
   if (!fMute) out_puts(buf);
   if (!fSuppress) fputsnl(buf, fh);

   do_range(fh, 2, /*Vertical range = %4.2fm (from */135,
	    /* at %4.2fm to */136, /* at %4.2fm)*/137);
   do_range(fh, 1, /*North-South range = %4.2fm (from */148,
	    /* at %4.2fm to */196, /* at %4.2fm)*/197);
   do_range(fh, 0, /*East-West range = %4.2fm (from */149,
	    /* at %4.2fm to */196, /* at %4.2fm)*/197);

   print_node_stats(fh);
   /* Also, could give:
    *  # nodes stations (ie have other than two references or are fixed)
    *  # fixed stations (list of?)
    */

   if (!fSuppress) fclose(fh);
}
