/* new3dout.c
 * .3dx writing routines
 * Copyright (C) 2000, 2001 Phil Underwood
 * Copyright (C) 2001, 2002, 2003 Olly Betts
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

#ifdef CHASM3DX

#include <stdio.h>
#include <math.h>

#include "3ddefs.h"
#include "debug.h"
#include "filename.h"

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

  SVX_ASSERT(twiglet->to);

  lib = twiglet->down;
  twiglet->count = 0;
  while (lib->right) {
    lib = lib->right;
    twiglet->count += scount(lib);
  }
  return (twiglet->count ? 1 : 0);
}

static void
cave_write_title(const char *title, img *p_img)
{
   putc(TITLE_3D, p_img->fh);
   fputcs(title, p_img->fh);
}

static int
cave_write_pos(img * p_img, pos *pid, prefix *pre)
{
   if (pid->id == 0) {
      const char *tag;
      unsigned int len;
      pid->id = (INT32_T)statcount;
      tag = pre->ident;
      len = cslen(tag) + 12 + 4;
      /* storage station name, 12 for data, 4 for id */
      putc(STATION_3D, p_img->fh);
      if (len == 0 || len > 255) {
	 if (putc(0, p_img->fh) == EOF) return EOF;
	 put32((INT32_T)len, p_img->fh);
	 if (ferror(p_img->fh)) return EOF;
      } else {
	 if (putc((unsigned char)len, p_img->fh) == EOF) return EOF;
      }
      put32((INT32_T)statcount, p_img->fh); /* station ID */
      put32((INT32_T)(pid->p[0] * 100.0), p_img->fh); /* X in cm */
      put32((INT32_T)(pid->p[1] * 100.0), p_img->fh); /* Y */
      put32((INT32_T)(pid->p[2] * 100.0), p_img->fh); /* Z */
      fputcs(tag, p_img->fh);
      statcount++;
   } else {
      /* we've already put this in the file, so just a link is needed */
      putc(STATLINK_3D, p_img->fh);
      putc(0x04, p_img->fh);
      put32((INT32_T)pid->id, p_img->fh);
   }
   return 0;
}

img *
cave_open_write(const char *fnm, const char *title)
{
   img *p_img;
   if (fDirectory(fnm)) {
      img_errno = IMG_DIRECTORY;
      return NULL;
   }

   p_img = (img *)xosmalloc(ossizeof(img));
   if (p_img == NULL) {
      img_errno = IMG_OUTOFMEMORY;
      return NULL;
   }

   p_img->fh = fopen(fnm, "wb");
   if (!p_img->fh) {
      osfree(p_img);
      img_errno = IMG_CANTOPENOUT;
      return NULL;
   }
   fputs("SVX3d", p_img->fh);
   putc(0x00, p_img->fh); /* Major version 0 */
   putc(0x0A, p_img->fh); /* Minor version 10*/
   put32(0, p_img->fh); /* dummy number for num of stations: fill later */
   cave_write_title(title, p_img);
   return p_img;
}

int
cave_error(void)
{
   return img_errno;
}

static void
cave_write_source(img * p_img, const char *source)
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
  putc(SOURCE_3D, p_img->fh);
  fputcs(source, p_img->fh);
  osfree(allocated_source);
}

static void
save3d(img *p_img, twig *sticky)
{
  unsigned int ltag;
  unsigned int stubcount;
  double err, length, offset;
  twig *twiglet;
  for (twiglet = sticky->right; twiglet; twiglet = twiglet->right) {
    if (twiglet->from) {
      if (!twiglet->to) { /* fixed point */
	cave_write_pos(p_img, twiglet->from->pos, twiglet->from);
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
	putc(LEG_3D, p_img->fh);
	put16((INT16_T)0x02, p_img->fh);
	putc(0x04, p_img->fh);
	put32((INT32_T)err, p_img->fh); /* output error in %*100 */
	cave_write_pos(p_img, twiglet->from->pos, twiglet->from);
	cave_write_pos(p_img, twiglet->to->pos, twiglet->to);
      }
    } else {
      if (twiglet->count) {
	putc(BRANCH_3D, p_img->fh);
	/* number of records  - legs + values */
	stubcount = 0;
	if (twiglet->source) stubcount++;
	if (twiglet->drawings) stubcount++;
	if (twiglet->instruments) stubcount++;
	if (twiglet->tape) stubcount++;
	if (twiglet->date) stubcount++;
	stubcount += twiglet->count;
	if (stubcount > 32767) {
	  put16(0, p_img->fh);
	  put32(stubcount, p_img->fh);
	} else {
	  put16((unsigned short)stubcount, p_img->fh);
	}
	ltag = cslen(twiglet->to->ident);
	if (ltag <= 255) {
	  putc((unsigned char)ltag, p_img->fh);
	} else {
	  putc(0, p_img->fh);
	  put32(ltag, p_img->fh);
	}
	fputcs(twiglet->to->ident, p_img->fh);
	if (twiglet->source) cave_write_source(p_img, twiglet->source);
	if (twiglet->date) {
	  putc(DATE_3D, p_img->fh);
	  fputcs(twiglet->date, p_img->fh);
	}
	if (twiglet->drawings) {
	  putc(DRAWINGS_3D, p_img->fh);
	  fputcs(twiglet->drawings, p_img->fh);
	}
	if (twiglet->instruments) {
	  putc(INSTRUMENTS_3D, p_img->fh);
	  fputcs(twiglet->instruments, p_img->fh);
	}
	if (twiglet->tape) {
	  putc(TAPE_3D, p_img->fh);
	  fputcs(twiglet->tape, p_img->fh);
	}
	save3d(p_img, twiglet->down);
      }
    }
  }
}

static void
cave_write_base_source(img *p_img)
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

  putc(BASE_SOURCE_3D, p_img->fh);
  fputcs(basesource, p_img->fh);

  /* get the actual file name */
  temp = leaf_from_fnm(firstfilename);
  putc(BASE_FILE_3D, p_img->fh);
  fputcs(temp, p_img->fh);
  osfree(temp);
}

int
cave_close(img *p_img)
{
   int result = 1;
   /* let's do the twiglet traverse! */
   twig *twiglet = rhizome->down;
   cave_write_base_source(p_img);
   scount(rhizome);
   save3d(p_img, twiglet);
   if (p_img->fh) {
      /* Write end of data marker */
      putc(END_3D, p_img->fh);
      /* and finally write how many stations there are */
      fseek(p_img->fh, 7L, SEEK_SET);
      put32((INT32_T)statcount, p_img->fh);
      if (ferror(p_img->fh)) result = 0;
      if (fclose(p_img->fh) == EOF) result = 0;
   }
   osfree(p_img);
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
