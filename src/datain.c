/* > datain.c
 * Reads in survey files, dealing with special characters, keywords & data
 * Copyright (C) 1991-2001 Olly Betts
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
#include <config.h>
#endif

#include <limits.h>
#include <stdarg.h>

#include "debug.h"
#include "cavern.h"
#include "filename.h"
#include "message.h"
#include "filelist.h"
#include "netbits.h"
#include "netskel.h"
#include "readval.h"
#include "datain.h"
#include "commands.h"
#include "out.h"
#ifdef NEW3DFORMAT
#include "new3dout.h"
#endif

/* This'll do for now... but we really ought to sort out something in
 * terms of FLT_EPSILON, DBL_EPSILON or LDBL_EPSILON as appropriate */
#define EPSILON ((real)(1e-6F))

#define MAX_KEYWORD_LEN 16

#define PRINT_COMMENT 0

#define var(I) (pcs->Var[(I)])

int ch;

/* Don't explicitly initialise as we can't set the jmp_buf - this has
 * static scope so will be initialised like this anyway */
parse file /* = { NULL, NULL, 0, NULL } */ ;

bool f_export_ok;

static long fpLineStart;

static void showline(const char *dummy, int n);

static void
error_list_parent_files(void)
{
   if (file.parent) {
      parse *p = file.parent;
      fprintf(STDERR, msg(/*In file included from */5));
      while (p) {
	 fprintf(STDERR, "%s:%d", p->filename, p->line);
	 p = p->parent;
	 /* FIXME: number of spaces here should probably vary with language (ick) */
	 if (p) fprintf(STDERR, ",\n                      ");
      }
      fprintf(STDERR, ":\n");
   }
}

void
compile_error(int en, ...)
{
   va_list ap;
   va_start(ap, en);
   error_list_parent_files();
   v_report(1, file.filename, file.line, en, ap);
   va_end(ap);
}

void
compile_warning(int en, ...)
{
   va_list ap;
   va_start(ap, en);
   error_list_parent_files();
   v_report(0, file.filename, file.line, en, ap);
   va_end(ap);
}

/* This function makes a note where to put output files */
static void
using_data_file(const char *fnm)
{
   if (!fnm_output_base) {
      /* was: fnm_output_base = base_from_fnm(fnm); */
      fnm_output_base = baseleaf_from_fnm(fnm);
   } else if (fnm_output_base_is_dir) {
      /* --output pointed to directory so use the leaf basename in that dir */
      char *lf, *p;
      lf = baseleaf_from_fnm(fnm);
      p = use_path(fnm_output_base, lf);
      osfree(lf);
      osfree(fnm_output_base);
      fnm_output_base = p;
      fnm_output_base_is_dir = 0;
   }
}

static void
skipword(void)
{
   while (!isBlank(ch) && !isEol(ch)) nextch();
}

extern void
skipblanks(void)
{
   while (isBlank(ch)) nextch();
}

extern void
showandskipline(const char *dummy, int n)
{
   showline(dummy, n);
   skipline();
   puts(msg(/*Ignoring entire line*/84));
}

extern void
skipline(void)
{
   while (!isEol(ch)) nextch();
}

/* display current line, marking n chars (n == INT_MAX => to end of line)
 * if n < 0 count backwards instead of forwards
 *
 * fpLineStart
 * v
 * 12  13   7.5& 120 -34
 *              ^ftell()
 */
