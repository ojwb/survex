/* > img.c
 * Routines for reading and writing Survex ".3d" image files
 * Copyright (C) 1993-1997 Olly Betts
 */

/*
1993.07.20 extracted image file reading code from prindept.c
1993.07.27 added write code and renamed to img.c
           tidied up
1993.07.28 added img_SCALE
1993.08.10 added code to write binary .3d files
1993.08.13 write .3d files in binary mode now ("wb" rather than "w")
           eliminated szDateStamp from img_open_write (use szTmp instead)
1993.08.15 standardised header
1993.08.16 added pfPthUsed argument to fopenWithPthAndExt (give NULL here)
1993.08.17 use EXT_SVX_3D to determine extension (if any) for image files
1993.09.22 added function list
1993.10.18 (BP) couple of img * casts on mallocs to stop warnings
1993.11.06 extracted messages and DATEFORMAT to msg file
1993.11.17 extended binary format to deal with crosses and labels
1993.11.18 malloc, free -> xosmalloc, osfree
           added call to fDirectory
1994.03.14 altered fopenWithPthAndExt to give filename actually used
1994.03.20 added img_error
1994.04.07 img_CROSS is now deprecated, and so gets filtered out
1994.04.12 fixed incorrect filtering of img_CROSS
1994.08.31 converted to use fputnl()
1994.09.21 fputsnl introduced
1994.09.22 define STANDALONE for use outside Survex
1994.10.08 sizeof() corrected to ossizeof()
1995.01.31 added version number check to img_open
           reindented
1995.10.11 getline(FILE*,char*) -> getline(char*,size_t,FILE*)
1996.02.19 fettled parameter names of getline()
1996.04.04 fixed 4 warnings
1997.06.05 added const
*/

/* Function list
img_open:        opens image file, reads header into img struct
img_open_write:  opens new image file & writes header
img_read_datum:  reads image data item in binary or ascii
img_write_datum: writes image data item in binary or ascii
img_close:       close image file
img_error:       gives message number of error if img_open* returned NULL
                 [may be overwritten by calls to msg()]
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>

#ifndef STANDALONE
# include "useful.h"
# include "error.h"
# include "filelist.h"
# define TIMENA msg(108)
# define TIMEFMT msg(107)
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
# define streq(S1,S2) (!strcmp((S1),(S2)))
# define fFalse 0
# define fTrue 1
# define bool int
/* dummy do {...} while(0) hack to permit stuff like
 * if (s) fputsnl(s,fh); else exit(1);
 * to work as intended
 */
# define fputsnl(SZ,FH) do {fputs((SZ),(FH));fputc('\n',(FH));} while(0)

static long get32( FILE *fh ) {
  long w;
  w =fgetc(fh);
  w|=(long)fgetc(fh)<<8l;
  w|=(long)fgetc(fh)<<16l;
  w|=(long)fgetc(fh)<<24l;
  return w;
}

static void put32( long w, FILE *fh ) {
  fputc( (char)(w     ), fh);
  fputc( (char)(w>> 8l), fh);
  fputc( (char)(w>>16l), fh);
  fputc( (char)(w>>24l), fh);
}

/* this reads a line from a stream to a buffer. Any eol chars are removed */
/* from the file and the length of the string including '\0' returned */
static int getline( char *buf, size_t len, FILE *fh ) {
  /* !HACK! len ignored here at present */
  int i=0;
  int ch;

  ch=fgetc(fh);
  while (ch!='\n' && ch!='\r' && ch!=EOF) {
    buf[i++]=ch;
    ch=fgetc(fh);
  }
  if (ch!=EOF) { /* remove any further eol chars (for DOS) */
    do {
      ch=fgetc(fh);
    } while (ch=='\n' || ch=='\r');
    ungetc(ch,fh); /* we don't want it, so put it back */
  }
  buf[i]='\0';
  return (i+1);
}
#endif
#include "img.h"

#define lenSzTmp 256
static char szTmp[lenSzTmp];

