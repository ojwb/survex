/* > wrap.c
 * Translate pre-0.90 survex invocation into 0.90 or later cavern invocation
 * Copyright (C) 1999 Olly Betts
 */

/* This isn't at all pretty, but it has a limited purpose and lifespan, so I'm
 * not going to invest a lot of time in it...
 */

/*
[...]
1996.03.24 changes for Translate table ; buffer -> cmdbuf
*/

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "filelist.h"
#include "osdepend.h"

/* FIXME: check return value */
#define xmalloc malloc

#define COMMAND_FILE_COMMENT_CHAR ';'

#define CR '\r'
#define LF '\n'

#if (OS==UNIX)
# define SWITCH_SYMBOLS "-" /* otherwise '/' causes ambiguity problems */
#else
# define SWITCH_SYMBOLS "/-"
#endif

#if (OS==RISCOS)
# define EXT_SVX_CMND "/svc" /* allows files to be read from DOS discs */
#else
# define EXT_SVX_CMND ".svc"
#endif

static void
fatal(const char *msg)
{
   puts(msg);
   exit(1);
}

/* Make fnm from pth and lf, inserting an FNM_SEP_LEV if appropriate */
static char *
UsePth(const char *pth, const char *lf)
{
   char *fnm;
   int len, len_total;
   int fAddSep = fFalse;

   len = strlen(pth);
   len_total = len + strlen(lf) + 1;

   /* if there's a path and it doesn't end in a separator, insert one */
   if (len && pth[len - 1] != FNM_SEP_LEV) {
#ifdef FNM_SEP_LEV2
      if (pth[len - 1] != FNM_SEP_LEV2) {
#endif
#ifdef FNM_SEP_DRV
         if (pth[len - 1] != FNM_SEP_DRV) {
#endif
            fAddSep = fTrue;
            len_total++;
#ifdef FNM_SEP_DRV
         }
#endif
#ifdef FNM_SEP_LEV2
      }
#endif
   }

   fnm = xmalloc(len_total);
   strcpy(fnm, pth);
   if (fAddSep) fnm[len++] = FNM_SEP_LEV;
   strcpy(fnm + len, lf);
   return fnm;
}

static FILE *
fopen_not_dir(const char *fnm, const char *mode)
{
   if (fDirectory(fnm)) return NULL;
   return fopen(fnm, mode);
}

/* Add ext to fnm, inserting an FNM_SEP_EXT if appropriate */
extern char * FAR
AddExt(const char *fnm, const char *ext)
{
   char * fnmNew;
   int len, len_total;
#ifdef FNM_SEP_EXT
   bool fAddSep = fFalse;
#endif

   len = strlen(fnm);
   len_total = len + strlen(ext) + 1;
#ifdef FNM_SEP_EXT
   if (ext[0] != FNM_SEP_EXT) {
      fAddSep = fTrue;
      len_total++;
   }
#endif

   fnmNew = xmalloc(len_total);
   strcpy(fnmNew, fnm);
#ifdef FNM_SEP_EXT
   if (fAddSep) fnmNew[len++] = FNM_SEP_EXT;
#endif
   strcpy(fnmNew + len, ext);
   return fnmNew;
}

/* fopen file, found using pth and fnm
 * pfnmUsed used to return filename used to open file (ignored if NULL)
 * or NULL if file didn't open
 */
static FILE *
fopenWithPthAndExt(const char * pth, const char * fnm, const char * szExt,
		   const char * szMode, char **pfnmUsed)
{
   char *fnmFull = NULL;
   FILE *fh = NULL;
   bool fAbs;

   /* if no pth treat fnm as absolute */
   fAbs = (pth == NULL || *pth == '\0' || fAbsoluteFnm(fnm));

   /* if appropriate, try it without pth */
   if (fAbs) {
      fh = fopen_not_dir(fnm, szMode);
      if (fh) {
	 if (pfnmUsed) {
	    fnmFull = xmalloc(strlen(fnm) + 1);
	    strcpy(fnmFull, fnm);
	 }
      } else {
	 if (szExt && *szExt) {
	    /* we've been given an extension so try using it */
	    fnmFull = AddExt(fnm, szExt);
	    fh = fopen_not_dir(fnmFull, szMode);
	 }
      }
   } else {
      /* try using path given - first of all without the extension */
      fnmFull = UsePth(pth, fnm);
      fh = fopen_not_dir(fnmFull, szMode);
      if (!fh) {
	 if (szExt && *szExt) {
	    /* we've been given an extension so try using it */
	    char *fnmTmp;
	    fnmTmp = fnmFull;
	    fnmFull = AddExt(fnmFull, szExt);
	    free(fnmTmp);
	    fh = fopen_not_dir(fnmFull, szMode);
	 }
      }
   }

   /* either it opened or didn't. If not, fh == NULL from fopen_not_dir() */

   /* free name if it didn't open or name isn't wanted */
   if (fh == NULL || pfnmUsed == NULL) free(fnmFull);
   if (pfnmUsed) *pfnmUsed = (fh ? fnmFull : NULL);
   return fh;
}

