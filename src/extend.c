/* extend.c
 * Produce an extended elevation
 * Copyright (C) 1995-2002 Olly Betts
 * Copyright (C) 2004 John Pybus
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

#include <float.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cmdline.h"
#include "debug.h"
#include "filelist.h"
#include "filename.h"
#include "hash.h"
#include "img.h"
#include "message.h"
#include "useful.h"

/* To save memory we should probably use the prefix hash for the prefix on
 * point labels (FIXME) */

typedef struct stn {
   const char *label;
   int flags;
   const struct stn *next;
} stn;

typedef struct POINT {
   img_point p;
   const stn *stns;
   unsigned int order;
   int fDir;
   int fDone;
   struct POINT *next;
} point;

typedef struct LEG {
   point *fr, *to;
   const char *prefix;
   int fDir;
   int fDone;
   int flags;
   struct LEG *next;
} leg;

#define DONE        0x01
#define BREAK_FR    0x04
#define BREAK_TO    0x08
#define BREAK       (BREAK_FR | BREAK_TO)


#define ELEFT  0x04
#define ERIGHT 0x08
#define ESWAP  0x10

static point headpoint = {{0, 0, 0}, NULL, 0, 0, 0, NULL};

static leg headleg = {NULL, NULL, NULL, 0, 0, 0, NULL};

static img *pimg;

static void do_stn(point *, double, const char *, int);

typedef struct pfx {
   const char *label;
   struct pfx *next;
} pfx;

static pfx **htab;

#define HTAB_SIZE 0x2000

static const char *
find_prefix(const char *prefix)
{
   pfx *p;
   int hash;

   SVX_ASSERT(prefix);

   hash = hash_string(prefix) & (HTAB_SIZE - 1);
   for (p = htab[hash]; p; p = p->next) {
      if (strcmp(prefix, p->label) == 0) return p->label;
   }

   p = osnew(pfx);
   p->label = osstrdup(prefix);
   p->next = htab[hash];
   htab[hash] = p;

   return p->label;
}

static point *
find_point(const img_point *pt)
{
   point *p;
   for (p = headpoint.next; p != NULL; p = p->next) {
      if (pt->x == p->p.x && pt->y == p->p.y && pt->z == p->p.z) {
	 return p;
      }
   }

   p = osmalloc(ossizeof(point));
   p->p = *pt;
   p->stns = NULL;
   p->order = 0;
   p->fDir = 0;
   p->fDone = 0;
   p->next = headpoint.next;
   headpoint.next = p;
   return p;
}

static void
add_leg(point *fr, point *to, const char *prefix, int flags)
{
   leg *l;
   fr->order++;
   to->order++;
   l = osmalloc(ossizeof(leg));
   l->fr = fr;
   l->to = to;
   if (prefix)
      l->prefix = find_prefix(prefix);
   else
      l->prefix = NULL;
   l->next = headleg.next;
   l->fDir = 0;
   l->fDone = 0;
   l->flags = flags;
   headleg.next = l;
}

static void
add_label(point *p, const char *label, int flags)
{
   stn *s = osnew(stn);
   s->label = osstrdup(label);
   s->flags = flags;
   s->next = p->stns;
   p->stns = s;
}

/* Read in config file */


/* lifted from img.c Should be put somewhere common? JPNP*/
static char *
getline_alloc(FILE *fh, size_t ilen)
{
   int ch;
   size_t i = 0;
   size_t len = ilen;
   char *buf = xosmalloc(len);
   if (!buf) return NULL;

   ch = getc(fh);
   while (ch != '\n' && ch != '\r' && ch != EOF) {
      buf[i++] = ch;
      if (i == len - 1) {
	 char *p;
	 len += len;
	 p = xosrealloc(buf, len);
	 if (!p) {
	    osfree(buf);
	    return NULL;
	 }
	 buf = p;
      }
      ch = getc(fh);
   }
   if (ch == '\n' || ch == '\r') {
      int otherone = ch ^ ('\n' ^ '\r');
      ch = getc(fh);
      /* if it's not the other eol character, put it back */
      if (ch != otherone) ungetc(ch, fh);
   }
   buf[i++] = '\0';
   return buf;
}

