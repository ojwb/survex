/* wrap.c
 * Translate pre-0.90 survex invocation into 0.90 or later cavern invocation
 * Copyright (C) 1999,2001 Olly Betts
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

/* This isn't at all pretty, but it has a limited purpose and lifespan,
 * so I'm not going to invest a lot of time in it...
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "filelist.h"
#include "osdepend.h"

/* RISC OS doesn't have exec*(), but system is close */
#define HAVE_EXECV 1
#if (OS==RISCOS)
# undef HAVE_EXECV
#elif (OS==WIN32) || (OS==MSDOS)
# include <process.h>
#else
# include <unistd.h>
#endif

static int output_to_command_file = 0;
static char *output_to = NULL;

static int fPercent = -1;

#define COMMAND_FILE_COMMENT_CHAR ';'

#define CR '\r'
#define LF '\n'

#if (OS==UNIX)
# define SWITCH_SYMBOLS "-" /* otherwise '/' causes ambiguity problems */
#else
# define SWITCH_SYMBOLS "/-"
#endif

#define EXT_SVX_CMND "svc"

#define STDERR stdout

static void
wr(const char *s)
{
   if (s) puts(s);
}

static void
fatal(const char *s, void (*fn)(const char *), const char *arg)
{
   fprintf(STDERR, "\nFatal error from survex: %s\n", s);
   if (fn) (fn)(arg);
   exit(EXIT_FAILURE);
}

static void *xmalloc(size_t size) {
   void *p = malloc(size);
   if (!p) fatal("Out of memory!", NULL, NULL);
   return p;
}

typedef struct list {
   const char *line;
   struct list *next;
} list;

static list *head = NULL, *tail = NULL;

