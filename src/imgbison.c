/* > imgbison.c
 * Routines for reading Dave Gibson's image files
 * based on Survex's img.c
 * Copyright (C) 1993-1996 Olly Betts
 */

/*
1996.05.02 wrote imgbison.c based on img.c
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

img *img_open( char *fnm, char *szTitle, char *szDateStamp ) {
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

  pimg->fh=fopenWithPthAndExt("",fnm,"spl","rb",NULL);
  if (pimg->fh==NULL) {
    osfree(pimg);
    img_errno=IMG_FILENOTFOUND;
    return NULL;
  }

  if (szTitle) strcpy(szTitle,"Anon HPGL");
  if (szDateStamp) strcpy(szDateStamp,"None");
  pimg->fLinePending=fFalse;  /* not in the middle of a 'LINE' command */
  pimg->fRead=fTrue; /* reading from this file */
  img_errno=IMG_NONE;
  return pimg;
}

img *img_open_write( char *fnm, char *szTitle, bool fBinary ) {
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

  pimg->fRead=fFalse; /* writing to this file */
  img_errno=IMG_NONE;
  return pimg;
}

int img_read_datum( img *pimg, char *sz, float *px, float *py, float *pz ) {
  int result;
  if (feof(pimg->fh))
    return img_STOP;
  if (fscanf(pimg->fh,"%s",szTmp)<1)
    return img_STOP; /* Nothing found */
  if (streq(szTmp,"PU"))
    result=img_MOVE;
  else if (streq(szTmp,"PD"))
    result=img_LINE;
  else if (streq(szTmp,"+++"))
    return img_STOP;
  else
    return img_BAD; /* unknown keyword */

  if (fscanf(pimg->fh,"%f%f",py,pz)<2)
    return img_BAD;
  *px=0.0f;
  *py/=100.0f;
  *pz/=100.0f;
  return result;
}

void img_write_datum( img *pimg, int code, char *sz,
                      float x, float y, float z ) {
  switch (code) {
    case img_MOVE:
      fprintf( pimg->fh, "PU %9.2f %9.2f\n", y, z );
      break;
    case img_LINE:
      fprintf( pimg->fh, "PD %9.2f %9.2f\n", y, z );
      break;
    default:
      break;
  }
}

void img_close( img *pimg ) {
  if (!pimg->fRead) /* ie writing a binary file */
    fprintf( pimg->fh, "+++\n" ); /* End of data marker */
  if (pimg->fh!=NULL)
    fclose(pimg->fh);
  osfree(pimg);
}
