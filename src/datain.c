/* > datain.c
 * Reads in survey files, dealing with special characters, keywords & data
 * Copyright (C) 1991-1998 Olly Betts
 */

/*
1993.01.19 fopen(<data_file>,"rt"); corrected to "rb"
1993.01.20 clino reading now scaled by cs.gradient_units not cs.bearing_units
1993.01.25 PC Compatibility OKed
1993.01.27 print %age (of chars) done
           also tell user which file we're returning to after *include
1993.02.23 #ifdef -> #if
           added #define PERCENTAGE to turn percentage code on/off
1993.02.28 added 90-to-up code - bit of a hack :(
1993.03.04 source while-fettled
1993.03.16 while re-fettled
1993.04.05 (W) added pthData hack
1993.04.06 PERCENTAGE set to 0 regardless if SEEK_END not defined
1993.04.07 reworked pthData hack to use UsePth()
1993.04.22 reworked to use fopenWithPthAndExt()
           isxxxx() macros -> isXxxx() to avoid possible collision with
             future extensions to C
1993.04.23 pthOutput code added
1993.05.20-ish (W) added #include "filelist.h"
1993.06.04 FALSE -> fFalse
1993.06.07 removed #undef PERCENTAGE #ifndef SEEK_END as it's been outhacked
1993.06.16 malloc() -> osmalloc(); syserr() -> fatal(); free() -> osfree()
1993.06.28 split addleg() into addleg() and addfakeleg()
1993.08.06 fettled error 'where' strings
1993.08.08 fettled debug code to be turned on & off by DEBUG
1993.08.09 simplified StnFromPfx()
1993.08.10 corrected wrong code for 90toUp
           added warning for | clino | > 90 degrees
           moved debugging flags to debug.h
1993.08.13 fettled header
1993.08.15 unknown commands cause warning and line is ignored
1993.08.16 filename -> fnm; fixed code dealing with paths of included files
           print "Processing data file '...'" only *after* successful fopen
           added fnmInput
           made immune to null padding
1993.08.21 fixed minor bug (does nested stuff work now?)
1993.10.24 added showline() to display line with error on
           tidied up prototypes and externs
1993.11.03 changed error routines
1993.11.06 extracted messages
1993.11.10 missed one
           merged read_prefix_default into read_prefix
           merged read_numeric_omit into read_numeric
           ... and read_numeric_default
           rearranged to use isData and improve efficiency
           cs.Style is now a function pointer
1993.11.11 check for compass <0 or >360 degrees
           check for tape < 0
           fettled data_normal() code a bit
1993.11.29 error 10->24; 16->25
1993.11.30 sorted out error vs fatal
           read_prefix now returns on failure, but gives error if !fOmit
           read_numeric and read_numeric_UD too
1993.12.01 now use longjmp on errors in readval functions
           showline prints tab as tab rather than ^I
1994.03.14 altered fopenWithPthAndExt to give filename actually used
1994.03.15 copes with directory extensions now
1994.03.16 extracted UsingDataFile()
           ordering of columns can now be varied
1994.03.19 added IGNORE and IGNOREALL to datum list
           fettled a tad
           showline() now prints to stderr
1994.03.21 added sd for plumbed legs
1994.03.23 minor fettle
1994.04.15 fixed recently-introduced zero length shot bug
1994.04.16 created netbits.h, readval.h
1994.04.18 created datain.h, commands.h; moved #define var(I) from survex.h
1994.04.27 removed unused translate; jbSkipLine -> datain.h
1994.05.05 added global defn for jbSkipLine
1994.05.12 PERCENTAGE -> NO_PERCENTAGE and fPercent added
1994.06.18 report unknown datum as bug; added data_diving
           showline calls now give area of error if appropriate
1994.06.20 overlong command name now indicated by ^^^^^ correctly
           crap at end of line all hi-lighted and rest of line ignored
           fixed minor bug introduced just recently
1994.06.21 added int argument to warning, error and fatal
1994.06.26 cs -> pcs
1994.06.27 added BEGIN and END; unknown keyword is now an error
1994.06.29 fettled code for when keyword is too long; added skipblanks()
1994.08.10 added feof() workaround for Steve's Linux box
1994.08.13 msc barfs if there's a func end() - now begin_block & end_block
1994.08.14 moved %age code to eliminate extra call to ftell & avoid feof bug
1994.08.16 added SOLVE command
1994.08.29 some use of out module
1994.08.31 completely converted to out module
1994.09.08 byte -> uchar
1994.09.19 clino reading of LEVEL or H => "0 & don't apply clino adjustment"
1994.09.20 global buffer szOut used
1994.10.01 stnlist now passed to solve_network
1994.11.08 added pcs->iStyle stuff to var() macro
1994.11.12 finished rejigging *calibrate, *data, *sd and *units
1994.11.13 now uses read_token and match_tok
           omitted clino gives vertical sd such of tape/sqrt(10)
1994.12.03 added FNM_SEP_LEV2
1995.01.29 no longer predivide posn error by 3.0 so must do it here
           *sd was missing from command token table
           fixed sd for LEVEL case
1995.02.21 fixed message 90 where it should have been 89
1995.03.25 osstrdup
1995.06.03 sorted table args to match_tok so we can binary chop in future
           clino reading can be omitted from the *data command
1995.06.13 fixed bug with omitted clino reading
1995.10.25 added check for adjusted tape being negative
1996.02.10 added set command
1996.02.22 fixed fNoClino bug
1996.03.24 parse structure introduced ; reworked showline() muchly
1997.08.22 added covariances
1998.03.21 fixed up to compile cleanly on Linux
1998.05.27 fettled to work with NO_COVARIANCES
1998.06.09 data filenames now converted from Unix/DOS format if necessary
1998.11.11 added *lrud (ignored for now)
*/