static void
showline(const char *dummy, int n)
{
   char sz[256];
   int i, o, c;
   int state;
   long fpCur;
   dummy = dummy; /* suppress warning */
   puts(msg(/*in this line:*/58));
   fpCur = ftell(file.fh);
   if (n < 0) {
      n = -n;
      fpCur -= n;
   }
   o = (int)(fpCur - fpLineStart - 1);
   fseek(file.fh, fpLineStart, SEEK_SET);
   nextch();
   i = 0;
   state = 0;
   c = o;
   /* FIXME: cope with really long lines */
   while (i < 255 && !isEol(ch)) {
      c--;
      if (c < 0) {
	 c = n; /* correct thing to do first time, harmless otherwise */
	 state++;
      }
      /* turn tabs into a single space for simplicity */
      if (ch == '\t') ch=' ';
      if (ch < ' ' || ch == 127) {
	 /* ctrl char other than tab */
	 sz[i++] = '^';
	 sz[i++] = ch < ' ' ? ch + 64 : '?';
	 /* and adjust o or n to take account of extra character */
	 switch (state) {
	 case 0: o++; break;
	 case 1: if (n < INT_MAX) n++; break;
	 default: break;
	 }
      } else
	 sz[i++]=ch;
      nextch();
   }
   sz[i] = '\0';
   puts(sz);
   n = min(n, i - o); /* cope with n==INT_MAX, or indeed just too big */
   if (n) {
      memset(sz, ' ', o);
      memset(sz + o, '^', n);
      sz[o + n] = '\0';
      puts(sz);
   }
   fseek(file.fh, fpCur, SEEK_SET);
}

#ifndef NO_PERCENTAGE
static long int filelen;
#endif

static void
process_bol(void)
{
   /* Note start of line for error reporting */
   fpLineStart = ftell(file.fh);

#ifndef NO_PERCENTAGE
   /* print %age of file done */
   if (filelen > 0) printf("%d%%\r", (int)(100 * fpLineStart / filelen));
#endif

   nextch();
   skipblanks();
}

static void
process_eol(void)
{
   int eolchar;

   skipblanks();

   if (isComm(ch)) while (!isEol(ch)) nextch();

   if (!isEol(ch)) {
      compile_error(/*End of line not blank*/15);
      skipline();
   }

   eolchar = ch;
   file.line++;
   /* skip any different eol characters so we get line counts correct on
    * DOS text files and similar, but don't count several adjacent blank
    * lines as one */
   while (ch != EOF) {
      nextch();
      if (ch == eolchar || !isEol(ch)) {
	 ungetc(ch, file.fh);
	 break;
      }
   }
}

static bool
process_non_data_line(void)
{
   process_bol();

   if (isData(ch)) return fFalse;

   if (isKeywd(ch)) {
      nextch();
      handle_command();
   }

   process_eol();

   return fTrue;
}

extern void
data_file(const char *pth, const char *fnm)
{
   int begin_lineno_store;
   parse file_store;

   {
      char *filename;
      FILE *fh = fopen_portable(pth, fnm, EXT_SVX_DATA, "rb", &filename);

      if (fh == NULL) {
	 compile_error(/*Couldn't open data file `%s'*/24, fnm);
	 return;
      }

      file_store = file;
      if (file.fh) file.parent = &file_store;
      file.fh = fh;
      file.filename = filename;
      file.line = 1;
   }

   /* FIXME: file.filename maybe? */
   if (fPercent) printf("%s:\n", fnm);

   using_data_file(file.filename);

   begin_lineno_store = pcs->begin_lineno;
   pcs->begin_lineno = 0;

#ifndef NO_PERCENTAGE
   /* Try to find how long the file is...
    * However, under ANSI fseek( ..., SEEK_END) may not be supported */
   filelen = 0;
   if (fPercent) {
      if (fseek(file.fh, 0l, SEEK_END) == 0) filelen = ftell(file.fh);
      rewind(file.fh); /* reset file ptr to start & clear any error state */
   }
#endif

#ifdef HAVE_SETJMP
   /* errors in nested functions can longjmp here */
   if (setjmp(file.jbSkipLine)) {
      /* do nothing special */
   }
#endif

   while (!feof(file.fh)) {
      if (!process_non_data_line()) {
#ifdef NEW3DFORMAT
	 twig *temp = limb;
#endif
	 f_export_ok = fFalse;
	 
	 /* style function returns 0 => error */
	 if (!(pcs->Style)()) {
#ifdef NEW3DFORMAT
	    /* we have just created a very naughty twiglet, and it must be
	     * punished */
	    osfree(limb);
	    limb = temp;
#endif
	 }
      }
   }

   /* don't allow *BEGIN at the end of a file, then *EXPORT in the
    * including file */
   f_export_ok = fFalse;

#ifndef NO_PERCENTAGE
   if (fPercent) putnl();
#endif

   if (pcs->begin_lineno) {
      error_in_file(file.filename, pcs->begin_lineno,
		    /*BEGIN with no matching END in this file*/23);
      skipline();
      /* Implicitly close any unclosed BEGINs from this file */
      do {
	 settings *pcsParent = pcs->next;
	 ASSERT(pcsParent);
	 free_settings(pcs);
	 pcs = pcsParent;
      } while (pcs->begin_lineno);
   }

   pcs->begin_lineno = begin_lineno_store;

   if (ferror(file.fh) || (fclose(file.fh) == EOF))
      fatalerror(/*Error reading file*/18); /* FIXME: name */

   file = file_store;

   /* set_current_fnm(""); not correct if filenames are nested */

   /* don't free this - it may be pointed to by prefix.file */
   /* osfree(file.filename); */
}