int lineno = 0;
point *start = NULL;

static char*
delimword(char *ln, char** lr)
{
   char *le;

   while (*ln == ' ' || *ln == '\t' || *ln == '\n' || *ln == '\r')
      ln++;

   le = ln;
   while (*le != ' ' && *le != '\t' && *le != '\n' && *le != '\r' && *le != ';' && *le != '\0')
      le++;

   if (*le == '\0' || *le == ';') {
      *lr = le;
   } else {
      *lr = le + 1;
   }
   
   *le = '\0';
   return ln;
}

static void
parseconfigline(char *ln)
{
   point *p;
   const stn *s;
   const stn *t;
   leg *l;
   char *lc = NULL;

   ln = delimword(ln, &lc);

   if (*ln == '\0') return;

   if (strcmp(ln, "*start")==0) {
      ln = delimword(lc, &lc);
      if (*ln == 0) fatalerror(/*Command without station name in config, line %i*/602, lineno);
      for (p = headpoint.next; p != NULL; p = p->next) {
	 for (s = p->stns; s; s = s->next) {
	    if (strcmp(s->label, ln)==0) {
	       start = p;
	       printf(msg(/*Starting from station %s*/604),ln);
	       putnl();
	       goto loopend;
	    }
	 }
      }
      warning(/*Failed to find station %s in config, line %i*/603, ln, lineno);
   } else if (strcmp(ln, "*eleft")==0) {
      char *ll = delimword(lc, &lc);
      if (*ll == 0) fatalerror(/*Command without station name in config, line %i*/602,lineno);
      ln = delimword(lc, &lc);
      if (*ln == 0) { /* One argument, look for point to switch at. */
	 for (p = headpoint.next; p != NULL; p = p->next) {
	    for (s = p->stns; s; s = s->next) {
	       if (strcmp(s->label, ll)==0) {
		  printf(msg(/*Plotting to the left from station %s*/605),ll);
		  putnl();
		  p->fDir = ELEFT;
		  goto loopend;
	       }
	    }
	 }
	 warning(/*Failed to find station %s in config, line %i*/603, ll, lineno);       
      } else { /* Two arguments look for a specific leg */
	 for (l = headleg.next; l; l=l->next) {
	    point * fr = l->fr;
	    point * to = l->to;
	    if (fr && to) {
	       for (s=fr->stns; s; s=s->next) {
		  int b = 0;
		  if (strcmp(s->label,ll)==0 || (strcmp(s->label, ln)==0 && (b = 1)) ) {
		     char * lr = (b ? ll : ln);
		     for (t=to->stns; t; t=t->next) {
			if (strcmp(t->label,lr)==0) {
			   printf(msg(/*Plotting to the left from leg %s -> %s*/607), s->label, t->label);
			   putnl();
			   l->fDir=ELEFT;
			   goto loopend;
			}
		     }
		  }
	       }
	    }
	 }
	 warning(/*Failed to find leg %s-%s in config, line %i*/606, ll, ln, lineno);
      }
   } else if (strcmp(ln, "*eright")==0) {
      char *ll = delimword(lc, &lc);
      if (*ll == 0) fatalerror(/*Command without station name in config, line %i*/602,lineno);
      ln = delimword(lc, &lc);
      if (*ln == 0) { /* One argument, look for point to switch at. */
	 for (p = headpoint.next; p != NULL; p = p->next) {
	    for (s = p->stns; s; s = s->next) {
	       if (strcmp(s->label, ll)==0) {
		  printf(msg(/*Plotting to the right from station %s*/608),ll);
		  putnl();
		  p->fDir = ERIGHT;
		  goto loopend;
	       }
	    }
	 }
	 warning(/*Failed to find station %s in config, line %i*/603, ll, lineno);      
      } else { /* Two arguments look for a specific leg */
	 for (l = headleg.next; l; l=l->next) {
	    point * fr = l->fr;
	    point * to = l->to;
	    if (fr && to) {
	       for (s=fr->stns; s; s=s->next) {
		  int b = 0;
		  if (strcmp(s->label,ll)==0 || (strcmp(s->label, ln)==0 && (b = 1)) ) {
		     char * lr = (b ? ll : ln);
		     for (t=to->stns; t; t=t->next) {
			if (strcmp(t->label,lr)==0) {
			   printf(msg(/*Plotting to the right from leg %s -> %s*/609), s->label, t->label);
			   printf("\n");
			   l->fDir=ERIGHT;
			   goto loopend;
			}
		     }
		  }
	       }
	    }
	 }
	 warning(/*Failed to find leg %s-%s in config, line %i*/606, ll, ln, lineno);
      }
   } else if (strcmp(ln, "*eswap")==0) {
      char *ll = delimword(lc, &lc);
      if (*ll == 0) fatalerror(/*Command without station name in config, line %i*/602,lineno);
      ln = delimword(lc, &lc);
      if (*ln == 0) { /* One argument, look for point to switch at. */
	 for (p = headpoint.next; p != NULL; p = p->next) {
	    for (s = p->stns; s; s = s->next) {
	       if (strcmp(s->label, ll)==0) {
		  printf(msg(/*Swapping plot direction from station %s*/615),ll);
		  putnl();
		  p->fDir = ESWAP;
		  goto loopend;
	       }
	    }
	 }
	 warning(/*Failed to find station %s in config, line %i*/603, ll, lineno);      
      } else { /* Two arguments look for a specific leg */
	 for (l = headleg.next; l; l=l->next) {
	    point * fr = l->fr;
	    point * to = l->to;
	    if (fr && to) {
	       for (s=fr->stns; s; s=s->next) {
		  int b = 0;
		  if (strcmp(s->label,ll)==0 || (strcmp(s->label, ln)==0 && (b = 1)) ) {
		     char * lr = (b ? ll : ln);
		     for (t=to->stns; t; t=t->next) {
			if (strcmp(t->label,lr)==0) {
			   printf(msg(/*Swapping plot direction from leg %s -> %s*/616), s->label, t->label);
			   printf("\n");
			   l->fDir = ESWAP;
			   goto loopend;
			}
		     }
		  }
	       }
	    }
	 }
	 warning(/*Failed to find leg %s-%s in config, line %i*/606, ll, ln, lineno);
      }
   } else if (strcmp(ln, "*break")==0) {
      char *ll = delimword(lc, &lc);
      if (*ll == 0) fatalerror(/*Command without station name in config, line %i*/602,lineno);
      ln = delimword(lc, &lc);
      if (*ln == 0) { /* One argument, look for point to break at. */
	 for (p = headpoint.next; p != NULL; p = p->next) {
	    for (s = p->stns; s; s = s->next) {
	       if (strcmp(s->label, ll)==0) {
		  printf(msg(/*Breaking survey at station %s*/610), ll);
		  putnl();
		  p->fDone = BREAK;
		  goto loopend;
	       }
	    }
	 }
	 warning(/*Failed to find station %s in config, line %i*/603, ll, lineno);  
      } else { /* Two arguments look for a specific leg */
	 for (l = headleg.next; l; l=l->next) {
	    point * fr = l->fr;
	    point * to = l->to;
	    if (fr && to ) {
	       for (s=fr->stns; s; s=s->next) {
		  int b = 0;
		  if (strcmp(s->label,ll)==0 || (strcmp(s->label, ln)==0 && (b = 1)) ) {
		     char * lr = (b ? ll : ln);
		     for (t=to->stns; t; t=t->next) {
			if (strcmp(t->label,lr)==0) {
			   printf(msg(/*Breaking survey at leg %s -> %s*/611), s->label, t->label);
			   putnl();
			   l->fDone = (b ? BREAK_TO : BREAK_FR);
			   goto loopend;
			}
		     }
		  }
	       }
	    }
	 }
	 warning(/*Failed to find leg %s-%s in config, line %i*/606, ll, ln, lineno);
      }
   } else {
      fatalerror(/*Don't understand command `%s' in config, line %i*/600, ln, lineno);
   }
 loopend:
   ln = delimword(lc, &lc);
   if (*ln != 0) {
      fatalerror(/*Unexpected content `%s' in config, line %i*/601, ln, lineno);
   }
}

