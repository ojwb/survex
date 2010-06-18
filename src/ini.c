/* ini.c
 * .ini file routines
 * Copyright (C) 1995-2001,2003 Olly Betts
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
#include <ctype.h>

#include "debug.h"
#include "filename.h"
#include "hash.h"
#include "ini.h"
#include "message.h"
#include "useful.h"

/* for testing separately from Survex */
#ifdef TEST_ALONE
/* says when two distinct strings hash to same value */
#define REPORT_COLLISION

/* good enough for testing */
#define strcasecmp(x,y) strcmp((x),(y))

char *
osstrdup(char *s)
{
   char *t;
   t = malloc(strlen(s) + 1);
   if (!t) {
      printf("out of memory\n");
      exit(1);
   }
   strcpy(t, s);
   return t;
}
#include <assert.h>
#define SVX_ASSERT(M) assert(M)
#endif

#if 0
void
ini_write(const char *section, const char *var, const char *value)
{
   FILE *fh = stdout;
   scan_for(section, var);

   static char *last_section = NULL;
   if (!last_section || strcasecmp(last_section, section) != 0) {
      if (last_section) putc('\n', fh);
      fprintf(fh, "[%s]\n", section);
      last_section = section;
   }
   fprintf(stdout, "%s=%s\n", var, value);
}
#endif

char **
ini_read(FILE **fh_list, const char *section, const char **vars)
{
   int n, c;
   char **vals;
   int *hash_tab;

   SVX_ASSERT(fh_list != NULL);
   SVX_ASSERT(section != NULL);
   SVX_ASSERT(vars != NULL);

   /* count how many variables to look up */
   n = 0;
   while (vars[n]) n++;

   hash_tab = malloc(n * sizeof(int));
   vals = malloc(n * sizeof(char*));
   if (!hash_tab || !vals) {
      free(hash_tab);
      return NULL;
   }

   /* calculate hashes (to save on strcmp-s) */
   for (c = 0; vars[c]; c++) {
      hash_tab[c] = hash_lc_string(vars[c]);
      vals[c] = NULL;
   }

   SVX_ASSERT(c == n); /* counted vars[] twice and got different answers! */

   while (1) {
      char *p, *var, *val;
      char buf[1024];
      int hash;

      int fInSection = 0;

      FILE *fh = *fh_list++;

      if (!fh) break;
      rewind(fh);
      while (1) {
	 if (feof(fh)) {
	    /* if (fWrite) { insert_section(); insert_lines(); fDone=1; } */
	    break;
	 }

	 /* read line and sort out terminator */
	 if (!fgets(buf, 1024, fh)) break;
	 p = strpbrk(buf, "\n\r");
	 if (p) *p = '\0';

	 /* skip blank lines */
	 if (buf[0] == '\0') continue;

	 /* deal with a section heading */
	 if (buf[0] == '[' && buf[strlen(buf) - 1] == ']') {
	    buf[strlen(buf) - 1] = '\0';
	    if (fInSection) {
	       /* if (fWrite) { insert_lines(); fDone=1; } */
	       /* now leaving wanted section, so stop on this file */
	       break;
	    }
	    /*         printf("[%s] [%s]\n", section, buf + 1);*/
	    if (!strcasecmp(section, buf + 1)) fInSection = 1;
	    continue;
	 }

	 if (!fInSection) continue;

	 p = strchr(buf, '=');
	 if (!p) continue; /* non-blank line with no = sign! */

	 *p = '\0';
	 var = buf;
	 val = p + 1;

	 /* hash the variable name and see if it's in the list passed in */
	 hash = hash_lc_string(var);
	 for (c = n - 1; c >= 0; c--) {
	    if (hash == hash_tab[c]) {
	       if (strcasecmp(var, vars[c]) == 0) {
		  /* if (fWrite) { replace_line(); hash_tab[c]=-1; } else */
		  vals[c] = osstrdup(val);
		  /* set to value hash can't generate to ignore further matches */
		  hash_tab[c] = -1;
	       } else {
#ifdef REPORT_COLLISION
		  printf("`%s' and `%s' both hash to %d!\n",var,vars[c],hash);
#endif
	       }
	    }
	 }
      }
   }

   free(hash_tab);
   return vals;
}

char **
ini_read_hier(FILE **fh_list, const char *section, const char **v)
{
   int i, j;
   char **vals;
   int *to;
   char **vars;

   SVX_ASSERT(fh_list != NULL);
   SVX_ASSERT(section != NULL);
   SVX_ASSERT(v != NULL);

   vals = ini_read(fh_list, section, v);
   if (!vals) return NULL;

/*{int i; printf("[%s]\n",section);for(i=0;v[i];i++)printf("%d:%s\"%s\"\n",i,v[i],vals[i]?vals[i]:"(null)");}*/
   i = 0;
   while (v[i]) i++;
   vars = malloc((i + 1) * sizeof(char*)); /* + 1 to include NULL */
   to = malloc(i * sizeof(int));
   if (!vars || !to) {
      free(vars);
      free(to);
      free(vals);
      return NULL;
   }
   memcpy(vars, v, (i + 1) * sizeof(char*));

   for (i = 1, j = 1; vars[i]; i++) {
/*      printf("%s: %s %d\n",vars[i],vals[i]?vals[i]:"(null)",to[i]);*/
      if (!vals[i]) {
	 vars[j] = vars[i];
	 to[j] = i;
	 j++;
      }
   }

   while (vals[0] && j > 1) {
      char **x;

      vars[j] = NULL;

      x = ini_read(fh_list, vals[0], (const char **)vars);
      if (!x) {
	 free(vals);
	 vals = NULL;
	 break;
      }

/*{int i; printf("[%s]\n",vals[0]);for(i=0;vars[i];i++)printf("%d:%s\"%s\"\n",i,vars[i],vals[i]?vals[i]:"(null)");}*/

      free(vals[0]);
      vals[0] = x[0];

      for (i = 1, j = 1; vars[i]; i++) {
/*         printf("%s: %s %d\n",vars[i],vals[i]?vals[i]:"(null)",to[i]);*/
	 if (x[i]) {
	    if (x) vals[to[i]] = x[i];
	 } else {
	    vars[j] = vars[i];
	    to[j] = to[i];
	    j++;
	 }
      }
      free(x);
   }

   free(vals[0]);
   vals[0] = NULL;
   free(vars);
   free(to);

   return vals;
}
