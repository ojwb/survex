/* > img.c
 * Routines for reading and writing Survex ".3d" image files
 * Copyright (C) 1993-2001 Olly Betts
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
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>

#ifndef STANDALONE
# include "useful.h"
# include "message.h"
# include "filename.h"
# include "filelist.h"
# define TIMENA msg(/*Date and time not available.*/108)
# define TIMEFMT msg(/*%a,%Y.%m.%d %H:%M:%S %Z*/107)
#else
# define TIMENA "Time not available."
# define TIMEFMT "%a,%Y.%m.%d %H:%M:%S %Z"
# define EXT_SVX_3D "3d"
# define xosmalloc(L) malloc((L))
# define osfree(P) free((P))
# define ossizeof(T) sizeof(T)
/* in non-standalone mode, this tests if a filename refers to a directory */
# define fDirectory(X) 0
/* open file FNM with mode MODE, maybe using path PTH and/or extension EXT */
/* path isn't used in img.c, but EXT is */
# define fopenWithPthAndExt(PTH,FNM,EXT,MODE,X) fopen(FNM,MODE)
# define fFalse 0
# define fTrue 1
# define bool int
/* dummy do {...} while(0) hack to permit stuff like
 * if (s) fputsnl(s,fh); else exit(1);
 * to work as intended
 */
# define fputsnl(SZ, FH) do {fputs((SZ), (FH)); putc('\n', (FH));} while(0)

static long
get32(FILE *fh)
{
   long w;
   w = getc(fh);
   w |= (long)getc(fh) << 8l;
   w |= (long)getc(fh) << 16l;
   w |= (long)getc(fh) << 24l;
   return w;
}

static void
put32(long w, FILE *fh)
{
   putc((char)(w), fh);
   putc((char)(w >> 8l), fh);
   putc((char)(w >> 16l), fh);
   putc((char)(w >> 24l), fh);
}

#endif
#include "img.h"

unsigned int img_output_version = 2;

#define lenSzTmp 256
static char szTmp[lenSzTmp];

#ifndef STANDALONE
static enum {
   IMG_NONE = 0,
   IMG_FILENOTFOUND = /*Couldn't open data file `%s'*/24,
   IMG_OUTOFMEMORY  = /*Out of memory %.0s*/38,
   IMG_DIRECTORY    = /*Filename `%s' refers to directory*/44,
   IMG_CANTOPENOUT  = /*Failed to open output file `%s'*/47,
   IMG_BADFORMAT    = /*Bad 3d image file `%s'*/106
} img_errno = IMG_NONE;
#else
static img_errcode img_errno = IMG_NONE;
#endif

/* Read a line from a stream to a buffer. Any eol chars are removed
 * from the file and the length of the string excluding '\0' is returned */
static int
getline(char *buf, size_t len, FILE *fh)
{
   size_t i = 0;
   int ch;

   ch = getc(fh);
   while (ch != '\n' && ch != '\r' && ch != EOF && i < len - 1) {
      buf[i++] = ch;
      ch = getc(fh);
   }
   if (ch == '\n' || ch == '\r') {
      /* remove any further eol chars (for DOS text files) */
      do {
	 ch = getc(fh);
      } while (ch == '\n' || ch == '\r');
      ungetc(ch, fh); /* we don't want it, so put it back */
   }
   buf[i] = '\0';
   return i;
}

#ifdef STANDALONE
img_errcode
img_error(void)
{
   return img_errno;
}
#else
int
img_error(void)
{
   return (int)img_errno;
}
#endif

