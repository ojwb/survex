/* > commands.c
 * Code for directives
 * Copyright (C) 1991-1998 Olly Betts
 */

/*
1992.07.29 Altered SPECIAL chars to new method
1992.10.26 Fettled code to neaten it
1993.01.20 calibrate() "adjusted"
1993.01.25 PC Compatibility OKed
1993.02.19 fettled indentation
1993.02.24 fname[] -> fnm[]
1993.02.27 error #38 -> case of error #5
1993.03.11 fettled indentation
1993.04.05 (w) added hacked pathsearch for data
1993.04.06 converted to use error.h
1993.04.22 out of memory -> error # 0
           isxxxx() macros -> isXxxx() to avoid possible collision with
             future extensions to C
1993.05.03 char * -> sz
1993.05.05 trimmed spaces at eol
1993.06.04 FALSE -> fFalse
1993.06.08 FEET_PER_METRE corrected to METRES_PER_FOOT
1993.06.11 units(): fixed bug and improved a lot (it was even nastier)
1993.06.16 syserr -> fatal
1993.06.28 now count equates as legs (is this right?)
           addleg() split into addleg() and addfakeleg()
1993.08.06 fettled error 'where' strings
1993.08.09 altered data structure (OLD_NODES compiles previous version)
           simplified StnFromPfx()
1993.08.11 NEW_NODES & POS(stn,d)
           FOR_EACH_STN( stn ) does for loop with stn looping thru' all nodes
1993.08.12 fixed equate_list bug under NEW_NODES
1993.08.13 another equate_list bug (could go wrong if one point fixed)
           also error reported if two fixed nodes are equated
1993.08.14 changed to unix eols
           tweaked error messages for fixed and equated nodes errors
           added warning for *fix with no coordinates
           added warning for *equate of 2 equal fixed points
           added warning for re-fixing a point at the same coordinates
           added error for 2nd *fix with no coordinates
1993.08.16 corrected free -> osfree
1993.11.03 changed error routines
1993.11.08 error 5 made fatal
           only NEW_NODES code left
1993.11.10 merged read_prefix_default into read_prefix
           merged read_numeric_omit into read_numeric
           ... and read_numeric_default
1993.11.11 minor tweaks
           fOmitAlready changed to stnOmitAlready
1993.11.29 error 5 -> 11
1993.11.30 fixed bug in units() - code still not wonderful though
           corrected wrong numbers for a couple of error messages
           sorted out error vs fatal (quite major fettling in places)
           read_prefix now returns on failure, but gives error if !fOmit
           read_numeric ditto (unless fDefault)
1993.12.01 longjmp now used on error, rather than returning HUGE_REAL
1994.03.16 invented get_token function to remove duplicated code
1994.03.19 added IGNORE and IGNOREALL to datum list
1994.04.16 created netbits.h, commands.h, readval.h
1994.04.18 created defaults.h, datain.h
           moved QUANTITY_*, UNITS_* from survex.h
1994.04.27 jbSkipLine is now in datain.h
1994.05.05 ch, fh, buffer now in datain.h
1994.06.18 added FROMDEPTH and TODEPTH and DIVING style data
           get_token skips any blanks
           showline calls now give area of error if appropriate
           skipline() called before longjmp() now [by showandskipline()]
1994.06.20 include filename will underline whole filename if too long
           fixed compiler warning in calibrate
1994.06.21 added int argument to warning, error and fatal
1994.06.22 now free cs.ordering
1994.06.26 cs -> pcs
1994.06.27 added begin() and end(); now #include "commline.h"
1994.06.29 added skipblanks()
1994.08.13 msc barfs if we have a fn named end() - now begin_block & end_block
1994.09.08 byte -> uchar
1994.11.08 added *sd command and rejigged layout
1994.11.12 finished rejigging *calibrate, *data, *sd and *units
1994.11.13 removed fDefault arg from read_numeric
           ordering is now flagged as freeable or not
1994.12.11 you can now quote *include-d filenames with spaces
1995.01.29 fixed bug in *sd which stopped units being read
           plumb and level are now recognised as quantities
1995.02.21 added BIT() to fix 16-bit int problems
1995.03.25 fettled
1995.06.03 sorted table args to match_tok so we can binary chop in future
           clino reading can be omitted from the *data command
           message 66 no longer used
1995.06.06 TABSIZE() macro added
1995.06.13 match_tok() now uses binary chop
1996.02.10 added rudimentary set_chars()
1996.02.13 improved set_chars()
1996.03.23 improved include() somewhat
1996.03.24 parse structure introduced to fix include() problem
1996.04.02 fixed some Borland C warnings
1996.04.15 SPECIAL_POINT -> SPECIAL_DECIMAL
1998.03.04 merged 2 deviant versions:
>1996.06.10 Next and Back added to datum list
>1997.08.22 added covariances
1998.03.21 fix for covariances
*/

