/* OS dependent filename manipulation routines
 * Copyright (c) Olly Betts 1998-2000
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

#include "filename.h"

#include <ctype.h>
#include <string.h>

/* FIXME: finish sorting out safe_fopen vs fopenWithPthAndExt... */

/* Wrapper for fopen which throws a fatal error if it fails.
 * Some versions of fopen() are quite happy to open a directory.
 * We aren't, so catch this case. */
extern FILE *
safe_fopen(const char *fnm, const char *mode)
{
   FILE *f;
   if (fDirectory(fnm))
      fatalerror(/*Filename '%s' refers to directory*/44, fnm);

   f = fopen(fnm, mode);
   if (!f) fatalerror(mode[0] == 'w' ?
		      /*Failed to open output file '%s'*/47 :
		      /*Couldn't open data file '%s'*/24, fnm);
   return f;
}

extern FILE *
safe_fopen_with_ext(const char *fnm, const char *ext, const char *mode)
{
   FILE *f;
   char *p;
   p = add_ext(fnm, ext);
   f = safe_fopen(p, mode);
   osfree(p);
   return f;
}

static FILE *
fopen_not_dir(const char *fnm, const char *mode)
{
   if (fDirectory(fnm)) return NULL;
   return fopen(fnm, mode);
}

extern char * FAR
path_from_fnm(const char *fnm)
{
   char *pth;
   const char *lf;
   int lenpth = 0;

   lf = strrchr(fnm, FNM_SEP_LEV);
#ifdef FNM_SEP_LEV2
   if (!lf) lf = strrchr(fnm, FNM_SEP_LEV2);
#endif
#ifdef FNM_SEP_DRV
   if (!lf) lf = strrchr(fnm, FNM_SEP_DRV);
#endif
   if (lf) lenpth = lf - fnm + 1;

   pth = osmalloc(lenpth + 1);
   memcpy(pth, fnm, lenpth);
   pth[lenpth] = '\0';

   return pth;
}

extern char *
base_from_fnm(const char *fnm)
{
   char *p;
   
   p = strrchr(fnm, FNM_SEP_EXT);
   /* Trim off any leaf extension, but dirs can have extensions too */
   if (p && !strrchr(p, FNM_SEP_LEV)
#ifdef FNM_SEP_LEV2
       && !strrchr(p, FNM_SEP_LEV2)
#endif
       ) {
      size_t len = (const char *)p - fnm;

      p = osmalloc(len + 1);
      memcpy(p, fnm, len);
      p[len] = '\0';
      return p;
   }

   return osstrdup(fnm);
}

extern char *
baseleaf_from_fnm(const char *fnm)
{
   const char *p;
   char *q;
   size_t len;
   
   p = fnm;
   q = strrchr(p, FNM_SEP_LEV);
   if (q) p = q + 1;
#ifdef FNM_SEP_LEV2
   q = strrchr(p, FNM_SEP_LEV2);
   if (q) p = q + 1;
#endif
   
   q = strrchr(p, FNM_SEP_EXT);
   if (q) len = (const char *)q - p; else len = strlen(p);

   q = osmalloc(len + 1);
   memcpy(q, p, len);
   q[len] = '\0';
   return q;
}

extern char * FAR
leaf_from_fnm(const char *fnm)
{
   char *lf;
   lf = strrchr(fnm, FNM_SEP_LEV);
   if (lf != NULL
#ifdef FNM_SEP_LEV2
       || (lf = strrchr(fnm, FNM_SEP_LEV2)) != NULL
#endif
#ifdef FNM_SEP_DRV
       || (lf = strrchr(fnm, FNM_SEP_DRV)) != NULL
#endif
       ) {
      return osstrdup(lf + 1);
   }
   return osstrdup(fnm);
}

/* Make fnm from pth and lf, inserting an FNM_SEP_LEV if appropriate */
extern char * FAR
use_path(const char *pth, const char *lf)
{
   char *fnm;
   int len, len_total;
   bool fAddSep = fFalse;

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

   fnm = osmalloc(len_total);
   strcpy(fnm, pth);
   if (fAddSep) fnm[len++] = FNM_SEP_LEV;
   strcpy(fnm + len, lf);
   return fnm;
}

/* Add ext to fnm, inserting an FNM_SEP_EXT if appropriate */
extern char * FAR
add_ext(const char *fnm, const char *ext)
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

   fnmNew = osmalloc(len_total);
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
extern FILE FAR *
fopenWithPthAndExt(const char * pth, const char * fnm, const char * szExt,
		   const char * szMode, char **pfnmUsed)
{
   char *fnmFull = NULL;
   FILE *fh = NULL;
   bool fAbs;

   /* if no pth treat fnm as absolute */
   fAbs = (pth == NULL || *pth == '\0' || fAbsoluteFnm(fnm));

   /* if appropriate, try it without pth */
   if (fAbs) {
      fh = fopen_not_dir(fnm, szMode);
      if (fh) {
	 if (pfnmUsed) fnmFull = osstrdup(fnm);
      } else {
	 if (szExt && *szExt) {
	    /* we've been given an extension so try using it */
	    fnmFull = add_ext(fnm, szExt);
	    fh = fopen_not_dir(fnmFull, szMode);
	 }
      }
   } else {
      /* try using path given - first of all without the extension */
      fnmFull = use_path(pth, fnm);
      fh = fopen_not_dir(fnmFull, szMode);
      if (!fh) {
	 if (szExt && *szExt) {
	    /* we've been given an extension so try using it */
	    char *fnmTmp;
	    fnmTmp = fnmFull;
	    fnmFull = add_ext(fnmFull, szExt);
	    osfree(fnmTmp);
	    fh = fopen_not_dir(fnmFull, szMode);
	 }
      }
   }

   /* either it opened or didn't. If not, fh == NULL from fopen_not_dir() */

   /* free name if it didn't open or name isn't wanted */
   if (fh == NULL || pfnmUsed == NULL) osfree(fnmFull);
   if (pfnmUsed) *pfnmUsed = (fh ? fnmFull : NULL);
   return fh;
}

/* Like fopenWithPthAndExt except that "foreign" paths are translated to
 * native ones (e.g. on Unix dir\file.ext -> dir/file.ext) */
FILE *
fopen_portable(const char *pth, const char *fnm, const char *ext,
	       const char *mode, char **pfnmUsed)
{
   FILE *fh = fopenWithPthAndExt(pth, fnm, ext, mode, pfnmUsed);
   if (fh == NULL) {
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
	 fh = fopenWithPthAndExt(pth, fnm_trans, ext, mode, pfnmUsed);

#if (OS==UNIX)
      /* as a last ditch measure, try lowercasing the filename */
      if (fh == NULL) {
	 f_changed = 0;
	 for (p = fnm_trans; *p ; p++)
	    if (isupper(*p)) {
	       *p = tolower(*p);
	       f_changed = 1;
	    }
	 if (f_changed)
	    fh = fopenWithPthAndExt(pth, fnm_trans, ext, mode, pfnmUsed);
      }
#endif
      osfree(fnm_trans);
#endif
   }
   return fh;
}
