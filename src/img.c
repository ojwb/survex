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

#include "img.h"
#ifdef IMG_HOSTED
# include "debug.h"
# include "filelist.h"
# include "filename.h"
# include "message.h"
# include "useful.h"
# define TIMENA msg(/*Date and time not available.*/108)
# define TIMEFMT msg(/*%a,%Y.%m.%d %H:%M:%S %Z*/107)
#else
# define INT32_T long
# define TIMENA "Time not available."
# define TIMEFMT "%a,%Y.%m.%d %H:%M:%S %Z"
# define EXT_SVX_3D "3d"
# define xosmalloc(L) malloc((L))
# define xosrealloc(L,S) realloc((L),(S))
# define osfree(P) free((P))
# define ossizeof(T) sizeof(T)
/* in IMG_HOSTED mode, this tests if a filename refers to a directory */
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
# define fputsnl(S, FH) do {fputs((S), (FH)); putc('\n', (FH));} while(0)
# define ASSERT(X)

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

unsigned int img_output_version = 3;

#define TMPBUFLEN 256
static char tmpbuf[TMPBUFLEN];

#ifdef IMG_HOSTED
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

#ifndef IMG_HOSTED
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
img_open_survey(const char *fnm, char *title_buf, char *date_buf,
		const char *survey)
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

   pimg->buf_len = 257;
   pimg->label_buf = xosmalloc(pimg->buf_len);
   if (!pimg->label_buf) {
      osfree(pimg);
      img_errno = IMG_OUTOFMEMORY;
      return NULL;
   }

   pimg->fh = fopenWithPthAndExt("", fnm, EXT_SVX_3D, "rb", NULL);
   if (pimg->fh == NULL) {
      osfree(pimg);
      img_errno = IMG_FILENOTFOUND;
      return NULL;
   }

   getline(tmpbuf, TMPBUFLEN, pimg->fh); /* id string */
   if (strcmp(tmpbuf, "Survex 3D Image File") != 0) {
      fclose(pimg->fh);
      osfree(pimg);
      img_errno = IMG_BADFORMAT;
      return NULL;
   }

   getline(tmpbuf, TMPBUFLEN, pimg->fh); /* file format version */
   pimg->version = (tolower(*tmpbuf) == 'b'); /* binary file iff B/b prefix */
   /* knock off the 'B' or 'b' if it's there */
   if (strcmp(tmpbuf + pimg->version, "v0.01") == 0) {
      pimg->flags = 0;
   } else if (pimg->version == 0 && tmpbuf[0] == 'v' &&
	      (tmpbuf[1] >= '2' || tmpbuf[1] <= '3') &&
	      tmpbuf[2] == '\0') {
      pimg->version = tmpbuf[1] - '0';
   } else {
      fclose(pimg->fh);
      osfree(pimg);
      img_errno = IMG_BADFORMAT;
      return NULL;
   }
   getline((title_buf ? title_buf : tmpbuf), TMPBUFLEN, pimg->fh);
   getline((date_buf ? date_buf : tmpbuf), TMPBUFLEN, pimg->fh);
   pimg->fRead = fTrue; /* reading from this file */
   img_errno = IMG_NONE;
   pimg->start = ftell(pimg->fh);

   /* for version 3 we use label to store the prefix for reuse */
   pimg->label_len = 0;

   pimg->survey = NULL;
   pimg->survey_len = 0;

   if (survey) {
      size_t len = strlen(survey);
      if (len) {
	 if (survey[len - 1] == '.') len--;
	 if (len) {
	    pimg->survey = osmalloc(len + 2);
	    memcpy(pimg->survey, survey, len);
	    pimg->survey[len] = '.';
	    pimg->survey[len + 1] = '\0';
	 }
      }
      pimg->survey_len = len;
   }

   /* [version 0] not in the middle of a 'LINE' command
    * [version 3] not in the middle of turning a LINE into a MOVE */
   pimg->pending = 0;

   return pimg;
}

void
img_rewind(img *pimg)
{
   fseek(pimg->fh, pimg->start, SEEK_SET);

   /* [version 0] not in the middle of a 'LINE' command
    * [version 3] not in the middle of turning a LINE into a MOVE */
   pimg->pending = 0;

   img_errno = IMG_NONE;
   
   /* for version 3 we use label to store the prefix for reuse */
   pimg->label_len = 0;
}