img *
img_open(const char *fnm, char *szTitle, char *szDateStamp)
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

   pimg->fh = fopenWithPthAndExt("", fnm, EXT_SVX_3D, "rb", NULL);
   if (pimg->fh == NULL) {
      osfree(pimg);
      img_errno = IMG_FILENOTFOUND;
      return NULL;
   }

   getline(szTmp, ossizeof(szTmp), pimg->fh); /* id string */
   if (strcmp(szTmp, "Survex 3D Image File") != 0) {
      fclose(pimg->fh);
      osfree(pimg);
      img_errno = IMG_BADFORMAT;
      return NULL;
   }

   getline(szTmp, ossizeof(szTmp), pimg->fh); /* file format version */
   pimg->version = (tolower(*szTmp) == 'b'); /* binary file iff B/b prefix */
   /* knock off the 'B' or 'b' if it's there */
   if (strcmp(szTmp + pimg->version, "v0.01") == 0) {
      pimg->flags = 0;
   } else if (pimg->version == 0 && strcmp(szTmp, "v2") == 0) {
      pimg->version = 2;
   } else {
      fclose(pimg->fh);
      osfree(pimg);
      img_errno = IMG_BADFORMAT;
      return NULL;
   }
   /* FIXME sizeof parameter is rather bogus here */
   getline((szTitle ? szTitle : szTmp), ossizeof(szTmp), pimg->fh);
   getline((szDateStamp ? szDateStamp : szTmp), ossizeof(szTmp), pimg->fh);
   pimg->fLinePending = fFalse; /* not in the middle of a 'LINE' command */
   pimg->fRead = fTrue; /* reading from this file */
   img_errno = IMG_NONE;
   pimg->start = ftell(pimg->fh);
   return pimg;
}

void
img_rewind(img *pimg)
{
   fseek(pimg->fh, pimg->start, SEEK_SET);
}

img *
img_open_write(const char *fnm, char *szTitle, bool fBinary)
{
   time_t tm;
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

   /* Output image file header */
   fputs("Survex 3D Image File\n", pimg->fh); /* file identifier string */
   if (img_output_version < 2) {
      pimg->version = fBinary ? 1 : 0;
      if (fBinary)
         fputs("Bv0.01\n", pimg->fh); /* binary file format version number */
      else
         fputs("v0.01\n", pimg->fh); /* file format version number */
   } else {
      pimg->version = 2;
      fprintf(pimg->fh, "v%d\n", pimg->version); /* file format version no. */
   }
   fputsnl(szTitle, pimg->fh);
   tm = time(NULL);
   if (tm == (time_t)-1) {
      fputsnl(TIMENA, pimg->fh);
   } else {
      /* output current date and time in format specified */
      strftime(szTmp, lenSzTmp, TIMEFMT, localtime(&tm));
      fputsnl(szTmp, pimg->fh);
   }
   pimg->fRead = fFalse; /* writing to this file */
   img_errno = IMG_NONE;
   return pimg;
}