#include <assert.h>
#include "survex.h"
#include "filename.h"
#include "message.h"
#include "netbits.h"
#include "commands.h"
#include "readval.h"
#include "defaults.h"
#include "datain.h"
#include "commline.h"
#include "debug.h"

#define MAX_KEYWORD_LEN 16

/* read token, giving error 'en' if it is too long and skipping line */
extern void get_token(int en) {
  int j;
  skipblanks();
  for( j=0 ; isalpha(ch) ; ) {
    buffer[j++]=toupper(ch);
    if (j>=MAX_KEYWORD_LEN)
      errorjmp(en,j,file.jbSkipLine);
    nextch();
  }
  buffer[j]='\0';
  /* printf("get_token() got "); puts((sz)buffer); */
}

/* match_tok() now uses binary chop
 * tab argument should be alphabetically sorted (ascending)
 */
extern int match_tok( sztok *tab, int tab_size, char *szTry ) {
  int a=0, b=tab_size-1, c;
  int r;
  assert(tab_size>0); /* catch empty table */
/*  printf("[%d,%d]",a,b); */
  while (a<=b) {
     c=(unsigned)(a+b)/2;
/*     printf(" %d",c); */
     r=strcmp(tab[c].sz,szTry);
     if (r==0)
       return tab[c].tok; /* match */
     if (r<0)
       a=c+1;
     else
       b=c-1;
  }
  return tab[tab_size].tok; /* no match */
}

/* NB match_units *doesn't* call get_token, so do it yourself if approp. */
static int match_units(void) {
  static sztok utab[] = {
    {"DEGREES",       UNITS_DEGS },
    {"DEGS",          UNITS_DEGS },
    {"FEET",          UNITS_FEET },
    {"GRADS",         UNITS_GRADS },
    {"METERS",        UNITS_METRES },
    {"METRES",        UNITS_METRES },
    {"METRIC",        UNITS_METRES },
    {"MILS",          UNITS_GRADS },
    {"MINUTES",       UNITS_MINUTES },
    {"PERCENT",       UNITS_PERCENT },
    {"PERCENTAGE",    UNITS_PERCENT },
    {"YARDS",         UNITS_YARDS },
    {NULL,            UNITS_NULL }
  };
  int units;
  units=match_tok(utab,TABSIZE(utab),(sz)buffer);
  if (units==UNITS_NULL)
    errorjmp(35,strlen((sz)buffer),file.jbSkipLine);
  if (units==UNITS_PERCENT) {
    NOT_YET;
  }
  return units;
}

/* returns mask with bit x set to indicate quantity x specified */
static ulong get_qlist(void) {
  static sztok qtab[] = {
    {"ANGLEOUTPUT",  Q_ANGLEOUTPUT },
    {"BEARING",      Q_BEARING },
    {"CLINO",        Q_GRADIENT },    /* alternative name */
    {"COMPASS",      Q_BEARING },     /* alternative name */
    {"COUNT",        Q_COUNT },
    {"COUNTER",      Q_COUNT },       /* alternative name */
    {"DECLINATION",  Q_DECLINATION },
    {"DEPTH",        Q_DEPTH },
    {"DX",           Q_DX },
    {"DY",           Q_DY },
    {"DZ",           Q_DZ },
    {"GRADIENT",     Q_GRADIENT },
    {"LENGTH",       Q_LENGTH },
    {"LENGTHOUTPUT", Q_LENGTHOUTPUT },
    {"LEVEL",        Q_LEVEL},
    {"PLUMB",        Q_PLUMB},
    {"POSITION",     Q_POS },
    {"TAPE",         Q_LENGTH },      /* alternative name */
    {NULL,           Q_NULL }
  };
  ulong qmask=0;
  int tok;
  for (;;) {
    get_token(34);
    tok=match_tok(qtab,TABSIZE(qtab),(sz)buffer);
    if (tok==Q_NULL)
      break; /* bail out if we reach the table end with no match */
    qmask|=BIT(tok);
  }
  return qmask;
}