#ifndef STANDALONE
static enum {IMG_NONE=0, IMG_FILENOTFOUND=24, IMG_OUTOFMEMORY=38,
 IMG_DIRECTORY=44, IMG_CANTOPENOUT=47, IMG_BADFORMAT=106} img_errno=IMG_NONE;
#else
static img_errcode img_errno=IMG_NONE;
#endif

#ifdef STANDALONE
img_errcode img_error( void ) {
  return img_errno;
}
#else
int img_error( void ) {
  return (int)img_errno;
}
#endif

img *img_open( const char *fnm, char *szTitle, char *szDateStamp ) {
  img *pimg;

  if (fDirectory(fnm)) {
    img_errno=IMG_DIRECTORY;
    return NULL;
  }

  pimg= (img *) xosmalloc(ossizeof(img));
  if (pimg==NULL) {
    img_errno=IMG_OUTOFMEMORY;
    return NULL;
  }

  pimg->fh=fopenWithPthAndExt("",fnm,EXT_SVX_3D,"rb",NULL);
  if (pimg->fh==NULL) {
    osfree(pimg);
    img_errno=IMG_FILENOTFOUND;
    return NULL;
  }

  getline(szTmp,ossizeof(szTmp),pimg->fh);   /* id string */
  if (!streq(szTmp,"Survex 3D Image File")) {
    fclose(pimg->fh);
    osfree(pimg);
    img_errno=IMG_BADFORMAT;
    return NULL;
  }
  getline(szTmp,ossizeof(szTmp),pimg->fh);           /* file format version */
  pimg->fBinary=(tolower(*szTmp)=='b'); /* binary file iff B or b prefix */
  /* knock off the 'B' or 'b' if it's there */
  if (!streq( pimg->fBinary ? szTmp+1 : szTmp ,"v0.01")) {
    fclose(pimg->fh);
    osfree(pimg);
    img_errno=IMG_BADFORMAT; /* ought to distinguish really !HACK! */
    return NULL;
  }
  /* !HACK! sizeof parameter is rather bogus here */
  getline( (szTitle     ? szTitle     : szTmp ), ossizeof(szTmp), pimg->fh );
  getline( (szDateStamp ? szDateStamp : szTmp ), ossizeof(szTmp), pimg->fh );
  pimg->fLinePending=fFalse;  /* not in the middle of a 'LINE' command */
  pimg->fRead=fTrue; /* reading from this file */
  img_errno=IMG_NONE;
  return pimg;
}

img *img_open_write( const char *fnm, char *szTitle, bool fBinary ) {
  time_t tm;
  img *pimg;

  if (fDirectory(fnm)) {
    img_errno=IMG_DIRECTORY;
    return NULL;
  }

  pimg= (img *) xosmalloc(ossizeof(img));
  if (pimg==NULL) {
    img_errno=IMG_OUTOFMEMORY;
    return NULL;
  }

  pimg->fh=fopen(fnm,"wb");
  if (!pimg->fh) {
    osfree(pimg);
    img_errno=IMG_CANTOPENOUT;
    return NULL;
  }

  /* Output image file header */
  /* (using puts() in case strings contain % characters) */
  fputs("Survex 3D Image File\n",pimg->fh); /* file identifier string */
  if (fBinary)
    fputs( "Bv0.01\n", pimg->fh );    /* binary file format version number */
  else
    fputs( "v0.01\n", pimg->fh );            /* file format version number */
  fputsnl(szTitle,pimg->fh);
  tm=time( NULL );
  if (tm==(time_t)-1)
    fputsnl(TIMENA,pimg->fh);
  else {
    /* output current date and time in format specified */
    strftime(szTmp, lenSzTmp, TIMEFMT, localtime(&tm));
    fputsnl( szTmp, pimg->fh );
  }
  pimg->fBinary=fBinary;
  pimg->fRead=fFalse; /* writing to this file */
  img_errno=IMG_NONE;
  return pimg;
}

