/* > diffpos.c */
/* Utility to compare two SURVEX .pos or .3d files */
/* Copyright (C) 1994,1996,1998,1999,2001 Olly Betts
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
# include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "cavern.h" /* just for REAL_EPSILON */
#include "cmdline.h"
#include "debug.h"
#include "filelist.h"
#include "img.h"
#include "namecmp.h"

#ifndef sqrd
# define sqrd(X) ((X)*(X))
#endif

/* macro to just convert argument to a string */
#define STRING(X) _STRING(X)
#define _STRING(X) #X

/* very small value for comparing floating point numbers with */
#define EPSILON (REAL_EPSILON * 1000)

/* default threshold is 1cm */
#define DFLT_MAX_THRESHOLD 0.01

static double threshold = DFLT_MAX_THRESHOLD;

static const struct option long_opts[] = {
   /* const char *name; int has_arg (0 no_argument, 1 required_*, 2 optional_*); int *flag; int val; */
   {"help", no_argument, 0, HLP_HELP},
   {"version", no_argument, 0, HLP_VERSION},
   {0, 0, 0, 0}
};

#define short_opts ""

static struct help_msg help[] = {
/*				<-- */
   {0, 0}
};

/* some (preferably prime) number for the hashing function */
#define HASH_PRIME 29363

typedef struct station {
   struct station *next;
   char *name;
   img_point pt;
} station;

typedef struct added {
   struct added *next;
   char *name;
} added;

static int
hash_string(const char *p)
{
   int hash;
   ASSERT(p != NULL); /* can't hash NULL */
   for (hash = 0; *p; p++)
      hash = (hash * HASH_PRIME + tolower(*(unsigned char*)p)) & 0x7fff;
   return hash;
}

static int
cmp_pname(const void *a, const void *b)
{
   return name_cmp(*(const char **)a, *(const char **)b);
}

static station **htab;
static bool fChanged = fFalse;

static added *added_list = NULL;;
OSSIZE_T c_added = 0;

static void
tree_init(void)
{
   size_t i;
   htab = osmalloc(0x2000 * sizeof(int));
   for (i = 0; i < 0x2000; i++) htab[i] = NULL;
}

static void
tree_insert(const char *name, const img_point *pt)
{
   int v = hash_string(name) & 0x1fff;
   station *stn;

   /* Catch duplicate labels - .3d files shouldn't have them, but some do
    * due to a couple of bugs in some versions of Survex
    */
   for (stn = htab[v]; stn; stn = stn->next) {
      if (strcmp(stn->name, name) == 0) return; /* found dup */
   }

   stn = osnew(station);
   stn->name = osstrdup(name);
   stn->pt = *pt;
   stn->next = htab[v];
   htab[v] = stn;
}

static void
tree_remove(const char *name, const img_point *pt)
{
   int v = hash_string(name) & 0x1fff;
   station **prev;
   station *p;
   
   for (prev = &htab[v]; *prev; prev = &((*prev)->next)) {
      if (strcmp((*prev)->name, name) == 0) break;
   }
   
   if (!*prev) {
      added *add = osnew(added);
      add->name = osstrdup(name);
      add->next = added_list;
      added_list = add;
      c_added++;
      fChanged = fTrue;
      return;
   }
   
   if (fabs(pt->x - (*prev)->pt.x) - threshold > EPSILON ||
       fabs(pt->y - (*prev)->pt.y) - threshold > EPSILON ||
       fabs(pt->z - (*prev)->pt.z) - threshold > EPSILON) {
      printf("Moved by (%3.2f,%3.2f,%3.2f): %s\n",
	     pt->x - (*prev)->pt.x,
	     pt->y - (*prev)->pt.y,
	     pt->z - (*prev)->pt.z,
	     name);
      fChanged = fTrue;
   }
   
   osfree((*prev)->name);
   p = *prev;
   *prev = p->next;
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
	 ASSERT(added_list);
	 names[i] = added_list->name;
	 old = added_list;
	 added_list = old->next;
	 osfree(old);
      }
      ASSERT(added_list == NULL);
      qsort(names, c_added, sizeof(char *), cmp_pname);
      for (i = 0; i < c_added; i++) {
	 printf("Added: %s\n", names[i]);
	 osfree(names[i]);
      }
      osfree(names);
   }

   for (i = 0; i < 0x2000; i++) {
      station *p;
      for (p = htab[i]; p; p = p->next) c++;
   }
   if (c == 0) return fChanged;

   names = osmalloc(c * ossizeof(char *));
   c = 0;
   for (i = 0; i < 0x2000; i++) {
      station *p;
      for (p = htab[i]; p; p = p->next) names[c++] = p->name;
   }
   qsort(names, c, sizeof(char *), cmp_pname);
   for (i = 0; i < c; i++) {
      printf("Deleted: %s\n", names[i]);
   }
   return fTrue;
}

