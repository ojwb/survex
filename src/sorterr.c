/* sorterr.c */
/* Sort a survex .err file */
/* Copyright (C) 2001,2002 Olly Betts
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

#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cmdline.h"
#include "filename.h"
#include "message.h"
#include "osalloc.h"
#include "whichos.h"

static const struct option long_opts[] = {
   /* const char *name; int has_arg (0 no_argument, 1 required_*, 2 optional_*); int *flag; int val; */
   {"horizontal", no_argument, 0, 'h'},
   {"vertical", no_argument, 0, 'v'},
   {"percentage", no_argument, 0, 'p'},
   {"per-leg", no_argument, 0, 'l'},
   {"replace", no_argument, 0, 'r'},
   {"help", no_argument, 0, HLP_HELP},
   {"version", no_argument, 0, HLP_VERSION},
   {0, 0, 0, 0}
};

#define short_opts "hvplr"

static struct help_msg help[] = {
/*				<-- */
   {HLP_ENCODELONG(0),          "Sort by horizontal error factor"},
   {HLP_ENCODELONG(1),          "Sort by vertical error factor"},
   {HLP_ENCODELONG(2),          "Sort by percentage error"},
   {HLP_ENCODELONG(3),          "Sort by error per leg"},
   {HLP_ENCODELONG(4),          "Replace .err file with resorted version"},
   {0, 0}
};

typedef struct {
   double err;
   long fpos;
} trav;

static void
skipline(const char *fnm, FILE *fh)
{
   int ch;
   do {
      ch = getc(fh);
   } while (ch != '\n' && ch != EOF);

   if (ch == EOF) {
      if (ferror(fh))
	 fatalerror_in_file(fnm, 0, /*Error reading file*/18);
      fatalerror_in_file(fnm, 0, /*Couldn't parse .err file*/112);
   }
}

static void
printline(const char *fnm, FILE *fh, FILE *fh_out)
{
   int ch;
   do {
      ch = getc(fh);
      if (ch != EOF && ch != '\r' && ch != '\n') putc(ch, fh_out);
   } while (ch != '\n' && ch != EOF);
   putc('\n', fh_out);

   if (ch == EOF) {
      if (ferror(fh))
	 fatalerror_in_file(fnm, 0, /*Error reading file*/18);
      fatalerror_in_file(fnm, 0, /*Couldn't parse .err file*/112);
   }
}

static int
cmp_trav(const void *a, const void *b)
{
   double diff = ((const trav *)a)->err - ((const trav *)b)->err;
   if (diff < 0) return -1;
   if (diff > 0) return 1;
   return 0;
}

