/* > readval.c
 * Read a datum from the current input file
 * Copyright (C) 1991-1994,1996,1997 Olly Betts
 */

/*
1992.07.22 Fixed bug which skipped '.' in prefix if ident was exactly
           IDENT_LEN chars long
1992.07.29 Case now forced properly
1992.07.29 special chars now processed better
1992.07.30 Unique now handled properly
1992.07.31 Clino of V *not* treated as +V any more
1992.10.25 Code tidied
1992.10.28 Nicko solved 'Loss of precision in wider context' warning
1993.01.17 Cured Wookey-spotted bug concerning digits after dec pt in
           read_numeric_default() and read_numeric_omit()
1993.01.20 Could copy cs.Unique+1 for prefix - cured
1993.01.20 Filename -> c.ReadVal
1993.01.25 PC Compatibility OKed
1993.02.28 prettied indenting
1993.03.04 x=f(); while(g(x)) { ... ; x=f(); } -> while(g(x=f())) { ... }
1993.04.08 error.h
1993.04.22 out of memory -> error # 0
           isxxxx() macros -> isXxxx() to avoid possible collision with
             future extensions to C
1993.06.04 FALSE -> fFalse
1993.06.11 n=n*10.0+... -> n=n*10.0f+... to suppress warnings if real = float
1993.06.16 malloc() -> osmalloc(); syserr() -> fatal()
1993.07.03 (WP) added (int) casts in front of cs.Unique to suppress warning
1993.07.17 changed uses of fgetpos/fsetpos to ftell/fseek
1993.08.11 added NEW_NODES
1993.08.13 fettled header
1993.08.15 sort each node of prefix tree left-right for speed and neatness
           added SMALL to make read_prefix use read_prefix_default
           deleted id_eq (no longer used)
           moved ident_cpy macro here, and use memcpy instead
1993.11.03 changed error routines
1993.11.08 only NEW_NODES code left
1993.11.10 merged read_prefix_default into read_prefix
           merged read_numeric_omit into read_numeric
           ... and read_numeric_default
           removed fSign from read_numeric_UD()
           fixed entrance.7. not being faulted bug (!)
           removed fungetc()s from read_prefix
           added isSign()
1993.11.30 read_prefix now returns on failure, but gives error if !fOmit
           so do read_numeric_UD and read_numeric (unless fDefault is set)
1993.12.01 now use longjmp in case of non-fatal errors
1993.12.16 added warning if ident truncated
1994.03.19 deleted duplicated call to error()
1994.04.16 created readval.h; created errorjmp macro
1994.04.21 added TRIM_PREFIX, to allocate only space needed for ident
1994.04.27 initialise cmp to fix potential bug; jbSkipLine is now in datain.h
1994.05.09 removed some stuff duplicated in datain.h
1994.05.12 #if0 -> #if 0
1994.06.18 showline calls now give area of error if appropriate
           errorjmp no longer wants a function name
1994.06.21 added int argument to warning, error and fatal
1994.06.26 cs -> pcs
1994.06.29 added skipblanks()
1994.09.19 clino reading of LEVEL or H => "0 & don't apply clino adjustment"
1994.10.08 tidied up read_prefix; added osnew()
1994.11.13 rearranged read_numeric(), removed read_numeric_UD
1996.03.24 parse structure introduced
1996.04.15 SPECIAL_POINT -> SPECIAL_DECIMAL
1996.05.05 removed spaces from before preprocessor directives
1997.08.24 removed limit on survey station name length
1998.03.21 fixed up to compile cleanly on Linux
*/

#include <stddef.h>

#include "cavein.h"
#include "filename.h"
#include "message.h"
#include "readval.h"
#include "datain.h"
#include "osalloc.h"

#if 1
# define ident_cmp strcmp
#else
/* We ideally want a collating order when "10" sorts before "2"
 * (and "10a" sorts between "10" and "11").
 * So we want an compare which is reflexive, transitive, and
 * anti-symmetric.  We also want "001" and "1" to not be equal...
 */
/* FIXME: badly flawed - 001 and 1 are regarded as the same for example */
/* ident_cmp returns -ve for <, 0 for =, +ve for > (like strcmp) */     
int ident_cmp(const char *a, const char *b) {
   if (isdigit(a[0])) {
      int i, j;
      if (!isdigit(b[0])) return -1;
      /*FIXME check errno, etc in case out of range*/
      i = strtoul(a, &a, 10);
      i -= strtoul(b, &b, 10);
      if (i) return i;
      }
   } else if (isdigit(b[0])) {
      return 1;
   }
   return strcmp(a,b);
}
#endif

