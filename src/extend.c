/* extend.c
 * Produce an extended elevation
 * Copyright (C) 1995-2002,2005,2010,2011,2013,2014,2016 Olly Betts
 * Copyright (C) 2004,2005 John Pybus
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

#include <float.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cmdline.h"
#include "debug.h"
#include "filelist.h"
#include "filename.h"
#include "hash.h"
#include "img_hosted.h"
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
   double X;
   const stn *stns;
   unsigned int order;
   char dir;
   char fDone;
   char fBroken;
   struct POINT *next;
} point;

typedef struct LEG {
   point *fr, *to;
   const char *prefix;
   char dir;
   char fDone;
   char broken;
   int flags;
   struct LEG *next;
} leg;

/* Values for leg.broken: */
#define BREAK_FR    0x01
#define BREAK_TO    0x02

/* Values for point.dir and leg.dir: */
#define ELEFT  0x01
#define ERIGHT 0x02
#define ESWAP  0x04

static point headpoint = {{0, 0, 0}, 0, NULL, 0, 0, 0, 0, NULL};

static leg headleg = {NULL, NULL, NULL, 0, 0, 0, 0, NULL};

static img *pimg_out;

static int show_breaks = 0;

static void do_stn(point *, double, const char *, int, int);

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
   p->X = HUGE_VAL;
   p->stns = NULL;
   p->order = 0;
   p->dir = 0;
   p->fDone = 0;
   p->fBroken = 0;
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
   l->dir = 0;
   l->fDone = 0;
   l->broken = 0;
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

   ch = GETC(fh);
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
      ch = GETC(fh);
   }
   if (ch == '\n' || ch == '\r') {
      int otherone = ch ^ ('\n' ^ '\r');
      ch = GETC(fh);
      /* if it's not the other eol character, put it back */
      if (ch != otherone) ungetc(ch, fh);
   }
   buf[i++] = '\0';
   return buf;
}

