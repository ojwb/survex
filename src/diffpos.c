/* diffpos.c */
/* Utility to compare two SURVEX .pos or .3d files */
/* Copyright (C) 1994,1996,1998-2003,2010,2011,2013,2014 Olly Betts
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
# include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "cmdline.h"
#include "debug.h"
#include "filelist.h"
#include "hash.h"
#include "img_hosted.h"
#include "namecmp.h"
#include "useful.h"

/* Don't complain if values mismatch by a tiny amount (1e-6m i.e. 0.001mm) */
#define TOLERANCE 1e-6

/* default threshold is 1cm */
#define DFLT_MAX_THRESHOLD 0.01

static double threshold = DFLT_MAX_THRESHOLD;

static const struct option long_opts[] = {
   /* const char *name; int has_arg (0 no_argument, 1 required_*, 2 optional_*); int *flag; int val; */
   {"survey", required_argument, 0, 's'},
   {"help", no_argument, 0, HLP_HELP},
   {"version", no_argument, 0, HLP_VERSION},
   {0, 0, 0, 0}
};

#define short_opts "s:"

static struct help_msg help[] = {
/*				<-- */
   {HLP_ENCODELONG(0),        /*only load the sub-survey with this prefix*/199, 0},
   {0, 0, 0}
};

/* We use a hashtable with linked list buckets - this is how many hash table
 * entries we have.  0x2000 with sizeof(void *) == 4 uses 32K. */
#define TREE_SIZE 0x2000

typedef struct station {
   struct station *next;
   char *name;
   img_point pt;
} station;

typedef struct added {
   struct added *next;
   char *name;
} added;

static int old_separator, new_separator, sort_separator;

static int
cmp_pname(const void *a, const void *b)
{
   return name_cmp(*(const char **)a, *(const char **)b, sort_separator);
}

static station **htab;
static bool fChanged = fFalse;

static added *added_list = NULL;
static OSSIZE_T c_added = 0;

static void
tree_init(void)
{
   size_t i;
   htab = osmalloc(TREE_SIZE * ossizeof(station *));
   for (i = 0; i < TREE_SIZE; i++) htab[i] = NULL;
}

static void
tree_insert(const char *name, const img_point *pt)
{
   int v = hash_string(name) & (TREE_SIZE - 1);
   station * stn = osnew(station);
   stn->name = osstrdup(name);
   stn->pt = *pt;
   stn->next = htab[v];
   htab[v] = stn;
}

static int
close_enough(const img_point * p1, const img_point * p2)
{
    return fabs(p1->x - p2->x) - threshold <= TOLERANCE &&
	   fabs(p1->y - p2->y) - threshold <= TOLERANCE &&
	   fabs(p1->z - p2->z) - threshold <= TOLERANCE;
}

static void
tree_remove(const char *name, const img_point *pt)
{
   /* We need to handle duplicate labels - normal .3d files shouldn't have them
    * (though some older ones do due to a couple of bugs in earlier versions of
    * Survex) but extended .3d files repeat the label where a loop is broken,
    * and data read from foreign formats might repeat labels.
    */
   int v = hash_string(name) & (TREE_SIZE - 1);
   station **prev;
   station *p;
   station **found = NULL;
   bool was_close_enough = fFalse;

   for (prev = &htab[v]; *prev; prev = &((*prev)->next)) {
      if (strcmp((*prev)->name, name) == 0) {
	 /* Handle stations with the same name.  Stations are inserted at the
	  * start of the linked list, so pick the *last* matching station in
	  * the list as then we match the first stations with the same name in
	  * each file.
	  */
	 if (close_enough(pt, &((*prev)->pt))) {
	    found = prev;
	    was_close_enough = fTrue;
	 } else if (!was_close_enough) {
	    found = prev;
	 }
      }
   }

   if (!found) {
      added *add = osnew(added);
      add->name = osstrdup(name);
      add->next = added_list;
      added_list = add;
      c_added++;
      fChanged = fTrue;
      return;
   }

   if (!was_close_enough) {
      /* TRANSLATORS: for diffpos: */
      printf(msg(/*Moved by (%3.2f,%3.2f,%3.2f): %s*/500),
	     pt->x - (*found)->pt.x,
	     pt->y - (*found)->pt.y,
	     pt->z - (*found)->pt.z,
	     name);
      putnl();
      fChanged = fTrue;
   }

   osfree((*found)->name);
   p = *found;
   *found = p->next;
   osfree(p);
}