/* Dinky macro to handle any case forcing needed */
#define docase(X) (pcs->Case==OFF?(X):(pcs->Case==UPPER?toupper(X):tolower(X)))

/* if prefix is omitted: if fOmit return NULL, otherwise use longjmp */
extern prefix *read_prefix( bool fOmit ) {
  prefix *back_ptr,*ptr;
  char *name;
  size_t name_len = 32;
  int    i;

  skipblanks();
  if (isRoot(ch)) {
    nextch();
    ptr=root;
    if (!isNames(ch)) {
      if (!isSep(ch))
        return ptr;
      nextch();
    }
  } else {
    ptr=pcs->Prefix;
  }

  i=0;
  do {
    /* get a new name buffer for each level */
    name = osmalloc( name_len );
    if (i) /* i==0 iff this is the first pass */ {
      i=0;
      nextch();
    }
    while (isNames(ch)) {
      if (i < pcs->Truncate) { /* truncate name? */
         name[i++] = docase(ch);
	 if (i >= name_len) {
	    name_len = name_len + name_len;
	    name = osrealloc( name, name_len );
	 }
      }
      nextch();
    }
    if (i==0) {
      if (!fOmit)
        errorjmp(7,1,file.jbSkipLine);
      osfree(name);
      return (prefix *)NULL;
    }

    if (i >= pcs->Truncate)
      warning(42,showline,NULL,i-pcs->Truncate);

    name[i++]='\0';
    name = osrealloc( name, i );
     
    back_ptr=ptr;
    ptr=ptr->down;
    if (ptr==NULL) { /* Special case first time around at each level */
      ptr=osnew(prefix);
      ptr->ident = name;
      ptr->right=NULL;
      ptr->down=NULL;
      ptr->pos=NULL;
      ptr->stn=NULL;
      ptr->up=back_ptr;
      back_ptr->down=ptr;
    } else {
      prefix *ptrPrev=NULL;
      int cmp = 1; /* result of ident_cmp ( -ve for <, 0 for =, +ve for > ) */
      while ((ptr!=NULL) && (cmp = ident_cmp(ptr->ident, name))<0) {
        ptrPrev=ptr;
        ptr=ptr->right;
      }
      if (cmp) /* ie we got to one that was higher, or the end */ {
        prefix *new;
        new=osnew(prefix);
	new->ident = name;
        if (ptrPrev==NULL)
          back_ptr->down=new;
        else
          ptrPrev->right=new;
        new->right=ptr;
        new->down=NULL;
        new->pos=NULL;
        new->stn=NULL;
        new->up=back_ptr;
        ptr=new;
      }
    }
    fOmit=fFalse; /* disallow after first level */
  } while (isSep(ch));
  return ptr;
}

/* if numeric expr is omitted: if fOmit return HUGE_REAL, else longjmp */
extern real read_numeric(bool fOmit) {
  bool fPositive,fDigits=fFalse;
  real n=(real)0.0;
  long int fp;
  int  chOld;

  skipblanks();
  fp=ftell(file.fh);
  chOld=ch;
  fPositive=!isMinus(ch);
  if (isSign(ch)) /* ie (isMinus(ch)||isPlus(ch)) */
    nextch();

  while (isdigit(ch)) {
    n=n*10.0f+(char)(ch-'0');
    nextch();
    fDigits=fTrue;
  }

  if (isDecimal(ch)) {
    real   mult=(real)1.0;
    nextch();
    while (isdigit(ch)) {
      mult*=(real).1;
      n+=(char)(ch-'0')*mult;
      fDigits=fTrue;
      nextch();
    }
  }

  /* !'fRead' => !fDigits so fDigits => 'fRead' */
  if (fDigits)
    return (fPositive ? n : -n);

  /* didn't read a valid number.  If it's optional, reset filepos & return */
  if (fOmit) {
    fseek( file.fh, fp, SEEK_SET );
    ch=chOld;
    return HUGE_REAL;
  }

  if (isOmit(chOld))
    errorjmp(8,1,file.jbSkipLine);
  errorjmp(9,1,file.jbSkipLine);
  return 0.0; /* for brain-fried compilers */
}
