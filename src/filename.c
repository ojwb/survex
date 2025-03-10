/* OS dependent filename manipulation routines
 * Copyright (c) Olly Betts 1998-2003,2004,2005,2010,2011,2014
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <config.h>

#include "filename.h"
#include "debug.h"
#include "osalloc.h"

#include <ctype.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>

typedef struct filelist {
   char *fnm;
   FILE *fh;
   struct filelist *next;
} filelist;

static filelist *flhead = NULL;

static void filename_register_output_with_fh(const char *fnm, FILE *fh);

/* fDirectory( fnm ) returns true if fnm is a directory; false if fnm is a
 * file, doesn't exist, or another error occurs (eg disc not in drive, ...)
 * NB If fnm has a trailing directory separator (e.g. “/” or “/home/olly/”
 * then it's assumed to be a directory even if it doesn't exist (as is an
 * empty string).
 */

bool
fDirectory(const char *fnm)
{
   struct stat buf;
   if (!fnm[0] || fnm[strlen(fnm) - 1] == FNM_SEP_LEV
#ifdef FNM_SEP_LEV2
       || fnm[strlen(fnm) - 1] == FNM_SEP_LEV2
#endif
       ) return 1;
   if (stat(fnm, &buf) != 0) return 0;
#ifdef S_ISDIR
   /* POSIX way */
   return S_ISDIR(buf.st_mode);
#else
   /* BSD way */
   return ((buf.st_mode & S_IFMT) == S_IFDIR);
#endif
}

/* safe_fopen should be used when writing a file
 * fopenWithPthAndExt should be used when reading a file
 */

/* Wrapper for fopen which throws a fatal error if it fails.
 * Some versions of fopen() are quite happy to open a directory.
 * We aren't, so catch this case. */
extern FILE *
safe_fopen(const char *fnm, const char *mode)
{
   FILE *f;
   SVX_ASSERT(mode[0] == 'w'); /* only expect to be used for writing */
   if (fDirectory(fnm))
      fatalerror(/*Filename “%s” refers to directory*/5, fnm);

   f = fopen(fnm, mode);
   if (!f) fatalerror(/*Failed to open output file “%s”*/3, fnm);

   filename_register_output_with_fh(fnm, f);
   return f;
}

/* Wrapper for fclose which throws a fatal error if there's been a write
 * error.
 */