static int tok_style(char *szStyle) {
  static sztok styletab[] = {
    {"DIVING",       STYLE_DIVING },
    {"NORMAL",       STYLE_NORMAL },
    {NULL,           STYLE_UNKNOWN }
  };
  return match_tok(styletab,TABSIZE(styletab),szStyle);
}

#define SPECIAL_UNKNOWN 0
extern void set_chars( void ) {
  static sztok chartab[] = {
    {"BLANK",     SPECIAL_BLANK },
    {"COMMENT",   SPECIAL_COMMENT },
    {"DECIMAL",   SPECIAL_DECIMAL },
    {"EOL",       SPECIAL_EOL }, /* EOL won't work well */
    {"KEYWORD",   SPECIAL_KEYWORD },
    {"MINUS",     SPECIAL_MINUS },
    {"NAMES",     SPECIAL_NAMES },
    {"OMIT",      SPECIAL_OMIT },
    {"PLUS",      SPECIAL_PLUS },
    {"ROOT",      SPECIAL_ROOT },
    {"SEPARATOR", SPECIAL_SEPARATOR },
    {NULL,        SPECIAL_UNKNOWN }
  };
  int mask;
  int i;
  get_token(0); /* !HACK! error message if not found */
  mask=match_tok(chartab,TABSIZE(chartab),(sz)buffer);
  if (!mask) {
    error(0,showandskipline,NULL,(int)strlen((sz)buffer)); /* !HACK! error message if not found */
    return;
  }
  /* if we're currently using an inherited translation table, allocate a new
   * table, and copy old one into it */
  if (pcs->next && pcs->next->Translate==pcs->Translate) {
     short *p;
     p=((short*)osmalloc(ossizeof(short)*257))+1;
     memcpy(p-1,pcs->Translate-1,sizeof(short)*257);
     pcs->Translate=p;
  }
  /* clear this flag for all non-alphanums */
  for( i=0; i<256 ; i++ )
    if (!isalnum(i))
      pcs->Translate[i]&=~mask;
  skipblanks();
  /* now set this flag for all specified chars */
  for( ; !isEol(ch) ; ) {
    if (!isalnum(ch))
      pcs->Translate[ch]|=mask;
    nextch();
  }
}

extern void set_prefix( void ) {
  pcs->Prefix=read_prefix(fFalse);
  /* !HACK! maybe warn that this command is now deprecated */
}

extern void begin_block( void ) {
  prefix *tag;
  push();
  tag=read_prefix(fTrue);
  pcs->tag=tag;
  if (tag)
    pcs->Prefix=tag;
}

extern void end_block( void ) {
  prefix *tag, *tagBegin;
  tagBegin=pcs->tag;
  if (!pop()) {
    error(192,showandskipline,NULL,0); /* more ENDs than BEGINs */
    return;
  }
  tag=read_prefix(fTrue); /* need to read using root *before* BEGIN */
  if (tag!=tagBegin) {
    if (tag)
      error(193,showandskipline,NULL,0); /* tag mismatch */
    else
      warning(194,showline,NULL,0); /* close tag omitted; open tag given */
  }
}

extern void fix_station( void ) {
  prefix *fix_name;
  node   *stn;
  static node *stnOmitAlready=NULL;
  real x,y,z;

  fix_name=read_prefix(fFalse);
  stn=StnFromPfx(fix_name);

  x=read_numeric(fTrue);
  if (x==HUGE_REAL) {
    if (stnOmitAlready) {
      if (stn!=stnOmitAlready)
        error(56,showandskipline,NULL,0);
      else
        warning(61,showline,NULL,0);
      return;
    }
    warning(54,showline,NULL,0);
    x=y=z=(real)0.0;
    stnOmitAlready=stn;
  } else {
    y=read_numeric(fFalse);
    z=read_numeric(fFalse);
  }

  if (!fixed(stn)) {
    POS(stn,0)=x;
    POS(stn,1)=y;
    POS(stn,2)=z;
    fix(stn);
    return;
  }

  if (x!=POS(stn,0) || y!=POS(stn,1) || z!=POS(stn,2)) {
    error(46,showandskipline,NULL,0);
    return;
  }
  warning(55,showline,NULL,0);
}