static const struct option long_opts[] = {
   /* const char *name; int has_arg (0 no_argument, 1 required_*, 2 optional_*); int *flag; int val; */
   {"survey", required_argument, 0, 's'},
   {"specfile", required_argument, 0, 'p'},
   {"help", no_argument, 0, HLP_HELP},
   {"version", no_argument, 0, HLP_VERSION},
   {0, 0, 0, 0}
};

#define short_opts "s:p:"

static struct help_msg help[] = {
/*				<-- */
   {HLP_ENCODELONG(0),          "only load the sub-survey with this prefix"},
   {HLP_ENCODELONG(1),          "apply specifications from the named file"},
   {0, 0}
};

int
main(int argc, char **argv)
{
   const char *fnm_in, *fnm_out;
   char *desc;
   img_point pt;
   int result;
   point *fr = NULL, *to;
   double zMax = -DBL_MAX;
   point *p;
   const char *survey = NULL;
   const char *specfile = NULL;

   msg_init(argv);

   cmdline_set_syntax_message("INPUT_3D_FILE [OUTPUT_3D_FILE]", NULL);
   cmdline_init(argc, argv, short_opts, long_opts, NULL, help, 1, 2);
   while (1) {
      int opt = cmdline_getopt();
      if (opt == EOF) break;
      if (opt == 's') survey = optarg;
      if (opt == 'p') specfile = optarg;
   }
   fnm_in = argv[optind++];
   if (argv[optind]) {
      fnm_out = argv[optind];
   } else {
      fnm_out = add_ext("extend", EXT_SVX_3D);
   }

   /* try to open image file, and check it has correct header */
   pimg = img_open_survey(fnm_in, survey);
   if (pimg == NULL) fatalerror(img_error(), fnm_in);

   putnl();
   puts(msg(/*Reading in data - please wait...*/105));

   htab = osmalloc(ossizeof(pfx*) * HTAB_SIZE);
   {
       int i;
       for (i = 0; i < HTAB_SIZE; ++i) htab[i] = NULL;
   }

   do {
      result = img_read_item(pimg, &pt);
      switch (result) {
      case img_MOVE:
	 fr = find_point(&pt);
	 break;
      case img_LINE:
	 if (!fr) {
	    result = img_BAD;
	    break;
	 }
	 to = find_point(&pt);
	 if (!(pimg->flags & (img_FLAG_SURFACE|img_FLAG_SPLAY)))
	    add_leg(fr, to, pimg->label, pimg->flags);
	 fr = to;
	 break;
      case img_LABEL:
	 to = find_point(&pt);
	 add_label(to, pimg->label, pimg->flags);
	 break;
      case img_BAD:
	 (void)img_close(pimg);
	 fatalerror(img_error(), fnm_in);
      }
   } while (result != img_STOP);

   desc = osmalloc(strlen(pimg->title) + 11 + 1);
   strcpy(desc, pimg->title);
   strcat(desc, " (extended)");

   (void)img_close(pimg);

   if (specfile) {
      FILE *fs = NULL;
      printf(msg(/*Applying specfile: `%s'*/613), specfile);
      putnl();
      fs = fopenWithPthAndExt("", specfile, NULL, "r", NULL);
      if (fs == NULL) fatalerror(/*Unable to open file*/93, specfile);
      while (!feof(fs)) {
	 char *lbuf = NULL;
	 lbuf = getline_alloc(fs, 32);
	 lineno++;
	 if (lbuf) {
	    parseconfigline(lbuf);
	 } else {
	    error(/*Error reading line %i from spec file*/612, lineno);
	    osfree(lbuf);
	    break;
	 }
	 osfree(lbuf);
      }
   }

   if (start == NULL) { /* i.e. start wasn't specified in specfile */
   
      /* start at the highest entrance with some legs attached */
      for (p = headpoint.next; p != NULL; p = p->next) {
	 if (p->order > 0 && p->p.z > zMax) {
	    const stn *s;
	    for (s = p->stns; s; s = s->next) {
	       if (s->flags & img_SFLAG_ENTRANCE) {
		  start = p;
		  zMax = p->p.z;
		  break;
	       }
	    }
	 }
      }
      if (start == NULL) {
	 /* if no entrances with legs, start at the highest 1-node */
	 for (p = headpoint.next; p != NULL; p = p->next) {
	    if (p->order == 1 && p->p.z > zMax) {
	       start = p;
	       zMax = p->p.z;
	    }
	 }
	 /* of course we may have no 1-nodes... */
	 if (start == NULL) {
	    for (p = headpoint.next; p != NULL; p = p->next) {
	       if (p->order != 0 && p->p.z > zMax) {
		  start = p;
		  zMax = p->p.z;
	       }
	    }
	    if (start == NULL) {
	       /* There are no legs - just pick the highest station... */
	       for (p = headpoint.next; p != NULL; p = p->next) {
		  if (p->p.z > zMax) {
		     start = p;
		     zMax = p->p.z;
		  }
	       }
	       if (!start) fatalerror(/*No survey data*/43);
	    }
	 }
      }
   }

   printf(msg(/*Writing out .3d file...*/614));
   putnl();
   pimg = img_open_write(fnm_out, desc, fTrue);

   do_stn(start, 0.0, NULL, ERIGHT); /* only does single connected component currently */
   if (!img_close(pimg)) {
      (void)remove(fnm_out);
      fatalerror(img_error(), fnm_out);
   }

   return EXIT_SUCCESS;
}

