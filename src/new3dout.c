/* new3dout.c
 * .3dx writing routines
 * Copyright (C) 2000, 2001 Phil Underwood
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

#include <config.h>
#include "cavern.h"

#ifdef NEW3DFORMAT

#include <stdio.h>
#include <math.h>

#include "3ddefs.h"
#include "debug.h"

twig *rhizome, *limb;
char *firstfilename, *startingdir;
int fUseNewFormat = 0;

static enum {
   IMG_NONE = 0,
   IMG_FILENOTFOUND = /*Couldn't open data file `%s'*/24,
   IMG_OUTOFMEMORY  = /*Out of memory %.0s*/38,
   IMG_DIRECTORY    = /*Filename `%s' refers to directory*/44,
   IMG_CANTOPENOUT  = /*Failed to open output file `%s'*/47,
   IMG_BADFORMAT    = /*Bad 3d image file `%s'*/106
} img_errno = IMG_NONE;

/* put counted string */
static int
fputcs(const char *s, FILE *fh)
{
   int len = strlen(s);
   /* if the label is empty or > 255 chars. store 0 in the length byte
    * followed by a 32 bit length value */
   if (len == 0 || len > 255) {
      if (putc(0, fh) == EOF) return EOF;
      put32((INT32_T)len, fh);
      if (ferror(fh)) return EOF;
   } else {
      if (putc((unsigned char)len, fh) == EOF) return EOF;
   }
   return fputs(s, fh);
}

/* length on disk of counted string */
static unsigned int
cslen(const char *s)
{
   int len = strlen(s);
   /* if the label is empty or > 255 chars. store 0 in the length byte
    * followed by a 32 bit length value */
   if (len == 0 || len > 255) return len + 5;
   return len + 1;
}

static char *basesource;
static int statcount = 1; /* this is the total number of things added */

static int
scount(twig *twiglet)
{
  twig *lib;

  if (twiglet->from) return 1;

  ASSERT(twiglet->to);

  lib = twiglet->down;
  twiglet->count = 0;
  while (lib->right) {
    lib = lib->right;
    twiglet->count += scount(lib);
  }
  return (twiglet->count ? 1 : 0);
}

void
cave_write_title(const char *title, img *pimg)
{
   putc(TITLE_3D, pimg->fh);
   fputcs(title, pimg->fh);
}

void
cave_write_stn(node *nod)
{
    nod = nod; /* avoid compiler warnings */
}

static int
cave_write_pos(pos *pid, prefix *pre)
{
   if (pid->id == 0) {
      const char *tag;
      unsigned int len;
      pid->id = (INT32_T)statcount;
      tag = pre->ident;
      len = cslen(tag) + 12 + 4;
      /* storage station name, 12 for data, 4 for id */
      putc(STATION_3D, pimg->fh);
      if (len == 0 || len > 255) {
	 if (putc(0, pimg->fh) == EOF) return EOF;
	 put32((INT32_T)len, pimg->fh);
	 if (ferror(pimg->fh)) return EOF;
      } else {
	 if (putc((unsigned char)len, pimg->fh) == EOF) return EOF;
      }
      put32((INT32_T)statcount, pimg->fh); /* station ID */
      put32((INT32_T)(pid->p[0] * 100.0), pimg->fh); /* X in cm */
      put32((INT32_T)(pid->p[1] * 100.0), pimg->fh); /* Y */
      put32((INT32_T)(pid->p[2] * 100.0), pimg->fh); /* Z */
      fputcs(tag, pimg->fh);
      statcount++;
   } else {
      /* we've already put this in the file, so just a link is needed */
      putc(STATLINK_3D, pimg->fh);
      putc(0x04, pimg->fh);
      put32((INT32_T)pid->id, pimg->fh);
   }
   return 0;
}

