/* > readval.c
 * Read a datum from the current input file
 * Copyright (C) 1991-2000 Olly Betts
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
# include <config.h>
#endif

#include "cavern.h"
#include "filename.h"
#include "message.h"
#include "readval.h"
#include "datain.h"
#include "osalloc.h"
#ifdef NEW3DFORMAT
#include "new3dout.h"
#endif

#ifdef HAVE_SETJMP
# define LONGJMP(JB) longjmp((JB), 1)
#else
# define LONGJMP(JB) exit(1)
#endif

/* ident_cmp returns -ve for <, 0 for =, +ve for > (like strcmp) */     
#if 1
# define ident_cmp strcmp
#else
/* We ideally want a collating order where "2" sorts before "10"
 * (and "10a" sorts between "10" and "11").
 * So we want an compare cmp(A,B) s.t.
 * 1) cmp(A,A) = 0
 * 2) cmp(A,B) < 0 iff cmp(B,A) > 0
 * 3) cmp(A,B) > 0 and cmp(B,C) > 0 => cmp(A,C) > 0
 * 4) cmp(A,B) = 0 iff strcmp(A,B) = 0 (e.g. "001" and "1" are not equal)
 */
/* FIXME: is this ordering OK? */
/* Would also be nice if "xxx2" sorted before "xxx10"... */
int
ident_cmp(const char *a, const char *b)
{
   if (isdigit(a[0])) {
      int i;
      if (!isdigit(b[0])) return -1;
      /* FIXME: check errno, etc in case out of range */
      i = strtoul(a, NULL, 10);
      i -= strtoul(b, NULL, 10);
      if (i) return i;
   } else if (isdigit(b[0])) {
      return 1;
   }
   return strcmp(a,b);
}
#endif

/* Dinky macro to handle any case forcing needed */
#define docase(X) (pcs->Case == OFF ? (X) :\
                   (pcs->Case == UPPER ? toupper(X) : tolower(X)))

/* if prefix is omitted: if fOmit return NULL, otherwise use longjmp */
static prefix *
read_prefix_(bool fOmit, bool fSuspectTypo)
{
   prefix *back_ptr, *ptr;
   char *name;
   size_t name_len = 32;
   int i;
   bool fNew;
   bool fImplicitPrefix = fTrue;

   skipblanks();
   if (isRoot(ch)) {
      nextch();
      ptr = root;
      if (!isNames(ch)) {
	 if (!isSep(ch)) return ptr;
	 nextch();
      }
      fImplicitPrefix = fFalse;
   } else {
      ptr = pcs->Prefix;
   }

   i = 0;
   do {
      fNew = fFalse;
      if (isSep(ch)) fImplicitPrefix = fFalse;
      /* get a new name buffer for each level */
      name = osmalloc(name_len);
      /* i==0 iff this is the first pass */
      if (i) {
	 i = 0;
	 nextch();
      }
      while (isNames(ch)) {
	 if (i < pcs->Truncate) {
	    /* truncate name */
	    name[i++] = docase(ch);
	    if (i >= name_len) {
	       name_len = name_len + name_len;
	       name = osrealloc(name, name_len);
	    }
	 }
	 nextch();
      }
      if (i == 0) {
	 osfree(name);
	 if (!fOmit) {
	    compile_error(/*Character `%c' not allowed in station name (use *SET NAMES to set allowed characters)*/7, ch);
	    skipline();
	    LONGJMP(file.jbSkipLine);
	 }
	 return (prefix *)NULL;
      }

      name[i++]='\0';
      name = osrealloc(name, i);
     
      back_ptr = ptr;
      ptr = ptr->down;
      if (ptr == NULL) {
	 /* Special case first time around at each level */
	 ptr = osnew(prefix);
	 ptr->ident = name;
	 ptr->right = ptr->down = NULL;
	 ptr->pos = NULL;
	 ptr->stn = NULL;
	 ptr->up = back_ptr;
	 ptr->filename = NULL;
	 /* FIXME: what if foo.1 is a station as well as a prefix?!? */
	 ptr->fSuspectTypo = fSuspectTypo && !fImplicitPrefix;
	 back_ptr->down = ptr;
	 fNew = fTrue;
#ifdef NEW3DFORMAT
	 if (fUseNewFormat) {
	    create_twig(ptr,file.filename);
	 }
#endif
      } else {
	 prefix *ptrPrev = NULL;
	 int cmp = 1; /* result of ident_cmp ( -ve for <, 0 for =, +ve for > ) */
	 while (ptr && (cmp = ident_cmp(ptr->ident, name))<0) {
	    ptrPrev = ptr;
	    ptr = ptr->right;
	 }
	 if (cmp) {
	    /* ie we got to one that was higher, or the end */
	    prefix *newptr = osnew(prefix);
	    newptr->ident = name;
	    if (ptrPrev == NULL)
	       back_ptr->down = newptr;
	    else
	       ptrPrev->right = newptr;
	    newptr->right = ptr;
	    newptr->down = NULL;
	    newptr->pos = NULL;
	    newptr->stn = NULL;
	    newptr->up = back_ptr;
	    newptr->filename = NULL;
	    newptr->fSuspectTypo = !fImplicitPrefix;
	    ptr = newptr;
	    fNew = fTrue;
#ifdef NEW3DFORMAT
	    if (fUseNewFormat) {
	       create_twig(ptr,file.filename);
	    }
#endif
	 }
      }
      fOmit = fFalse; /* disallow after first level */
   } while (isSep(ch));
   /* don't warn about a station is refered to twice */
   if (!fNew) ptr->fSuspectTypo = fFalse;
   return ptr;
}