static real
handle_plumb(bool *pfPlumbed)
{
   typedef enum {
      CLINO_NULL=-1, CLINO_UP, CLINO_DOWN, CLINO_LEVEL
   } clino_tok;
   static sztok clino_tab[] = {
      {"D",     CLINO_DOWN},
      {"DOWN",  CLINO_DOWN},
      {"H",     CLINO_LEVEL},
      {"LEVEL", CLINO_LEVEL},
      {"U",     CLINO_UP},
      {"UP",    CLINO_UP},
      {NULL,    CLINO_NULL}
   };
   static real clinos[] = {(real)(PI/2.0),(real)(-PI/2.0),(real)0.0};
   clino_tok tok;

   skipblanks();
   if (isalpha(ch)) {
      long fp = ftell(file.fh);
      get_token();
      tok = match_tok(clino_tab, TABSIZE(clino_tab));
      if (tok != CLINO_NULL) {
	 *pfPlumbed = fTrue;
	 return clinos[tok];
      }
      fseek(file.fh, fp, SEEK_SET);
   } else if (isSign(ch)) {
      int chOld = ch;
      nextch();
      if (toupper(ch) == 'V') {
	 nextch();
	 *pfPlumbed = fTrue;
	 return (!isMinus(chOld) ? PI / 2.0 : -PI / 2.0);
      }

      if (isOmit(chOld)) {
	 *pfPlumbed = fFalse;
	 /* no clino reading, so assume 0 with large sd */
	 return (real)0.0;
      }
   }
   return HUGE_REAL;
}