img *
cave_open_write(const char *fnm, const char *title)
{
   img *pimg;
   if (fDirectory(fnm)) {
      img_errno = IMG_DIRECTORY;
      return NULL;
   }

   pimg = (img *)xosmalloc(ossizeof(img));
   if (pimg == NULL) {
      img_errno = IMG_OUTOFMEMORY;
      return NULL;
   }

   pimg->fh = fopen(fnm, "wb");
   if (!pimg->fh) {
      osfree(pimg);
      img_errno = IMG_CANTOPENOUT;
      return NULL;
   }
   fputs("SVX3d", pimg->fh);
   putc(0x00, pimg->fh); /* Major version 0 */
   putc(0x0A, pimg->fh); /* Minor version 10*/
   put32(0, pimg->fh); /* dummy number for num of stations: fill later */
   cave_write_title(title, pimg);
   return pimg;
}

int
cave_error(void)
{
   return img_errno;
}

void
cave_write_leg(linkfor *leg)
{
   leg = leg; /* avoid compiler warnings */
}

void
cave_write_source(const char *source)
{
  /* create a relative path, given an absolute dir, and an absolute file */
  /* part 1. find where they differ */
  char *allocated_source = NULL;
  /* is it an absolute? */
  if (!fAbsoluteFnm(source)) {
    allocated_source = use_path(startingdir, source);
    source = allocated_source;
  }

  /* strip source down to just a file name... */
  if (strncmp(source, basesource, strlen(basesource)) != 0) {
    printf("Warning: source links may not work\n");
  } else {
    source = source + strlen(basesource);
  }
  putc(SOURCE_3D, pimg->fh);
  fputcs(source, pimg->fh);
  osfree(allocated_source);
}

static void
save3d(twig *sticky)
{
  unsigned int ltag;
  unsigned int stubcount;
  double err, length, offset;
  twig *twiglet;
  for (twiglet = sticky->right; twiglet; twiglet = twiglet->right) {
    if (twiglet->from) {
      if (!twiglet->to) { /* fixed point */
	cave_write_pos(twiglet->from->pos,twiglet->from);
      } else { /*leg */
	/* calculate an average percentage error, based on %age change of polars */
	/* offset is how far the _to_ point is from where we thought it would be */
	offset = hypot(twiglet->to->pos->p[2] - (twiglet->from->pos->p[2] +
						 twiglet->delta[2]),
		       hypot(twiglet->to->pos->p[0] - (twiglet->from->pos->p[0] +
						       twiglet->delta[0]),
			     twiglet->to->pos->p[1] - (twiglet->from->pos->p[1] +
						       twiglet->delta[1])));
	/* length is our reconstituted expected length */
	length = hypot(hypot(twiglet->delta[0], twiglet->delta[1]),
		       twiglet->delta[2]);
#ifdef DEBUG
	if (strcmp(twiglet->from->ident, "86") == 0)
	  printf("deltas... %g %g %g length %g offset %g",
		 twiglet->delta[0], twiglet->delta[1], twiglet->delta[2],
		 length, offset);
#endif
	if (fabs(length) < 0.01) {
	  err = 0; /* What is the error for a leg with zero length?
		    * Phil -  he say none. */
	} else {
	  err = 10000.0 * offset / length;
	}
	putc(LEG_3D, pimg->fh);
	put16((INT16_T)0x02, pimg->fh);
	putc(0x04, pimg->fh);
	put32((INT32_T)err, pimg->fh); /* output error in %*100 */
	cave_write_pos(twiglet->from->pos, twiglet->from);
	cave_write_pos(twiglet->to->pos, twiglet->to);
      }
    } else {
      if (twiglet->count) {
	putc(BRANCH_3D, pimg->fh);
	/* number of records  - legs + values */
	stubcount = 0;
	if (twiglet->source) stubcount++;
	if (twiglet->drawings) stubcount++;
	if (twiglet->instruments) stubcount++;
	if (twiglet->tape) stubcount++;
	if (twiglet->date) stubcount++;
	stubcount += twiglet->count;
	if (stubcount > 32767) {
	  put16(0, pimg->fh);
	  put32(stubcount, pimg->fh);
	} else {
	  put16((unsigned short)stubcount, pimg->fh);
	}
	ltag = cslen(twiglet->to->ident);
	if (ltag < 255) {
	  putc((unsigned char)(ltag + 1), pimg->fh);
	} else {
	  putc(0, pimg->fh);
	  put32(ltag + 1, pimg->fh);
	}
	fputcs(twiglet->to->ident, pimg->fh);
	if (twiglet->source) cave_write_source(twiglet->source);
	if (twiglet->date) {
	  putc(DATE_3D, pimg->fh);
	  fputcs(twiglet->date, pimg->fh);
	}
	if (twiglet->drawings) {
	  putc(DRAWINGS_3D, pimg->fh);
	  fputcs(twiglet->drawings, pimg->fh);
	}
	if (twiglet->instruments) {
	  putc(INSTRUMENTS_3D, pimg->fh);
	  fputcs(twiglet->instruments, pimg->fh);
	}
	if (twiglet->tape) {
	  putc(TAPE_3D, pimg->fh);
	  fputcs(twiglet->tape, pimg->fh);
	}
	save3d(twiglet->down);
      }
    }
  }
}