static int lineno = 0;
static point *start = NULL;

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
parseconfigline(const char *fnm, char *ln)
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
      if (*ln == 0)
	 /* TRANSLATORS: Here "station" is a survey station, not a train station. */
	 fatalerror_in_file(fnm, lineno, /*Expecting station name*/28);
      for (p = headpoint.next; p != NULL; p = p->next) {
	 for (s = p->stns; s; s = s->next) {
	    if (strcmp(s->label, ln)==0) {
	       start = p;
	       /* TRANSLATORS: for extend: "extend" is starting to produce an extended elevation from station %s */
	       printf(msg(/*Starting from station %s*/512),ln);
	       putnl();
	       goto loopend;
	    }
	 }
      }
      /* TRANSLATORS: for extend: the user specified breaking a loop or
       * changing extend direction at this station, but we didn’t find it in
       * the 3d file */
      warning_in_file(fnm, lineno, /*Failed to find station %s*/510, ln);
   } else if (strcmp(ln, "*eleft")==0) {
      char *ll = delimword(lc, &lc);
      if (*ll == 0)
	 fatalerror_in_file(fnm, lineno, /*Expecting station name*/28);
      ln = delimword(lc, &lc);
      if (*ln == 0) {
	 /* One argument - look for station to switch at. */
	 for (p = headpoint.next; p != NULL; p = p->next) {
	    for (s = p->stns; s; s = s->next) {
	       if (strcmp(s->label, ll)==0) {
		  /* TRANSLATORS: for extend: */
		  printf(msg(/*Extending to the left from station %s*/513), ll);
		  putnl();
		  p->dir = ELEFT;
		  goto loopend;
	       }
	    }
	 }
	 warning_in_file(fnm, lineno, /*Failed to find station %s*/510, ll);
      } else {
	 /* Two arguments - look for a specified leg. */
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
			   /* TRANSLATORS: for extend: */
			   printf(msg(/*Extending to the left from leg %s → %s*/515), s->label, t->label);
			   putnl();
			   l->dir = ELEFT;
			   goto loopend;
			}
		     }
		  }
	       }
	    }
	 }
	 /* TRANSLATORS: for extend: the user specified breaking a loop or
	  * changing extend direction at this leg, but we didn’t find it in the
	  * 3d file */
	 warning_in_file(fnm, lineno, /*Failed to find leg %s → %s*/511, ll, ln);
      }
   } else if (strcmp(ln, "*eright")==0) {
      char *ll = delimword(lc, &lc);
      if (*ll == 0)
	 fatalerror_in_file(fnm, lineno, /*Expecting station name*/28);
      ln = delimword(lc, &lc);
      if (*ln == 0) {
	 /* One argument - look for station to switch at. */
	 for (p = headpoint.next; p != NULL; p = p->next) {
	    for (s = p->stns; s; s = s->next) {
	       if (strcmp(s->label, ll)==0) {
		  /* TRANSLATORS: for extend: */
		  printf(msg(/*Extending to the right from station %s*/514), ll);
		  putnl();
		  p->dir = ERIGHT;
		  goto loopend;
	       }
	    }
	 }
	 warning_in_file(fnm, lineno, /*Failed to find station %s*/510, ll);
      } else {
	 /* Two arguments - look for a specified leg. */
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
			   /* TRANSLATORS: for extend: */
			   printf(msg(/*Extending to the right from leg %s → %s*/516), s->label, t->label);
			   putnl();
			   l->dir=ERIGHT;
			   goto loopend;
			}
		     }
		  }
	       }
	    }
	 }
	 warning_in_file(fnm, lineno, /*Failed to find leg %s → %s*/511, ll, ln);
      }
   } else if (strcmp(ln, "*eswap")==0) {
      char *ll = delimword(lc, &lc);
      if (*ll == 0)
	 fatalerror_in_file(fnm, lineno, /*Expecting station name*/28);
      ln = delimword(lc, &lc);
      if (*ln == 0) {
	 /* One argument - look for station to switch at. */
	 for (p = headpoint.next; p != NULL; p = p->next) {
	    for (s = p->stns; s; s = s->next) {
	       if (strcmp(s->label, ll)==0) {
		  /* TRANSLATORS: for extend: */
		  printf(msg(/*Swapping extend direction from station %s*/519),ll);
		  putnl();
		  p->dir = ESWAP;
		  goto loopend;
	       }
	    }
	 }
	 warning_in_file(fnm, lineno, /*Failed to find station %s*/510, ll);
      } else {
	 /* Two arguments - look for a specified leg. */
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
			   /* TRANSLATORS: for extend: */
			   printf(msg(/*Swapping extend direction from leg %s → %s*/520), s->label, t->label);
			   putnl();
			   l->dir = ESWAP;
			   goto loopend;
			}
		     }
		  }
	       }
	    }
	 }
	 warning_in_file(fnm, lineno, /*Failed to find leg %s → %s*/511, ll, ln);
      }
   } else if (strcmp(ln, "*break")==0) {
      char *ll = delimword(lc, &lc);
      if (*ll == 0)
	 fatalerror_in_file(fnm, lineno, /*Expecting station name*/28);
      ln = delimword(lc, &lc);
      if (*ln == 0) {
	 /* One argument - look for specified station to break at. */
	 for (p = headpoint.next; p != NULL; p = p->next) {
	    for (s = p->stns; s; s = s->next) {
	       if (strcmp(s->label, ll)==0) {
		  /* TRANSLATORS: for extend: */
		  printf(msg(/*Breaking survey loop at station %s*/517), ll);
		  putnl();
		  p->fBroken = 1;
		  goto loopend;
	       }
	    }
	 }
	 warning_in_file(fnm, lineno, /*Failed to find station %s*/510, ll);
      } else {
	 /* Two arguments - look for specified leg and disconnect it at the
	  * first station. */
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
			   /* TRANSLATORS: for extend: */
			   printf(msg(/*Breaking survey loop at leg %s → %s*/518), s->label, t->label);
			   putnl();
			   l->broken = (b ? BREAK_TO : BREAK_FR);
			   goto loopend;
			}
		     }
		  }
	       }
	    }
	 }
	 warning_in_file(fnm, lineno, /*Failed to find leg %s → %s*/511, ll, ln);
      }
   } else {
      fatalerror_in_file(fnm, lineno, /*Unknown command “%s”*/12, ln);
   }
 loopend:
   ln = delimword(lc, &lc);
   if (*ln != 0) {
      fatalerror_in_file(fnm, lineno, /*End of line not blank*/15);
      /* FIXME: give ln as context? */
   }
}

