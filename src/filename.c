#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

/* OS dependent filename manipulation routines */
#include "filename.h"
#include <string.h>

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
		      /*Failed to open output file*/47 :
		      /*Couldn't open data file*/24, fnm);
   return f;
}

FILE *
fopen_not_dir(const char *fnm, const char *mode)
{
   if (fDirectory(fnm)) return NULL;
   return fopen(fnm, mode);
}

extern char * FAR
PthFromFnm(const char *fnm)
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

extern char * FAR
LfFromFnm(const char *fnm)
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
UsePth(const char *pth, const char *lf)
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
AddExt(const char *fnm, const char *ext)
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
	    fnmFull = AddExt(fnm, szExt);
	    fh = fopen_not_dir(fnmFull, szMode);
	 }
      }
   } else {
      /* try using path given - first of all without the extension */
      fnmFull = UsePth(pth, fnm);
      fh = fopen_not_dir(fnmFull, szMode);
      if (!fh) {
	 if (szExt && *szExt) {
	    /* we've been given an extension so try using it */
	    char *fnmTmp;
	    fnmTmp = fnmFull;
	    fnmFull = AddExt(fnmFull, szExt);
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