img *
img_open_write(const char *fnm, char *title_buf, bool fBinary)
{
   time_t tm;
   img *pimg;

   fBinary = fBinary;

   if (fDirectory(fnm)) {
      img_errno = IMG_DIRECTORY;
      return NULL;
   }

   pimg = (img *)xosmalloc(ossizeof(img));
   if (pimg == NULL) {
      img_errno = IMG_OUTOFMEMORY;
      return NULL;
   }

   pimg->buf_len = 257;
   pimg->label_buf = xosmalloc(pimg->buf_len);
   if (!pimg->label_buf) {
      osfree(pimg);
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
      pimg->version = 1;
      fputs("Bv0.01\n", pimg->fh); /* binary file format version number */
   } else {
      pimg->version = (img_output_version > 2) ? 3 : 2;
      fprintf(pimg->fh, "v%d\n", pimg->version); /* file format version no. */
   }
   fputsnl(title_buf, pimg->fh);
   tm = time(NULL);
   if (tm == (time_t)-1) {
      fputsnl(TIMENA, pimg->fh);
   } else {
      /* output current date and time in format specified */
      strftime(tmpbuf, TMPBUFLEN, TIMEFMT, localtime(&tm));
      fputsnl(tmpbuf, pimg->fh);
   }
   pimg->fRead = fFalse; /* writing to this file */
   img_errno = IMG_NONE;

   /* for version 3 we use label to store the prefix for reuse */
   pimg->label_buf[0] = '\0';
   pimg->label_len = 0;

   return pimg;
}