extern char *
PthFromFnm(const char *fnm)
{
   char *pth;
   const char *lf;
   int lenpth = 0;

   lf = strrchr(fnm, FNM_SEP_LEV);
#ifdef FNM_SEP_LEV2
   if (!lf) lf = strrchr(fnm, FNM_SEP_LEV2);
#endif
#ifdef FNM_SEP_DRV
   if (!lf) lf = strrchr(fnm, FNM_SEP_DRV);
#endif
   if (lf) lenpth = lf - fnm + 1;

   pth = xmalloc(lenpth + 1);
   memcpy(pth, fnm, lenpth);
   pth[lenpth] = '\0';

   return pth;
}

static FILE *fout;

typedef enum {COMMAND,COMMAND_FILENAME,SPECIAL_CHARS,SVX_FILE,TITLE} Mode;

static void process_command( char * string, char * pth );
static void command_file( char * pth, char * fnm );
static void checkmode(Mode mode,void (*fn)( char *, int ),char * szArg);
static void skipopt( char * sz, int n );

#define TITLE_LEN 80
char szSurveyTitle[TITLE_LEN];
int fExplicitTitle=fFalse;
static Mode mode;

static void process_command_line(int argc, char **argv) {
  int j;
  mode=COMMAND;

  *szSurveyTitle='\0';

  if (argc<2) 
    fatal("No arguments given");

  for(j=0;++j<argc;)
    process_command(argv[j],"");

  /* Check if everything's okay */
  checkmode(mode,skipopt,NULL);
}

static char *datafile(char *fnm,char *pth) {
   /* if no pth treat fnm as absolute */
   if (pth != NULL && *pth != '\0' && !fAbsoluteFnm(fnm)) {
      fnm = UsePth(pth, fnm);
      fprintf(fout, "*include \"%s\"\n", fnm);
      free(fnm);
   } else {
      fprintf(fout, "*include \"%s\"\n", fnm);
   }
   return "";
}

static char *process_command_mode( char *string, char *pth ) {
  char ch, *sz=string;
  ch=*(sz++);
  if (strchr(SWITCH_SYMBOLS,ch)!=NULL) {
    char chOpt;
    int fSwitch=fTrue;
    chOpt=toupper(*sz);
    sz++;
    if (chOpt=='!') {
      fSwitch=fFalse;
      chOpt=toupper(*sz);
      sz++;
      if (strchr("DFST",chOpt)) {
        /* negating these option doesn't make sense */
	// FIXME: error(27,skipopt,string);
        return sz;
      }
    }
    switch (chOpt) {
      case 'A': /* Ascii */
        if (fSwitch) fputs(" --ascii", stdout);
        break;
      case 'C': /* Case */
        if (fSwitch) {
          chOpt=*sz++;
          switch (toupper(chOpt)) {
	   case 'U':
	     fprintf(fout, "*case toupper\n");
	     break;
	   case 'L':
	     fprintf(fout, "*case tolower\n");
	     break;
	   default: //FIXME: error(28,skipopt,string,0);
          }
        } else {
	  fprintf(fout, "*case preserve\n");
	}
        break;
      case 'D': /* Data File (so fnames can begin with '-' or '/' */
        mode=SVX_FILE;
        break;
      case 'F': /* Command File */
        mode=COMMAND_FILENAME;
        break;
      case 'N': /* NinetyToUp */
        fprintf(fout, "*infer plumbs %s\n", (fSwitch ? "on" : "off"));
        break;
      case 'P': /* Percentage */
#ifndef NO_PERCENTAGE
        if (fSwitch) printf(" --percentage");
#endif
        break;
      case 'S': /* Special Characters */
        mode=SPECIAL_CHARS;
        break;
      case 'T': /* Title */
        mode=TITLE;
        break;
      case 'U': /* Unique */
        if (fSwitch) {
          int ln=0;
          ch=*(sz++); /* if we have no digits, ln=0 & error is given */
          while (isdigit(ch)) {
            ln=ln*10+(ch-'0');
            ch=*(sz++);
          }
          sz--;
	   // FIXME:          if (ln < 1) error(29,skipopt,string,0);
	  fprintf(fout, "*truncate %d\n", ln);
        } else
	  fprintf(fout, "*truncate 0\n"); // FIXME: ick
        break;
      default:
       // FIXME: error(30,skipopt,string,0);
    }
  } else {
    switch (ch) {
      case '(': /* Push settings */
        fprintf(fout, "*begin\n");
        break;
      case ')': /* Pull settings */
        fprintf(fout, "*end\n");
        break;
      case '@': /* command line extension (same as -f) */
        mode=COMMAND_FILENAME;
        break;
      default: /* assume it is a filename of a data file */
        sz=datafile(sz-1,pth);
        break;
    }
  }
  return sz;
}

