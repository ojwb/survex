/* > commline.c
 * Command line processing for Survex data reduction program
 * Copyright (C) 1991-1996 Olly Betts
 */

/*
1992.07.20 Fiddled with filename handling bits
1992.10.25 Minor tidying
1993.01.20ish Corrected fopen() mode to "rb" from "rt" (was retimestamping)
1993.01.25 PC Compatibility OKed
1993.02.24 pthData added
1993.02.28 changed to -!X to turn option X off ( & -!C, -CU, -CL )
           indentation prettied up
1993.04.05 (W) added pthData hack
1993.04.06 added case COMMAND: to switch statements to suppress GCC warning
1993.04.07 added catch for no command line arguments
1993.04.08 de-octaled numbers
1993.04.22 out of memory -> error # 0
1993.04.23 added path & extension gubbins & removed extern sz pthData
1993.04.26 hacked on survey pseudo-title code (nasty)
1993.05.03 settings.Ascii -> swttings.fAscii
1993.05.20-ish (W) added #include "filelist.h"
1993.05.23 now use path from .svc file for output if no other path found yet
1993.05.27 fixed above so it'll compile
1993.06.04 FALSE -> fFalse
1993.06.11 fixed -U option so that multiple digit numbers should work
1993.06.16 malloc() -> osmalloc(); syserr() -> fatal(); free() -> osfree()
1993.06.28 changed (!pthOutput) to (pthutput==NULL)
           command_file() now osfree()s strings returned by PthFromFnm()
1993.08.06 changed 'c.CommLine' -> commline.c in error 'where' strings
1993.08.13 fettled header
1993.08.16 fixed code dealing with paths of included files (.svx & .svc)
           added fnmInput
           command line files read in text mode (no null-padding problems)
1993.08.21 fixed minor bug (does nested stuff work now?)
1993.09.23 (W) replaced errors 25 & 26 with identical 1 & 2
1993.10.24 changed OptionChars to more sensible SWITCH_SYMBOLS
           fettled a little
1993.11.03 changed error routines
1993.11.29 added @ <fname> to mean the same as -f <fname>
           errors 1-3 -> 18-20; 23->26 ; 24->23
1993.11.30 sorted out error vs fatal
1994.01.05 added workaround for msc (probable) bug in toupper(*(sz++))
1994.01.17 toupper(*(sz++)) bug is because msc's ctype.h defines it as a
           macro if not in ANSI mode - recoded to be friendly to non-ANSI ccs
1994.03.14 altered fopenWithPthAndExt to give filename actually used
1994.03.15 moved typedef ... Mode; here from survex.h
           added -D and -T switches
           copes with directory extensions now
1994.03.16 extracted UsingDataFile()
1994.03.19 error output all to stderr
1994.04.06 removed unused fQuote
1994.04.16 added -O command line switch
1994.04.17 fixed bug in -O command line switch; added -OI
1994.04.18 created commline.h
1994.05.12 adjusted -O switches; added -P to control percentages
1994.06.20 reordered function to suppress compiler warning
1994.06.21 added int argument to warning, error and fatal
1994.06.26 cs -> pcs; pcs->next used instead of braces_stack
1994.06.27 split off push() and pop() so we can use them from BEGIN and END
1994.08.27 removed all explicit calls to printf, puts, etc
1994.09.13 a change so small even diff might miss it
1994.10.08 added osnew
1994.11.13 added settings.fOrderingFreeable
           rearranged command line options code to make it almost readable
1994.11.14 mode is now static, external to any functions
1994.12.03 kludged fix for multi-word titles
1994.12.11 -f "Name with spaces" should work on command line now
1995.01.21 fixed bug Tim Long reported with -T not absorbing titles
1995.01.31 removed unused message note and moved comment
1996.03.24 changes for Translate table ; buffer -> cmdbuf
*/

#include "survex.h"
#include "error.h"
#include "filelist.h"
#include "datain.h"
#include "commline.h"

typedef enum {COMMAND,COMMAND_FILENAME,SPECIAL_CHARS,SVX_FILE,TITLE} Mode;

static void process_command( sz string, sz pth );
static void command_file( sz pth, sz fnm );
static void checkmode(Mode mode,void (*fn)( sz, int ),sz szArg);
static void skipopt( sz sz, int n );

#define TITLE_LEN 80
char szSurveyTitle[TITLE_LEN];
bool fExplicitTitle=fFalse;
static Mode mode;

void push(void) {
  settings *pcsNew;
  pcsNew=osnew(settings);
/*  memcpy(pcsNew,pcs,ossizeof(settings)); */
  *pcsNew=*pcs; /* copy contents */
  pcsNew->next=pcs;
  pcs=pcsNew;
  pcs->fOrderingFreeable=fFalse; /* don't free it - parent still using it */
/*  pcs->fTranslateFreeable=fFalse; */ /* don't free it - parent still using it */
}

bool pop(void) {
  settings *pcsParent;
  pcsParent=pcs->next;
  if (pcsParent==NULL)
    return fFalse;
  if (pcs->fOrderingFreeable)
    osfree(pcs->ordering); /* not used by parent */
  if (pcs->Translate!=pcsParent->Translate)
    osfree(pcs->Translate-1); /* not used by parent */
  osfree(pcs);
  pcs=pcsParent;
  return fTrue;
}