static void
cave_write_base_source(void)
{
  char *temp;
  /* is it an absolute? */
  if (fAbsoluteFnm(firstfilename)) {
    basesource = path_from_fnm(firstfilename);
  } else {
    temp = use_path(startingdir,firstfilename);
    basesource = path_from_fnm(temp);
    osfree(temp);
  }

#if 0
  /* remove any ./../. etcs but not today */
  temp = osstrdup(basesource);
  realpath(temp, basesource);
#endif

  putc(BASE_SOURCE_3D, pimg->fh);
  fputcs(basesource, pimg->fh);

  /* get the actual file name */
  temp = leaf_from_fnm(firstfilename);
  putc(BASE_FILE_3D, pimg->fh);
  fputcs(temp, pimg->fh);
  osfree(temp);
}

int
cave_close(img *pimg)
{
   int result = 1;
   /* let's do the twiglet traverse! */
   twig *twiglet = rhizome->down;
   cave_write_base_source();
   scount(rhizome);
   save3d(twiglet);
   if (pimg->fh) {
      /* Write end of data marker */
      putc(END_3D, pimg->fh);
      /* and finally write how many stations there are */
      fseek(pimg->fh, 7L, SEEK_SET);
      put32((INT32_T)statcount, pimg->fh);
      if (ferror(pimg->fh)) result = 0;
      if (fclose(pimg->fh) == EOF) result = 0;
   }
   osfree(pimg);
   return result;
}

void
create_twig(prefix *pre, const char *fname)
{
  twig *twiglet;
  twig *lib;
  lib = get_twig(pre->up); /* get the active twig for parent's prefix */
  twiglet = osnew(twig);
  twiglet->from = NULL;
  twiglet->to = pre;
  twiglet->sourceval=0; /* lowest priority */
  pre->twig_link = twiglet; /* connect both ways */
  twiglet->right = NULL;
  twiglet->drawings = twiglet->date = twiglet->tape = twiglet->instruments
     = NULL;
  twiglet->source = osstrdup(fname);
  if (lib) {
    twiglet->count = lib->count + 1;
    /* ie we are changing the global attachment update...*/
    if (lib == limb) limb = twiglet;
    lib->right = twiglet;
    twiglet->up = lib->up;
  } else {
    twiglet->up = NULL;
  }
  lib = twiglet;
  twiglet = osnew(twig);
  twiglet->to = twiglet->from = NULL;
  twiglet->up = lib;
  twiglet->sourceval = 0;
  twiglet->drawings = twiglet->source = twiglet->instruments
     = twiglet->date = twiglet->tape = NULL;
  twiglet->down = twiglet->right = NULL;
  twiglet->count=0;
  lib->down = twiglet;
}

twig *
get_twig(prefix *pre)
{
   twig *temp;
   if (pre && pre->twig_link) {
     temp = pre->twig_link->down;
     if (temp) while (temp->right) temp = temp->right;
     return temp;
   }
   return NULL;
}

#endif