static void process_command( char * string, char * pth ) {
  char * sz=string;
/*  printf("%d >%s<\n",mode,sz); */
  while (*sz!='\0') {
    switch (mode) {
      case COMMAND:
        sz=process_command_mode(sz,pth);
        break;
      case SVX_FILE:
        sz=datafile(sz,pth);
        mode=COMMAND;
        break;
      case COMMAND_FILENAME:
        mode=COMMAND;
        command_file( pth, sz );
        sz="";
        break;
      case SPECIAL_CHARS: /* Special Chars */
       // FIXME:        NOT_YET;
        /* Write this bit */
        break;
      case TITLE: /* Survey title */
        fprintf(fout, "*title \"%s\"\n", sz);
        sz+=strlen(sz); /* advance past it */
        mode=COMMAND;
        break;
    }
  }
}

#define COMMAND_BUFFER_LEN 256
static char buffer[COMMAND_BUFFER_LEN];

static void command_file( char * pth, char * fnm ) {
  FILE *fh;
  int   byte;
  char  cmdbuf[COMMAND_BUFFER_LEN];
  int   i;
  char *    fnmUsed;
  int  fQuoted;

  fh=fopenWithPthAndExt( pth, fnm, EXT_SVX_CMND, "r", &fnmUsed );
  if (fh==NULL) {
     //FIXME:error(22,skipopt,fnm,0);
    return;
  }
  pth=PthFromFnm(fnmUsed);
/*  UsingDataFile( fnmUsed );*/
  free(fnmUsed); /* not needed now */

  byte=fgetc(fh);
  while (byte!=EOF) {
    fQuoted=fFalse;
    i=0;
    if (byte=='\"') {
      fQuoted=fTrue;
      byte=fgetc(fh);
    }
    while ( (byte!=EOF) && (byte!=COMMAND_FILE_COMMENT_CHAR) ) {
      if (isspace(byte) || (byte==CR))
        byte=' ';
      /* if it's quoted, a quote ends it, else whitespace ends it */
      if (byte == (fQuoted ? '\"' : ' ') )
        break;
      cmdbuf[i++]=byte;
      if (i>=COMMAND_BUFFER_LEN) {
        buffer[COMMAND_BUFFER_LEN-1]='\0';
	 //FIXME:error(26,skipopt,cmdbuf,0);
        i=0; /* kills line */
        byte=COMMAND_FILE_COMMENT_CHAR; /* skips to end of line */
        break;
      }
      byte=fgetc(fh);
    }
    if (byte==COMMAND_FILE_COMMENT_CHAR) { /* NB fiddle above if modifying */
      while ((byte!=LF) && (byte!=CR) && (byte!=EOF))
        byte=fgetc(fh);
    }
    /* skip character if not EOF */
    if (byte!=EOF)
      byte=fgetc(fh);
    if (i>0) {
      cmdbuf[i]='\0';
      process_command( cmdbuf, pth );
    }
  }
  if (ferror(fh)||(fclose(fh)==EOF))
     {} // FIXME: error(23,wr,fnm,0);
/* checkmode(mode,skipopt,fnm); */
  free(pth);
}

static void checkmode(Mode mode,void (*fn)( char *, int ),char * szArg) {
  int errnum;
  switch (mode) {
    case SVX_FILE:
      errnum=10; break;
    case COMMAND_FILENAME:
      errnum=18; break;
    case SPECIAL_CHARS:
      errnum=19; break;
    default:
      return;
  }
   // FIXME:error(errnum,fn,szArg,0);
}

static void skipopt( char * sz, int n ) {
  if (sz)
      puts(sz); // FIXME: n
  puts("FIXME:"/*msg(83)*/);
}

#define MYTMP "__xxxtmp.svx"

#if 0
static void error(void) {
   printf("Syntax error in command line arguments\n");
   exit(1);
}
#endif

int main(int argc, char **argv) {

   fout = fopen(MYTMP, "w");
   if (!fout) {
      printf("Couldn't open temporary file `%s'\n", MYTMP);
      exit(1);
   }

   printf("cavern");
   
   process_command_line(argc, argv);

   fclose(fout);

   /*
   while (*argv) {
      printf(" %s", *argv++);
   }
   */
   printf(" "MYTMP"\n");

   return 0;
}