static int
process_normal(prefix *fr, prefix *to, real tape, real comp, real clin,
	       real count, bool fToFirst, bool fNoClino, bool fTopofil,
	       bool fPlumbed)
{
   real dx, dy, dz;
   real vx, vy, vz;
#ifndef NO_COVARIANCES
   real cxy, cyz, czx;
#endif

   bool fNoComp;

#if 0
   print_prefix(fr);
   printf("->");
   print_prefix(to);
   printf(" %.2f %.2f %.2f\n",tape,comp,clin);
#endif

   if (fTopofil) {
      count *= pcs->units[Q_COUNT];
   } else {
      if (tape < (real)0.0) {
	 compile_warning(/*Negative tape reading*/60);
      }

      tape *= pcs->units[Q_LENGTH];
   }

   fNoComp = (comp == HUGE_REAL);
   if (!fNoComp) {
      comp *= pcs->units[Q_BEARING];
      if (comp < (real)0.0 || comp - PI * 2 > EPSILON) {
	 compile_warning(/*Suspicious compass reading*/59);
      }
   }

   if (!fPlumbed && !fNoClino) {
      clin *= pcs->units[Q_GRADIENT];
      if (fabs(clin) - PI / 2 > EPSILON) {
	 compile_warning(/*Clino reading over 90 degrees (absolute value)*/51);
      }
   }

   if (fTopofil) {
      tape = count * pcs->sc[Q_COUNT];
   } else {
      tape = (tape - pcs->z[Q_LENGTH]) * pcs->sc[Q_LENGTH];
   }

   /* adjusted tape is negative -- probably the calibration is wrong */
   if (tape < (real)0.0) {
      /* FIXME: different message for topofil? */
      compile_warning(/*Negative adjusted tape reading*/79);
   }

   if ((fPlumbed && clin != (real)0) ||
       (pcs->f90Up && (fabs(clin - PI / 2) < EPSILON))) {
      /* plumbed */
      if (!fNoComp) {
	 compile_warning(/*Compass reading given on plumbed leg*/21);
      }

      dx = dy = (real)0.0;
      dz = (clin > (real)0.0) ? tape : -tape;
      vx = vy = var(Q_POS) / 3.0 + dz * dz * var(Q_PLUMB);
      vz = var(Q_POS) / 3.0 + var(Q_LENGTH);
#ifndef NO_COVARIANCES
      cxy = cyz = czx = (real)0.0; /* FIXME: do this properly */
#endif
   } else {
      /* clino */
      real L2, cosG, LcosG, cosG2, sinB, cosB, dx2, dy2, dz2, v, V;
      if (fNoComp) {
	 compile_error(/*Compass reading may not be omitted here*/14);
	 showandskipline(NULL, 0);
	 return 0;
      }
      if (tape == (real)0.0) {
	 dx = dy = dz = (real)0.0;
	 vx = vy = vz = (real)(var(Q_POS) / 3.0); /* Position error only */
#ifndef NO_COVARIANCES
	 cxy = cyz = czx = (real)0.0;
#endif
#if DEBUG_DATAIN_1
	 printf("Zero length leg: vx = %f, vy = %f, vz = %f\n", vx, vy, vz);
#endif
      } else {
	 real sinGcosG;
	 comp = (comp - pcs->z[Q_BEARING]) * pcs->sc[Q_BEARING];
	 comp -= pcs->z[Q_DECLINATION];
         /* LEVEL case */
	 if (!fPlumbed && !fNoClino)
	    clin = (clin - pcs->z[Q_GRADIENT]) * pcs->sc[Q_GRADIENT];
/*
printf("fPlumbed %d fNoClino %d\n",fPlumbed,fNoClino);
printf("clin %.2f\n",clin);
*/

#if DEBUG_DATAIN
	 printf("    %4.2f %4.2f %4.2f\n", tape, comp, clin);
#endif
	 cosG = cos(clin);
	 LcosG = tape * cosG;
	 sinB = sin(comp);
	 cosB = cos(comp);
#if DEBUG_DATAIN_1
	 printf("sinB = %f, cosG = %f, LcosG = %f\n", sinB, cosG, LcosG);
#endif
	 dx = LcosG * sinB;
	 dy = LcosG * cosB;
	 dz = tape * sin(clin);
/*      printf("%.2f\n",clin); */
#if DEBUG_DATAIN_1
	 printf("dx = %f\ndy = %f\ndz = %f\n", dx, dy, dz);
#endif
	 dx2 = dx * dx;
	 L2 = tape * tape;
	 V = var(Q_LENGTH) / L2;
	 dy2 = dy * dy;
	 cosG2 = cosG * cosG;
	 sinGcosG = sin(clin) * cosG;
	 dz2 = dz * dz;
#ifdef NO_COVARIANCES
	 /* take into account variance in LEVEL case */
	 v = dz2 * var(fPlumbed ? Q_LEVEL : Q_GRADIENT);
	 vx = (var(Q_POS) / 3.0 + dx2 * V + dy2 * var(Q_BEARING) +
	       (.5 + sinB * sinB * cosG2) * v);
	 vy = (var(Q_POS) / 3.0 + dy2 * V + dx2 * var(Q_BEARING) +
	       (.5 + cosB * cosB * cosG2) * v);
	 if (!fNoClino)
	    vz = (var(Q_POS) / 3.0 + dz2 * V + L2 * cosG2 * var(Q_GRADIENT));
	 else
	    vz = (var(Q_POS) / 3.0 + L2 * (real)0.1);
	 /* if no clino, assume sd=tape/sqrt(10) so 3sds = .95*tape */
	 /* for Surveyor87 errors: vx=vy=vz=var(Q_POS)/3.0; */
#else
	 /* take into account variance in LEVEL case */
	 v = dz2 * var(fPlumbed ? Q_LEVEL : Q_GRADIENT);
	 vx = var(Q_POS) / 3.0 + dx2 * V + dy2 * var(Q_BEARING) +
	    (sinB * sinB * v);
	 vy = var(Q_POS) / 3.0 + dy2 * V + dx2 * var(Q_BEARING) +
	    (cosB * cosB * v);
	 if (!fNoClino)
	    vz = (var(Q_POS) / 3.0 + dz2 * V + L2 * cosG2 * var(Q_GRADIENT));
	 else
	    vz = (var(Q_POS) / 3.0 + L2 * (real)0.1);
	 /* if no clino, assume sd=tape/sqrt(10) so 3sds = .95*tape */
	 /* usual covariance formulae are fine in no clino case since
	  * dz = 0 so value of var(Q_GRADIENT) is ignored */
	 cxy = sinB * cosB * (var(Q_LENGTH) * cosG2 + var(Q_GRADIENT) * dz2)
	       - var(Q_BEARING) * dx * dy;
	 czx = var(Q_LENGTH) * sinB * sinGcosG - var(Q_GRADIENT) * dx * dz;
	 cyz = var(Q_LENGTH) * cosB * sinGcosG - var(Q_GRADIENT) * dy * dz;
#if 0
	 printf("vx = %6.3f, vy = %6.3f, vz = %6.3f\n", vx, vy, vz);
	 printf("cxy = %6.3f, cyz = %6.3f, czx = %6.3f\n", cxy, cyz, czx);
#endif
#if 0
	 cxy = cyz = czx = (real)0.0; /* for now */
#endif
#endif
#if DEBUG_DATAIN_1
	 printf("In DATAIN.C, vx = %f, vy = %f, vz = %f\n", vx, vy, vz);
#endif
      }
   }
#if DEBUG_DATAIN_1
   printf("Just before addleg, vx = %f\n", vx);
#endif
   /*printf("dx,dy,dz = %.2f %.2f %.2f\n\n", dx, dy, dz);*/
   addlegbyname(fr, to, fToFirst, dx, dy, dz, vx, vy, vz
#ifndef NO_COVARIANCES
		, cyz, czx, cxy
#endif
		);

#ifdef NEW3DFORMAT
   if (fUseNewFormat) {
      /* new twiglet and insert into twig tree */
      twig *twiglet = osnew(twig);
      twiglet->from = fr;
      twiglet->to = to;
      twiglet->down = twiglet->right = NULL;
      twiglet->source = twiglet->drawings
	= twiglet->date = twiglet->instruments = twiglet->tape = NULL;
      twiglet->up = limb->up;
      limb->right = twiglet;
      limb = twiglet;

      /* record pre-fettling deltas */
      twiglet->delta[0] = dx;
      twiglet->delta[1] = dy;
      twiglet->delta[2] = dz;
   }
#endif
   return 1;
}