#include <limits.h>

#include "debug.h"
#include "cavein.h"
#include "filename.h"
#include "message.h"
#include "filelist.h"
#include "netbits.h"
#include "network.h"
#include "readval.h"
#include "datain.h"
#include "commands.h"
#include "out.h"

/* This'll do for now... but we really ought to sort out something */
/* in terms of FLT_EPSILON, DBL_EPSILON or LDBL_EPSILON as appropriate */
#define EPSILON ((real)(1e-6F))

#define MAX_KEYWORD_LEN 16

#define PRINT_COMMENT 0

#define var(I) (pcs->Var[(I)])

/* global defns */
int ch;
parse file;

static bool fNullPaddedEOF(void);
static long fpLineStart;

/* used in commline.c and (here) in datain.c */
/* Used to note where to put output files */
extern void UsingDataFile(const char *fnm) {
   /* pthOutput, fnmInput declared in survex.h */
   if (!pthOutput) {
      char *sz;
      pthOutput = PthFromFnm(fnm);
      fnmInput = osstrdup(fnm);
      /* Trim of any leaf extension, but directories can have extensions too */
      sz = strrchr(fnmInput, FNM_SEP_EXT);
      if (sz) {
	 char *szLev;
	 szLev = strrchr(sz, FNM_SEP_LEV);
#ifdef FNM_SEP_LEV2
	 if (!szLev) szLev = strrchr(sz, FNM_SEP_LEV2);
#endif
	 if (!szLev) *sz = '\0'; /* trim off any leaf extension */
      }
   }
}

static void skipword(void) {
   while (!isBlank(ch) && !isEol(ch)) nextch();
}

extern void skipblanks(void) {
   while (isBlank(ch)) nextch();
}

extern void showandskipline(const char *dummy, int n) {
   showline(dummy, n);
   skipline();
   out_info(msg(84));
}

extern void skipline(void) {
   while (!isEol(ch)) nextch();
}

/* display current line, marking n chars (n==INT_MAX => to end of line)
 *
 * fpLineStart
 * v
 * 12  13   7.5& 120 -34
 *              ^ftell()
 */
