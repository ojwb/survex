/* > datain.c
 * Reads in survey files, dealing with special characters, keywords & data
 * Copyright (C) 1991-1998 Olly Betts
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

/* This'll do for now... but we really ought to sort out something in
 * terms of FLT_EPSILON, DBL_EPSILON or LDBL_EPSILON as appropriate */
#define EPSILON ((real)(1e-6F))

#define MAX_KEYWORD_LEN 16

#define PRINT_COMMENT 0

#define var(I) (pcs->Var[(I)])

int ch;

parse file = {
   NULL,
   NULL,
   0,
   NULL
};

static long fpLineStart;

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
   out_puts(msg(/*Ignoring entire line*/84));
}

extern void
skipline(void)
{
   while (!isEol(ch)) nextch();
}

/* display current line, marking n chars (n == INT_MAX => to end of line)
 *
 * fpLineStart
 * v
 * 12  13   7.5& 120 -34
 *              ^ftell()
 */
extern void
showline(const char *dummy, int n)
{
   char sz[256];
   int ch;
   int i, o, c;
   int state;
   long fpCur;
   dummy = dummy; /* suppress warning */
   out_puts(msg(/*in this line:*/58));
   fpCur = ftell(file.fh);
   o = (int)(fpCur - fpLineStart - 1);
   fseek(file.fh, fpLineStart, SEEK_SET);
   nextch();
   i = 0;
   state = 0;
   c = o;
   while (!isEol(ch)) {
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
   out_puts(sz);
   n = min(n, i - o); /* cope with n==INT_MAX, or indeed just too big */
   if (n /*&& o + n < 80 !HACK!*/) {
      memset(sz, ' ', o);
      memset(sz + o, '^', n);
      sz[o + n] = '\0';
      out_puts(sz);
   }
   fseek(file.fh, fpCur, SEEK_SET);
}

extern void
data_file(const char *pth, const char *fnm)
{
#ifndef NO_PERCENTAGE
   volatile long int filelen;
#endif

   file.fh = fopenWithPthAndExt(pth, fnm, EXT_SVX_DATA, "rb", &file.filename);
   if (file.fh == NULL) {
#if (OS==RISCOS) || (OS==UNIX)
      int f_changed = 0;
      char *fnm_trans, *p;
#if OS==RISCOS
      char *q;
#endif
      fnm_trans = osstrdup(fnm);
#if OS==RISCOS
      q = fnm_trans;
#endif
      for (p = fnm_trans; *p; p++) {
	 switch (*p) {
#if (OS==RISCOS)
         /* swap either slash to a dot, and a dot to a forward slash */
	 /* but .. goes to ^ */
         case '.':
	    if (p[1] == '.') {
	       *q++ = '^';
	       p++; /* skip second dot */
	    } else {
	       *q++ = '/';
	    }
	    f_changed = 1;
	    break;
         case '/': case '\\':
	    *q++ = '.';
	    f_changed = 1;
	    break;
	 default:
	    *q++ = *p; break;
#else
         case '\\': /* swap a backslash to a forward slash */
	    *p = '/';
	    f_changed = 1;
	    break;
#endif
	 }
      }
#if OS==RISCOS
      *q = '\0';
#endif
      if (f_changed)
	 file.fh = fopenWithPthAndExt(pth, fnm_trans, EXT_SVX_DATA, "rb",
				      &file.filename);

#if (OS==UNIX)
      /* as a last ditch measure, try lowercasing the filename */
      if (file.fh == NULL) {
	 f_changed = 0;
	 for (p = fnm_trans; *p ; p++)
	    if (isupper(*p)) {
	       *p = tolower(*p);
	       f_changed = 1;
	    }
	 if (f_changed)
	    file.fh = fopenWithPthAndExt(pth, fnm_trans, EXT_SVX_DATA, "rb",
					 &file.filename);
      }
#endif
      osfree(fnm_trans);
#endif
      if (file.fh == NULL) {
	 compile_error(/*Couldn't open data file '%s'*/24, fnm);
	 /* print "ignoring..." maybe !HACK! */
	 return;
      }
   }

#if 0
   sprintf(out_buf, msg(/*Processing data file '%s'*/122), fnm);
   out_current_action(out_buf);
#endif
   out_set_fnm(fnm); /* FIXME: file.filename maybe? */

   using_data_file(file.filename);

#ifndef NO_PERCENTAGE
   /* Try to find how long the file is...
    * However, under ANSI fseek( ..., SEEK_END) may not be supported */
   filelen = 0;
   if (fPercent) {
      if (fseek(file.fh, 0l, SEEK_END) == 0) filelen = ftell(file.fh);
      rewind(file.fh); /* reset file ptr to start & clear any error state */
   }
#endif

   file.line = 1;
   
#ifdef HAVE_SETJMP
   /* errors in nested functions can longjmp here */
   if (setjmp(file.jbSkipLine)) {
      /* do nothing special */ 
   }
#endif
   
   while (!feof(file.fh)) {
      /* Note start of line for error reporting */
      fpLineStart = ftell(file.fh);
#ifndef NO_PERCENTAGE
      /* print %age of file done */
      if (filelen > 0) out_set_percentage((int)(100 * fpLineStart / filelen));
#endif

      nextch();
      skipblanks();

      if (isData(ch)) {
	 /* style function returns 0 => error, so start next line */
	 if (!(pcs->Style)()) {
	    skipline();
	    continue;
	 }
      } else if (isKeywd(ch)) {
	 nextch();
	 handle_command();
      }

      skipblanks();
      if (!isEol(ch)) {
	 if (!isComm(ch)) {
	    compile_error(/*End of line not blank*/15);
	    showline(NULL, INT_MAX);
	    out_puts(msg(/*Ignoring rest of line*/89));
	 }
	 skipline();
      }

      /* FIXME need to cope with "\r"-only end of lines - this will work
       * for Unix and DOS text files though (RISC OS is the same as UNIX) */
      if (ch == '\n') file.line++;
   }

#ifndef NO_PERCENTAGE
   if (fPercent) putnl();
#endif

   /* FIXME: check than we've had all *begin-s *end-ed */
   
   if (ferror(file.fh) || (fclose(file.fh) == EOF))
      compile_error(/*Error reading file*/18);

   /* set_current_fnm(""); not correct if filenames are nested */
   osfree(file.filename);
}

extern int
data_normal(void)
{
   /* Horrible hack this, rewrite when I get the chance */
   /* It's getting better incrementally */
   prefix *fr_name, *to_name;
   real dx, dy, dz;
   real vx, vy, vz;
#ifndef NO_COVARIANCES
   real cxy, cyz, czx;
#endif

   real tape, comp, clin;
   bool fNoComp, fPlumbed = fFalse;
   bool fNoClino;
   
   datum *ordering;
   
   /* ordering may omit clino reading, so set up default here */
   /* this is also used if clino reading is the omit character */
   clin = (real)0.0; /* no clino reading, so assume 0 with large sd */
   fNoClino = fTrue;

   for (ordering = pcs->ordering ; ; ordering++) {
      skipblanks();
      switch (*ordering) {
       case Fr: fr_name = read_prefix(fFalse); break;
       case To: to_name = read_prefix(fFalse); break;
       case Tape: tape = read_numeric(fFalse); break;
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
		get_token();
		tok = match_tok(clino_tab, TABSIZE(clino_tab));
		if (tok != CLINO_NULL) {
		   fPlumbed = fTrue;
		   clin = clinos[tok];
		   break;
		}
	     } else if (isSign(ch)) {
		int chOld = ch;
		nextch();
		if (toupper(ch)=='V') {
		   nextch();
		   fPlumbed=fTrue;
		   clin = (!isMinus(chOld) ? PI / 2.0 : -PI / 2.0);
		   break;
		}
		
		if (isOmit(chOld)) {
		   clin = (real)0.0; /* no clino reading, so assume 0 with large sd */
		   /* default reading is set up above, but overwritten */
		   break;
		}
	     }
	     
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
       case IgnoreAll:
	 skipline();
	 /* fall through */
       case End:
	 goto dataread;
       default: BUG("Unknown datum in ordering");
      }
   }
   
   dataread:

#if 0
   print_prefix(fr_name);
   printf("->");
   print_prefix(to_name);
   printf(" %.2f %.2f %.2f\n",tape,comp,clin);
#endif
	    
   if (tape < (real)0.0) {
      compile_warning(/*Negative tape reading*/60);
      showline(NULL, 0);
   }

   tape *= pcs->units[Q_LENGTH];

   fNoComp = (comp == HUGE_REAL);
   if (!fNoComp) {
      comp *= pcs->units[Q_BEARING];
      if (comp < (real)0.0 || comp - PI * 2 > EPSILON) {
	 compile_warning(/*Suspicious compass reading*/59);
	 showline(NULL, 0);
      }
   }

   if (!fPlumbed && !fNoClino) {
      clin *= pcs->units[Q_GRADIENT];
      if (fabs(clin) - PI / 2 > EPSILON) {
	 compile_warning(/*Clino reading over 90 degrees (absolute value)*/51);
	 showline(NULL, 0);
      }
   }

#if 0
   printf("fPlumbed %d fNoClino %d\n",fPlumbed,fNoClino);
   printf("clin %.2f\n",clin);
#endif

#if DEBUG_DATAIN
   /* out of date! */
   printf("### %4.2f %4.2f %4.2f\n", tape, comp, clin);
   printf("leng %f, bear %f, grad %f\n",
	  pcs->length_units, pcs->bearing_units, pcs->gradient_units);
   printf("tz %f, tsc %f, cz %f, csc %f, iz %f, isc %f, decz %f\n",
	  pcs->tape_zero, pcs->tape_sc, pcs->comp_zero, pcs->comp_sc,
	  pcs->clin_zero, pcs->clin_sc, pcs->decl_zero);
#endif

   tape = (tape - pcs->z[Q_LENGTH]) * pcs->sc[Q_LENGTH];

   /* adjusted tape is negative -- probably the calibration is wrong */
   if (tape < (real)0.0) {
      compile_warning(/*Negative adjusted tape reading*/79);
      showline(NULL, 0);
   }
   
   if ((fPlumbed && clin != (real)0) ||
       (pcs->f90Up && (fabs(clin - PI / 2) < EPSILON))) {
      /* plumbed */
      if (!fNoComp) {
	 compile_warning(/*Compass reading given on plumbed leg*/21);
	 showline(NULL, INT_MAX);
      }

      dx = dy = (real)0.0;
      dz = (clin > (real)0.0) ? tape : -tape;
      vx = vy = var(Q_POS) / 3.0 + dz * dz * var(Q_PLUMB);
      vz = var(Q_POS) / 3.0 + var(Q_LENGTH);
#ifndef NO_COVARIANCES
      cxy = cyz = czx = (real)0.0; /* !HACK! do this properly */
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
	 dz2 = dz * dz;
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
#ifndef NO_COVARIANCES
	 cxy = sinB * cosB * 
	    ((var(Q_LENGTH) - var(Q_BEARING) * L2) * cosG2 +
	     var(Q_GRADIENT) * L2 * (sin(clin) * sin(clin)));
	 czx = sinB * sin(clin) * cosG *
	    (var(Q_LENGTH) - var(Q_GRADIENT) * L2);
	 cyz = cosB * sin(clin) * cosG *
	    (var(Q_LENGTH) - var(Q_GRADIENT) * L2);
#if 1
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
   addleg(StnFromPfx(fr_name), StnFromPfx(to_name), dx, dy, dz, vx, vy, vz
#ifndef NO_COVARIANCES
	  , cyz, czx, cxy
#endif
	  );
   return 1;
}

extern int
data_diving(void)
{
   prefix *fr_name, *to_name;
   real dx, dy, dz;
   real vx, vy, vz;

   real tape, comp;
   real fr_depth, to_depth;

   datum *ordering;
  
   for (ordering = pcs->ordering; ; ordering++) {
      skipblanks();
      switch (*ordering) {
      case Fr: fr_name = read_prefix(fFalse); break;
      case To: to_name = read_prefix(fFalse); break;
      case Tape: tape = read_numeric(fFalse); break;
      case Comp: comp = read_numeric(fFalse); break;
      case FrDepth: fr_depth = read_numeric(fFalse); break;
      case ToDepth: to_depth = read_numeric(fFalse); break;
      case Ignore: skipword(); break;
      case IgnoreAll:
	 skipline();
	 /* drop through */
      case End:
	 goto dataread;
      default: BUG("Unknown datum in ordering");
      }
   }

   dataread:

   if (tape < (real)0.0) {
      compile_warning(/*Negative tape reading*/60);
      showline(NULL, 0);
   }

   tape *= pcs->units[Q_LENGTH];
   comp *= pcs->units[Q_BEARING];
   if (comp < (real)0.0 || comp - PI * 2 > EPSILON) {
      compile_warning(/*Suspicious compass reading*/59);
      showline(NULL, 0);
   }

   tape = (tape - pcs->z[Q_LENGTH]) * pcs->sc[Q_LENGTH];
   /* assuming depths are negative */
   dz = (to_depth - fr_depth) * pcs->sc[Q_DEPTH];

   /* adjusted tape is negative -- probably the calibration is wrong */
   if (tape < (real)0.0) {
      compile_warning(/*Negative adjusted tape reading*/79);
      showline(NULL, 0);
   }

   /* check if tape is less than depth change */
   if (tape < fabs(dz)) {
      compile_warning(/*Tape reading is less than change in depth*/62);
      showline(NULL, 0);
   }

   if (tape == (real)0.0) {
      dx = dy = dz = (real)0.0;
      vx = vy = vz = (real)(var(Q_POS) / 3.0); /* Position error only */
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
#ifdef NO_COVARIANCES
   addleg(StnFromPfx(fr_name), StnFromPfx(to_name), dx, dy, dz, vx, vy, vz);
#else
   addleg(StnFromPfx(fr_name), StnFromPfx(to_name), dx, dy, dz,
	  vx, vy, vz, 0, 0, 0); /* !HACK! need covariances */
#endif
   return 1;
}