/* tape/compass/clino and topofil (fromcount/tocount/compass/clino) */
extern int
data_normal(void)
{
   prefix *fr = NULL, *to = NULL;
   reading first_stn = End;

   real tape = 0, comp = 0, clin, frcount = 0, tocount = 0;
   bool fTopofil = fFalse, fMulti = fFalse; /* features of style... */
   bool fNoClino, fPlumbed;

   reading *ordering;

   again:

   fPlumbed = fFalse;
   
   /* ordering may omit clino reading, so set up default here */
   /* this is also used if clino reading is the omit character */
   clin = (real)0.0; /* no clino reading, so assume 0 with large sd */
   fNoClino = fTrue;

   for (ordering = pcs->ordering ; ; ordering++) {
      skipblanks();
      switch (*ordering) {
       case Fr:
	  fr = read_prefix_stn(fFalse);
	  if (first_stn == End) first_stn = Fr;
	  break;
       case To:
	  to = read_prefix_stn(fFalse);
	  if (first_stn == End) first_stn = To;
	  break;
       case Station:
	  fr = to;
	  to = read_prefix_stn(fFalse);
	  first_stn = To;
	  break;
       case Count:
	  frcount = tocount;
	  tocount = read_numeric(fFalse);
	  break;
       case Tape: tape = read_numeric(fFalse); break;
       case FrCount: frcount = read_numeric(fFalse); break;
       case ToCount: tocount = read_numeric(fFalse); fTopofil = fTrue; break;
       case Comp: {
	  comp = read_numeric(fTrue);
	  if (comp == HUGE_REAL) {
	     if (!isOmit(ch)) {
		compile_error(/*Expecting numeric field*/9);
		showandskipline(NULL, 1);
		return 0;
	     }
	     nextch();
	  }
	  break;
       }
       case Clino: {
	  clin = read_numeric(fTrue);
	  if (clin == HUGE_REAL) {
	     clin = handle_plumb(&fPlumbed);
	     if (clin != HUGE_REAL) break;
	     compile_error(/*Expecting numeric field*/9);
	     showandskipline(NULL, 1);
	     return 0;
	  }
	  /* we've got a real clino reading, so set clear the flag */
	  fNoClino = fFalse;
	  break;
       }
       case Ignore:
	 skipword(); break;
       case IgnoreAllAndNewLine:
	 skipline();
	 /* fall through */
       case Newline:
	 if (fr != NULL) {
	    int r;
	    r = process_normal(fr, to, tape, comp, clin, tocount - frcount,
			       first_stn == To, fNoClino, fTopofil, fPlumbed);
	    if (!r) skipline();
	 }
	 fMulti = fTrue;	    
	 while (1) {
	    process_eol();
	    process_bol();
	    if (isData(ch)) break;
	    if (!isComm(ch)) return 1;
	 }
	 break;
       case IgnoreAll:
	 skipline();
	 /* fall through */
       case End:
	 if (!fMulti) {
	    int r;
	    r = process_normal(fr, to, tape, comp, clin, tocount - frcount,
			       first_stn == To, fNoClino, fTopofil, fPlumbed);
	    process_eol();
	    return r;
	 }
	 while (1) {
	    process_eol();
	    process_bol();
	    if (isData(ch)) break;
	    if (!isComm(ch)) return 1;
	 }
	 goto again;
       default: BUG("Unknown reading in ordering");
      }
   }
}