static void
do_stn(point *p, double X, const char *prefix, int dir)
{
   leg *l, *lp;
   double dX;
   const stn *s;

   for (s = p->stns; s; s = s->next) {
      img_write_item(pimg, img_LABEL, s->flags, s->label, X, 0, p->p.z);
   }
   if (p->fDone & BREAK) {
     return;
   }

   lp = &headleg;
   for (l = lp->next; l; lp = l, l = lp->next) {
      if (l->fDone & DONE) {
	 /* this case happens if a recursive call causes the next leg to be
	  * removed, leaving our next pointing to a leg which has been dealt
	  * with... */
      } else if (l->prefix == prefix) {
	 if (l->to == p) {
	    if (l->fDone & BREAK_TO) continue;
	    lp->next = l->next;
	    /* adjust direction of extension if necessary */
	    if (l->to->fDir == ESWAP)               dir = (dir==ERIGHT ? ELEFT : ERIGHT);
	    else if (l->to->fDir & (ERIGHT|ELEFT))  dir = l->to->fDir;
	    if (l->fDir == ESWAP)                   dir = (dir==ERIGHT ? ELEFT : ERIGHT);
	    else if (l->fDir & (ERIGHT|ELEFT))      dir = l->fDir;

	    dX = hypot(l->fr->p.x - l->to->p.x, l->fr->p.y - l->to->p.y);
	    if (dir == ELEFT) dX *= -1.0;
	    img_write_item(pimg, img_MOVE, 0, NULL, X + dX, 0, l->fr->p.z);
	    img_write_item(pimg, img_LINE, l->flags, l->prefix,
			   X, 0, l->to->p.z);
	    l->fDone |= DONE;
	    do_stn(l->fr, X + dX, l->prefix, dir);
	    l = lp;
	 } else if (l->fr == p) {
	    if (l->fDone & BREAK_FR) continue;
	    lp->next = l->next;
	    /* adjust direction of extension if necessary */
	    if (l->fr->fDir == ESWAP)               dir = (dir==ERIGHT ? ELEFT : ERIGHT);
	    else if (l->fr->fDir & (ERIGHT|ELEFT))  dir = l->fr->fDir;
	    if (l->fDir == ESWAP)                   dir = (dir==ERIGHT ? ELEFT : ERIGHT);
	    else if (l->fDir & (ERIGHT|ELEFT))      dir = l->fDir;

	    dX = hypot(l->fr->p.x - l->to->p.x, l->fr->p.y - l->to->p.y);
	    if (dir == ELEFT) dX *= -1.0;
	    img_write_item(pimg, img_MOVE, 0, NULL, X, 0, l->fr->p.z);
	    img_write_item(pimg, img_LINE, l->flags, l->prefix,
			   X + dX, 0, l->to->p.z);
	    l->fDone |= DONE;
	    do_stn(l->to, X + dX, l->prefix, dir);
	    l = lp;
	 }
      }
   }
   lp = &headleg;
   for (l = lp->next; l; lp = l, l = lp->next) {
      if (l->fDone & DONE) {
	 /* this case happens iff a recursive call causes the next leg to be
	  * removed, leaving our next pointing to a leg which has been dealt
	  * with... */
      } else {
	 if (l->to == p) {
	    if (l->fDone & BREAK_TO) continue;
	    lp->next = l->next;
	    /* adjust direction of extension if necessary */
	    if (l->to->fDir == ESWAP)               dir = (dir==ERIGHT ? ELEFT : ERIGHT);
	    else if (l->to->fDir & (ERIGHT|ELEFT))  dir = l->to->fDir;
	    if (l->fDir == ESWAP)                   dir = (dir==ERIGHT ? ELEFT : ERIGHT);
	    else if (l->fDir & (ERIGHT|ELEFT))      dir = l->fDir;
	    
	    dX = hypot(l->fr->p.x - l->to->p.x, l->fr->p.y - l->to->p.y);
	    if (dir == ELEFT) dX *= -1.0;
	    img_write_item(pimg, img_MOVE, 0, NULL, X + dX, 0, l->fr->p.z);
	    img_write_item(pimg, img_LINE, l->flags, l->prefix,
			   X, 0, l->to->p.z);
	    l->fDone |= DONE;
	    do_stn(l->fr, X + dX, l->prefix, dir);
	    l = lp;
	 } else if (l->fr == p) {
	    if (l->fDone & BREAK_FR) continue;
	    lp->next = l->next;
	    /* adjust direction of extension if necessary */
	    if (l->fr->fDir == ESWAP)               dir = (dir==ERIGHT ? ELEFT : ERIGHT);
	    else if (l->fr->fDir & (ERIGHT|ELEFT))  dir = l->fr->fDir;
	    if (l->fDir == ESWAP)                   dir = (dir==ERIGHT ? ELEFT : ERIGHT);
	    else if (l->fDir & (ERIGHT|ELEFT))      dir = l->fDir;

	    dX = hypot(l->fr->p.x - l->to->p.x, l->fr->p.y - l->to->p.y);
	    if (dir == ELEFT) dX *= -1.0;
	    img_write_item(pimg, img_MOVE, 0, NULL, X, 0, l->fr->p.z);
	    img_write_item(pimg, img_LINE, l->flags, l->prefix,
			   X + dX, 0, l->to->p.z);
	    l->fDone |= DONE;
	    do_stn(l->to, X + dX, l->prefix, dir);
	    l = lp;
	 }
      }
   }
}
