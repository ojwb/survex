/* datain.c
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
#include "str.h"

#define EPSILON (REAL_EPSILON * 1000)

#define MAX_KEYWORD_LEN 16

#define PRINT_COMMENT 0

#define var(I) (pcs->Var[(I)])

int ch;

/* Don't explicitly initialise as we can't set the jmp_buf - this has
 * static scope so will be initialised like this anyway */
parse file /* = { NULL, NULL, 0, fFalse, NULL } */ ;

bool f_export_ok;

static filepos fpLineStart;

void
get_pos(filepos *fp)
{
   fp->ch = ch;
   fp->offset = ftell(file.fh);
   if (fp->offset == -1)
      fatalerror_in_file(file.filename, 0, /*Error reading file*/18);
}

void
set_pos(const filepos *fp)
{
   ch = fp->ch;
   if (fseek(file.fh, fp->offset, SEEK_SET) == -1)
      fatalerror_in_file(file.filename, 0, /*Error reading file*/18);
}

static void
push_back(int c)
{
   if (c != EOF && ungetc(c, file.fh) == EOF)
      fatalerror_in_file(file.filename, 0, /*Error reading file*/18);
}

static void
error_list_parent_files(void)
{
   if (!file.reported_where && file.parent) {
      parse *p = file.parent;      
      const char *m = msg(/*In file included from */5);
      size_t len = strlen(m);

      fprintf(STDERR, m);
      m = msg(/*from */3);

      /* Suppress reporting of full include tree for further errors
       * in this file */
      file.reported_where = fTrue;

      while (p) {
	 /* Force re-report of include tree for further errors in
	  * parent files */
	 p->reported_where = fFalse;
	 fprintf(STDERR, "%s:%d", p->filename, p->line);
	 p = p->parent;
	 if (p) fprintf(STDERR, ",\n%*s", (int)len, m);
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
compile_error_token(int en)
{
   char *p = NULL;
   static int len;
   s_zero(&p);
   skipblanks();
   while (!isBlank(ch) && !isEol(ch)) {
      s_catchar(&p, &len, ch);
      nextch();
   }
   compile_error(en, p ? p : "");
   osfree(p);
   skipline();
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
skipline(void)
{
   while (!isEol(ch)) nextch();
}

#ifndef NO_PERCENTAGE
static long int filelen;
#endif

static void
process_bol(void)
{
   /* Note start of line for error reporting */
   get_pos(&fpLineStart);

#ifndef NO_PERCENTAGE
   /* print %age of file done */
   if (filelen > 0)
      printf("%d%%\r", (int)(100 * fpLineStart.offset / filelen));
#endif

   nextch();
   skipblanks();
}

static void
process_eol(void)
{
   int eolchar;

   skipblanks();

   if (!isEol(ch)) {
      if (!isComm(ch)) compile_error(/*End of line not blank*/15);
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
	 push_back(ch);
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
      FILE *fh;
      if (!pth) {
	 /* file specified on command line - don't do special translation */
	 fh = fopenWithPthAndExt(pth, fnm, EXT_SVX_DATA, "rb", &filename);
      } else {
	 fh = fopen_portable(pth, fnm, EXT_SVX_DATA, "rb", &filename);
      }

      if (fh == NULL) {
	 compile_error(/*Couldn't open data file `%s'*/24, fnm);
	 return;
      }

      file_store = file;
      if (file.fh) file.parent = &file_store;
      file.fh = fh;
      file.filename = filename;
      file.line = 1;
      file.reported_where = fFalse;
   }

   if (fPercent) printf("%s:\n", fnm);

   using_data_file(file.filename);

   begin_lineno_store = pcs->begin_lineno;
   pcs->begin_lineno = 0;

#ifndef NO_PERCENTAGE
   /* Try to find how long the file is...
    * However, under ANSI fseek( ..., SEEK_END) may not be supported */
   filelen = 0;
   if (fPercent) {
      if (fseek(file.fh, 0l, SEEK_END) == 0) {
	 filepos fp;
	 get_pos(&fp);
	 filelen = fp.offset;
      }
      rewind(file.fh); /* reset file ptr to start & clear any error state */
   }
#endif

#ifdef HAVE_SETJMP_H
   /* errors in nested functions can longjmp here */
   if (setjmp(file.jbSkipLine)) {
      process_eol();
   }
#endif

   while (!feof(file.fh) && !ferror(file.fh)) {
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
      /* Implicitly close any unclosed BEGINs from this file */
      do {
	 settings *pcsParent = pcs->next;
	 ASSERT(pcsParent);
	 free_settings(pcs);
	 pcs = pcsParent;
      } while (pcs->begin_lineno);
   }

   pcs->begin_lineno = begin_lineno_store;

   if (ferror(file.fh))
      fatalerror_in_file(file.filename, 0, /*Error reading file*/18);

   (void)fclose(file.fh);

   file = file_store;

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
   static real clinos[] = {(real)(M_PI/2.0),(real)(-M_PI/2.0),(real)0.0};
   clino_tok tok;

   skipblanks();
   if (isalpha(ch)) {
      filepos fp;
      get_pos(&fp);
      get_token();
      tok = match_tok(clino_tab, TABSIZE(clino_tab));
      if (tok != CLINO_NULL) {
	 *pfPlumbed = fTrue;
	 return clinos[tok];
      }
      set_pos(&fp);
   } else if (isSign(ch)) {
      int chOld = ch;
      nextch();
      if (toupper(ch) == 'V') {
	 nextch();
	 *pfPlumbed = fTrue;
	 return (!isMinus(chOld) ? M_PI / 2.0 : -M_PI / 2.0);
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

   if (pcs->f0Eq && tape == (real)0.0) {
      process_equate(fr, to);
      return 1;
   }

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
      if (comp < (real)0.0 || comp - M_PI * 2 > EPSILON) {
	 compile_warning(/*Suspicious compass reading*/59);
      }
   }

   if (!fPlumbed && !fNoClino) {
      clin *= pcs->units[Q_GRADIENT];
      if (fabs(clin) - M_PI / 2 > EPSILON) {
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
      /* TRANSLATE different message for topofil? */
      compile_warning(/*Negative adjusted tape reading*/79);
   }

   if ((fPlumbed && clin != (real)0) ||
       (pcs->f90Up && (fabs(clin - M_PI / 2) < EPSILON))) {
      /* plumbed */
      if (!fNoComp) {
	 compile_warning(/*Compass reading given on plumbed leg*/21);
      }

      dx = dy = (real)0.0;
      dz = (clin > (real)0.0) ? tape : -tape;
      vx = vy = var(Q_POS) / 3.0 + dz * dz * var(Q_PLUMB);
      vz = var(Q_POS) / 3.0 + var(Q_LENGTH);
#ifndef NO_COVARIANCES
      /* Correct values - no covariances in this case! */
      cxy = cyz = czx = (real)0.0;
#endif
   } else {
      /* clino */
      real L2, cosG, LcosG, cosG2, sinB, cosB, dx2, dy2, dz2, v, V;
      if (fNoComp) {	 
	 compile_error(/*Compass reading may not be omitted except on plumbed legs*/14);
	 skipline();
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
   bool fNoClino, fPlumbed, fRev;

   reading *ordering;

   again:

   fPlumbed = fFalse;
   fRev = fFalse;

   /* ordering may omit clino reading, so set up default here */
   /* this is also used if clino reading is the omit character */
   clin = (real)0.0; /* no clino reading, so assume 0 with large sd */
   fNoClino = fTrue;

   for (ordering = pcs->ordering ; ; ordering++) {
      skipblanks();
      switch (*ordering) {
       case Fr:
	  fr = read_prefix_stn(fFalse, fTrue);
	  if (first_stn != End) first_stn = Fr;
	  break;
       case To:
	  to = read_prefix_stn(fFalse, fTrue);
	  if (first_stn != End) first_stn = To;
	  break;
       case Station:
	  fr = to;
	  to = read_prefix_stn(fFalse, fFalse);
	  first_stn = To;
	  break;
       case Dir:
	  switch(toupper(ch)) {
	   case 'F':
	     break;
	   case 'B':
	     fRev = fTrue;
	     break;
	   default:
	     compile_error(/*Found `%c', expecting `F' or `B'*/131, ch);
	     skipline();
	     process_eol();
	     return 0;
	  }
	  break;
       case Count:
	  frcount = tocount;
	  tocount = read_numeric(fFalse);
	  break;
       case Tape: tape = read_numeric(fFalse); break;
       case FrCount: frcount = read_numeric(fFalse); break;
       case ToCount: tocount = read_numeric(fFalse); fTopofil = fTrue; break;
       case Comp: comp = read_numeric_or_omit(); break;
       case Clino: {
	  clin = read_numeric(fTrue);
	  if (clin == HUGE_REAL) {
	     clin = handle_plumb(&fPlumbed);
	     if (clin != HUGE_REAL) break;
	     compile_error_token(/*Expecting numeric field, found `%s'*/9);
	     process_eol();
	     return 0;
	  }
	  /* we've got a real clino reading, so clear the flag */
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
	    if (fRev) {
	       prefix *t = fr;
	       fr = to;
	       to = t;
	    }
	    r = process_normal(fr, to, tape, comp, clin, tocount - frcount,
			       (first_stn == To) ^ fRev, fNoClino, fTopofil,
			       fPlumbed);
	    if (!r) skipline();
	 }
	 fMulti = fTrue;	    
	 while (1) {
	    process_eol();
	    process_bol();
	    if (isData(ch)) break;
	    if (!isComm(ch)) {
	       push_back(ch);
	       return 1;
	    }
	 }
	 break;
       case IgnoreAll:
	 skipline();
	 /* fall through */
       case End:
	 if (!fMulti) {
	    int r;
	    if (fRev) {
	       prefix *t = fr;
	       fr = to;
	       to = t;
	    }
	    r = process_normal(fr, to, tape, comp, clin, tocount - frcount,
			       (first_stn == To) ^ fRev, fNoClino, fTopofil,
			       fPlumbed);
	    process_eol();
	    return r;
	 }
	 do {
	    process_eol();
	    process_bol();
	 } while (isComm(ch));
	 goto again;
       default: BUG("Unknown reading in ordering");
      }
   }
}

static int
process_diving(prefix *fr, prefix *to, real tape, real comp,
	       real frdepth, real todepth, bool fToFirst)
{
   real dx, dy, dz;
   real vx, vy, vz;
#ifndef NO_COVARIANCES
   real cxy = 0, cyz = 0, czx = 0;
#endif

   if (pcs->f0Eq && tape == (real)0.0 && frdepth == todepth) {
      process_equate(fr, to);
      return 1;
   }
   
   if (tape < (real)0.0) {
      compile_warning(/*Negative tape reading*/60);
   }

   tape *= pcs->units[Q_LENGTH];
   if (comp != HUGE_REAL) {
      comp *= pcs->units[Q_BEARING];
      if (comp < (real)0.0 || comp - M_PI * 2 > EPSILON) {
         compile_warning(/*Suspicious compass reading*/59);
      }
   }

   tape = (tape - pcs->z[Q_LENGTH]) * pcs->sc[Q_LENGTH];
   /* assuming depths are negative */
   dz = (todepth - frdepth) * pcs->sc[Q_DEPTH];

   /* adjusted tape is negative -- probably the calibration is wrong */
   if (tape < (real)0.0) {
      compile_warning(/*Negative adjusted tape reading*/79);
   }

   /* check if tape is less than depth change */
   if (tape < fabs(dz)) {
      /* FIXME: allow margin of error based on variances? */
      compile_warning(/*Tape reading is less than change in depth*/62);
   }

   if (tape == (real)0.0 && dz == 0.0) {
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
      real L2, sinB, cosB, dz2, D2;
      comp = (comp - pcs->z[Q_BEARING]) * pcs->sc[Q_BEARING];
      comp -= pcs->z[Q_DECLINATION];
      sinB = sin(comp);
      cosB = cos(comp);
      L2 = tape * tape;
      dz2 = dz * dz;
      D2 = L2 - dz2;
      if (D2 <= (real)0.0) {
	 real vsum = var(Q_LENGTH) + 2 * var(Q_DEPTH);
	 dx = dy = (real)0.0;
	 vx = vy = var(Q_POS) / 3.0;
	 vz = var(Q_POS) / 3.0 + var(Q_LENGTH) * 2 * var(Q_DEPTH) / vsum;
	 if (dz > 0) {
	    dz = (dz * var(Q_LENGTH) + tape * 2 * var(Q_DEPTH)) / vsum;
	 } else {
	    dz = (dz * var(Q_LENGTH) - tape * 2 * var(Q_DEPTH)) / vsum;
	 }
      } else {
	 real D = sqrt(D2);
	 real F = var(Q_LENGTH) * L2 + 2 * var(Q_DEPTH) * D2;
	 dx = D * sinB;
	 dy = D * cosB;

	 vx = var(Q_POS) / 3.0 +
	    sinB * sinB * F / D2 + var(Q_BEARING) * dy * dy;
	 vy = var(Q_POS) / 3.0 +
	    cosB * cosB * F / D2 + var(Q_BEARING) * dx * dx;
	 vz = var(Q_POS) / 3.0 + 2 * var(Q_DEPTH);

#ifndef NO_COVARIANCES
	 cxy = sinB * cosB * (F / D2 + var(Q_BEARING) * D2);
	 cyz = -2 * var(Q_DEPTH) * dy / D;
	 czx = -2 * var(Q_DEPTH) * dx / D;
#endif
      }
   }
   addlegbyname(fr, to, fToFirst, dx, dy, dz, vx, vy, vz
#ifndef NO_COVARIANCES
		, cxy, cyz, czx
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
data_diving(void)
{
   prefix *fr = NULL, *to = NULL;

   real tape = 0, comp = 0;
   real frdepth = 0, todepth = 0;

   bool fMulti = fFalse;
   bool fRev;

   reading first_stn = End;

   reading *ordering;

   again:

   fRev = fFalse;

   for (ordering = pcs->ordering; ; ordering++) {
      skipblanks();
      switch (*ordering) {
       case Fr:
	 fr = read_prefix_stn(fFalse, fTrue);
	 if (first_stn == End) first_stn = Fr;
	 break;
       case To:
	 to = read_prefix_stn(fFalse, fTrue);
	 if (first_stn == End) first_stn = To;
	 break;
       case Station:
	 fr = to;
	 to = read_prefix_stn(fFalse, fFalse);
	 first_stn = To;
	 break;
       case Dir:
	  switch(toupper(ch)) {
	   case 'F':
	     break;
	   case 'B':
	     fRev = fTrue;
	     break;
	   default:
	     compile_error(/*Found `%c', expecting `F' or `B'*/131, ch);
	     skipline();
	     process_eol();
	     return 0;
	  }
	  break;
       case Tape: tape = read_numeric(fFalse); break;
       case Comp: comp = read_numeric_or_omit(); break;
       case FrDepth: frdepth = read_numeric(fFalse); break;
       case ToDepth: todepth = read_numeric(fFalse); break;
       case Depth:
	  frdepth = todepth;
	  todepth = read_numeric(fFalse);
	  break;
       case Dz:
	  frdepth = 0;
	  todepth = read_numeric(fFalse);
	  break;
       case Ignore: skipword(); break;
       case IgnoreAllAndNewLine:
	 skipline();
	 /* fall through */
       case Newline:
	 if (fr != NULL) {
	    int r;
	    if (fRev) {
	       prefix *t = fr;
	       fr = to;
	       to = t;
	    }
	    r = process_diving(fr, to, tape, comp, frdepth, todepth,
			       (first_stn == To) ^ fRev);
	    if (!r) skipline();
	 }
	 fMulti = fTrue;
	 while (1) {
	    process_eol();
	    process_bol();
	    if (isData(ch)) break;
	    if (!isComm(ch)) {
	       push_back(ch);
	       return 1;
	    }
	 }
	 break;
       case IgnoreAll:
	 skipline();
	 /* fall through */
       case End:
	 if (!fMulti) {
	    int r;
	    if (fRev) {
	       prefix *t = fr;
	       fr = to;
	       to = t;
	    }
	    r = process_diving(fr, to, tape, comp, frdepth, todepth,
			       (first_stn == To) ^ fRev);
	    process_eol();
	    return r;
	 }
	 do {
	    process_eol();
	    process_bol();
	 } while (isComm(ch));
	 goto again;
      default: BUG("Unknown reading in ordering");
      }
   }
}

static int
process_cartesian(prefix *fr, prefix *to, real dx, real dy, real dz,
		  bool fToFirst)
{
   dx = (dx * pcs->units[Q_DX] - pcs->z[Q_DX]) * pcs->sc[Q_DX];
   dy = (dy * pcs->units[Q_DY] - pcs->z[Q_DY]) * pcs->sc[Q_DY];
   dz = (dz * pcs->units[Q_DZ] - pcs->z[Q_DZ]) * pcs->sc[Q_DZ];

   addlegbyname(fr, to, fToFirst, dx, dy, dz, var(Q_DX), var(Q_DY), var(Q_DZ)
#ifndef NO_COVARIANCES
		, 0, 0, 0
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
data_cartesian(void)
{
   prefix *fr = NULL, *to = NULL;
   real dx = 0, dy = 0, dz = 0;

   bool fMulti = fFalse;

   reading first_stn = End;

   reading *ordering;

   again:

   for (ordering = pcs->ordering ; ; ordering++) {
      skipblanks();
      switch (*ordering) {
       case Fr:
	  fr = read_prefix_stn(fFalse, fTrue);
	  if (first_stn == End) first_stn = Fr;
	  break;
       case To:
	  to = read_prefix_stn(fFalse, fTrue);
	  if (first_stn == End) first_stn = To;
	  break;
       case Station:
	  fr = to;
	  to = read_prefix_stn(fFalse, fFalse);
	  first_stn = To;
	  break;
       case Dx: dx = read_numeric(fFalse); break;
       case Dy: dy = read_numeric(fFalse); break;
       case Dz: dz = read_numeric(fFalse); break;
       case Ignore:
	 skipword(); break;
       case IgnoreAllAndNewLine:
	 skipline();
	 /* fall through */
       case Newline:
	 if (fr != NULL) {
	    int r;
	    r = process_cartesian(fr, to, dx, dy, dz, first_stn == To);
	    if (!r) skipline();
	 }
	 fMulti = fTrue;
	 while (1) {
	    process_eol();
	    process_bol();
	    if (isData(ch)) break;
	    if (!isComm(ch)) {
	       push_back(ch);
	       return 1;
	    }
	 }
	 break;
       case IgnoreAll:
	 skipline();
	 /* fall through */
       case End:
	 if (!fMulti) {
	    int r = process_cartesian(fr, to, dx, dy, dz, first_stn == To);
	    process_eol();
	    return r;
	 }
	 do {
	    process_eol();
	    process_bol();
	 } while (isComm(ch));
	 goto again;
       default: BUG("Unknown reading in ordering");
      }
   }
}

static int
process_cylpolar(prefix *fr, prefix *to, real tape, real comp,
		 real frdepth, real todepth, bool fToFirst)
{
   real dx, dy, dz;
   real vx, vy, vz;
#ifndef NO_COVARIANCES
   real cxy = 0;
#endif

   if (pcs->f0Eq && tape == (real)0.0 && frdepth == todepth) {
      process_equate(fr, to);
      return 1;
   }
   
   if (tape < (real)0.0) {
      compile_warning(/*Negative tape reading*/60);
   }

   tape *= pcs->units[Q_LENGTH];
   if (comp != HUGE_REAL) {
      comp *= pcs->units[Q_BEARING];
      if (comp < (real)0.0 || comp - M_PI * 2 > EPSILON) {
         compile_warning(/*Suspicious compass reading*/59);
      }
   }

   tape = (tape - pcs->z[Q_LENGTH]) * pcs->sc[Q_LENGTH];
   /* assuming depths are negative */
   dz = (todepth - frdepth) * pcs->sc[Q_DEPTH];

   /* adjusted tape is negative -- probably the calibration is wrong */
   if (tape < (real)0.0) {
      compile_warning(/*Negative adjusted tape reading*/79);
   }

   if (comp == HUGE_REAL) {
      /* plumb */
      dx = dy = (real)0.0;
      vx = vy = var(Q_POS) / 3.0 + dz * dz * var(Q_PLUMB);
      vz = var(Q_POS) / 3.0 + 2 * var(Q_DEPTH);
   } else {
      real sinB, cosB;
      comp = (comp - pcs->z[Q_BEARING]) * pcs->sc[Q_BEARING];
      comp -= pcs->z[Q_DECLINATION];

      sinB = sin(comp);
      cosB = cos(comp);

      dx = tape * sinB;
      dy = tape * cosB;

      vx = var(Q_POS) / 3.0 +
	 var(Q_LENGTH) * sinB * sinB + var(Q_BEARING) * dy * dy;
      vy = var(Q_POS) / 3.0 +
	 var(Q_LENGTH) * cosB * cosB + var(Q_BEARING) * dx * dx;
      vz = var(Q_POS) / 3.0 + 2 * var(Q_DEPTH);

#ifndef NO_COVARIANCES
      cxy = (var(Q_LENGTH) - var(Q_BEARING) * tape * tape) * sinB * cosB;
#endif
   }
   addlegbyname(fr, to, fToFirst, dx, dy, dz, vx, vy, vz
#ifndef NO_COVARIANCES
		, cxy, 0, 0
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
data_cylpolar(void)
{
   prefix *fr = NULL, *to = NULL;

   real tape = 0, comp = 0;
   real frdepth = 0, todepth = 0;

   bool fMulti = fFalse;
   bool fRev;

   reading first_stn = End;

   reading *ordering;

   again:

   fRev = fFalse;

   for (ordering = pcs->ordering; ; ordering++) {
      skipblanks();
      switch (*ordering) {
       case Fr:
	 fr = read_prefix_stn(fFalse, fTrue);
	 if (first_stn == End) first_stn = Fr;
	 break;
       case To:
	 to = read_prefix_stn(fFalse, fTrue);
	 if (first_stn == End) first_stn = To;
	 break;
       case Station:
	 fr = to;
	 to = read_prefix_stn(fFalse, fFalse);
	 first_stn = To;
	 break;
       case Dir:
	  switch(toupper(ch)) {
	   case 'F':
	     break;
	   case 'B':
	     fRev = fTrue;
	     break;
	   default:
	     compile_error(/*Found `%c', expecting `F' or `B'*/131, ch);
	     skipline();
	     process_eol();
	     return 0;
	  }
	  break;
       case Tape: tape = read_numeric(fFalse); break;
       case Comp: comp = read_numeric_or_omit(); break;
       case FrDepth: frdepth = read_numeric(fFalse); break;
       case ToDepth: todepth = read_numeric(fFalse); break;
       case Depth:
	  frdepth = todepth;
	  todepth = read_numeric(fFalse);
	  break;
       case Dz:
	  frdepth = 0;
	  todepth = read_numeric(fFalse);
	  break;
       case Ignore: skipword(); break;
       case IgnoreAllAndNewLine:
	 skipline();
	 /* fall through */
       case Newline:
	 if (fr != NULL) {
	    int r;
	    if (fRev) {
	       prefix *t = fr;
	       fr = to;
	       to = t;
	    }
	    r = process_cylpolar(fr, to, tape, comp, frdepth, todepth,
				 (first_stn == To) ^ fRev);
	    if (!r) skipline();
	 }
	 fMulti = fTrue;
	 while (1) {
	    process_eol();
	    process_bol();
	    if (isData(ch)) break;
	    if (!isComm(ch)) {
	       push_back(ch);
	       return 1;
	    }
	 }
	 break;
       case IgnoreAll:
	 skipline();
	 /* fall through */
       case End:
	 if (!fMulti) {
	    int r;
	    if (fRev) {
	       prefix *t = fr;
	       fr = to;
	       to = t;
	    }
	    r = process_cylpolar(fr, to, tape, comp, frdepth, todepth,
				 (first_stn == To) ^ fRev);
	    process_eol();
	    return r;
	 }
	 do {
	    process_eol();
	    process_bol();
	 } while (isComm(ch));
	 goto again;
      default: BUG("Unknown reading in ordering");
      }
   }
}

static int
process_nosurvey(prefix *fr, prefix *to, bool fToFirst)
{
   nosurveylink *link;
   int shape;

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
      /* delta is only used to calculate error - pass zero and cope
       * elsewhere */
      twiglet->delta[0] = twiglet->delta[1] = twiglet->delta[2] = 0;
   }
#endif

   /* Suppress "unused fixed point" warnings for these stations
    * We do this if it's a 0 or 1 node - 1 node might be an sdfix
    */
   shape = fr->shape;
   if (shape == 0 || shape == 1) fr->shape = -1 - shape;
   shape = to->shape;
   if (shape == 0 || shape == 1) to->shape = -1 - shape;

   /* add to linked list which is dealt with after network is solved */
   link = osnew(nosurveylink);
   if (fToFirst) {
      link->to = StnFromPfx(to);
      link->fr = StnFromPfx(fr);
   } else {
      link->fr = StnFromPfx(fr);
      link->to = StnFromPfx(to);
   }
   link->flags = pcs->flags;
   link->next = nosurveyhead;
   nosurveyhead = link;
   return 1;
}

extern int
data_nosurvey(void)
{
   prefix *fr = NULL, *to = NULL;

   bool fMulti = fFalse;

   reading first_stn = End;

   reading *ordering;

   again:

   for (ordering = pcs->ordering ; ; ordering++) {
      skipblanks();
      switch (*ordering) {
       case Fr:
	  fr = read_prefix_stn(fFalse, fTrue);
	  if (first_stn == End) first_stn = Fr;
	  break;
       case To:
	  to = read_prefix_stn(fFalse, fTrue);
	  if (first_stn == End) first_stn = To;
	  break;
       case Station:
	  fr = to;
	  to = read_prefix_stn(fFalse, fFalse);
	  first_stn = To;
	  break;
       case Ignore:
	 skipword(); break;
       case IgnoreAllAndNewLine:
	 skipline();
	 /* fall through */
       case Newline:
	 if (fr != NULL) {
	    int r;
	    r = process_nosurvey(fr, to, first_stn == To);
	    if (!r) skipline();
	 }
	 if (ordering[1] == End) {
	    do {
	       process_eol();
	       process_bol();
	    } while (isComm(ch));
	    if (!isData(ch)) {
	       push_back(ch);
	       return 1;
	    }
	    goto again;
	 }
	 fMulti = fTrue;
	 while (1) {
	    process_eol();
	    process_bol();
	    if (isData(ch)) break;
	    if (!isComm(ch)) {
	       push_back(ch);
	       return 1;
	    }
	 }
	 break;
       case IgnoreAll:
	 skipline();
	 /* fall through */
       case End:
	 if (!fMulti) {
	    int r = process_nosurvey(fr, to, first_stn == To);
	    process_eol();
	    return r;
	 }
	 do {
	    process_eol();
	    process_bol();
	 } while (isComm(ch));
	 goto again;
       default: BUG("Unknown reading in ordering");
      }
   }
}

/* totally ignore a line of survey data */
extern int
data_ignore(void)
{
   skipline();
   process_eol();
   return 1;
}