extern int
data_diving(void)
{
   prefix *fr = NULL, *to = NULL;
   real dx, dy, dz;
   real vx, vy, vz;

   real tape = 0, comp = 0;
   real fr_depth = 0, to_depth = 0;

   reading first_stn = End;

   reading *ordering;

   for (ordering = pcs->ordering; ; ordering++) {
      skipblanks();
      switch (*ordering) {
       case Fr:
	  fr = read_prefix_stn(fFalse);
	  if (first_stn == End) first_stn = Fr;
	  break;
       case To:
	  to = read_prefix_stn(fFalse);
	  if (first_stn == End) first_stn = To;
	  break;
      case Tape: tape = read_numeric(fFalse); break;
      case Comp:
      	  comp = read_numeric(fTrue);
       	  if (comp == HUGE_REAL) {
	     if (!isOmit(ch)) {
		compile_error(/*Expecting numeric field*/9);
		showandskipline(NULL, 1);
		return 0;
	     }
	     nextch();
	  }
	  break;
      case FrDepth: fr_depth = read_numeric(fFalse); break;
      case ToDepth: to_depth = read_numeric(fFalse); break;
      case Ignore: skipword(); break;
      case IgnoreAll:
	 skipline();
	 /* drop through */
      case End:
	 goto dataread;
      default: BUG("Unknown reading in ordering");
      }
   }

   dataread:

   if (to == fr) {
      compile_error(/*Survey leg with same station (`%s') at both ends - typing error?*/50,
		    sprint_prefix(to));
      return 1;
   }
   
   if (tape < (real)0.0) {
      compile_warning(/*Negative tape reading*/60);
   }

   tape *= pcs->units[Q_LENGTH];
   if (comp != HUGE_REAL) {
      comp *= pcs->units[Q_BEARING];
      if (comp < (real)0.0 || comp - PI * 2 > EPSILON) {
         compile_warning(/*Suspicious compass reading*/59);
      }
   }

   tape = (tape - pcs->z[Q_LENGTH]) * pcs->sc[Q_LENGTH];
   /* assuming depths are negative */
   dz = (to_depth - fr_depth) * pcs->sc[Q_DEPTH];

   /* adjusted tape is negative -- probably the calibration is wrong */
   if (tape < (real)0.0) {
      compile_warning(/*Negative adjusted tape reading*/79);
   }

   /* check if tape is less than depth change */
   if (tape < fabs(dz)) {
      /* FIXME: allow margin of error based on variances? */
      compile_warning(/*Tape reading is less than change in depth*/62);
   }

   if (tape == (real)0.0) {
      dx = dy = dz = (real)0.0;
      vx = vy = vz = (real)(var(Q_POS) / 3.0); /* Position error only */
   } else if (comp == HUGE_REAL) {
      /* plumb */
      dx = dy = (real)0.0;
      if (dz < 0) tape = -tape;
      dz = (dz * var(Q_LENGTH) + tape * 2 * var(Q_DEPTH))
         / (var(Q_LENGTH) * 2 * var(Q_DEPTH));
      vx = vy = var(Q_POS) / 3.0 + dz * dz * var(Q_PLUMB);
      vz = var(Q_POS) / 3.0 + var(Q_LENGTH) * 2 * var(Q_DEPTH)
                              / (var(Q_LENGTH) + var(Q_DEPTH));
   } else {
      real L2, sinB, cosB, dx2, dy2, dz2, v, V, D2;
      comp = (comp - pcs->z[Q_BEARING]) * pcs->sc[Q_BEARING];
      comp -= pcs->z[Q_DECLINATION];
      sinB = sin(comp);
      cosB = cos(comp);
      L2 = tape * tape;
      dz2 = dz * dz;
      D2 = L2 - dz2;
      if (D2 <= (real)0.0) {
	 dx = dy = (real)0.0;
      } else {
	 real D = sqrt(D2);
	 dx = D * sinB;
	 dy = D * cosB;
      }
      V = 2 * dz2 + L2;
      if (D2 <= V) {
	 vx = vy = var(Q_POS) / 3.0 + V;
      } else {
	 dx2 = dx * dx;
	 dy2 = dy * dy;
	 V = var(Q_LENGTH) / L2;
	 v = dz2 * var(Q_DEPTH) / D2;
	 vx = var(Q_POS) / 3.0 + dx2 * V + dy2 * var(Q_BEARING) +
	    sinB * sinB * v;
	 vy = var(Q_POS) / 3.0 + dy2 * V + dx2 * var(Q_BEARING) +
	    cosB * cosB * v;
      }
      vz = var(Q_POS) / 3.0 + 2 * var(Q_DEPTH);
   }
   addlegbyname(fr, to, (first_stn == To), dx, dy, dz, vx, vy, vz
#ifndef NO_COVARIANCES
		, 0, 0, 0 /* FIXME: need covariances */
#endif
		);
#ifdef NEW3DFORMAT
   if (fUseNewFormat) {
      /*new twiglet and insert into twig tree*/
      twig *twiglet = osnew(twig);
      twiglet->from = fr;
      twiglet->to = to;
      twiglet->down = twiglet->right = NULL;
      twiglet->source = twiglet->drawings
	= twiglet->date = twiglet->instruments = twiglet->tape = NULL;
      twiglet->up = limb->up;
      limb->right = twiglet;
      limb = twiglet;

      /* record pre-fettling deltas */
      twiglet->delta[0] = dx;
      twiglet->delta[1] = dy;
      twiglet->delta[2] = dz;
   }
#endif

   return 1;
}

