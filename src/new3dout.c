/* > new3dout.c
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

/* majority of new file interface... */
#include <config.h>
#include "math.h"

#include "new3dout.h"
#ifdef NEW3DFORMAT
twig *rhizome, *limb;
char *firstfilename, *startingdir;
int fUseNewFormat = 0;

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
  if (twiglet->from) return 1;

  if (twiglet->to) {
    twig *lib = twiglet->down;
    twiglet->count = 0;
    while (lib->right) {
      lib = lib->right;
      twiglet->count += scount(lib);
    }
    return (twiglet->count ? 1 : 0);
  }

  /* errr no - this should happen for fixed points... */
  printf("This should not happen. Ever\n");
  return 1;
  /* we should never reach above function - it's just here to  *
   * cheer up the compiler*/
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

static void
cave_write_pos(pos *pid, prefix *pre)
{
  unsigned char length;
  unsigned char ltag;
  const char *tag;
  if (pid->id == 0) {
    pid->id = (INT32_T)statcount;
    tag = pre->ident;
    ltag = strlen(tag);
    length = ltag + 12 + 4 + 1;
    /* FIXME: assumes that length of name is a single byte */
    /* ltag for station name, 12 for data, 4 for id, 1 for length of name */
    putc(STATION_3D, pimgOut->fh);
    putc(length, pimgOut->fh);
    put32((INT32_T)statcount, pimgOut->fh); /* station ID */
    put32((INT32_T)(pid->p[0] * 100.0), pimgOut->fh); /* X in cm */
    put32((INT32_T)(pid->p[1] * 100.0), pimgOut->fh); /* Y */
    put32((INT32_T)(pid->p[2] * 100.0), pimgOut->fh); /* Z */
    fputcs(tag, pimgOut->fh);
    statcount++;
  } else {
    /* we've already put this in the file, so just a link is needed */
    putc(STATLINK_3D, pimgOut->fh);
    putc(0x04, pimgOut->fh);
    put32((INT32_T)pid->id, pimgOut->fh);
  }
}

img *
cave_open_write(const char *fnm, const char *title)
{
   img *pimg;
   if (fDirectory(fnm)) {
     /*      img_errno = IMG_DIRECTORY;*/
      return NULL;
   }

   pimg = (img *)xosmalloc(ossizeof(img));
   if (pimg == NULL) {
     /* img_errno = IMG_OUTOFMEMORY; */
      return NULL;
   }

   pimg->fh = fopen(fnm, "wb");
   if (!pimg->fh) {
      osfree(pimg);
      /* img_errno = IMG_CANTOPENOUT; */
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
   /* FIXME: */
   printf("A cave error has occured. Hmmm\n");
   return 0;
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
  /* FIXME: temp bodge */
  if (fAbsoluteFnm(source)) {
    /* strip source down to just a file name... */
    if (strncmp(source, basesource, strlen(basesource)) != 0) {
      printf("Warning: source links may not work\n");
    } else {
      source = source + strlen(basesource);
    }
  }
  putc(SOURCE_3D, pimgOut->fh);
  fputcs(source, pimgOut->fh);
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
	err = 0.0;
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
	  err = 0; /*what is the error for a leg with zero length?
	  	      *Phil -  he say none. */
	} else {
	  err = 10000.0 * offset / length;
	}
	putc(LEG_3D, pimgOut->fh);
	put16((INT16_T)0x02, pimgOut->fh);
	putc(0x04, pimgOut->fh);
	put32((INT32_T)err, pimgOut->fh); /* output error in %*100 */
	cave_write_pos(twiglet->from->pos, twiglet->from);
	cave_write_pos(twiglet->to->pos, twiglet->to);
      }
    } else {
      if (twiglet->count) {
        putc(BRANCH_3D, pimgOut->fh);
	/* number of records  - legs + values */
	stubcount = 0;
	if (twiglet->source) stubcount++;
	if (twiglet->drawings) stubcount++;
	if (twiglet->instruments) stubcount++;
	if (twiglet->tape) stubcount++;
	if (twiglet->date) stubcount++;
	stubcount += twiglet->count;
	if (stubcount > 32767) {
	  put16(0, pimgOut->fh);
	  put32(stubcount, pimgOut->fh);
	} else {
	  put16((unsigned short)stubcount, pimgOut->fh);
	}
	ltag = cslen(twiglet->to->ident);
	if (ltag < 255) {
	  putc((unsigned char)(ltag + 1), pimgOut->fh);
	} else {
	  putc(0, pimgOut->fh);
	  put32(ltag + 1, pimgOut->fh);
	}
	fputcs(twiglet->to->ident, pimgOut->fh);
	if (twiglet->source) cave_write_source(twiglet->source);
	if (twiglet->date) {
	  putc(DATE_3D, pimgOut->fh);
	  fputcs(twiglet->date, pimgOut->fh);
	}
	if (twiglet->drawings) {
	  putc(DRAWINGS_3D, pimgOut->fh);
	  fputcs(twiglet->drawings, pimgOut->fh);
	}
	if (twiglet->instruments) {
	  putc(INSTRUMENTS_3D, pimgOut->fh);
	  fputcs(twiglet->instruments, pimgOut->fh);
	}
	if (twiglet->tape) {
	  putc(TAPE_3D, pimgOut->fh);
	  fputcs(twiglet->tape, pimgOut->fh);
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
  /* remove any ./../. etcs but not today*/
  temp = osstrdup(basesource);
  realpath(temp, basesource);
#endif

  putc(BASE_SOURCE_3D, pimgOut->fh);
  fputcs(basesource, pimgOut->fh);

  /* get the actual file name */
  temp = leaf_from_fnm(firstfilename);
  putc(BASE_FILE_3D, pimgOut->fh);
  fputcs(temp, pimgOut->fh);
  osfree(temp);
}

void
cave_close(img *pimg)
{
   /* let's do the twiglet traverse! */
   twig *twiglet = rhizome->down;
   cave_write_base_source();
   scount(rhizome);
   save3d(twiglet);
   if (pimg->fh) {
      /* If writing a binary file, write end of data marker */
      putc(END_3D,pimg->fh);
      /* and finally write how many stations there are */
      fseek(pimg->fh,7L,SEEK_SET);
      put32((INT32_T)statcount,pimg->fh);
      fclose(pimg->fh);
   }
   /* FIXME: don't understand this comment:
    * memory leak, but it doesn't like this for some reason */
   osfree(pimg);
}

void
create_twig(prefix *pre, const char *fname)
{
  twig *twiglet;
  twig *lib;
  lib = get_twig(pre->up); /* get the active twig for parent's prefix*/
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