extern void equate_list( void ) {
  node   *stn1,*stn2;
  prefix *name1,*name2;
  bool   fOnlyOneStn=fTrue; /* to trap eg *equate entrance.6 */

  name1=read_prefix(fFalse);
  stn1=StnFromPfx( name1 );
  while (fTrue) {
    stn2=stn1;
    name2=name1;
    name1=read_prefix(fTrue);
    if (name1==NULL) {
      if (fOnlyOneStn)
        error(33,showandskipline,NULL,0);
      return;
    }
    fOnlyOneStn=fFalse;
    stn1=StnFromPfx( name1 );
    /* equate nodes if not already equated */
    if (name1->pos!=name2->pos) {
      node *stn;
      pos *posReplace; /* need this as name2->pos gets changed during loop */
      pos *posWith;
      if (fixed(stn1)) {
        if (fixed(stn2)) {
         /* both are fixed, but let them off iff their coordinates match */
         int d;
         for( d=2 ; d>=0 ; d-- )
           if ( name1->pos->p[d]!=name2->pos->p[d] ) {
             error(52,showandskipline,NULL,1);
             return;
           }
         warning(53,showline,NULL,1);
      }
      posReplace=name2->pos; /* stn1 is fixed, so replace the other's pos */
      posWith=name1->pos;
    } else {
      posReplace=name1->pos; /* stn1 isn't fixed, so replace its pos */
      posWith=name2->pos;
    }

    /* replace all refs to posReplace with references to pos of stn1 */
    FOR_EACH_STN( stn )
      if (stn->name->pos==posReplace)
        stn->name->pos=posWith;
    /* free the (now-unused) old pos */
    osfree(posReplace);
    /* count equates as legs for now... */
#ifdef NO_COVARIANCES
    addleg(stn1,stn2,(real)0.0,(real)0.0,(real)0.0,
                     (real)0.0,(real)0.0,(real)0.0);
#else
    addleg(stn1,stn2,(real)0.0,(real)0.0,(real)0.0,
                     (real)0.0,(real)0.0,(real)0.0,
                     (real)0.0,(real)0.0,(real)0.0);
#endif
    }
  }
}