static void
parse_3d_file(const char *fnm,
	      void (*tree_func)(const char *, const img_point *))
{
   img_point pt;
   int result;
 
   img *pimg = img_open(fnm, NULL, NULL);
   if (!pimg) fatalerror(img_error(), fnm);

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
	 fatalerror(/*Bad 3d image file `%s'*/106, fnm);
      }
   } while (result != img_STOP);
      
   img_close(pimg);
}

static void
parse_pos_file(const char *fnm,
	       void (*tree_func)(const char *, const img_point *))
{
   FILE *fh;
   img_point pt;
   char *buf;
   size_t buf_len = 256;

   buf = osmalloc(buf_len);
   
   fh = fopen(fnm, "rb");
   if (!fh) fatalerror(/*Couldn't open file `%s'*/93, fnm);

   while (1) {
      size_t off = 0;
      long fp = ftell(fh);
      if (fscanf(fh, "(%lf,%lf,%lf ) ", &pt.x, &pt.y, &pt.z) != 3) {
	 int ch;
	 fseek(fh, SEEK_SET, fp);
	 ch = getc(fh);
	 if (ch == EOF) break;

	 printf("%s: Ignoring: ", fnm);
	 while (ch != '\n' && ch != '\r' && ch != EOF) {
	    putchar(ch);
	    ch = getc(fh);
	 }
	 putchar('\n');
	 
	 if (feof(fh)) break;
	 continue;
      }
      buf[0] = '\0';
      while (!feof(fh)) {
	 if (!fgets(buf + off, buf_len - off, fh)) {
	    /* FIXME */
	    break;
	 }
	 off += strlen(buf + off);
	 if (off && buf[off - 1] == '\n') {
	    buf[off - 1] = '\0';
	    break;
	 }
	 buf_len += buf_len;
	 buf = osrealloc(buf, buf_len);
      }
      tree_func(buf, &pt);
   }
   fclose(fh);

   osfree(buf);
}

static void
parse_file(const char *fnm,
	   void (*tree_func)(const char *, const img_point *))
{
   static const char ext3d[] = EXT_SVX_3D;
   size_t len = strlen(fnm);
   if (len > sizeof(ext3d) && fnm[len - sizeof(ext3d)] == FNM_SEP_EXT &&
       strcmp(fnm + len - sizeof(ext3d) + 1, ext3d) == 0) {
      parse_3d_file(fnm, tree_func);
   } else {
      parse_pos_file(fnm, tree_func);
   }
}

int
main(int argc, char **argv)
{
   char *fnm1, *fnm2;
   msg_init(argv[0]);

   cmdline_set_syntax_message("FILE1 FILE2 [THRESHOLD]",
			      "FILE1 and FILE2 can be .pos or .3d files\n" 
			      "THRESHOLD is the max. ignorable change along "
			      "any axis in metres (default "
			      STRING(DFLT_MAX_THRESHOLD)")");
   cmdline_init(argc, argv, short_opts, long_opts, NULL, help, 2, 3);
   while (cmdline_getopt() != EOF) {
      /* do nothing */
   }
   fnm1 = argv[optind++];
   fnm2 = argv[optind++];
   if (argv[optind]) {
      optarg = argv[optind];
      threshold = cmdline_double_arg();
   }

   tree_init();

   parse_file(fnm1, tree_insert);

   parse_file(fnm2, tree_remove);
   
   return tree_check() ? EXIT_FAILURE : EXIT_SUCCESS;
}
