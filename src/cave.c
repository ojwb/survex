/* > cave.c
 * Routines for reading and writing Survex ".3d" image files
 * Copyright (C) 1993-1997 Olly Betts
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/* Function list
cave_open:        opens image file, reads header into cave struct
cave_open_write:  opens new image file & writes header
cave_read_datum:  reads image data item in binary or ascii
cave_write_datum: writes image data item in binary or ascii
cave_close:       close image file
cave_error:       gives message number of error if cave_open* returned NULL
                 [may be overwritten by calls to msg()]
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
/* path isn't used in cave.c, but EXT is */
# define fopenWithPthAndExt(PTH,FNM,EXT,MODE,X) fopen(FNM,MODE)
# define streq(S1, S2) (!strcmp((S1), (S2)))
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

/* this reads a line from a stream to a buffer. Any eol chars are removed */
/* from the file and the length of the string including '\0' returned */
static int
getline(char *buf, size_t len, FILE *fh)
{
   /* FIXME len ignored here at present */
   int i = 0;
   int ch;

   ch = getc(fh);
   while (ch != '\n' && ch != '\r' && ch != EOF) {
      buf[i++] = ch;
      ch = getc(fh);
   }
   if (ch != EOF) { /* remove any further eol chars (for DOS) */
      do {
	 ch = getc(fh);
      } while (ch == '\n' || ch == '\r');
      ungetc(ch, fh); /* we don't want it, so put it back */
   }
   buf[i] = '\0';
   return (i + 1);
}
#endif
#include "cave.h"

#define lenSzTmp 256
static char szTmp[lenSzTmp];

#ifndef STANDALONE
static enum { CAVE_NONE = 0, CAVE_FILENOTFOUND = 24,
   CAVE_OUTOFMEMORY = 38, CAVE_DIRECTORY = 44, CAVE_CANTOPENOUT = 47,
   CAVE_BADFORMAT = 106 } cave_errno = CAVE_NONE;
#else
static cave_errcode cave_errno = CAVE_NONE;
#endif

#ifdef STANDALONE
cave_errcode
cave_error(void)
{
   return cave_errno;
}
#else
int
cave_error(void)
{
   return (int)cave_errno;
}
#endif

cave *
cave_open(const char *fnm, char *szTitle, char *szDateStamp)
{
   cave *pcave;

   if (fDirectory(fnm)) {
      cave_errno = CAVE_DIRECTORY;
      return NULL;
   }

   pcave = (cave *)xosmalloc(ossizeof(cave));
   if (pcave == NULL) {
      cave_errno = CAVE_OUTOFMEMORY;
      return NULL;
   }

   pcave->fh = fopenWithPthAndExt("", fnm, EXT_SVX_3D, "rb", NULL);
   if (pcave->fh == NULL) {
      osfree(pcave);
      cave_errno = CAVE_FILENOTFOUND;
      return NULL;
   }

   getline(szTmp, ossizeof(szTmp), pcave->fh); /* id string */
   if (!streq(szTmp, "Survex 3D Image File")) {
      fclose(pcave->fh);
      osfree(pcave);
      cave_errno = CAVE_BADFORMAT;
      return NULL;
   }

   getline(szTmp, ossizeof(szTmp), pcave->fh); /* file format version */
   pcave->fBinary = (tolower(*szTmp) == 'b'); /* binary file iff B/b prefix */
   /* knock off the 'B' or 'b' if it's there */
   if (!streq(pcave->fBinary ? szTmp + 1 : szTmp, "v0.01")) {
      fclose(pcave->fh);
      osfree(pcave);
      cave_errno = CAVE_BADFORMAT; /* FIXME ought to distinguish really */
      return NULL;
   }
   /* FIXME sizeof parameter is rather bogus here */
   getline((szTitle ? szTitle : szTmp), ossizeof(szTmp), pcave->fh);
   getline((szDateStamp ? szDateStamp : szTmp), ossizeof(szTmp), pcave->fh);
   pcave->fLinePending = fFalse; /* not in the middle of a 'LINE' command */
   pcave->fRead = fTrue; /* reading from this file */
   cave_errno = CAVE_NONE;
   return pcave;
}

cave *
cave_open_write(const char *fnm, char *szTitle, bool fBinary)
{
   time_t tm;
   cave *pcave;

   if (fDirectory(fnm)) {
      cave_errno = CAVE_DIRECTORY;
      return NULL;
   }

   pcave = (cave *)xosmalloc(ossizeof(cave));
   if (pcave == NULL) {
      cave_errno = CAVE_OUTOFMEMORY;
      return NULL;
   }

   pcave->fh = fopen(fnm, "wb");
   if (!pcave->fh) {
      osfree(pcave);
      cave_errno = CAVE_CANTOPENOUT;
      return NULL;
   }

   /* Output image file header */
   fputs("Survex 3D Image File\n", pcave->fh); /* file identifier string */
   if (fBinary)
      fputs("Bv0.01\n", pcave->fh); /* binary file format version number */
   else
      fputs("v0.01\n", pcave->fh); /* file format version number */
   
   fputsnl(szTitle, pcave->fh);
   tm = time(NULL);
   if (tm == (time_t)-1) {
      fputsnl(TIMENA, pcave->fh);
   } else {
      /* output current date and time in format specified */
      strftime(szTmp, lenSzTmp, TIMEFMT, localtime(&tm));
      fputsnl(szTmp, pcave->fh);
   }
   pcave->fBinary = fBinary;
   pcave->fRead = fFalse; /* writing to this file */
   cave_errno = CAVE_NONE;
   return pcave;
}