extern int
data_cartesian(void)
{
   prefix *fr = NULL, *to = NULL;
   real dx = 0, dy = 0, dz = 0;

   reading first_stn = End;

   reading *ordering;

   for (ordering = pcs->ordering ; ; ordering++) {
      skipblanks();
      switch (*ordering) {
       case Fr:
	  fr = read_prefix_stn(fFalse);
	  if (first_stn == End) first_stn = Fr;
	  break;
       case To:
	  to = read_prefix_stn(fFalse);
	  if (first_stn == End) first_stn = To;
	  break;
       case Dx: dx = read_numeric(fFalse); break;
       case Dy: dy = read_numeric(fFalse); break;
       case Dz: dz = read_numeric(fFalse); break;
       case Ignore:
	 skipword(); break;
       case IgnoreAll:
	 skipline();
	 /* fall through */
       case End:
	 goto dataread;
       default: BUG("Unknown reading in ordering");
      }
   }

   dataread:

   if (to == fr) {
      compile_error(/*Survey leg with same station (`%s') at both ends - typing error?*/50,
		    sprint_prefix(to));
      return 1;
   }

   dx = (dx * pcs->units[Q_DX] - pcs->z[Q_DX]) * pcs->sc[Q_DX];
   dy = (dy * pcs->units[Q_DY] - pcs->z[Q_DY]) * pcs->sc[Q_DY];
   dz = (dz * pcs->units[Q_DZ] - pcs->z[Q_DZ]) * pcs->sc[Q_DZ];

   addlegbyname(fr, to, (first_stn == To), dx, dy, dz,
		var(Q_DX), var(Q_DY), var(Q_DZ)
#ifndef NO_COVARIANCES
		, 0, 0, 0 /* FIXME: need covariances */
#endif
		);

#ifdef NEW3DFORMAT
   if (fUseNewFormat) {
      /* new twiglet and insert into twig tree */
      twig *twiglet = osnew(twig);
      twiglet->from = fr;
      twiglet->to = to;
      twiglet->down = twiglet->right = NULL;
      twiglet->source = twiglet->drawings
	= twiglet->date = twiglet->instruments = twiglet->tape = NULL;
      twiglet->up = limb->up;
      limb->right = twiglet;
      limb = twiglet;

      /* record pre-fettling deltas */
      twiglet->delta[0] = dx;
      twiglet->delta[1] = dy;
      twiglet->delta[2] = dz;
   }
#endif

   return 1;
}