extern void showline(const char *dummy, int n) {
#if 1
   char sz[256];
   int ch;
   int i, o, c;
   int state;
   long fpCur;
   dummy = dummy; /* suppress warning */
   out_info(msg(58));
   fpCur = ftell(file.fh);
   o = (int)(fpCur - fpLineStart - 1);
   fseek(file.fh, fpLineStart, SEEK_SET);
   nextch();
   i = 0;
   state = 0;
   c = o;
   while (!isEol(ch)) {
      c--;
      if (c<0) {
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
   out_info(sz);
   n = min(n, i - o); /* cope with n==INT_MAX, or indeed just too big */
   if (n /*&& o + n < 80 !HACK!*/) {
      memset(sz, ' ', o);
      memset(sz + o, '^', n);
      sz[o + n] = '\0';
      out_info(sz);
   }
   fseek(file.fh, fpCur, SEEK_SET);
#else
  char sz[256];
  char *p;
  int ch;
  long c,c2;
  long i,j;
  long fp, fpEnd, fpCur;
  dummy=dummy; /* suppress warning */
  out_info(msg(58));
  fpCur=ftell(file.fh);
  if (c2==INT_MAX) {
    c2=1;
    fp=fpCur;
    fpEnd=LONG_MAX;
  } else {
    c2=n;
    fp=fpCur-n;
    fpEnd=fpCur;
  }
  fseek(file.fh,fpLineStart,SEEK_SET);
  c=fp-fpLineStart-1;
  nextch();
  p=sz;
  i=fp-ftell(file.fh);
  j=fpEnd-fp;
  while (!isEol(ch)) {
    if (ch=='\t')
      ch=' '; /* turn tabs into a single space for simplicity */
    if (ch<' '||ch==127) {
      /* ctrl char other than tab */
      *p++='^';
      *p++=ch<' '?ch+64:'?';
      if (i>0) {
        i--;
        c++;
      } else {
        if (j>0) {
          j--;
          c2++;
        }
      }
    } else
      *p++=ch;
    nextch();
  }
  if (fpEnd==LONG_MAX)
    c2+=ftell(file.fh)-fp; /* eh? think this should be removed */
  *p='\0';
  out_info(sz);
  p=sz;
  if (c2) if (c+c2<80) {
    /* things are a bit bogus otherwise */
    for( ; c ; c-- )
      *p++=' ';
    for( ; c2 ; c2-- )
      *p++='^';
    *p='\0';
    out_info(sz);
  }
  fseek(file.fh,fpCur,SEEK_SET);
#endif
}

extern void data_file( const char *pth, const char *fnm ) {
#ifndef NO_PERCENTAGE
  long int filelen;
#endif
  char *fnmUsed;

  file.fh = fopenWithPthAndExt( pth, fnm, EXT_SVX_DATA, "rb", &fnmUsed );
  if (file.fh == NULL) {
#if (OS==RISCOS) || (OS==UNIX)
     char *fnm_trans, *p;
#if OS==RISCOS
     char *q;
#endif
     fnm_trans = osstrdup(fnm);
#if OS==RISCOS
     q = fnm_trans;
#endif
     for ( p = fnm_trans ; *p ; p++ ) {
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
	   break;
         case '/': case '\\':
           *q++ = '.'; break;
	 default:
	   *q++ = *p; break;
#else
         /* swap a backslash to a forward slash */
         case '\\':
           *p = '/'; break;
#endif
        }
     }
#if OS==RISCOS
     *q = '\0';
#endif
     file.fh = fopenWithPthAndExt( pth, fnm_trans, EXT_SVX_DATA, "rb",
                                   &fnmUsed );
     osfree(fnm_trans);
#endif
     if (file.fh == NULL) {
        error(24,wr,fnm,0);
	/* print "ignoring..." maybe !HACK! */
        return;
     }
  }

  sprintf(out_buf,msg(122),fnm);
  out_current_action(out_buf);
  out_set_fnm(fnm); /* fnmUsed maybe?  !HACK! */

  UsingDataFile(fnmUsed);

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
   if (setjmp(file.jbSkipLine)) /* errors in nested functions can longjmp here */
     { /* do nothing special */ }
#endif

   while (!feof(file.fh)) {
    fpLineStart=ftell(file.fh); /* Note start of line for error reporting */
#ifndef NO_PERCENTAGE
    if (filelen>0) /* print %age of file done */
      out_set_percentage((int)(100*fpLineStart/filelen));
#endif

    nextch();
    skipblanks();

    if (isData(ch))
       (pcs->Style)(); /* call function */
    else if (isKeywd(ch)) {
       nextch();
       handle_command(fnmUsed, fnm);
    }

    skipblanks();
    if (!isEol(ch)) {
      if (!isComm(ch)) {
        if (fNullPaddedEOF())
          warning(57,showline,NULL,INT_MAX);
        else
          error( ( iscntrl(ch) ? 13 : 15 ) ,showline,NULL,INT_MAX);
        out_info(msg(89)); /* "Ignoring rest of line" */
      }
      skipline();
    }
  }

  if (ferror(file.fh)||(fclose(file.fh)==EOF))
    error(25,wr,fnm,0);
  sprintf(out_buf,msg(124),fnm);
  out_info(out_buf);
  /* set_current_fnm(""); not correct if filenames are nested */
  free(fnmUsed);
}

static bool fNullPaddedEOF(void) {
   long fp = ftell(file.fh);
   int ch;
   bool f;
   nextch();
   while (ch == '\0' && !feof(file.fh)) nextch();
   f = feof(file.fh);
   fseek(file.fh, fp, SEEK_SET);
   return f;
}

extern void data_normal( void ) {
  /* Horrible hack this, rewrite when I get the chance */
  /* It's getting better incrementally */
  prefix *fr_name,*to_name;
  real    dx,dy,dz;
  real    vx,vy,vz;
#ifndef NO_COVARIANCES
  real    cxy,cyz,czx;
#endif

  real tape,comp,clin;
  bool fNoComp,fPlumbed=fFalse;
  bool fNoClino;

  datum *ordering;

  /* ordering may omit clino reading, so set up default here */
  /* this is also used if clino reading is the omit character */
  clin=(real)0.0; /* no clino reading, so assume 0 with large sd */
  fNoClino=fTrue;

  for ( ordering=pcs->ordering ; ; ordering++ ) {
    skipblanks();
    switch (*ordering) {
      case Fr:    fr_name=read_prefix(fFalse); break;
      case To:    to_name=read_prefix(fFalse); break;
      case Tape:  tape=read_numeric(fFalse); break;
      case Comp: {
        comp=read_numeric(fTrue);
        if (comp==HUGE_REAL) {
          if (!isOmit(ch))
            errorjmp(9,1,file.jbSkipLine);
          nextch();
        }
        break;
      }
      case Clino: {
        clin=read_numeric(fTrue);
        if (clin!=HUGE_REAL) {
          /* we've got a real clino reading, so set clear the flag */
          fNoClino=fFalse;
        } else {
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
          get_token(9);
          tok = match_tok(clino_tab, TABSIZE(clino_tab));
          if (tok!=CLINO_NULL) {
            fPlumbed=fTrue;
            clin=clinos[tok];
          } else {
            int chOld=ch;
            long fp=ftell(file.fh);
            if (isSign(ch)) /* ie (isMinus(ch)||isPlus(ch)) */ {
              nextch();
              if (toupper(ch)=='V') {
                nextch();
                fPlumbed=fTrue;
                clin=(!isMinus(chOld) ? PI/2.0 : -PI/2.0);
                break;
              }
            }
            if (!isOmit(chOld))
              errorjmp(9,1,file.jbSkipLine);
            fseek( file.fh, fp, SEEK_SET );
            nextch();
            clin=(real)0.0; /* no clino reading, so assume 0 with large sd */
            /* default reading is set up above, but overwritten */
            break;
          }
        }
        break;
      }
      case Ignore: skipword(); break;
      case IgnoreAll: skipline(); goto dataread;
      case End:   goto dataread;
      default: BUG("Unknown datum in ordering");
    }
  }

  dataread:

/*
print_prefix(fr_name);
printf("->");
print_prefix(to_name);
printf(" %.2f %.2f %.2f\n",tape,comp,clin);
*/

  if (tape<(real)0.0)
    warning(60,showline,NULL,0);

  tape*=pcs->units[Q_LENGTH];

  fNoComp=(comp==HUGE_REAL);
  if (!fNoComp) {
    comp*=pcs->units[Q_BEARING];
    if (comp<(real)0.0 || comp-PI*2 > EPSILON)
      warning(59,showline,NULL,0);
  }

  if (!fPlumbed && !fNoClino) {
    clin*=pcs->units[Q_GRADIENT];
    if (fabs(clin)-PI/2 > EPSILON)
      warning(51,showline,NULL,0);
  }

/*
printf("fPlumbed %d fNoClino %d\n",fPlumbed,fNoClino);
printf("clin %.2f\n",clin);
*/

#if DEBUG_DATAIN
/* out of date! */
  printf("### %4.2f %4.2f %4.2f\n",tape,comp,clin);
  printf("leng %f, bear %f, grad %f\n",pcs->length_units,pcs->bearing_units,pcs->gradient_units);
  printf("tz %f, tsc %f, cz %f, csc %f, iz %f, isc %f, decz %f\n",pcs->tape_zero,pcs->tape_sc,pcs->comp_zero,pcs->comp_sc,pcs->clin_zero,pcs->clin_sc,pcs->decl_zero);
#endif

  tape=(tape - pcs->z[Q_LENGTH])*pcs->sc[Q_LENGTH];

  /* adjusted tape is negative -- probably the calibration is wrong */
  if (tape<(real)0.0)
    warning(79,showline,NULL,0);

  if ((fPlumbed && clin!=(real)0) ||
      (pcs->f90Up && (fabs(clin-PI/2)<EPSILON)) ) {
    /* plumbed */
    if (!fNoComp)
      warning(21,showline,NULL,0);
    dx=dy=(real)0.0;
    dz=(clin>(real)0.0)?tape:-tape;
    vx=vy=var(Q_POS)/3.0+dz*dz*var(Q_PLUMB);
    vz=var(Q_POS)/3.0+var(Q_LENGTH);
#ifndef NO_COVARIANCES
    cxy=cyz=czx=(real)0.0; /* !HACK! do this properly */
#endif
  } else {
    /* clino */
    real L2,cosG,LcosG,cosG2,sinB,cosB,dx2,dy2,dz2,v,V;
    if (fNoComp) {
      error(14,showandskipline,NULL,0);
      return;
    }
    if (tape==(real)0.0) {
      dx=dy=dz=(real)0.0;
      vx=vy=vz=(real)(var(Q_POS)/3.0); /* Position error only */
#ifndef NO_COVARIANCES
      cxy=cyz=czx=(real)0.0;
#endif
#if DEBUG_DATAIN_1
      printf("Zero length leg: vx = %f, vy = %f, vz = %f\n",vx,vy,vz);
#endif
    } else {
      comp = ( comp - pcs->z[Q_BEARING] ) * pcs->sc[Q_BEARING];
      comp -= pcs->z[Q_DECLINATION];
      if (!fPlumbed && !fNoClino) /* LEVEL case */
        clin=(clin- pcs->z[Q_GRADIENT])*pcs->sc[Q_GRADIENT];
/*
printf("fPlumbed %d fNoClino %d\n",fPlumbed,fNoClino);
printf("clin %.2f\n",clin);
*/

#if DEBUG_DATAIN
      printf("    %4.2f %4.2f %4.2f\n",tape,comp,clin);
#endif
      cosG=cos(clin);
      LcosG=tape*cosG;
      sinB=sin(comp);
      cosB=cos(comp);
#if DEBUG_DATAIN_1
      printf("sinB = %f, cosG = %f, LcosG = %f\n",sinB,cosG,LcosG);
#endif
      dx=LcosG*sinB;
      dy=LcosG*cosB;
      dz=tape*sin(clin);
/*      printf("%.2f\n",clin); */
#if DEBUG_DATAIN_1
      printf("dx = %f\ndy = %f\ndz = %f\n",dx,dy,dz);
#endif
      dx2=dx*dx;
      L2=tape*tape;
      V=var(Q_LENGTH)/L2;
      dy2=dy*dy;
      cosG2=cosG*cosG;
      dz2=dz*dz;
      /* take into account variance in LEVEL case */
      v=dz2*var( fPlumbed ? Q_LEVEL : Q_GRADIENT );
      vx=(var(Q_POS)/3.0+dx2*V+dy2*var(Q_BEARING)+(.5+sinB*sinB*cosG2)*v);
      vy=(var(Q_POS)/3.0+dy2*V+dx2*var(Q_BEARING)+(.5+cosB*cosB*cosG2)*v);
      if (!fNoClino)
        vz=(var(Q_POS)/3.0+dz2*V+L2*cosG2*var(Q_GRADIENT));
      else
        vz=(var(Q_POS)/3.0+L2*(real)0.1);
        /* if no clino, assume sd=tape/sqrt(10) so 3sds = .95*tape */
      /* for Surveyor87 errors: vx=vy=vz=var(Q_POS)/3.0; */
#ifndef NO_COVARIANCES
      cxy = sinB*cosB*
	 ( (var(Q_LENGTH) - var(Q_BEARING)*L2) * cosG2 +
	  var(Q_GRADIENT)*L2*(sin(clin)*sin(clin)) );
      czx = sinB*sin(clin)*cosG*( var(Q_LENGTH) - var(Q_GRADIENT)*L2 );
      cyz = cosB*sin(clin)*cosG*( var(Q_LENGTH) - var(Q_GRADIENT)*L2 );
#if 1
    cxy=cyz=czx=(real)0.0; /* for now */
#endif
#endif
#if DEBUG_DATAIN_1
      printf("In DATAIN.C, vx = %f, vy = %f, vz = %f\n",vx,vy,vz);
#endif
    }
  }
#if DEBUG_DATAIN_1
  printf("Just before addleg, vx = %f\n",vx);
#endif
/*printf("dx,dy,dz = %.2f %.2f %.2f\n\n",dx,dy,dz);*/
  addleg(StnFromPfx(fr_name),StnFromPfx(to_name),dx,dy,dz,vx,vy,vz
#ifndef NO_COVARIANCES
	 ,cyz,czx,cxy
#endif
	 );
}

extern void data_diving( void ) {
  prefix *fr_name,*to_name;
  real    dx,dy,dz;
  real    vx,vy,vz;

  real tape,comp;
  real fr_depth,to_depth;

  datum *ordering;

  for ( ordering=pcs->ordering ; ; ordering++ ) {
    skipblanks();
    switch (*ordering) {
      case Fr:    fr_name=read_prefix(fFalse); break;
      case To:    to_name=read_prefix(fFalse); break;
      case Tape:  tape=read_numeric(fFalse); break;
      case Comp:  comp=read_numeric(fFalse); break;
      case FrDepth: fr_depth=read_numeric(fFalse); break;
      case ToDepth: to_depth=read_numeric(fFalse); break;
      case Ignore: skipword(); break;
      case IgnoreAll: skipline(); goto dataread;
      case End:   goto dataread;
      default: BUG("Unknown datum in ordering");
    }
  }

  dataread:

  if (tape<(real)0.0)
    warning(60,showline,NULL,0);

  tape*=pcs->units[Q_LENGTH];
  comp*=pcs->units[Q_BEARING];
  if (comp<(real)0.0 || comp-PI*2 > EPSILON)
    warning(59,showline,NULL,0);

  tape=(tape-pcs->z[Q_LENGTH])*pcs->sc[Q_LENGTH];
  dz=(to_depth-fr_depth)*pcs->sc[Q_DEPTH]; /* assuming depths are negative */

  /* adjusted tape is negative -- probably the calibration is wrong */
  if (tape<(real)0.0)
    warning(79,showline,NULL,0);

  /* check if tape is less than depth change */
  if (tape<fabs(dz))
    warning(62,showline,NULL,0);

  if (tape==(real)0.0) {
    dx=dy=dz=(real)0.0;
    vx=vy=vz=(real)(var(Q_POS)/3.0); /* Position error only */
  } else {
    real L2,sinB,cosB,dx2,dy2,dz2,v,V,D2;
    comp = ( comp - pcs->z[Q_BEARING] ) * pcs->sc[Q_BEARING];
    comp -= pcs->z[Q_DECLINATION];
    sinB=sin(comp);
    cosB=cos(comp);
    L2=tape*tape;
    dz2=dz*dz;
    D2=L2-dz2;
    if (D2<=(real)0.0)
      dx=dy=(real)0.0;
    else {
      real D=sqrt(D2);
      dx=D*sinB;
      dy=D*cosB;
    }
    V=2*dz2+L2;
    if (D2<=V)
      vx=vy=var(Q_POS)/3.0+V;
    else {
      dx2=dx*dx;
      dy2=dy*dy;
      V=var(Q_LENGTH)/L2;
      v=dz2*var(Q_DEPTH)/D2;
      vx=var(Q_POS)/3.0+dx2*V+dy2*var(Q_BEARING)+sinB*sinB*v;
      vy=var(Q_POS)/3.0+dy2*V+dx2*var(Q_BEARING)+cosB*cosB*v;
    }
    vz=var(Q_POS)/3.0+2*var(Q_DEPTH);
  }
#ifdef NO_COVARIANCES
  addleg(StnFromPfx(fr_name),StnFromPfx(to_name),dx,dy,dz,vx,vy,vz);
#else
  addleg(StnFromPfx(fr_name),StnFromPfx(to_name),dx,dy,dz,vx,vy,vz,0,0,0); /* !HACK! need covariances */
#endif
}