int
img_read_item(img *pimg, img_point *p)
{
   int result;
   pimg->flags = 0;
   pimg->label = pimg->label_buf;

   if (pimg->version == 3) {
      int opt;
      if (pimg->pending >= 0x80) {
	 *p = pimg->mv;
	 pimg->flags = (int)(pimg->pending) & 0x3f;
	 pimg->pending = 0;
	 return img_LINE; // FIXME
      }
      again3: /* label to goto if we get a prefix */
      opt = getc(pimg->fh);
      switch (opt >> 6) {
       case 0:
	 if (opt == 0) {
	    if (!pimg->label_len) return img_STOP; /* end of data marker */
	    pimg->label_len = 0;
	    goto again3;
	 }
	 if (opt < 15) {
	    /* 1-14 mean trim that many levels from current prefix */
	    int c;
	    /* zero prefix using "0" */
	    if (pimg->label_len <= 16) {
	       img_errno = IMG_BADFORMAT;
	       return img_BAD;
	    }
	    c = pimg->label_len - 16;
	    opt &= 0x07;
	    while (pimg->label_buf[c] != '.' || --opt > 0) {
	       if (--c < 0) {
		  /* zero prefix using "0" */
		  img_errno = IMG_BADFORMAT;
		  return img_BAD;
	       }
	    }
	    c++;
	    pimg->label_buf[c] = '\0';
	    pimg->label_len = c;
	    goto again3;
	 }
	 if (opt == 15) {
	    result = img_MOVE;
	    break;
	 }
	 /* 16-31 mean remove (n - 15) characters from the prefix */
	 pimg->label_len -= (opt - 15);
	 /* zero prefix using 0 */
	 if (pimg->label_len <= 0) {
	    img_errno = IMG_BADFORMAT;
	    return img_BAD;
	 }
	 goto again3;
       case 1: case 2: {
	 char *q;
	 size_t len;

	 len = getc(pimg->fh);
	 if (len == 0xfe) {
	    len += get16(pimg->fh);
	 } else if (len == 0xff) {
	    len = get32(pimg->fh);
	    if (len < 0xfe + 0xffff) {
	       img_errno = IMG_BADFORMAT;
	       return img_BAD;
	    }
	 }

	 q = pimg->label_buf + pimg->label_len;
	 pimg->label_len += len;
	 if (pimg->label_len >= pimg->buf_len) {
	    pimg->label_buf = xosrealloc(pimg->label_buf, pimg->label_len + 1);
	    if (!pimg->label_buf) {
	       img_errno = IMG_OUTOFMEMORY;
	       return img_BAD;
	    }
	    pimg->buf_len = pimg->label_len + 1;
	 }
	 if (len && fread(q, len, 1, pimg->fh) != 1) {
	    img_errno = IMG_BADFORMAT;
	    return img_BAD;
	 }
	 q[len] = '\0';

	 result = opt & 0x40 ? img_LABEL : img_LINE;

	 if (pimg->survey_len) {
	    size_t l = pimg->survey_len;
	    const char *s = pimg->label_buf;
	    if (result == img_LINE) {
	       if (strncmp(pimg->survey, s, l) != 0 ||
		   !(s[l] == '.' || s[l] == '\0')) {
		  pimg->mv.x = get32(pimg->fh) / 100.0;
		  pimg->mv.y = get32(pimg->fh) / 100.0;
		  pimg->mv.z = get32(pimg->fh) / 100.0;
		  pimg->pending = 15;
	          goto again3;
	       }
	    } else {
	       if (strncmp(pimg->survey, s, l + 1) != 0) {
		  fseek(pimg->fh, 12, SEEK_CUR);
		  goto again3;
	       }
	    }
	    pimg->label += l;
	    /* skip the dot if there */
	    if (*pimg->label) pimg->label++;
	 }

	 if (result == img_LINE && pimg->pending) {
	    *p = pimg->mv;
	    pimg->mv.x = get32(pimg->fh) / 100.0;
	    pimg->mv.y = get32(pimg->fh) / 100.0;
	    pimg->mv.z = get32(pimg->fh) / 100.0;
	    pimg->pending = opt;
	    return img_MOVE;
	 }
	 pimg->flags = (int)opt & 0x3f;
	 break;
       }
       default:
	 img_errno = IMG_BADFORMAT;
	 return img_BAD;
      }
      p->x = get32(pimg->fh) / 100.0;
      p->y = get32(pimg->fh) / 100.0;
      p->z = get32(pimg->fh) / 100.0;
      pimg->pending = 0;
      return result;
   } else if (pimg->version > 0) {
      static long opt_lookahead = 0;
      static double x = 0.0, y = 0.0, z = 0.0;
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
       case 2: case 3: {
	 char *q;
	 int ch;
	 result = img_LABEL;
	 ch = getc(pimg->fh);
	 if (ch == EOF) {
	    img_errno = IMG_BADFORMAT;
	    return img_BAD;
	 }
	 if (ch != '\\') ungetc(ch, pimg->fh);
	 fgets(pimg->label_buf, 257, pimg->fh);
	 q = pimg->label_buf + strlen(pimg->label_buf) - 1;
	 if (*q != '\n') {
	    img_errno = IMG_BADFORMAT;
	    return img_BAD;
	 }
	 /* Ignore empty labels in some .3d files (caused by a bug) */
	 if (q == pimg->label_buf) goto again;
	 *q = '\0';
	 pimg->flags = img_SFLAG_UNDERGROUND; /* no flags given... */
	 if (opt == 2) goto done;
	 break;
       }
       case 6: case 7: {
	 size_t len;
	 result = img_LABEL;
	 if (opt == 7)
	    pimg->flags = getc(pimg->fh);
	 else
	    pimg->flags = img_SFLAG_UNDERGROUND; /* no flags given... */
	 len = get32(pimg->fh);
	 /* Ignore empty labels in some .3d files (caused by a bug) */
	 if (len == 0) goto again;
	 if (len >= pimg->buf_len) {
	    pimg->label_buf = xosrealloc(pimg->label_buf, len + 1);
	    if (!pimg->label_buf) {
	       img_errno = IMG_OUTOFMEMORY;
	       return img_BAD;
	    }
	    pimg->buf_len = len + 1;
	 }
	 if (fread(pimg->label_buf, len, 1, pimg->fh) != 1) {
	    img_errno = IMG_BADFORMAT;
	    return img_BAD;
	 }
	 break;
       }
       case 4:
	 result = img_MOVE;
	 break;
       case 5:
	 result = img_LINE;
	 break;
       default:
	 switch ((int)opt & 0xc0) {
	  case 0x80:
	    pimg->flags = (int)opt & 0x3f;
	    result = img_LINE;
	    break;
	  case 0x40: {
	    char *q;
	    pimg->flags = (int)opt & 0x3f;
	    result = img_LABEL;
	    if (!fgets(pimg->label_buf, 257, pimg->fh)) {
	       img_errno = IMG_BADFORMAT;
	       return img_BAD;
	    }
	    q = pimg->label_buf + strlen(pimg->label_buf) - 1;
	    /* Ignore empty-labels in some .3d files (caused by a bug) */
	    if (q == pimg->label_buf) goto again;
	    if (*q != '\n') {
	       img_errno = IMG_BADFORMAT;
	       return img_BAD;
	    }
	    *q = '\0';
	    break;
	  }
	  case 0xc0:
	    /* use this for an extra leg or station flag if we need it */
	  default:
	    img_errno = IMG_BADFORMAT;
	    return img_BAD;
	 }
	 break;
      }
      x = get32(pimg->fh) / 100.0;
      y = get32(pimg->fh) / 100.0;
      z = get32(pimg->fh) / 100.0;

      if (result == img_LABEL) {
	 if (pimg->survey_len &&
	  (strncmp(pimg->label_buf, pimg->survey, pimg->survey_len + 1) != 0))
	    goto again;
	 pimg->label += pimg->survey_len + 1;
      }

      done:
      p->x = x;
      p->y = y;
      p->z = z;

      if (result == img_MOVE && pimg->version == 1) {
	 /* peek at next code and see if it's an old-style label */
	 opt_lookahead = get32(pimg->fh);
	 if (opt_lookahead == 2) return img_read_item(pimg, p);
      }

      return result;
   } else {
      ascii_again:
      if (feof(pimg->fh)) return img_STOP;
      if (pimg->pending) {
	 pimg->pending = 0;
	 result = img_LINE;
      } else {
	 /* Stop if nothing found */
	 if (fscanf(pimg->fh, "%s", tmpbuf) < 1) return img_STOP;
	 if (strcmp(tmpbuf, "move") == 0)
	    result = img_MOVE;
	 else if (strcmp(tmpbuf, "draw") == 0)
	    result = img_LINE;
	 else if (strcmp(tmpbuf, "line") == 0) {
	    /* set flag to indicate to process second triplet as LINE */
	    pimg->pending = 1;
	    result = img_MOVE;
	 } else if (strcmp(tmpbuf, "cross") == 0) {
	    if (fscanf(pimg->fh, "%lf%lf%lf", &p->x, &p->y, &p->z) < 3) {
	       img_errno = IMG_BADFORMAT;	       
	       return img_BAD;
	    }
	    goto ascii_again;
	 } else if (strcmp(tmpbuf, "name") == 0) {
	    int ch;
	    ch = getc(pimg->fh);
	    if (ch == EOF) {
	       img_errno = IMG_BADFORMAT;
	       return img_BAD;
	    }
	    if (ch != '\\') ungetc(ch, pimg->fh);
	    if (fscanf(pimg->fh, "%256s", pimg->label_buf) < 1 ||
		strlen(pimg->label_buf) == 256) {
	       img_errno = IMG_BADFORMAT;
	       return img_BAD;
	    }
	    result = img_LABEL;
	 } else {
	    img_errno = IMG_BADFORMAT;
	    return img_BAD; /* unknown keyword */
	 }
      }

      if (fscanf(pimg->fh, "%lf%lf%lf", &p->x, &p->y, &p->z) < 3) {
	 img_errno = IMG_BADFORMAT;
	 return img_BAD;
      }

      if (result == img_LABEL) {
	 if (pimg->survey_len &&
	  (strncmp(pimg->label_buf, pimg->survey, pimg->survey_len + 1) != 0))
	    goto ascii_again;
	 pimg->label += pimg->survey_len + 1;
      }

      return result;
   }
}