extern void process_command_line( int argc, sz argv[] ) {
  int j;
  mode=COMMAND;

  *szSurveyTitle='\0';

  if (argc<2) /* No arguments given */
    fatal(49,NULL,NULL,0);

  for(j=0;++j<argc;)
    process_command(argv[j],"");

  /* Check if everything's okay */
  checkmode(mode,skipopt,NULL);
  if (pcs->next) {
    error(20,skipopt,NULL,0);
    while (pop()) /* free any stacked stuff */
      { }
  }
}

static char* datafile(char *fnm,char *pth) {
  char *lf;
  lf=LfFromFnm(fnm);
  if (!fExplicitTitle && strlen(lf)+strlen(szSurveyTitle)+2<=TITLE_LEN) {
    /* !HACK! should cope better with exceeding title buffer */
    char* p=szSurveyTitle;
    if (*p) /* If there's already a file in the list */ {
      p+=strlen(szSurveyTitle);
      *p++=' ';
    }
    strcpy(p,lf);
  }
  /* default path is path of command line file (if we're in one) */
  data_file( pth, fnm );
  return ""; /* start next token */
}

static char *process_command_mode( char *string, char *pth ) {
  char ch, *sz=string;
  ch=*(sz++);
  if (strchr(SWITCH_SYMBOLS,ch)!=NULL) {
    char chOpt;
    bool fSwitch=fTrue;
    chOpt=toupper(*sz);
    sz++;
    if (chOpt=='!') {
      fSwitch=fFalse;
      chOpt=toupper(*sz);
      sz++;
      if (strchr("DFST",chOpt)) {
        /* negating these option doesn't make sense */
        error(27,skipopt,string,0);
        return sz;
      }
    }
    switch (chOpt) {
      case 'A': /* Ascii */
        pcs->fAscii=fSwitch;
        break;
      case 'C': /* Case */
        if (fSwitch) {
          chOpt=*sz++;
          switch (toupper(chOpt)) {
            case 'U': pcs->Case=UPPER; break;
            case 'L': pcs->Case=LOWER; break;
            default:  error(28,skipopt,string,0);
          }
        } else
          pcs->Case=OFF;
        break;
      case 'D': /* Data File (so fnames can begin with '-' or '/' */
        mode=SVX_FILE;
        break;
      case 'F': /* Command File */
        mode=COMMAND_FILENAME;
        break;
      case 'N': /* NinetyToUp */
        pcs->f90Up=fSwitch;
        break;
      case 'O': /* Optimizations level used to solve network */ {
        extern long optimize; /* defined in network.c */
        optimize=0;
        /* -!O and -O both mean turn all optimizations off */
        if (fSwitch) {
          sz--;
          do {
            sz++;
            switch (toupper(*sz)) {
              case 'N':
              case 'L': optimize|= 1; break; /* lollipops */
              case 'P': optimize|= 2; break; /* parallel legs */
              case 'I': optimize|= 4; break; /* iterative matrix soln*/
              case 'D': optimize|= 8; break; /* delta-star */
              case 'S': optimize|=16; break; /* split station list */
            }
          } while (*sz!='\0' && *sz!=' ');
        }
        break;
      }
      case 'P': /* Percentage */
#ifndef NO_PERCENTAGE
        fPercent=fSwitch;
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
          if (ln>IDENT_LEN || ln<1)
            error(29,skipopt,string,0);
          else
            pcs->Unique=ln;
        } else
          pcs->Unique=IDENT_LEN;
        break;
      default:
        error(30,skipopt,string,0);
    }
  } else {
    switch (ch) {
      case '(': /* Push settings */
        push();
        break;
      case ')': /* Pull settings */
        if (!pop())
          error(31,skipopt,string,0);
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

static void process_command( sz string, sz pth ) {
  sz sz=string;
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
        NOT_YET;
        /* Write this bit */
        break;
      case TITLE: /* Survey title */
        if (!fExplicitTitle) {
          fExplicitTitle=fTrue;
          *szSurveyTitle='\0'; /* Throw away any partial default title */
        }
        if (strlen(sz)+strlen(szSurveyTitle)+3<=TITLE_LEN) {
          char *p=szSurveyTitle;
          /* !HACK! should cope better with exceeding title buffer... */
          if (*p) /* If there's already a title in the list */ {
            p+=strlen(szSurveyTitle);
            *p++=',';
            *p++=' ';
          }
          strcpy(p,sz);
        }
        sz+=strlen(sz); /* advance past it */
        mode=COMMAND;
        break;
    }
  }
}

static void command_file( sz pth, sz fnm ) {
  FILE *fh;
  int   byte;
  char  cmdbuf[COMMAND_BUFFER_LEN];
  int   i;
  sz    fnmUsed;
  bool  fQuoted;

  fh=fopenWithPthAndExt( pth, fnm, EXT_SVX_CMND, "r", &fnmUsed );
  if (fh==NULL) {
    error(22,skipopt,fnm,0);
    return;
  }
  pth=PthFromFnm(fnmUsed);
  UsingDataFile( fnmUsed );
  osfree(fnmUsed); /* not needed now */

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
        error(26,skipopt,cmdbuf,0);
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
    error(23,wr,fnm,0);
/* checkmode(mode,skipopt,fnm); */
  osfree(pth);
}

static void checkmode(Mode mode,void (*fn)( sz, int ),sz szArg) {
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
  error(errnum,fn,szArg,0);
}

static void skipopt( sz sz, int n ) {
  if (sz)
    wr(sz,n);
  wr(msg(83),0);
}