int img_read_datum( img *pimg, char *sz, float *px, float *py, float *pz ) {
  static float x=0.0f, y=0.0f, z=0.0f;
  int result;
  if (pimg->fBinary) {
    long opt;
    again: /* label to goto if we get a cross */
    opt = get32(pimg->fh);
    if (opt==-1)
      return img_STOP; /* end of data marker */
    switch (opt) {
      case 1:
        (void)get32(pimg->fh);
        (void)get32(pimg->fh);
        (void)get32(pimg->fh);
        goto again;
      case 2:
        fgets(sz,256,pimg->fh);
        sz+=strlen(sz)-1;
        if (*sz!='\n')
         return img_BAD;
        *sz='\0';
        result = img_LABEL;
        goto done;
      case 4: result=img_MOVE; break;
      case 5: result=img_LINE; break;
      default: return img_BAD;
    }
    x = (float)get32(pimg->fh)/100.0f;
    y = (float)get32(pimg->fh)/100.0f;
    z = (float)get32(pimg->fh)/100.0f;
    done:
    *px=x; *py=y; *pz=z;
    return result;
  } else {
    ascii_again:
    if (feof(pimg->fh))
      return img_STOP;
    if (pimg->fLinePending) {
      pimg->fLinePending=fFalse;
      result=img_LINE;
    } else {
      if (fscanf(pimg->fh,"%s",szTmp)<1)
        return img_STOP; /* Nothing found */
      if (streq(szTmp,"move"))
        result=img_MOVE;
      else if (streq(szTmp,"draw"))
        result=img_LINE;
      else if (streq(szTmp,"line")) {
        pimg->fLinePending=fTrue; /* to get second triplet processed as a LINE */
        result=img_MOVE;
      } else if (streq(szTmp,"cross")) {
        if (fscanf(pimg->fh,"%f%f%f",px,py,pz)<3)
          return img_BAD;
        goto ascii_again;
      } else if (streq(szTmp,"name")) {
        if (fscanf(pimg->fh,"%s",sz)<1)
          return img_BAD;
        result=img_LABEL;
      } else if (streq(szTmp,"scale")) {
        if (fscanf(pimg->fh,"%f",px)<1)
          return img_BAD;
        *py=*pz=*px;
        return img_SCALE;
      } else
        return img_BAD; /* unknown keyword */
    }

    if (fscanf(pimg->fh,"%f%f%f",px,py,pz)<3)
       return img_BAD;
    return result;
  }
}

void img_write_datum( img *pimg, int code, const char *sz,
                      float x, float y, float z ) {
  if (pimg->fBinary) {
    float Sc=(float)100.0; /* Output in cm */
    long opt=0;
    switch (code) {
      case img_LABEL: /* put a move before each label */
      case img_MOVE:
        opt=4; break;
      case img_LINE:
        opt=5; break;
      case img_CROSS:
        opt=1; break;
      case img_SCALE: /* fprintf( pimg->fh, "scale %9.2f\n", x ); */
      default: /* ignore for now */
        break;
    }
    if (opt) {
      put32(opt,pimg->fh);
      put32((long)(x*Sc),pimg->fh);
      put32((long)(y*Sc),pimg->fh);
      put32((long)(z*Sc),pimg->fh);
    }
    if (code==img_LABEL) {
      put32(2,pimg->fh);
      fputsnl(sz,pimg->fh);
    }
  } else {
    switch (code) {
      case img_MOVE:
        fprintf( pimg->fh, "move %9.2f %9.2f %9.2f\n", x, y, z );
        break;
      case img_LINE:
        fprintf( pimg->fh, "draw %9.2f %9.2f %9.2f\n", x, y, z );
        break;
      case img_CROSS:
        fprintf( pimg->fh, "cross %9.2f %9.2f %9.2f\n", x, y, z );
        break;
      case img_LABEL:
        fprintf( pimg->fh, "name %s %9.2f %9.2f %9.2f\n", sz, x, y, z );
        break;
      case img_SCALE:
        fprintf( pimg->fh, "scale %9.2f\n", x );
        break;
      default:
        break;
    }
  }
}

void img_close( img *pimg ) {
  if (pimg->fBinary && !pimg->fRead) /* ie writing a binary file */
    put32(-1L,pimg->fh); /* End of data marker */
  if (pimg->fh!=NULL)
    fclose(pimg->fh);
  osfree(pimg);
}