static const struct option long_opts[] = {
   /* const char *name; int has_arg (0 no_argument, 1 required_*, 2 optional_*); int *flag; int val; */
   {"survey", required_argument, 0, 's'},
   {"specfile", required_argument, 0, 'p'},
   {"show-breaks", no_argument, 0, 'b' },
   {"help", no_argument, 0, HLP_HELP},
   {"version", no_argument, 0, HLP_VERSION},
   {0, 0, 0, 0}
};

#define short_opts "s:p:b"

static struct help_msg help[] = {
/*				<-- */
   {HLP_ENCODELONG(0),        /*only load the sub-survey with this prefix*/199, 0},
   /* TRANSLATORS: --help output for extend --specfile option */
   {HLP_ENCODELONG(1),        /*.espec file to control extending*/90, 0},
   /* TRANSLATORS: --help output for extend --show-breaks option */
   {HLP_ENCODELONG(2),        /*show breaks with surface survey legs in output*/91, 0},
   {0, 0, 0}
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
   img *pimg;
   int have_xsect = 0;

   msg_init(argv);

   /* TRANSLATORS: Part of extend --help */
   cmdline_set_syntax_message(/*INPUT_3D_FILE [OUTPUT_3D_FILE]*/267, 0, NULL);
   cmdline_init(argc, argv, short_opts, long_opts, NULL, help, 1, 2);
   while (1) {
      int opt = cmdline_getopt();
      if (opt == EOF) break;
      switch (opt) {
	 case 'b':
	    show_breaks = 1;
	    break;
	 case 's':
	    survey = optarg;
	    break;
	 case 'p':
	    specfile = optarg;
	    break;
      }
   }
   fnm_in = argv[optind++];
   if (argv[optind]) {
      fnm_out = argv[optind];
   } else {
      char * base_in = base_from_fnm(fnm_in);
      char * base_out = osmalloc(strlen(base_in) + 8);
      strcpy(base_out, base_in);
      strcat(base_out, "_extend");
      fnm_out = add_ext(base_out, EXT_SVX_3D);
      osfree(base_in);
      osfree(base_out);
   }

   /* try to open image file, and check it has correct header */
   pimg = img_open_survey(fnm_in, survey);
   if (pimg == NULL) fatalerror(img_error2msg(img_error()), fnm_in);

   putnl();
   puts(msg(/*Reading in data - please wait…*/105));

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
	 fatalerror(img_error2msg(img_error()), fnm_in);
	 break;
      case img_XSECT:
	 have_xsect = 1;
	 break;
      }
   } while (result != img_STOP);

   desc = osstrdup(pimg->title);

   if (specfile) {
      FILE *fs = NULL;
      char *fnm_used;
      /* TRANSLATORS: for extend: */
      printf(msg(/*Applying specfile: “%s”*/521), specfile);
      putnl();
      fs = fopenWithPthAndExt("", specfile, NULL, "r", &fnm_used);
      if (fs == NULL) fatalerror(/*Couldn’t open file “%s”*/24, specfile);
      while (!feof(fs)) {
	 char *lbuf = getline_alloc(fs, 32);
	 lineno++;
	 if (!lbuf)
	    fatalerror_in_file(fnm_used, lineno, /*Error reading file*/18);
	 parseconfigline(fnm_used, lbuf);
	 osfree(lbuf);
      }
      osfree(fnm_used);
   }

   if (start == NULL) {
      /* *start wasn't specified in specfile.
       *
       * Start at the highest entrance with some legs attached.
       */
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
	 /* If no entrances with legs, start at the highest 1-node. */
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

   /* TRANSLATORS: for extend:
    * Used to tell the user that a file is being written - %s is the filename
    */
   printf(msg(/*Writing %s…*/522), fnm_out);
   putnl();
   pimg_out = img_open_write(fnm_out, desc, img_FFLAG_EXTENDED);

   /* Only does single connected component currently. */
   do_stn(start, 0.0, NULL, ERIGHT, 0);

   if (have_xsect) {
      img_rewind(pimg);
      /* Read ahead on pimg before writing pimg_out so we find out if an
       * img_XSECT_END comes next. */
      char * label = NULL;
      int flags = 0;
      do {
	 result = img_read_item(pimg, &pt);
	 if (result == img_XSECT || result == img_XSECT_END) {
	    if (label) {
	       if (result == img_XSECT_END)
		  flags |= img_XFLAG_END;
	       img_write_item(pimg_out, img_XSECT, flags, label, 0, 0, 0);
	       osfree(label);
	       label = NULL;
	    }
	 }
	 if (result == img_XSECT) {
	    label = osstrdup(pimg->label);
	    flags = pimg->flags;
	    pimg_out->l = pimg->l;
	    pimg_out->r = pimg->r;
	    pimg_out->u = pimg->u;
	    pimg_out->d = pimg->d;
	 }
      } while (result != img_STOP);
   }

   (void)img_close(pimg);

   if (!img_close(pimg_out)) {
      (void)remove(fnm_out);
      fatalerror(img_error2msg(img_error()), fnm_out);
   }

   return EXIT_SUCCESS;
}