/* if prefix is omitted: if fOmit return NULL, otherwise use longjmp */
extern prefix *
read_prefix(bool fOmit)
{
   return read_prefix_(fOmit, fFalse);
}

/* if prefix is omitted: if fOmit return NULL, otherwise use longjmp */
/* Same as read_prefix but implicit checks are made */
extern prefix *
read_prefix_check_implicit(bool fOmit)
{
   return read_prefix_(fOmit, fTrue);
}

/* if numeric expr is omitted: if fOmit return HUGE_REAL, else longjmp */
extern real
read_numeric(bool fOmit)
{
   bool fPositive, fDigits = fFalse;
   real n = (real)0.0;
   long int fp;
   int chOld;

   skipblanks();
   fp = ftell(file.fh);
   chOld = ch;
   fPositive = !isMinus(ch);
   if (isSign(ch)) nextch();

   while (isdigit(ch)) {
      n = n * 10.0f + (char)(ch - '0');
      nextch();
      fDigits = fTrue;
   }

   if (isDecimal(ch)) {
      real mult = (real)1.0;
      nextch();
      while (isdigit(ch)) {
	 mult *= (real).1;
	 n += (char)(ch - '0') * mult;
	 fDigits = fTrue;
	 nextch();
      }
   }

   /* !'fRead' => !fDigits so fDigits => 'fRead' */
   if (fDigits) return (fPositive ? n : -n);

   /* didn't read a valid number.  If it's optional, reset filepos & return */
   if (fOmit) {
      fseek(file.fh, fp, SEEK_SET);
      ch = chOld;
      return HUGE_REAL;
   }

   if (isOmit(chOld)) {
      compile_error(/*Field may not be omitted*/8);
   } else {
      compile_error(/*Expecting numeric field*/9);
   }
   showandskipline(NULL, 1);
   LONGJMP(file.jbSkipLine);
   return 0.0; /* for brain-fried compilers */
}

extern unsigned int
read_uint(void)
{
   unsigned int n = 0;
   skipblanks();
   if (!isdigit(ch)) {
      compile_error(/*Expecting numeric field*/9);
      showandskipline(NULL, 1);
      LONGJMP(file.jbSkipLine);
   }
   while (isdigit(ch)) {
      n = n * 10 + (char)(ch - '0');
      nextch();
   }
   return n;
}
