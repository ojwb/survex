/* datain.c
 * Reads in survey files, dealing with special characters, keywords & data
 * Copyright (C) 1991-2002 Olly Betts
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

#define var(I) (pcs->Var[(I)])

int ch;

typedef enum {CTYPE_OMIT, CTYPE_READING, CTYPE_PLUMB, CTYPE_HORIZ} clino_type;

/* Don't explicitly initialise as we can't set the jmp_buf - this has
 * static scope so will be initialised like this anyway */
parse file /* = { NULL, NULL, 0, fFalse, NULL } */ ;

bool f_export_ok;

static real value[Fr - 1];
#define VAL(N) value[(N)-1]
static real variance[Fr - 1];
#define VAR(N) variance[(N)-1]

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
      const char *m = msg(/*In file included from*/5);
      size_t len = strlen(m);

      fprintf(STDERR, m);
      m = msg(/*from*/3);

      /* Suppress reporting of full include tree for further errors
       * in this file */
      file.reported_where = fTrue;

      while (p) {
	 /* Force re-report of include tree for further errors in
	  * parent files */
	 p->reported_where = fFalse;
	 fprintf(STDERR, " %s:%d", p->filename, p->line);
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
compile_error_skip(int en, ...)
{
   va_list ap;
   va_start(ap, en);
   error_list_parent_files();
   v_report(1, file.filename, file.line, en, ap);
   va_end(ap);
   skipline();
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
   compile_error_skip(en, p ? p : "");
   osfree(p);
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

static void
read_reading(reading r, bool f_optional)
{
   int n_readings;
   q_quantity q;
   VAL(r) = read_numeric(f_optional, &n_readings);
   switch (r) {
      case Tape: q = Q_LENGTH; break;
      case Comp: q = Q_BEARING; break;
      case BackComp: q = Q_BACKBEARING; break;
      case Clino: q = Q_GRADIENT; break;
      case BackClino: q = Q_BACKGRADIENT; break;
      case FrDepth: case ToDepth: q = Q_DEPTH; break;
      case Dx: q = Q_DX; break;
      case Dy: q = Q_DY; break;
      case Dz: q = Q_DZ; break;
      case FrCount: case ToCount: q = Q_COUNT; break;
      default: BUG("Unexpected case");
   }
   VAR(r) = var(q);
   if (n_readings > 1) VAR(r) /= sqrt(n_readings);
}

static void
read_bearing_or_omit(reading r)
{
   int n_readings;
   q_quantity q;
   VAL(r) = read_numeric_or_omit(&n_readings);
   switch (r) {
      case Comp: q = Q_BEARING; break;
      case BackComp: q = Q_BACKBEARING; break;
      default: BUG("Unexpected case");
   }
   VAR(r) = var(q);
   if (n_readings > 1) VAR(r) /= sqrt(n_readings);
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
	 int r;
#ifdef NEW3DFORMAT
	 twig *temp = limb;
#endif
	 f_export_ok = fFalse;
	 switch (pcs->style) {
	  case STYLE_NORMAL:
	  case STYLE_DIVING:
	  case STYLE_CYLPOLAR:
	    r = data_normal();
    	    break;
	  case STYLE_CARTESIAN:
	    r = data_cartesian();
	    break;
	  case STYLE_NOSURVEY:
	    r = data_nosurvey();
	    break;
	  case STYLE_IGNORE:
	    r = data_ignore();
	    break;
	  default:
	    BUG("bad style");
	 }
	 /* style function returns 0 => error */
#ifdef NEW3DFORMAT
	 if (!r) {
	    /* we have just created a very naughty twiglet, and it must be
	     * punished */
	    osfree(limb);
	    limb = temp;
	 }
#endif
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
	 SVX_ASSERT(pcsParent);
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
mod2pi(real a)
{
   return a - floor(a / (2 * M_PI)) * (2 * M_PI);
}

static real
handle_plumb(clino_type *p_ctype)
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
   static real clinos[] = {(real)M_PI_2, (real)(-M_PI_2), (real)0.0};
   clino_tok tok;

   skipblanks();
   if (isalpha(ch)) {
      filepos fp;
      get_pos(&fp);
      get_token();
      tok = match_tok(clino_tab, TABSIZE(clino_tab));
      if (tok != CLINO_NULL) {
	 *p_ctype = (clinos[tok] == CLINO_LEVEL ? CTYPE_HORIZ : CTYPE_PLUMB);
	 return clinos[tok];
      }
      set_pos(&fp);
   } else if (isSign(ch)) {
      int chOld = ch;
      nextch();
      if (toupper(ch) == 'V') {
	 nextch();
	 *p_ctype = CTYPE_PLUMB;
	 return (!isMinus(chOld) ? M_PI_2 : -M_PI_2);
      }

      if (isOmit(chOld)) {
	 *p_ctype = CTYPE_OMIT;
	 /* no clino reading, so assume 0 with large sd */
	 return (real)0.0;
      }
   } else if (isOmit(ch)) {
      /* OMIT char may not be a SIGN char too so we need to check here as
       * well as above... */
      nextch();
      *p_ctype = CTYPE_OMIT;
      /* no clino reading, so assume 0 with large sd */
      return (real)0.0;
   }
   return HUGE_REAL;
}

static void
warn_readings_differ(int msg, real diff)
{
   char buf[64];
   char *p;
   sprintf(buf, "%.2f", deg(fabs(diff)));
   p = strchr(buf, '.');
   if (p) {
      char *z = p;
      while (*++p) {
	 if (*p != '0') z = p + 1;
      }
      if (*z) *z = '\0';
   }
   compile_warning(msg, buf);
}

static bool
handle_comp_units(void)
{
   bool fNoComp = fTrue;
   if (VAL(Comp) != HUGE_REAL) {
      fNoComp = fFalse;
      VAL(Comp) *= pcs->units[Q_BEARING];
      if (VAL(Comp) < (real)0.0 || VAL(Comp) - M_PI * 2.0 > EPSILON) {
	 compile_warning(/*Suspicious compass reading*/59);
	 VAL(Comp) = mod2pi(VAL(Comp));
      }
   }
   if (VAL(BackComp) != HUGE_REAL) {
      fNoComp = fFalse;
      VAL(BackComp) *= pcs->units[Q_BACKBEARING];
      if (VAL(BackComp) < (real)0.0 || VAL(BackComp) - M_PI * 2.0 > EPSILON) {
	 /* FIXME: different message for BackComp? */
	 compile_warning(/*Suspicious compass reading*/59);
	 VAL(BackComp) = mod2pi(VAL(BackComp));
      }
   }
   return fNoComp;
}

static real
handle_compass(real *p_var)
{
   real var = VAR(Comp);
   real comp = VAL(Comp);
   real backcomp = VAL(BackComp);
   if (comp != HUGE_REAL) {
      comp = (comp - pcs->z[Q_BEARING]) * pcs->sc[Q_BEARING];
      comp -= pcs->z[Q_DECLINATION];
   }
   if (backcomp != HUGE_REAL) {
      backcomp = (backcomp - pcs->z[Q_BACKBEARING])
	      * pcs->sc[Q_BACKBEARING];
      backcomp -= pcs->z[Q_DECLINATION];
      backcomp -= M_PI;
      if (comp != HUGE_REAL) {
	 real diff = comp - backcomp;
	 real adj = fabs(diff) > M_PI ? M_PI : 0;
	 diff -= floor((diff + M_PI) / (2 * M_PI)) * 2 * M_PI;
	 if (sqrd(diff / 2.0) > var + VAR(Q_BACKBEARING)) {
	    /* fore and back readings differ by more than 2 sds */
	    warn_readings_differ(/*Compass reading and back compass reading disagree by %s degrees*/98, diff);
	 }
	 comp = (comp / var + backcomp / VAR(BackClino));
	 var = (var + VAR(BackClino)) / 4;
	 comp *= var;
	 comp += adj;
      } else {
	 comp = backcomp;
	 var = VAR(BackClino);
      }
   }
   *p_var = var;
   return comp;
}

static int
process_normal(prefix *fr, prefix *to, bool fToFirst,
	       clino_type ctype, clino_type backctype)
{
   real tape = VAL(Tape);
   real comp = VAL(Comp);
   real clin = VAL(Clino);
   real backclin = VAL(BackClino);

   real dx, dy, dz;
   real vx, vy, vz;
#ifndef NO_COVARIANCES
   real cxy, cyz, czx;
#endif

   bool fNoComp;

   /* adjusted tape is negative -- probably the calibration is wrong */
   if (tape < (real)0.0) {
      /* TRANSLATE different message for topofil? */
      compile_warning(/*Negative adjusted tape reading*/79);
   }

   fNoComp = handle_comp_units();

   if (ctype == CTYPE_READING) {
      real diff_from_abs90;
      clin *= pcs->units[Q_GRADIENT];
      /* percentage scale */
      if (pcs->f_clino_percent) clin = atan(clin);
      diff_from_abs90 = fabs(clin) - M_PI_2;
      if (diff_from_abs90 > EPSILON) {
	 compile_warning(/*Clino reading over 90 degrees (absolute value)*/51);
      } else if (pcs->f90Up && diff_from_abs90 > -EPSILON) {
	 ctype = CTYPE_PLUMB;
      }
   }

   if (backctype == CTYPE_READING) {
      real diff_from_abs90;
      backclin *= pcs->units[Q_BACKGRADIENT];
      /* percentage scale */
      if (pcs->f_backclino_percent) backclin = atan(backclin);
      diff_from_abs90 = fabs(backclin) - M_PI_2;
      if (diff_from_abs90 > EPSILON) {
	 /* FIXME: different message for BackClino? */
	 compile_warning(/*Clino reading over 90 degrees (absolute value)*/51);
      } else if (pcs->f90Up && diff_from_abs90 > -EPSILON) {
	 backctype = CTYPE_PLUMB;
      }
   }

   if (ctype != CTYPE_OMIT && backctype != CTYPE_OMIT && ctype != backctype) {
      compile_error_skip(/*Clino and BackClino readings must be of the same type*/84);
      return 0;
   }

   if (ctype == CTYPE_PLUMB || backctype == CTYPE_PLUMB) {
      /* plumbed */
      if (!fNoComp) {
	 /* FIXME: Different message for BackComp? */
	 compile_warning(/*Compass reading given on plumbed leg*/21);
      }

      dx = dy = (real)0.0;
      if (ctype != CTYPE_OMIT) {
	 if (backctype != CTYPE_OMIT && (clin > 0) == (backclin > 0)) {
	    /* We've got two UPs or two DOWNs - FIXME: not ideal message */
	    compile_error_skip(/*Clino and BackClino readings must be of the same type*/84);
	    return 0;
	 }
	 dz = (clin > (real)0.0) ? tape : -tape;
      } else {
	 dz = (backclin < (real)0.0) ? tape : -tape;
      }
      vx = vy = var(Q_POS) / 3.0 + dz * dz * var(Q_PLUMB);
      vz = var(Q_POS) / 3.0 + VAR(Tape);
#ifndef NO_COVARIANCES
      /* Correct values - no covariances in this case! */
      cxy = cyz = czx = (real)0.0;
#endif
   } else {
      /* Each of ctype and backctype are either CTYPE_READING/CTYPE_HORIZ
       * or CTYPE_OMIT */
      /* clino */
      real L2, cosG, LcosG, cosG2, sinB, cosB, dx2, dy2, dz2, v, V;
      if (fNoComp) {
	 compile_error_skip(/*Compass reading may not be omitted except on plumbed legs*/14);
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
	 /* take into account variance in LEVEL case */
	 real var_clin = var(Q_LEVEL);
	 real var_comp;
	 real comp = handle_compass(&var_comp);
	 /* ctype != CTYPE_READING is LEVEL case */
	 if (ctype == CTYPE_READING) {
	    clin = (clin - pcs->z[Q_GRADIENT]) * pcs->sc[Q_GRADIENT];
	    var_clin = VAR(Clino);
	 }
	 if (backctype == CTYPE_READING) {
	    backclin = (backclin - pcs->z[Q_BACKGRADIENT])
	       * pcs->sc[Q_BACKGRADIENT];
	    if (ctype == CTYPE_READING) {
	       if (sqrd((clin + backclin) / 3.0) >
		     var_clin + VAR(BackClino)) {
		  /* fore and back readings differ by more than 3 sds */
		  warn_readings_differ(/*Clino reading and back clino reading disagree by %s degrees*/99, clin + backclin);
	       }
	       clin = (clin / var_clin - backclin / VAR(BackClino));
	       var_clin = (var_clin + VAR(BackClino)) / 4;
	       clin *= var_clin;
	    } else {
	       clin = -backclin;
	       var_clin = VAR(BackClino);
	    }
	 }

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
	 V = VAR(Tape) / L2;
	 dy2 = dy * dy;
	 cosG2 = cosG * cosG;
	 sinGcosG = sin(clin) * cosG;
	 dz2 = dz * dz;
	 v = dz2 * var_clin;
#ifdef NO_COVARIANCES
	 vx = (var(Q_POS) / 3.0 + dx2 * V + dy2 * var_comp +
	       (.5 + sinB * sinB * cosG2) * v);
	 vy = (var(Q_POS) / 3.0 + dy2 * V + dx2 * var_comp +
	       (.5 + cosB * cosB * cosG2) * v);
	 if (ctype == CTYPE_OMIT && backctype == CTYPE_OMIT) {
	    /* if no clino, assume sd=tape/sqrt(10) so 3sds = .95*tape */
	    vz = var(Q_POS) / 3.0 + L2 * (real)0.1;
	 } else {
	    vz = var(Q_POS) / 3.0 + dz2 * V + L2 * cosG2 * var_clin;
	 }
	 /* for Surveyor87 errors: vx=vy=vz=var(Q_POS)/3.0; */
#else
	 vx = var(Q_POS) / 3.0 + dx2 * V + dy2 * var_comp +
	    (sinB * sinB * v);
	 vy = var(Q_POS) / 3.0 + dy2 * V + dx2 * var_comp +
	    (cosB * cosB * v);
	 if (ctype == CTYPE_OMIT && backctype == CTYPE_OMIT) {
	    /* if no clino, assume sd=tape/sqrt(10) so 3sds = .95*tape */
	    vz = var(Q_POS) / 3.0 + L2 * (real)0.1;
	 } else {
	    vz = var(Q_POS) / 3.0 + dz2 * V + L2 * cosG2 * var_clin;
	 }
	 /* usual covariance formulae are fine in no clino case since
	  * dz = 0 so value of var_clin is ignored */
	 cxy = sinB * cosB * (VAR(Tape) * cosG2 + var_clin * dz2)
	       - var_comp * dx * dy;
	 czx = VAR(Tape) * sinB * sinGcosG - var_clin * dx * dz;
	 cyz = VAR(Tape) * cosB * sinGcosG - var_clin * dy * dz;
#if 0
	 printf("vx = %6.3f, vy = %6.3f, vz = %6.3f\n", vx, vy, vz);
	 printf("cxy = %6.3f, cyz = %6.3f, czx = %6.3f\n", cxy, cyz, czx);
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

static int
process_diving(prefix *fr, prefix *to, bool fToFirst, bool fDepthChange)
{
   real tape = VAL(Tape);

   real dx, dy, dz;
   real vx, vy, vz;
#ifndef NO_COVARIANCES
   real cxy = 0, cyz = 0, czx = 0;
#endif

   handle_comp_units();

   /* depth gauge readings increase upwards with default calibration */
   if (fDepthChange) {
      SVX_ASSERT(VAL(FrDepth) == 0.0);
      dz = VAL(ToDepth) * pcs->units[Q_DEPTH] - pcs->z[Q_DEPTH];
      dz *= pcs->sc[Q_DEPTH];
   } else {
      dz = VAL(ToDepth) - VAL(FrDepth);
      dz *= pcs->units[Q_DEPTH] * pcs->sc[Q_DEPTH];
   }

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
   } else if (VAL(Comp) == HUGE_REAL &&
	      VAL(BackComp) == HUGE_REAL) {
      /* plumb */
      dx = dy = (real)0.0;
      if (dz < 0) tape = -tape;
      /* FIXME: Should use FrDepth sometimes... */
      dz = (dz * VAR(Tape) + tape * 2 * VAR(ToDepth))
	 / (VAR(Tape) * 2 * VAR(ToDepth));
      vx = vy = var(Q_POS) / 3.0 + dz * dz * var(Q_PLUMB);
      /* FIXME: Should use FrDepth sometimes... */
      vz = var(Q_POS) / 3.0 + VAR(Tape) * 2 * VAR(ToDepth)
			      / (VAR(Tape) + VAR(ToDepth));
   } else {
      real L2, sinB, cosB, dz2, D2;
      real var_comp;
      real comp = handle_compass(&var_comp);
      sinB = sin(comp);
      cosB = cos(comp);
      L2 = tape * tape;
      dz2 = dz * dz;
      D2 = L2 - dz2;
      if (D2 <= (real)0.0) {
	 /* FIXME: Should use FrDepth sometimes... */
	 real vsum = VAR(Tape) + 2 * VAR(ToDepth);
	 dx = dy = (real)0.0;
	 vx = vy = var(Q_POS) / 3.0;
	 /* FIXME: Should use FrDepth sometimes... */
	 vz = var(Q_POS) / 3.0 + VAR(Tape) * 2 * VAR(ToDepth) / vsum;
	 if (dz > 0) {
	    /* FIXME: Should use FrDepth sometimes... */
	    dz = (dz * VAR(Tape) + tape * 2 * VAR(ToDepth)) / vsum;
	 } else {
	    dz = (dz * VAR(Tape) - tape * 2 * VAR(ToDepth)) / vsum;
	 }
      } else {
	 real D = sqrt(D2);
	 /* FIXME: Should use FrDepth sometimes... */
	 real F = VAR(Tape) * L2 + 2 * VAR(ToDepth) * D2;
	 dx = D * sinB;
	 dy = D * cosB;

	 vx = var(Q_POS) / 3.0 +
	    sinB * sinB * F / D2 + var_comp * dy * dy;
	 vy = var(Q_POS) / 3.0 +
	    cosB * cosB * F / D2 + var_comp * dx * dx;
	 /* FIXME: Should use FrDepth sometimes... */
	 vz = var(Q_POS) / 3.0 + 2 * VAR(ToDepth);

#ifndef NO_COVARIANCES
	 cxy = sinB * cosB * (F / D2 + var_comp * D2);
	 /* FIXME: Should use FrDepth sometimes... */
	 cyz = -2 * VAR(ToDepth) * dy / D;
	 czx = -2 * VAR(ToDepth) * dx / D;
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

static int
process_cartesian(prefix *fr, prefix *to, bool fToFirst)
{
   real dx = (VAL(Dx) * pcs->units[Q_DX] - pcs->z[Q_DX]) * pcs->sc[Q_DX];
   real dy = (VAL(Dy) * pcs->units[Q_DY] - pcs->z[Q_DY]) * pcs->sc[Q_DY];
   real dz = (VAL(Dz) * pcs->units[Q_DZ] - pcs->z[Q_DZ]) * pcs->sc[Q_DZ];

   addlegbyname(fr, to, fToFirst, dx, dy, dz, VAR(Dx), VAR(Dy), VAR(Dz)
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
       case Dx: case Dy: case Dz:
	 read_reading(*ordering, fFalse);
	 break;
       case Ignore:
	 skipword(); break;
       case IgnoreAllAndNewLine:
	 skipline();
	 /* fall through */
       case Newline:
	 if (fr != NULL) {
	    int r;
	    r = process_cartesian(fr, to, first_stn == To);
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
	    int r = process_cartesian(fr, to, first_stn == To);
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
process_cylpolar(prefix *fr, prefix *to, bool fToFirst, bool fDepthChange)
{
   real tape = VAL(Tape);
   real frdepth = VAL(FrDepth);
   real todepth = VAL(ToDepth);

   real dx, dy, dz;
   real vx, vy, vz;
#ifndef NO_COVARIANCES
   real cxy = 0;
#endif

   handle_comp_units();

   /* depth gauge readings increase upwards with default calibration */
   if (fDepthChange) {
      SVX_ASSERT(VAL(FrDepth) == 0.0);
      dz = VAL(ToDepth) * pcs->units[Q_DEPTH] - pcs->z[Q_DEPTH];
      dz *= pcs->sc[Q_DEPTH];
   } else {
      dz = VAL(ToDepth) - VAL(FrDepth);
      dz *= pcs->units[Q_DEPTH] * pcs->sc[Q_DEPTH];
   }

   /* adjusted tape is negative -- probably the calibration is wrong */
   if (tape < (real)0.0) {
      compile_warning(/*Negative adjusted tape reading*/79);
   }

   if (VAL(Comp) == HUGE_REAL && VAL(BackComp) == HUGE_REAL) {
      /* plumb */
      dx = dy = (real)0.0;
      vx = vy = var(Q_POS) / 3.0 + dz * dz * var(Q_PLUMB);
      /* FIXME: Should use FrDepth sometimes... */
      vz = var(Q_POS) / 3.0 + 2 * VAR(ToDepth);
   } else {
      real sinB, cosB;
      real var_comp;
      real comp = handle_compass(&var_comp);
      sinB = sin(comp);
      cosB = cos(comp);

      dx = tape * sinB;
      dy = tape * cosB;

      vx = var(Q_POS) / 3.0 +
	 VAR(Tape) * sinB * sinB + var_comp * dy * dy;
      vy = var(Q_POS) / 3.0 +
	 VAR(Tape) * cosB * cosB + var_comp * dx * dx;
      /* FIXME: Should use FrDepth sometimes... */
      vz = var(Q_POS) / 3.0 + 2 * VAR(ToDepth);

#ifndef NO_COVARIANCES
      cxy = (VAR(Tape) - var_comp * tape * tape) * sinB * cosB;
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

/* Process tape/compass/clino, diving, and cylpolar styles of survey data
 * Also handles topofil (fromcount/tocount or count) in place of tape */
extern int
data_normal(void)
{
   prefix *fr = NULL, *to = NULL;
   reading first_stn = End;

   bool fTopofil = fFalse, fMulti = fFalse;
   bool fRev;
   clino_type ctype, backctype;
   bool fDepthChange;

   reading *ordering;

   VAL(Tape) = 0;
   VAL(Comp) = VAL(BackComp) = HUGE_VAL;
   VAL(FrCount) = VAL(ToCount) = 0;
   VAL(FrDepth) = VAL(ToDepth) = 0;

   again:

   fRev = fFalse;
   ctype = backctype = CTYPE_OMIT;
   fDepthChange = fFalse;

   /* ordering may omit clino reading, so set up default here */
   /* this is also used if clino reading is the omit character */
   VAL(Clino) = VAL(BackClino) = 0;

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
	  /* FIXME: use get_token so we can give a better error if the user
	   * gives a word rather than "F" or "B" */
	  switch(toupper(ch)) {
	   case 'F':
	     break;
	   case 'B':
	     fRev = fTrue;
	     break;
	   default:
	     compile_error_skip(/*Found `%c', expecting `F' or `B'*/131, ch);
	     process_eol();
	     return 0;
	  }
	  nextch();
	  break;
       case Tape:
	  read_reading(Tape, fFalse);
	  if (VAL(Tape) < (real)0.0)
	     compile_warning(/*Negative tape reading*/60);
	  break;
       case Count:
	  VAL(FrCount) = VAL(ToCount);
	  read_reading(ToCount, fFalse);
	  break;
       case FrCount:
	  read_reading(FrCount, fFalse);
	  break;
       case ToCount:
	  read_reading(ToCount, fFalse);
	  fTopofil = fTrue;
	  break;
       case Comp:
	  read_bearing_or_omit(Comp);
	  break;
       case BackComp:
	  read_bearing_or_omit(BackComp);
	  break;
       case Clino:
	  read_reading(Clino, fTrue);
	  if (VAL(Clino) == HUGE_REAL) {
	     VAL(Clino) = handle_plumb(&ctype);
	     if (VAL(Clino) != HUGE_REAL) break;
	     compile_error_token(/*Expecting numeric field, found `%s'*/9);
	     process_eol();
	     return 0;
	  }
	  ctype = CTYPE_READING;
	  break;
       case BackClino:
	  read_reading(BackClino, fTrue);
	  if (VAL(BackClino) == HUGE_REAL) {
	     VAL(BackClino) = handle_plumb(&backctype);
	     if (VAL(BackClino) != HUGE_REAL) break;
	     compile_error_token(/*Expecting numeric field, found `%s'*/9);
	     process_eol();
	     return 0;
	  }
	  backctype = CTYPE_READING;
	  break;
       case FrDepth:
	  read_reading(FrDepth, fFalse);
	  break;
       case ToDepth:
	  read_reading(ToDepth, fFalse);
	  break;
       case Depth:
	  VAL(FrDepth) = VAL(ToDepth);
	  read_reading(ToDepth, fFalse);
	  break;
       case DepthChange:
	  fDepthChange = fTrue;
	  VAL(FrDepth) = 0;
	  read_reading(ToDepth, fFalse);
	  break;
       case Ignore:
	  skipword();
	  break;
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
	     if (fTopofil)
		VAL(Tape) = VAL(ToCount) - VAL(FrCount);
	     /* Note: frdepth == todepth test works regardless of fDepthChange
	      * (frdepth always zero, todepth is change of depth) and also
	      * works for STYLE_NORMAL (both remain 0) */
	     if (pcs->f0Eq && VAL(Tape) == (real)0.0 &&
		 VAL(FrDepth) == VAL(ToDepth)) {
		process_equate(fr, to);
		goto inferred_equate;
	     }
	     if (fTopofil) {
		VAL(Tape) *= pcs->units[Q_COUNT] * pcs->sc[Q_COUNT];
	     } else {
		VAL(Tape) *= pcs->units[Q_LENGTH];
		VAL(Tape) -= pcs->z[Q_LENGTH];
		VAL(Tape) *= pcs->sc[Q_LENGTH];
	     }
	     switch (pcs->style) {
	      case STYLE_NORMAL:
		r = process_normal(fr, to, (first_stn == To) ^ fRev,
				   ctype, backctype);
		break;
	      case STYLE_DIVING:
		r = process_diving(fr, to, (first_stn == To) ^ fRev,
				   fDepthChange);
		break;
	      case STYLE_CYLPOLAR:
		r = process_cylpolar(fr, to, (first_stn == To) ^ fRev,
				     fDepthChange);
		break;
	      default:
		BUG("bad style");
	     }
	     if (!r) skipline();
	  }
          inferred_equate:
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
	     if (fTopofil) VAL(Tape) = VAL(ToCount) - VAL(FrCount);
	     /* Note: frdepth == todepth test works regardless of fDepthChange
	      * (frdepth always zero, todepth is change of depth) and also
	      * works for STYLE_NORMAL (both remain 0) */
	     if (pcs->f0Eq && VAL(Tape) == (real)0.0 &&
		 VAL(FrDepth) == VAL(ToDepth)) {
		process_equate(fr, to);
		process_eol();
		return 1;
	     }
	     if (fTopofil) {
		VAL(Tape) *= pcs->units[Q_COUNT] * pcs->sc[Q_COUNT];
	     } else {
		VAL(Tape) *= pcs->units[Q_LENGTH];
		VAL(Tape) -= pcs->z[Q_LENGTH];
		VAL(Tape) *= pcs->sc[Q_LENGTH];
	     }
	     switch (pcs->style) {
	      case STYLE_NORMAL:
		r = process_normal(fr, to, (first_stn == To) ^ fRev,
				   ctype, backctype);
		break;
	      case STYLE_DIVING:
		r = process_diving(fr, to, (first_stn == To) ^ fRev,
				   fDepthChange);
		break;
	      case STYLE_CYLPOLAR:
		r = process_cylpolar(fr, to, (first_stn == To) ^ fRev,
				     fDepthChange);
		break;
	      default:
		BUG("bad style");
	     }
	     process_eol();
	     return r;
	  }
	  do {
	     process_eol();
	     process_bol();
	  } while (isComm(ch));
	  goto again;
       default:
	  BUG("Unknown reading in ordering");
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