void data( void ) {
  static sztok dtab[]= {
#ifdef SVX_MULTILINEDATA
    {"BACK",         Back },
#endif
    {"BEARING",      Comp },
    {"FROM",         Fr },
    {"FROMDEPTH",    FrDepth },
    {"GRADIENT",     Clino },
    {"IGNORE",       Ignore },
    {"IGNOREALL",    IgnoreAll },
    {"LENGTH",       Tape },
#ifdef SVX_MULTILINEDATA
    {"NEXT",         Next },
#endif
    {"TO",           To },
    {"TODEPTH",      ToDepth },
    { NULL,          End }
  };
/*typedef enum { End=0, Fr, To, Tape, Comp, Clino, BackComp, BackClino,
  FrDepth, ToDepth, dx, dy, dz, FrCount, ToCount, dr,
  Ignore, IgnoreAll } datum;*/

  static style fn[]={data_normal,data_diving};

  /* readings which may be given for each style */
  static ulong mask[]={
    BIT(Fr)|BIT(To)|BIT(Tape)|BIT(Comp)|BIT(Clino),
    BIT(Fr)|BIT(To)|BIT(Tape)|BIT(Comp)|BIT(FrDepth)|BIT(ToDepth)
  };
  /* readings which may be omitted for each style */
  static ulong mask_optional[]={
    BIT(Clino),
    0
  };

  int style, k=0, kMac;
  datum *new_order, d;
  unsigned long m,mUsed=0;

  kMac = 6; /* minimum for NORMAL style */
  new_order = osmalloc( kMac*sizeof(datum) );

  get_token(65);
  style=tok_style((char*)buffer);
  if (style==STYLE_UNKNOWN) {
    error(65,showandskipline,NULL,(int)strlen((sz)buffer));
    return;
  }
  pcs->Style=fn[style-1];
#ifdef SVX_MULTILINEDATA
  m=mask[style-1]|BIT(Back)|BIT(Next)|BIT(Ignore)|BIT(IgnoreAll)|BIT(End);
#else
  m=mask[style-1]|BIT(Ignore)|BIT(IgnoreAll)|BIT(End);
#endif

  skipblanks();
  /* olde syntax had optional field for survey grade, so allow an omit */
  if (isOmit(ch))
    nextch();

  do {
    get_token(68);
    d=match_tok(dtab,TABSIZE(dtab),(sz)buffer);
    if (((m>>d)&1)==0) {
      /* token not valid for this data style */
      error(63,showandskipline,NULL,(int)strlen((sz)buffer));
      return;
    }
#ifdef SVX_MULTILINEDATA
    /* Check for duplicates unless it's a special datum:
     *   IGNORE,NEXT (duplicates allowed)
     *   END,IGNOREALL (not possible)
     */
    if (d==Next && (mUsed & BIT(Back))) {
      /* !HACK! "... back ... next ..." not allowed */
    }
#define mask_dup_ok (BIT(Ignore)|BIT(End)|BIT(IgnoreAll)|BIT(Next))
    if (!(mask_dup_ok & BIT(d))) {
#else 
    /* check for duplicates unless it's IGNORE (duplicates allowed) */
    /* or End/IGNOREALL (not possible) */
    if (d!=Ignore && d!=End && d!=IgnoreAll) {
/*
      cRealData++;
      if (cRealData>cData) {
        error(66,showandskipline,NULL,(int)strlen((sz)buffer));
        return;
      }
*/
#endif
      if (mUsed & BIT(d)) {
        error(67,showandskipline,NULL,(int)strlen((sz)buffer));
        return;
      }
      mUsed |= BIT(d); /* used to catch duplicates */
    }
    if (k>=kMac) {
      kMac=kMac*2;
      new_order=osrealloc( new_order, kMac*sizeof(datum) );
    }
    new_order[k++]=d;
#define mask_done (BIT(End)|BIT(IgnoreAll))
  } while (!(mask_done & BIT(d)));

  if ( (mUsed | mask_optional[style-1]) != mask[style-1] ) {
    osfree(new_order);
    error(64,showandskipline,NULL,1); /* too few */
    return;
  }
  if (pcs->fOrderingFreeable)
    osfree(pcs->ordering);
  pcs->ordering=new_order;
  pcs->fOrderingFreeable=fTrue;
}

/* masks for units which are length and angles respectively */
#define LEN_UMASK (BIT(UNITS_METRES)|BIT(UNITS_FEET)|BIT(UNITS_YARDS))
#define ANG_UMASK (BIT(UNITS_DEGS)|BIT(UNITS_GRADS)|BIT(UNITS_MINUTES))

/* ordering must be the same as the units enum */
static real factor_tab[]={1.0,METRES_PER_FOOT,(METRES_PER_FOOT*3.0),
                          (PI/180.0),(PI/200.0),0.0,(PI/180.0/60.0)};

void units( void ) {
  int units, quantity;
  ulong qmask, m; /* mask with bit x set to indicate quantity x specified */
  real factor;
  qmask=get_qlist();
  if (qmask==0) {
    badquantity:
    error(34,showandskipline,NULL,(int)strlen((sz)buffer));
    return;
  }
  /* If factor given then read units else units in buffer already */
  factor=read_numeric( fTrue );
  if (factor==HUGE_REAL) {
    factor=(real)1.0;
    /* If factor given then read units else units in buffer already */
  } else {
    if (*buffer!='\0') /* eg *UNITS LENGTH BOLLOX 3.5 METRES */
      goto badquantity;
    get_token(35); /* get units */
  }
  units=match_units();
  factor*=factor_tab[units];

  if ( ((qmask & LEN_QMASK) && !(BIT(units)&LEN_UMASK)) ||
       ((qmask & ANG_QMASK) && !(BIT(units)&ANG_UMASK)) ) {
    error(37,showandskipline,NULL,(int)strlen((sz)buffer));
    return;
  }
  if (qmask & (BIT(Q_LENGTHOUTPUT)|BIT(Q_ANGLEOUTPUT)) ) {
    NOT_YET;
  }
  for( quantity=0, m=BIT(quantity) ; m<=qmask ; quantity++, m<<=1 )
    if (qmask & m)
      pcs->units[quantity]=factor;
}

void calibrate( void ) {
  real  sc,z; /* added so we don't modify current values if error given */
  ulong qmask, m;
  int quantity;
  qmask=get_qlist();
  if (qmask==0) {
    error(34,showandskipline,NULL,(int)strlen((sz)buffer));
    return;
  }
  /* useful check? */
  if ( ((qmask & LEN_QMASK)) && ((qmask & ANG_QMASK)) ) {
    /* mixed angles/lengths */
  }
  /* check for things with no valid calibration (like station posn) !HACK! */
/*  error(39,showandskipline,NULL,(int)strlen((sz)buffer));
    return; */
  z=read_numeric(fFalse);
  sc=read_numeric(fTrue);
  /* check for declination scale !HACK! */
  /* error(40,showandskipline,NULL,0); */
  if (sc==HUGE_REAL)
    sc=(real)1.0;
  for( quantity=0, m=BIT(quantity) ; m<=qmask ; quantity++, m<<=1 )
    if (qmask & m) {
      pcs->z[quantity]=pcs->units[quantity]*z;
      pcs->sc[quantity]=sc;
    }
}

#define EQ(S) streq((char*)buffer,(S))
void set_default( void ) {
  get_token(36);
  if (EQ("ALL")) {
    NOT_YET;
  } else if (EQ("CALIBRATE")) {
    default_calib(pcs);
  } else if (EQ("DATA")) {
    default_style(pcs);default_grade(pcs);
  } else if (EQ("GRADE")) {
    NOT_YET;
  } else if (EQ("UNITS")) {
    default_units(pcs);
  } else {
    error(41,showandskipline,NULL,(int)strlen((sz)buffer));
  }
}
#undef EQ

/* SETJMP_FIX doesn't -- the problem was elsewhere */
#define SETJMP_FIX 0
void include( sz pth ) {
#if SETJMP_FIX
  char *fnm;
#else
  char   fnm[FILENAME_MAX];
#endif
  int    c;
  parse  file_store;
  prefix *root_store;
  int    ch_store;
  bool   fQuoted=fFalse;
#if SETJMP_FIX
  fnm=osmalloc(FILENAME_MAX);
#endif
  skipblanks();
  if (ch=='\"') {
    fQuoted=fTrue;
    nextch();
  }
  for( c=0 ; !isEol(ch) ; ) {
    if (ch == (fQuoted ? '\"' : ' ') ) {
      nextch();
      fQuoted=fFalse;
      break;
    }
    if (c>=FILENAME_MAX) {
      error(32,showandskipline,NULL,c);
      return;
    }
    fnm[c++]=ch;
    nextch();
  }

  if (fQuoted) {
    error(69,showandskipline,NULL,c);
    return;
  }
  fnm[c]='\0';
  root_store=root;
  root=pcs->Prefix; /* Root for include file is current prefix */
  file_store=file;
  ch_store=ch;
  data_file( pth, fnm );
  root=root_store; /* and restore root */
  file=file_store;
  ch=ch_store;

#if SETJMP_FIX
  osfree(fnm);
#endif
}

extern void set_sd(void) {
  real sd, variance;
  int units;
  ulong qmask, m;
  int quantity;
  qmask=get_qlist();
  if (qmask==0) {
    error(34,showandskipline,NULL,(int)strlen((sz)buffer));
    return;
  }
  sd=read_numeric(fFalse);
  if (sd<(real)0.0) {
    /* complain about negative sd !HACK! */
  }
  get_token(35);
  units=match_units();
  if ( ((qmask & LEN_QMASK) && !(BIT(units)&LEN_UMASK)) ||
       ((qmask & ANG_QMASK) && !(BIT(units)&ANG_UMASK)) ) {
    error(37,showandskipline,NULL,(int)strlen((sz)buffer));
    return;
  }
  sd*=factor_tab[units];
  variance = sqrd(sd);
  for( quantity=0, m=BIT(quantity) ; m<=qmask ; quantity++, m<<=1 )
    if (qmask & m)
      pcs->Var[quantity] = variance;
}