static void
write_v3label(img *pimg, int opt, const char *s)
{
   size_t len, n, dot;

   /* find length of common prefix */
   dot = 0;
   for (len = 0; s[len] == pimg->label_buf[len] && s[len] != '\0'; len++) {
      if (s[len] == '.') dot = len + 1;
   }

   ASSERT(len <= pimg->label_len);
   n = pimg->label_len - len;
   if (len == 0) {
      if (pimg->label_len) putc(0, pimg->fh);
   } else if (n <= 16) {
      if (n) putc(n + 15, pimg->fh);
   } else if (dot == 0) {
      if (pimg->label_len) putc(0, pimg->fh);
      len = 0;      
   } else {
      const char *p = pimg->label_buf + dot;
      n = 1;
      for (len = pimg->label_len - dot - 17; len; len--) {
	 if (*p++ == '.') n++;
      }
      if (n <= 14) {
	 putc(n, pimg->fh);
	 len = dot;
      } else {
	 if (pimg->label_len) putc(0, pimg->fh);
	 len = 0;
      }
   }

   n = strlen(s + len);
   putc(opt, pimg->fh);
   if (n < 0xfe) {
      putc(n, pimg->fh);
   } else if (n < 0xffff + 0xfe) {
      putc(0xfe, pimg->fh);
      put16(n - 0xfe, pimg->fh);
   } else {
      putc(0xff, pimg->fh);
      put32(n, pimg->fh);
   }
   fwrite(s + len, n, 1, pimg->fh);

   n += len;
   pimg->label_len = n;
   if (n >= pimg->buf_len) {
      char *p = xosrealloc(pimg->label_buf, n + 1);
      if (p) {
	 pimg->label_buf = p;
	 pimg->buf_len = n + 1;
      } else {
	 /* can't store this station, so just reset to no prefix */
	 if (pimg->label_len) {
	    /* can't here... putc(0, pimg->fh); */
	    abort();
	    pimg->label_buf[0] = '\0';
	    pimg->label_len = 0;
	 }
      }
   }
   memcpy(pimg->label_buf + len, s + len, n - len + 1);
}