static void
add_to_list(const char *s)
{
   list *x;
   x = xmalloc(sizeof(list));
   x->line = s;
   x->next = NULL;
   if (!tail) {
      head = tail = x;
   } else {
      tail->next = x;
      tail = x;
   }
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
static char *
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
fopenWithPthAndExt(const char * pth, const char * fnm, const char * ext,
		   const char * mode, char **pfnmUsed)
{
   char *fnmFull = NULL;
   FILE *fh = NULL;
   bool fAbs;

   /* if no pth treat fnm as absolute */
   fAbs = (pth == NULL || *pth == '\0' || fAbsoluteFnm(fnm));

   /* if appropriate, try it without pth */
   if (fAbs) {
      fh = fopen_not_dir(fnm, mode);
      if (fh) {
	 if (pfnmUsed) {
	    fnmFull = xmalloc(strlen(fnm) + 1);
	    strcpy(fnmFull, fnm);
	 }
      } else {
	 if (ext && *ext) {
	    /* we've been given an extension so try using it */
	    fnmFull = AddExt(fnm, ext);
	    fh = fopen_not_dir(fnmFull, mode);
	 }
      }
   } else {
      /* try using path given - first of all without the extension */
      fnmFull = UsePth(pth, fnm);
      fh = fopen_not_dir(fnmFull, mode);
      if (!fh) {
	 if (ext && *ext) {
	    /* we've been given an extension so try using it */
	    char *fnmTmp;
	    fnmTmp = fnmFull;
	    fnmFull = AddExt(fnmFull, ext);
	    free(fnmTmp);
	    fh = fopen_not_dir(fnmFull, mode);
	 }
      }
   }

   /* either it opened or didn't. If not, fh == NULL from fopen_not_dir() */

   /* free name if it didn't open or name isn't wanted */
   if (fh == NULL || pfnmUsed == NULL) free(fnmFull);
   if (pfnmUsed) *pfnmUsed = (fh ? fnmFull : NULL);
   return fh;
}

static char *
PthFromFnm(const char *fnm)
{
   char *pth;
   const char *lf;
   int lenpth = 0;

   lf = strrchr(fnm, FNM_SEP_LEV);
#ifdef FNM_SEP_LEV2
   {
      const char *lf2 = strrchr(lf + 1, FNM_SEP_LEV2);
      if (lf2) lf = lf2;
   }
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

typedef enum {COMMAND,COMMAND_FILENAME,SVX_FILE,TITLE} Mode;

static void process_command(const char * string, const char * pth);
static void command_file(const char * pth, const char * fnm);
static void checkmode(void (*fn)(const char *), const char * arg);
static void skipopt(const char * s);

#define TITLE_LEN 80
static char SurveyTitle[TITLE_LEN];

static Mode mode;

static void process_command_line(int argc, char **argv) {
  int j;
  mode=COMMAND;

  *SurveyTitle='\0';

  if (argc<2)
    fatal("No arguments given", NULL, NULL);

  for (j = 0; ++j < argc; ) process_command(argv[j], "");

  /* Check if everything's okay */
  checkmode(skipopt,NULL);
}

static void
include(const char *fnm)
{
   char *p;
   p = xmalloc(strlen(fnm) + 13);
   sprintf(p, "*include \"%s\"\n", fnm);
   add_to_list(p);
}

static const char *
datafile(const char *fnm, const char *pth)
{
   /* if no pth treat fnm as absolute */
   if (pth != NULL && *pth != '\0' && !fAbsoluteFnm(fnm)) {
      char *f = UsePth(pth, fnm);
      include(f);
      if (!output_to) {
	 output_to = f;
      } else {
	 free(f);
      }
   } else {
      include(fnm);
      if (!output_to) {
	 output_to = xmalloc(strlen(fnm) + 1);
	 strcpy(output_to, fnm);
      }
   }
   return "";
}

static const char *
process_command_mode(const char *string, const char *pth)
{
  char ch;
  const char *s = string;
  ch = *(s++);
  if (strchr(SWITCH_SYMBOLS,ch)!=NULL) {
    char chOpt;
    int fSwitch=fTrue;
    chOpt=toupper(*s);
    s++;
    if (chOpt=='!') {
      fSwitch=fFalse;
      chOpt=toupper(*s);
      s++;
      if (strchr("DFST",chOpt)) {
        /* negating these option doesn't make sense */
	fatal("Not a boolean option - `!' prefix not meaningful",
	      skipopt, string);
      }
    }
    switch (chOpt) {
      case 'A': /* Ascii */ /* ignore nowadays */
        break;
      case 'C': /* Case */
        if (fSwitch) {
          chOpt=*s++;
          switch (toupper(chOpt)) {
	   case 'U':
             add_to_list("*case toupper\n");
	     break;
	   case 'L':
             add_to_list("*case tolower\n");
	     break;
	   default: fatal("Expected U or L", skipopt, string);
          }
        } else {
	  add_to_list("*case preserve\n");
	}
        break;
      case 'D': /* Data File (so fnames can begin with '-' or '/' */
        mode=SVX_FILE;
        break;
      case 'F': /* Command File */
        mode=COMMAND_FILENAME;
        break;
      case 'N': /* NinetyToUp */
        if (fSwitch)
	  add_to_list("*infer plumbs on\n");
        else
	  add_to_list("*infer plumbs off\n");
        break;
      case 'P': /* Percentage */
        fPercent = fSwitch;
        break;
#if 0
      /* -S was never implemented */
      case 'S': /* Special Characters */
        mode=SPECIAL_CHARS;
        break;
#endif
      case 'T': /* Title */
        mode=TITLE;
        break;
      case 'U': /* Unique */
        if (fSwitch) {
	  char *p;
          int ln = 0;
          ch = *(s++); /* if we have no digits, ln=0 & error is given */
          while (isdigit(ch)) {
            ln = ln * 10 + (ch - '0');
            ch = *(s++);
          }
          s--;
	  if (ln < 1) {
	    fatal("Syntax: -U<uniqueness> where uniqueness > 0",
	          skipopt, string);
	  }
	  p = xmalloc(32);
	  sprintf(p, "*truncate %d\n", ln);
	  add_to_list(p);
        } else {
	  add_to_list("*truncate off\n");
	}
        break;
      default:
       fatal("Unknown command",skipopt,string);
    }
  } else {
    switch (ch) {
      case '(': /* Push settings */
        add_to_list("*begin\n");
        break;
      case ')': /* Pull settings */
        add_to_list("*end\n");
        break;
      case '@': /* command line extension (same as -f) */
        mode=COMMAND_FILENAME;
        break;
      default: /* assume it is a filename of a data file */
        s = datafile(s - 1, pth);
        break;
    }
  }
  return s;
}

static void
process_command(const char * string, const char * pth)
{
  const char *s = string;
/*  printf("%d >%s<\n",mode,s); */
  while (*s) {
    switch (mode) {
      case COMMAND:
        s = process_command_mode(s, pth);
        break;
      case SVX_FILE:
        s = datafile(s, pth);
        mode=COMMAND;
        break;
      case COMMAND_FILENAME:
        mode = COMMAND;
        command_file(pth, s);
        s = "";
        break;
      case TITLE: {
	 /* Survey title */
	 char *p;
	 p = xmalloc(strlen(s) + 11);
	 sprintf(p, "*title \"%s\"\n", s);
	 add_to_list(p);
	 s += strlen(s); /* advance past it */
	 mode=COMMAND;
	 break;
      }
    }
  }
}

#define COMMAND_BUFFER_LEN 256
static char buffer[COMMAND_BUFFER_LEN];

static void
command_file(const char *pth, const char *fnm)
{
  FILE *fh;
  int   ch;
  char  cmdbuf[COMMAND_BUFFER_LEN];
  int   i;
  char *fnmUsed, *path;
  int  fQuoted;

  fh=fopenWithPthAndExt( pth, fnm, EXT_SVX_CMND, "r", &fnmUsed );
  if (fh==NULL) {
    fatal("Couldn't open command file",skipopt,fnm);
    return;
  }
  path = PthFromFnm(fnmUsed);
  if (!output_to) {
     output_to = xmalloc(strlen(fnmUsed) + 1);
     strcpy(output_to, fnmUsed);
     output_to_command_file = 1;
  }
  free(fnmUsed); /* not needed now */

  ch = getc(fh);
  while (ch!=EOF) {
    fQuoted=fFalse;
    i=0;
    if (ch=='\"') {
      fQuoted=fTrue;
      ch = getc(fh);
    }
    while ( (ch!=EOF) && (ch!=COMMAND_FILE_COMMENT_CHAR) ) {
      if (isspace(ch) || (ch==CR))
        ch=' ';
      /* if it's quoted, a quote ends it, else whitespace ends it */
      if (ch == (fQuoted ? '\"' : ' ') )
        break;
      cmdbuf[i++]=ch;
      if (i>=COMMAND_BUFFER_LEN) {
        buffer[COMMAND_BUFFER_LEN-1]='\0';
	fatal("Command too long",skipopt,cmdbuf);
        i=0; /* kills line */
        ch=COMMAND_FILE_COMMENT_CHAR; /* skips to end of line */
        break;
      }
      ch = getc(fh);
    }
    if (ch==COMMAND_FILE_COMMENT_CHAR) { /* NB fiddle above if modifying */
      while ((ch!=LF) && (ch!=CR) && (ch!=EOF))
        ch = getc(fh);
    }
    /* skip character if not EOF */
    if (ch!=EOF)
      ch = getc(fh);
    if (i>0) {
      cmdbuf[i]='\0';
      process_command(cmdbuf, path);
    }
  }
  if (ferror(fh) || fclose(fh) == EOF) {
     fatal("Error reading command file", wr, fnm);
  }
/* checkmode(skipopt,fnm); */
  free(path);
}

static void
checkmode(void (*fn)(const char *), const char * arg)
{
  const char *s;
  switch (mode) {
    case SVX_FILE:
      s = "Data filename expected after -D";
      break;
    case COMMAND_FILENAME:
      s = "Command filename expected after -F";
      break;
#if 0
   case SPECIAL_CHARS:
      s = "Special characters list expected after -S";
      break;
#endif
    default:
      return;
  }
  fatal(s, fn, arg);
}

static void
skipopt(const char *s)
{
   if (s) puts(s);
   puts("Skipping bad command line option");
}

#if (OS==RISCOS)
#define MYTMP "__svxtmp"
#else
#define MYTMP "__svxtmp.svx"
#endif

int
main(int argc, char **argv)
{
   char *args[6];
   int i;
#ifndef HAVE_EXECV
   char *cmd, *p;
   size_t len;
#endif

   if (argv[1]) {
       if (strcmp(argv[1], "--version") == 0) {
	   puts(PRETTYPACKAGE" "VERSION);
	   exit(0);
       }

       if (strcmp(argv[1], "--help") == 0) {
	   printf(PRETTYPACKAGE" "VERSION"\n\n"
"Syntax: %s [OPTION]... FILE...\n\n"
"This is a compatibility wrapper to help users convert from survex to cavern.\n"
"You should run cavern in preference - this wrapper will be removed at some\n"
"future date.\n", argv[0]);
	   exit(0);
       }
   }

   puts("Hello, welcome to...");
   puts("  ______   __   __   ______    __   __   _______  ___   ___");
   puts(" / ____|| | || | || |  __ \\\\  | || | || |  ____|| \\ \\\\ / //");
   puts("( ((___   | || | || | ||_) )) | || | || | ||__     \\ \\/ //");
   puts(" \\___ \\\\  | || | || |  _  //   \\ \\/ //  |  __||     >  <<");
   puts(" ____) )) | ||_| || | ||\\ \\\\    \\  //   | ||____   / /\\ \\\\");
   puts("|_____//   \\____//  |_|| \\_||    \\//    |______|| /_// \\_\\\\");
   putchar('\n');

   puts("This is a compatibility wrapper to help users convert from survex to cavern.\n"
        "You should run cavern in preference - this wrapper will be removed at some\n"
        "future date.\n");

   process_command_line(argc, argv);

   args[0] = UsePth(PthFromFnm(argv[0]), "cavern");

   i = 1;
   /* ick: honour -P or -!P if specified, else use new default on no %ages */
   if (fPercent == 1) {
      static char arg[] = "--percentage";
      args[i++] = arg;
   }
   if (output_to) {
      /* Only need --output if there was a command file, or we're processing
       * a .svx file which is in a different directory */
      if (output_to_command_file
	  || strchr(output_to, FNM_SEP_LEV)
#ifdef FNM_SEP_LEV2
	  || strchr(output_to, FNM_SEP_LEV2)
#endif
#ifdef FNM_SEP_DRV
	  || strchr(output_to, FNM_SEP_DRV)
#endif
	  ) {
	 static char arg[] = "--output";
	 args[i++] = arg;
	 args[i++] = output_to;
      }
   }
   /* don't bother with temporary file if there's only one file to process */
   if (head == tail && strncmp(head->line, "*include \"", 10) == 0) {
      char *p = xmalloc(strlen(head->line + 10) + 1);
      strcpy(p, head->line + 10);
      /* knock off quote and \n */
      p[strlen(p) - 2] = '\0';
      args[i++] = p;
      args[i] = NULL;

      puts("Run cavern as:\n");

      i = 0;
      while (args[i]) {
         /* may need to quote arguments, but for now just quote arguments with
          * spaces and let the user sort out anything else */
	 if (i) putchar(' ');
         if (strchr(args[i], ' ')) {
	    printf("\"%s\"", args[i]);
	 } else {
	    fputs(args[i], stdout);
	 }
         i++;
      }
      putchar('\n');
   } else {
      FILE *fout;
      static char arg[] = MYTMP;
      args[i++] = arg;
      args[i] = NULL;

      fout = fopen(MYTMP, "w");
      if (!fout) {
         fatal("Couldn't open temporary file:", wr, MYTMP);
         exit(1);
      }

      fputs("; This file was automatically generated from a pre-0.90 survex command line\n"
	    "; Process this file using the command line:\n"
	    ";", fout);

      puts("Run cavern as:\n");

      i = 0;
      while (args[i]) {
         /* may need to quote arguments, but for now just quote arguments with
          * spaces and let the user sort out anything else */
	 if (i) putchar(' ');
         if (strchr(args[i], ' ')) {
	    fprintf(fout, " \"%s\"", args[i]);
	    printf("\"%s\"", args[i]);
	 } else {
	    fprintf(fout, " %s", args[i]);
	    fputs(args[i], stdout);
	 }
         i++;
      }
      fputs("\n\n", fout);
      puts("\n\nwhere "MYTMP" contains:\n");

      while (head) {
         fputs(head->line, fout);
         fputs(head->line, stdout);
         head = head->next;
      }

      if (ferror(fout) || fclose(fout) == EOF) {
	 (void)remove(MYTMP);
	 fatal("Error writing temporary file", wr, MYTMP);
      }
   }

#ifdef HAVE_EXECV
   return execvp(args[0], args);
#else
   len = 1;
   i = 0;
   while (args[i]) {
      len += strlen(args[i]) + 3;
      i++;
   }
   cmd = xmalloc(len);

   p = cmd;
   strcpy(p, args[0]);
   i = 1;
   while (args[i]) {
      p += strlen(p);
      sprintf(p, " \"%s\"", args[i]);
      i++;
   }
   return system(cmd);
#endif
}