int
img_read_item(img *pimg, char *sz, img_point *p)
{
   static double x = 0.0, y = 0.0, z = 0.0;
   static long opt_lookahead = 0;
   int result;
   if (pimg->version > 0) {
      long opt;
      again: /* label to goto if we get a cross */
      if (pimg->version == 1) {
	 if (opt_lookahead) {
	    opt = opt_lookahead;
	    opt_lookahead = 0;
	 } else {
	    opt = get32(pimg->fh);
	 }
      } else {
         opt = getc(pimg->fh);
      }
      switch (opt) {
       case -1: case 0:
	 return img_STOP; /* end of data marker */
       case 1:
	 (void)get32(pimg->fh);
	 (void)get32(pimg->fh);
	 (void)get32(pimg->fh);
	 goto again;
       case 2: case 3:
	 result = img_LABEL;
	 fgets(sz, 256, pimg->fh);
	 sz += strlen(sz) - 1;
	 if (*sz != '\n') return img_BAD;
	 *sz = '\0';
	 if (opt == 2) goto done;
	 break;
       case 4:
	 result = img_MOVE;
         pimg->flags = 0;
	 break;
       case 5:
	 result = img_LINE;
	 break;
       default:
	 if (!(opt & 0x80)) return img_BAD;
	 pimg->flags = (int)opt & 0x7f;
	 result = img_LINE;
	 break;
      }
      x = get32(pimg->fh) / 100.0;
      y = get32(pimg->fh) / 100.0;
      z = get32(pimg->fh) / 100.0;
      done:
      p->x = x;
      p->y = y;
      p->z = z;

      if (result == img_MOVE && pimg->version == 1) {
	 /* peek at next code and see if it's img_LABEL */
         opt_lookahead = get32(pimg->fh);
	 if (opt_lookahead == img_LABEL) return img_read_item(pimg, sz, p);
      }
	 
      return result;
   } else {
      ascii_again:
      if (feof(pimg->fh)) return img_STOP;
      if (pimg->fLinePending) {
	 pimg->fLinePending = fFalse;
	 result = img_LINE;
      } else {
	 /* Stop if nothing found */
	 if (fscanf(pimg->fh, "%s", szTmp) < 1) return img_STOP;
	 if (strcmp(szTmp, "move") == 0)
	    result = img_MOVE;
	 else if (strcmp(szTmp, "draw") == 0)
	    result = img_LINE;
	 else if (strcmp(szTmp, "line") == 0) {
	    /* set flag to indicate to process second triplet as LINE */
	    pimg->fLinePending = fTrue;
	    result = img_MOVE;
	 } else if (strcmp(szTmp, "cross") == 0) {
	    if (fscanf(pimg->fh, "%lf%lf%lf", &p->x, &p->y, &p->z) < 3)
	       return img_BAD;
	    goto ascii_again;
	 } else if (strcmp(szTmp, "name") == 0) {
	    if (fscanf(pimg->fh,"%s", sz) < 1) return img_BAD;
	    result = img_LABEL;
	 } else
	    return img_BAD; /* unknown keyword */
      }

      if (fscanf(pimg->fh, "%lf%lf%lf", &p->x, &p->y, &p->z) < 3) return img_BAD;
      return result;
   }
}

void
img_write_item(img *pimg, int code, const char *s,
	       double x, double y, double z)
{
   if (!pimg) return;
   if (pimg->version > 0) {
      double Sc = 100.0; /* Output in cm */
      INT32_T opt = 0;
      switch (code) {
       case img_LABEL:
	 if (pimg->version == 1) {
	    /* put a move before each label */
	    img_write_item(pimg, img_MOVE, NULL, x, y, z);
	    put32(2, pimg->fh);
	    fputsnl(s, pimg->fh);
	    return;
	 }
	 putc(3, pimg->fh);
	 fputsnl(s, pimg->fh);
	 opt = 0;
	 break;	  
       case img_MOVE:
	 opt = 4;
	 break;
       case img_LINE:
         if (pimg->version > 1) {
	    opt = 128;
	    if (s) opt |= *s;
	    break;
         }
	 opt = 5;
	 break;
       case img_CROSS: /* ignore */
	 return;
       default: /* ignore for now */
	 return;
      }
      if (pimg->version == 1) {
	 put32(opt, pimg->fh);
      } else {
	 if (opt) putc(opt, pimg->fh);
      }
      put32((INT32_T)(x * Sc), pimg->fh);
      put32((INT32_T)(y * Sc), pimg->fh);
      put32((INT32_T)(z * Sc), pimg->fh);
   } else {
      switch (code) {
       case img_MOVE:
	 fprintf(pimg->fh, "move %9.2f %9.2f %9.2f\n", x, y, z);
	 break;
       case img_LINE:
	 fprintf(pimg->fh, "draw %9.2f %9.2f %9.2f\n", x, y, z);
	 break;
       case img_CROSS: /* ignore */
	 break;
       case img_LABEL:
	 fprintf(pimg->fh, "name %s %9.2f %9.2f %9.2f\n", s, x, y, z);
	 break;
       default:
	 break;
      }
   }
}

void
img_close(img *pimg)
{
   if (pimg) {
      if (pimg->fh) {
	 /* If writing a binary file, write end of data marker */
	 if (pimg->version > 0 && !pimg->fRead) {
	    if (pimg->version == 1) {
	       put32((INT32_T)-1, pimg->fh);
	    } else {
	       putc(0, pimg->fh);
	    }
	 }
	 fclose(pimg->fh);
      }
      osfree(pimg);
   }
}