void
img_write_item(img *pimg, int code, int flags, const char *s,
	       double x, double y, double z)
{
   if (!pimg) return;
   if (pimg->version == 3) {
      int opt = 0;
      switch (code) {
       case img_LABEL:
	 write_v3label(pimg, 0x40 | flags, s);
	 opt = 0;
	 break;
       case img_MOVE:
	 opt = 15;
	 break;
       case img_LINE:
	 write_v3label(pimg, 0x80 | flags, s ? s : "");
	 opt = 0;
	 break;
       default: /* ignore for now */
	 return;
      }
      if (opt) putc(opt, pimg->fh);
      /* Output in cm */
      put32((INT32_T)(x * 100.0), pimg->fh);
      put32((INT32_T)(y * 100.0), pimg->fh);
      put32((INT32_T)(z * 100.0), pimg->fh);
   } else {
      size_t len;
      INT32_T opt = 0;
      ASSERT(pimg->version > 0);
      switch (code) {
       case img_LABEL:
	 if (pimg->version == 1) {
	    /* put a move before each label */
	    img_write_item(pimg, img_MOVE, 0, NULL, x, y, z);
	    put32(2, pimg->fh);
	    fputsnl(s, pimg->fh);
	    return;
	 }
	 len = strlen(s);
	 if (len > 255 || strchr(s, '\n')) {
	    /* long label - not in early incarnations of v2 format, but few
	     * 3d files will need these, so better not to force incompatibility
	     * with a new version I think... */
	    putc(7, pimg->fh);
	    putc(flags, pimg->fh);
	    put32(len, pimg->fh);
	    fputs(s, pimg->fh);
	 } else {
	    putc(0x40 | (flags & 0x3f), pimg->fh);
	    fputsnl(s, pimg->fh);
	 }
	 opt = 0;
	 break;
       case img_MOVE:
	 opt = 4;
	 break;
       case img_LINE:
         if (pimg->version > 1) {
	    opt = 0x80 | (flags & 0x3f);
	    break;
         }
	 opt = 5;
	 break;
       default: /* ignore for now */
	 return;
      }
      if (pimg->version == 1) {
	 put32(opt, pimg->fh);
      } else {
	 if (opt) putc(opt, pimg->fh);
      }
      /* Output in cm */
      put32((INT32_T)(x * 100.0), pimg->fh);
      put32((INT32_T)(y * 100.0), pimg->fh);
      put32((INT32_T)(z * 100.0), pimg->fh);
   }
}

void
img_close(img *pimg)
{
   if (pimg) {
      if (pimg->fh) {
	 if (pimg->fRead) {
	    osfree(pimg->survey);
	 } else {
	    /* write end of data marker */
	    switch (pimg->version) {
	     case 1:
	       put32((INT32_T)-1, pimg->fh);
	       break;
	     case 2:
	       putc(0, pimg->fh);
	       break;
	     case 3:
	       if (pimg->label_len) putc(0, pimg->fh);
	       putc(0, pimg->fh);
	       break;
	    }
	 }
	 fclose(pimg->fh);
      }
      osfree(pimg->label_buf);
      osfree(pimg);
   }
}