static int
tree_check(void)
{
   size_t c = 0;
   char **names;
   size_t i;

   if (c_added) {
      names = osmalloc(c_added * ossizeof(char *));
      for (i = 0; i < c_added; i++) {
	 added *old;
	 SVX_ASSERT(added_list);
	 names[i] = added_list->name;
	 old = added_list;
	 added_list = old->next;
	 osfree(old);
      }
      SVX_ASSERT(added_list == NULL);
      sort_separator = new_separator;
      qsort(names, c_added, sizeof(char *), cmp_pname);
      for (i = 0; i < c_added; i++) {
	 /* TRANSLATORS: for diffpos: */
	 printf(msg(/*Added: %s*/501), names[i]);
	 putnl();
	 osfree(names[i]);
      }
      osfree(names);
   }

   for (i = 0; i < TREE_SIZE; i++) {
      station *p;
      for (p = htab[i]; p; p = p->next) c++;
   }
   if (c == 0) return fChanged;

   names = osmalloc(c * ossizeof(char *));
   c = 0;
   for (i = 0; i < TREE_SIZE; i++) {
      station *p;
      for (p = htab[i]; p; p = p->next) names[c++] = p->name;
   }
   sort_separator = old_separator;
   qsort(names, c, sizeof(char *), cmp_pname);
   for (i = 0; i < c; i++) {
      /* TRANSLATORS: for diffpos: */
      printf(msg(/*Deleted: %s*/502), names[i]);
      putnl();
   }
   return fTrue;
}

static int
parse_file(const char *fnm, const char *survey,
	   void (*tree_func)(const char *, const img_point *))
{
   img_point pt;
   int result;
   int separator;

   img *pimg = img_open_survey(fnm, survey);
   if (!pimg) fatalerror(img_error2msg(img_error()), fnm);
   separator = pimg->separator;

   do {
      result = img_read_item(pimg, &pt);
      switch (result) {
       case img_MOVE:
       case img_LINE:
	 break;
       case img_LABEL:
	 tree_func(pimg->label, &pt);
	 break;
       case img_BAD:
	 img_close(pimg);
	 fatalerror(img_error2msg(img_error()), fnm);
      }
   } while (result != img_STOP);

   img_close(pimg);
   return separator;
}

int
main(int argc, char **argv)
{
   char *fnm1, *fnm2;
   const char *survey = NULL;

   msg_init(argv);

   /* TRANSLATORS: Part of diffpos --help */
   cmdline_set_syntax_message(/*FILE1 FILE2 [THRESHOLD]*/218,
			      /* TRANSLATORS: Part of diffpos --help */
			      /*FILE1 and FILE2 can be .pos or .3d files\nTHRESHOLD is the max. ignorable change along any axis in metres (default %s)*/255,
			      STRING(DFLT_MAX_THRESHOLD));
   cmdline_init(argc, argv, short_opts, long_opts, NULL, help, 2, 3);
   while (1) {
      int opt = cmdline_getopt();
      if (opt == EOF) break;
      if (opt == 's') survey = optarg;
   }
   fnm1 = argv[optind++];
   fnm2 = argv[optind++];
   if (argv[optind]) {
      optarg = argv[optind];
      threshold = cmdline_double_arg();
   }

   tree_init();

   old_separator = parse_file(fnm1, survey, tree_insert);

   new_separator = parse_file(fnm2, survey, tree_remove);

   return tree_check() ? EXIT_FAILURE : EXIT_SUCCESS;
}