extern void
safe_fclose(FILE *f)
{
   SVX_ASSERT(f);
   /* NB: use of | rather than || - we always want to call fclose() */
   if (FERROR(f) | (fclose(f) == EOF)) {
      filelist *p;
      for (p = flhead; p != NULL; p = p->next)
	 if (p->fh == f) break;

      if (p && p->fnm) {
	 const char *fnm = p->fnm;
	 p->fnm = NULL;
	 p->fh = NULL;
	 (void)remove(fnm);
	 fatalerror(/*Error writing to file “%s”*/7, fnm);
      }
      /* f wasn't opened with safe_fopen(), so we don't know the filename. */
      fatalerror(/*Error writing to file*/111);
   }
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

extern char *
path_from_fnm(const char *fnm)
{
   char *pth;
   const char *lf;
   int lenpth = 0;

   lf = strrchr(fnm, FNM_SEP_LEV);
#ifdef FNM_SEP_LEV2
   {
      const char *lf2 = strrchr(lf ? lf + 1 : fnm, FNM_SEP_LEV2);
      if (lf2) lf = lf2;
   }
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
   if (p && !strchr(p, FNM_SEP_LEV)
#ifdef FNM_SEP_LEV2
       && !strchr(p, FNM_SEP_LEV2)
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

extern char *
leaf_from_fnm(const char *fnm)
{
   const char *lf;
   lf = strrchr(fnm, FNM_SEP_LEV);
   if (lf) fnm = lf + 1;
#ifdef FNM_SEP_LEV2
   lf = strrchr(fnm, FNM_SEP_LEV2);
   if (lf) fnm = lf + 1;
#endif
#ifdef FNM_SEP_DRV
   lf = strrchr(fnm, FNM_SEP_DRV);
   if (lf) fnm = lf + 1;
#endif
   return osstrdup(fnm);
}

/* Make fnm from pth and lf, inserting an FNM_SEP_LEV if appropriate */
extern char *
use_path(const char *pth, const char *lf)
{
   char *fnm;
   int len, len_total;
   bool fAddSep = false;

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
	    fAddSep = true;
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
extern char *
add_ext(const char *fnm, const char *ext)
{
   char * fnmNew;
   int len, len_total;
   bool fAddSep = false;

   len = strlen(fnm);
   len_total = len + strlen(ext) + 1;
   if (ext[0] != FNM_SEP_EXT) {
      fAddSep = true;
      len_total++;
   }

   fnmNew = osmalloc(len_total);
   strcpy(fnmNew, fnm);
   if (fAddSep) fnmNew[len++] = FNM_SEP_EXT;
   strcpy(fnmNew + len, ext);
   return fnmNew;
}

#ifdef _WIN32

/* NB "c:fred" isn't relative. Eg "c:\data\c:fred" won't work */
static bool
fAbsoluteFnm(const char *fnm)
{
   /* <drive letter>: or \<path> or /<path>
    * or \\<host>\... or //<host>/... */
   unsigned char ch = (unsigned char)*fnm;
   return ch == '/' || ch == '\\' ||
       (ch && fnm[1] == ':' && (ch | 32) >= 'a' && (ch | 32) <= 'z');
}

#else

static bool
fAbsoluteFnm(const char *fnm)
{
   return (fnm[0] == '/');
}

#endif

/* fopen file, found using pth and fnm
 * fnmUsed is used to return filename used to open file (ignored if NULL)
 * or NULL if file didn't open
 */
extern FILE *
fopenWithPthAndExt(const char *pth, const char *fnm, const char *ext,
		   const char *mode, char **fnmUsed)
{
   char *fnmFull = NULL;
   FILE *fh = NULL;

   /* Don't try to use pth if it is unset or empty, or if the filename is
    * already absolute.
    */
   if (pth == NULL || *pth == '\0' || fAbsoluteFnm(fnm)) {
      fh = fopen_not_dir(fnm, mode);
      if (fh) {
	 if (fnmUsed) fnmFull = osstrdup(fnm);
      } else {
	 if (ext && *ext) {
	    /* we've been given an extension so try using it */
	    fnmFull = add_ext(fnm, ext);
	    fh = fopen_not_dir(fnmFull, mode);
	 }
      }
   } else {
      /* try using path given - first of all without the extension */
      fnmFull = use_path(pth, fnm);
      fh = fopen_not_dir(fnmFull, mode);
      if (!fh) {
	 if (ext && *ext) {
	    /* we've been given an extension so try using it */
	    char *fnmTmp;
	    fnmTmp = fnmFull;
	    fnmFull = add_ext(fnmFull, ext);
	    osfree(fnmTmp);
	    fh = fopen_not_dir(fnmFull, mode);
	 }
      }
   }

   /* either it opened or didn't. If not, fh == NULL from fopen_not_dir() */

   /* free name if it didn't open or name isn't wanted */
   if (fh == NULL || fnmUsed == NULL) osfree(fnmFull);
   if (fnmUsed) *fnmUsed = (fh ? fnmFull : NULL);
   return fh;
}

/* Like fopenWithPthAndExt except that "foreign" paths are translated to
 * native ones (e.g. on Unix dir\file.ext -> dir/file.ext) */
FILE *
fopen_portable(const char *pth, const char *fnm, const char *ext,
	       const char *mode, char **fnmUsed)
{
   FILE *fh = fopenWithPthAndExt(pth, fnm, ext, mode, fnmUsed);
   if (fh == NULL) {
#ifndef _WIN32
      bool changed = false;
      char *fnm_trans = osstrdup(fnm);
      for (char *p = fnm_trans; *p; p++) {
	 switch (*p) {
	 case '\\': /* swap a backslash to a forward slash */
	    *p = '/';
	    changed = true;
	    break;
	 }
      }
      if (changed)
	 fh = fopenWithPthAndExt(pth, fnm_trans, ext, mode, fnmUsed);

      /* To help users process data that originated on a case-insensitive
       * filing system, try lowercasing the filename if not found.
       */
      if (fh == NULL) {
	 bool had_lower = false;
	 changed = false;
	 for (char *p = fnm_trans; *p ; p++) {
	    unsigned char ch = *p;
	    if (isupper(ch)) {
	       *p = tolower(ch);
	       changed = true;
	    } else if (islower(ch)) {
	       had_lower = true;
	    }
	 }
	 if (changed)
	    fh = fopenWithPthAndExt(pth, fnm_trans, ext, mode, fnmUsed);

	 /* If that fails, try upper casing the initial character of the leaf. */
	 if (fh == NULL) {
	    char *leaf = strrchr(fnm_trans, '/');
	    leaf = (leaf ? leaf + 1 : fnm_trans);
	    if (islower((unsigned char)*leaf)) {
	       *leaf = toupper((unsigned char)*leaf);
	       fh = fopenWithPthAndExt(pth, fnm_trans, ext, mode, fnmUsed);
	    }
	    if (fh == NULL && had_lower) {
	       /* Finally, try upper casing the filename if it wasn't all
		* upper case to start with. */
	       for (char *p = fnm_trans; *p ; p++) {
		  *p = toupper((unsigned char)*p);
	       }
	       fh = fopenWithPthAndExt(pth, fnm_trans, ext, mode, fnmUsed);
	    }
	 }
      }
      osfree(fnm_trans);
#endif
   }
   return fh;
}

void
filename_register_output(const char *fnm)
{
   filelist *p = osnew(filelist);
   SVX_ASSERT(fnm);
   p->fnm = osstrdup(fnm);
   p->fh = NULL;
   p->next = flhead;
   flhead = p;
}

static void
filename_register_output_with_fh(const char *fnm, FILE *fh)
{
   filelist *p = osnew(filelist);
   SVX_ASSERT(fnm);
   p->fnm = osstrdup(fnm);
   p->fh = fh;
   p->next = flhead;
   flhead = p;
}

void
filename_delete_output(void)
{
   while (flhead) {
      filelist *p = flhead;
      flhead = flhead->next;
      if (p->fnm) {
	 (void)remove(p->fnm);
	 osfree(p->fnm);
      }
      osfree(p);
   }
}