int
main(int argc, char **argv)
{
   char *fnm;
   FILE *fh;
   char sortby = 'A';
   size_t len = 1024;
   trav *blk = osmalloc(1024 * sizeof(trav));
   size_t next = 0;
   size_t howmany = 0;
   FILE *fh_out = stdout;
   char *fnm_out = NULL;

   msg_init(argv);

   cmdline_set_syntax_message("ERR_FILE [HOW MANY]", NULL); /* TRANSLATE */
   cmdline_init(argc, argv, short_opts, long_opts, NULL, help, 1, 2);
   while (1) {
      int opt = cmdline_getopt();
      if (opt == EOF) break;
      switch (opt) {
       case 'h': case 'v': case 'p': case 'l':
	 sortby = toupper(opt);
	 break;
       case 'r':
	 fh_out = NULL;
	 break;
      }
   }

   fnm = argv[optind++];
   if (argv[optind]) howmany = atoi(argv[optind]);

   fh = fopen(fnm, "rb");
   if (!fh) fatalerror(/*Couldn't open file `%s'*/93, fnm);

   /* 4 line paragraphs, separated by blank lines...
    * 041.verhall.12 - 041.verhall.13
    * Original length   2.97m (  1 legs), moved   0.04m ( 0.04m/leg). Error   1.19%
    * 0.222332
    * H: 0.224749 V: 0.215352
    *
    */
   while (1) {
      int ch;
      if (next == len) {
	 len += len;
	 blk = osrealloc(blk, len * ossizeof(trav));
      }
      blk[next].fpos = ftell(fh);
      ch = getc(fh);
      if (ch == EOF) break;
      skipline(fnm, fh);
      switch (sortby) {
       case 'A':
	 skipline(fnm, fh);
	 if (fscanf(fh, "%lf", &blk[next].err) != 1) {
	    baderrfile:
	    fatalerror_in_file(fnm, 0, /*Couldn't parse .err file*/112);
	 }
	 skipline(fnm, fh);
	 skipline(fnm, fh);
	 break;
       case 'H': case 'V':
	 skipline(fnm, fh);
	 skipline(fnm, fh);
	 do {
	    ch = getc(fh);
	    if (ch == '\n' || ch == EOF) goto baderrfile;
	 } while (ch != sortby);
	 if (fscanf(fh, ":%lf", &blk[next].err) != 1) goto baderrfile;
	 skipline(fnm, fh);
	 break;
       case 'P':
	 do {
	    ch = getc(fh);
	    if (ch == '\n' || ch == EOF) goto baderrfile;
	 } while (ch != ')');
	 do {
	    ch = getc(fh);
	    if (ch == '\n' || ch == EOF) goto baderrfile;
	 } while (ch != ')');
	 do {
	    ch = getc(fh);
	    if (ch == '\n' || ch == EOF) goto baderrfile;
	 } while (!isdigit(ch));
	 ungetc(ch, fh);
	 if (fscanf(fh, "%lf", &blk[next].err) != 1) goto baderrfile;
	 skipline(fnm, fh);
	 skipline(fnm, fh);
	 skipline(fnm, fh);
	 break;
       case 'L':
	 do {
	    ch = getc(fh);
	    if (ch == '\n' || ch == EOF) goto baderrfile;
	 } while (ch != ')');
	 do {
	    ch = getc(fh);
	    if (ch == '\n' || ch == EOF) goto baderrfile;
	 } while (ch != '(');
	 if (fscanf(fh, "%lf", &blk[next].err) != 1) goto baderrfile;
	 skipline(fnm, fh);
	 skipline(fnm, fh);
	 skipline(fnm, fh);
	 break;
      }
      skipline(fnm, fh);
      next++;
   }

   if (next == 0) {
      /* no entries - nothing more to do whether -r is specified or not */
      exit(EXIT_SUCCESS);
   }

   qsort(blk, next, sizeof(trav), cmp_trav);

   if (fh_out == NULL) {
      char *base = base_from_fnm(fnm);
      fnm_out = add_ext(base, "tmp");
      osfree(base);
      fh_out = safe_fopen(fnm_out, "w");
   }

   do {
      --next;
      if (fseek(fh, blk[next].fpos, SEEK_SET) == -1)
	 fatalerror_in_file(fnm, 0, /*Error reading file*/18);

      printline(fnm, fh, fh_out);
      printline(fnm, fh, fh_out);
      printline(fnm, fh, fh_out);
      printline(fnm, fh, fh_out);
      putc('\n', fh_out);
      if (howmany && --howmany == 0) break;
   } while (next);
   fclose(fh);

   if (fnm_out) {
      safe_fclose(fh_out);
#if (OS!=UNIX) && !defined(__DJGPP__)
      /* UNIX rename atomically replaces, so doesn't need this.
       * DJGPP "removes destination first if it exists" according to docs
       *    (testing confirms this) so doesn't need this.
       * BorlandC won't overwrite (from tests) so needs this code
       * WIN32 won't overwrite (from tests) so needs this code.
       * RISC OS won't overwrite on some filing systems, so needs this code.
       */
      remove(fnm);
#endif
      rename(fnm_out, fnm);
   }

   return 0;
}