extern int
data_nosurvey(void)
{
   prefix *fr = NULL, *to = NULL;
   nosurveylink *link;

   reading first_stn = End;

   reading *ordering;

   for (ordering = pcs->ordering ; ; ordering++) {
      skipblanks();
      switch (*ordering) {
       case Fr:
	  fr = read_prefix_stn(fFalse);
	  if (first_stn == End) first_stn = Fr;
	  break;
       case To:
	  to = read_prefix_stn(fFalse);
	  if (first_stn == End) first_stn = To;
	  break;
       case Ignore:
	 skipword(); break;
       case IgnoreAll:
	 skipline();
	 /* fall through */
       case End:
	 goto dataread;
       default: BUG("Unknown reading in ordering");
      }
   }

   dataread:

   if (to == fr) {
      compile_error(/*Survey leg with same station (`%s') at both ends - typing error?*/50,
		    sprint_prefix(to));
      return 1;
   }
   
#ifdef NEW3DFORMAT
   if (fUseNewFormat) {
      /* new twiglet and insert into twig tree */
      twig *twiglet = osnew(twig);
      twiglet->from = fr;
      twiglet->to = to;
      twiglet->down = twiglet->right = NULL;
      twiglet->source = twiglet->drawings
	= twiglet->date = twiglet->instruments = twiglet->tape = NULL;
      twiglet->up = limb->up;
      limb->right = twiglet;
      limb = twiglet;
      /* FIXME: check with PhilU what to do here... */
      twiglet->delta[0] = twiglet->delta[1] = twiglet->delta[2] = 0;
   }
#endif

   /* add to linked list which is dealt with after network is solved */
   link = osnew(nosurveylink);
   if (first_stn == To) {
      link->to = StnFromPfx(to);
      link->fr = StnFromPfx(fr);
   } else {
      link->fr = StnFromPfx(fr);
      link->to = StnFromPfx(to);
   }
   link->next = nosurveyhead;
   nosurveyhead = link;
   return 1;
}

/* totally ignore a line of survey data */
extern int
data_ignore(void)
{
   skipline();
   return 1;
}
