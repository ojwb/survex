/* > useful.c
 * Copyright (C) 1993-1996 Olly Betts
 */

/*
1993.05.04 added getline()
1993.05.12 (W) added FNTYPE to deal with diff DOS mem models (non-ANSI)
1993.05.13 added get16() and get32()
1993.05.27 fettled a bit & suppressed RISCOS warning
1993.06.07 FNTYPE -> FAR to aid comprehension
1993.07.19 "common.h" -> "osdepend.h"
1993.07.20 added w32 and w16 for 16 and 32 bit words
1993.08.12 removed #include <stdio.h> as useful.h does it
1993.08.13 fettled header
1993.11.18 added fDirectory
1993.11.28 Borland C++ has stat(), so use UNIX code for DOS too
1993.12.04 fettled fDirectory so Borland C++ like it
1993.12.09 moved fDirectory to osdepend.c
1994.03.19 added strcmpi for DJGPP (and anyone else who needs it)
1994.04.19 fettled
1994.11.19 added LITTLE_ENDIAN stuff for probable speed up
1995.02.02 FAR (func)() must be (FAR func)()
1995.10.11 fixed useful_getline to take buffer length
1996.02.19 fixed useful_getline prototype
1996.03.24 fixed strcmpi() to include ctype.h
1996.04.01 strcmpi -> stricmp
1998.03.21 case insensitive string compare -> strcasecmp.c
1998.03.22 changed to use autoconf's WORDS_BIGENDIAN
*/

#include "useful.h"
#include "osdepend.h"

#ifndef WORDS_BIGENDIAN

/* used by macro versions of useful_get<nn> functions */
w16 useful_w16;
w32 useful_w32;

/* the numbers in the file are little endian, so use fread/fwrite */
extern void (FAR useful_put16)( w16 w, FILE *fh ) {
   fwrite(&w,2,1,fh);
}

extern void (FAR useful_put32)( w32 w, FILE *fh ) {
   fwrite(&w,4,1,fh);
}

extern w16 (FAR useful_get16)( FILE *fh ) {
   w16 w;
   fread(&w,2,1,fh);
   return w;
}

extern w32 (FAR useful_get32)( FILE *fh ) {
   w32 w;
   fread(&w,4,1,fh);
   return w;
}

#else

extern void FAR useful_put16( w16 w, FILE *fh ) {
   fputc( (char)(w     ), fh);
   fputc( (char)(w>> 8l), fh);
}

extern void FAR useful_put32( w32 w, FILE *fh ) {
   fputc( (char)(w     ), fh);
   fputc( (char)(w>> 8l), fh);
   fputc( (char)(w>>16l), fh);
   fputc( (char)(w>>24l), fh);
}

extern w16 FAR useful_get16( FILE *fh ) {
   w16 w;
   w =fgetc(fh);
   w|=(w16)(fgetc(fh)<<8l);
   return w;
}

extern w32 FAR useful_get32( FILE *fh ) {
   w32 w;
   w =fgetc(fh);
   w|=(w32)fgetc(fh)<<8l;
   w|=(w32)fgetc(fh)<<16l;
   w|=(w32)fgetc(fh)<<24l;
   return w;
}
#endif

/* this reads a line from a stream to a buffer. Any eol chars are removed
 * from the file and the length of the string including '\0' returned
 */
extern int FAR useful_getline( char *buf, OSSIZE_T len, FILE *fh ) {
   int i=0;
   int ch;

   ch=fgetc(fh);
   while (ch!='\n' && ch!='\r' && ch!=EOF) {
      if (i>=len-1) {
         printf("LINE TOO LONG!\n"); /* !HACK! */
         exit(1);
      }
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