static int adjust_direction(int dir, int by) {
    if (by == ESWAP)
	return dir ^ (ELEFT|ERIGHT);
    if (by)
	return by;
    return dir;
}

static void
do_stn(point *p, double X, const char *prefix, int dir, int labOnly)
{
   leg *l, *lp;
   double dX;
   const stn *s;
   int odir = dir;

   for (s = p->stns; s; s = s->next) {
      img_write_item(pimg_out, img_LABEL, s->flags, s->label, X, 0, p->p.z);
   }
   if (show_breaks && p->X != HUGE_VAL && p->X != X) {
      /* Draw "surface" leg between broken stations. */
      img_write_item(pimg_out, img_MOVE, 0, NULL, p->X, 0, p->p.z);
      img_write_item(pimg_out, img_LINE, img_FLAG_SURFACE, NULL, X, 0, p->p.z);
   }
   p->X = X;
   if (labOnly || p->fBroken) {
      return;
   }

   /* It's better to follow legs along a survey, so make two passes and only
    * follow legs in the same survey for the first pass.
    */
   for (int try_all = 0; try_all != 2; ++try_all) {
      lp = &headleg;
      for (l = lp->next; l; lp = l, l = lp->next) {
	 dir = odir;
	 if (l->fDone) {
	    /* this case happens iff a recursive call causes the next leg to be
	     * removed, leaving our next pointing to a leg which has been dealt
	     * with... */
	    continue;
	 }
	 if (!try_all && l->prefix != prefix) {
	    continue;
	 }
	 if (l->to == p) {
	    if (l->broken & BREAK_TO) continue;
	    lp->next = l->next;
	    /* adjust direction of extension if necessary */
	    dir = adjust_direction(dir, l->to->dir);
	    dir = adjust_direction(dir, l->dir);

	    dX = hypot(l->fr->p.x - l->to->p.x, l->fr->p.y - l->to->p.y);
	    if (dir == ELEFT) dX = -dX;
	    img_write_item(pimg_out, img_MOVE, 0, NULL, X + dX, 0, l->fr->p.z);
	    img_write_item(pimg_out, img_LINE, l->flags, l->prefix,
			   X, 0, l->to->p.z);
	    l->fDone = 1;
	    do_stn(l->fr, X + dX, l->prefix, dir, (l->broken & BREAK_FR));
	    l = lp;
	 } else if (l->fr == p) {
	    if (l->broken & BREAK_FR) continue;
	    lp->next = l->next;
	    /* adjust direction of extension if necessary */
	    dir = adjust_direction(dir, l->fr->dir);
	    dir = adjust_direction(dir, l->dir);

	    dX = hypot(l->fr->p.x - l->to->p.x, l->fr->p.y - l->to->p.y);
	    if (dir == ELEFT) dX = -dX;
	    img_write_item(pimg_out, img_MOVE, 0, NULL, X, 0, l->fr->p.z);
	    img_write_item(pimg_out, img_LINE, l->flags, l->prefix,
			   X + dX, 0, l->to->p.z);
	    l->fDone = 1;
	    do_stn(l->to, X + dX, l->prefix, dir, (l->broken & BREAK_TO));
	    l = lp;
	 }
      }
   }
}