int
cave_read_datum(cave *pcave, char *sz, float *px, float *py, float *pz)
{
   static float x = 0.0f, y = 0.0f, z = 0.0f;
   int result;
   if (pcave->fBinary) {
      long opt;
      again: /* label to goto if we get a cross */
      opt = get32(pcave->fh);
      switch (opt) {
       case -1:
	 return cave_STOP; /* end of data marker */
       case 1:
	 (void)get32(pcave->fh);
	 (void)get32(pcave->fh);
	 (void)get32(pcave->fh);
	 goto again;
       case 2:
	 fgets(sz, 256, pcave->fh);
	 sz += strlen(sz) - 1;
	 if (*sz != '\n') return cave_BAD;
	 *sz = '\0';
	 result = cave_LABEL;
	 goto done;
       case 4:
	 result = cave_MOVE;
	 break;
       case 5:
	 result = cave_LINE;
	 break;
       default:
	 return cave_BAD;
      }
      x = (float)get32(pcave->fh) / 100.0f;
      y = (float)get32(pcave->fh) / 100.0f;
      z = (float)get32(pcave->fh) / 100.0f;
      done:
      *px = x;
      *py = y;
      *pz = z;
      return result;
   } else {
      ascii_again:
      if (feof(pcave->fh)) return cave_STOP;
      if (pcave->fLinePending) {
	 pcave->fLinePending = fFalse;
	 result = cave_LINE;
      } else {
	 /* Stop if nothing found */
	 if (fscanf(pcave->fh, "%s", szTmp) < 1) return cave_STOP;
	 if (streq(szTmp, "move"))
	    result = cave_MOVE;
	 else if (streq(szTmp, "draw"))
	    result = cave_LINE;
	 else if (streq(szTmp, "line")) {
	    pcave->fLinePending = fTrue; /* flag to process second triplet as LINE */
	    result = cave_MOVE;
	 } else if (streq(szTmp, "cross")) {
	    if (fscanf(pcave->fh, "%f%f%f", px, py, pz) < 3) return cave_BAD;
	    goto ascii_again;
	 } else if (streq(szTmp, "name")) {
	    if (fscanf(pcave->fh,"%s", sz) < 1) return cave_BAD;
	    result = cave_LABEL;
	 } else if (streq(szTmp, "scale")) {
	    if (fscanf(pcave->fh,"%f", px) < 1) return cave_BAD;
	    *py = *pz = *px;
	    return cave_SCALE;
	 } else
	    return cave_BAD; /* unknown keyword */
      }

      if (fscanf(pcave->fh, "%f%f%f", px, py, pz) < 3) return cave_BAD;
      return result;
   }
}

void
cave_write_datum(cave *pcave, int code, const char *sz,
		float x, float y, float z)
{
   if (pcave->fBinary) {
      float Sc = (float)100.0; /* Output in cm */
      long opt = 0;
      switch (code) {
       case cave_LABEL: /* put a move before each label */
       case cave_MOVE:
	 opt = 4;
	 break;
       case cave_LINE:
	 opt = 5;
	 break;
      case cave_CROSS:
	 opt = 1;
	 break;
       case cave_SCALE: /* fprintf( pcave->fh, "scale %9.2f\n", x ); */
       default: /* ignore for now */
	 break;
      }
      if (opt) {
	 put32(opt, pcave->fh);
	 put32((long)(x * Sc), pcave->fh);
	 put32((long)(y * Sc), pcave->fh);
	 put32((long)(z * Sc), pcave->fh);
      }
      if (code == cave_LABEL) {
	 put32(2, pcave->fh);
	 fputsnl(sz, pcave->fh);
      }
   } else {
      switch (code) {
       case cave_MOVE:
	 fprintf(pcave->fh, "move %9.2f %9.2f %9.2f\n", x, y, z);
	 break;
       case cave_LINE:
	 fprintf(pcave->fh, "draw %9.2f %9.2f %9.2f\n", x, y, z);
	 break;
       case cave_CROSS:
	 fprintf(pcave->fh, "cross %9.2f %9.2f %9.2f\n", x, y, z);
	 break;
       case cave_LABEL:
	 fprintf(pcave->fh, "name %s %9.2f %9.2f %9.2f\n", sz, x, y, z);
	 break;
       case cave_SCALE:
	 fprintf(pcave->fh, "scale %9.2f\n", x);
	 break;
       default:
	 break;
      }
   }
}

void
cave_close(cave *pcave)
{
   if (pcave->fh) {
      /* If writing a binary file, write end of data marker */   
      if (pcave->fBinary && !pcave->fRead) put32(-1L, pcave->fh);
      fclose(pcave->fh);
   }
   osfree(pcave);
}
