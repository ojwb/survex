/* > osalloc.h
 * Function prototypes for OS dep. malloc &c - funcs in error.c
 * Copyright (C) 1996,1997 Olly Betts
 */

#ifndef OSALLOC_H /* only include once */
#define OSALLOC_H

/* define TOMBSTONES to enable tombstones on malloc blocks
 * for bounds checking */
/*#define TOMBSTONES 1*/

#include "osdepend.h"

/* OSSIZE_T is to osmalloc, etc what size_t is to malloc, etc */
#ifdef NO_FLATDOS
# include "alloc.h"
# define osfree(p) farfree((p))
# define xosmalloc(s) farmalloc((s))
# define xosrealloc(p, s) farrealloc((p), (s))
# define OSSIZE_T long
#else
# ifndef TOMBSTONES
#  define osfree(p) free((p))
#  define xosmalloc(s) malloc((s))
#  define xosrealloc(p, s) realloc((p), (s))
# else
extern void osfree(void *p);
/* ick: */
#  define xosmalloc(s) osmalloc((s))
#  define xosrealloc(p, s) osrealloc((p), (s))
# endif
# define OSSIZE_T size_t
#endif

/* NB No extra () around X as sizeof((char*)) doesn't work */
#define ossizeof(X) ((OSSIZE_T)sizeof(X))
/* C++ like malloc thingy -- call osnew(<type>) eg. osnew(float) */
#define osnew(T) (T*)osmalloc(ossizeof(T))

extern void FAR *osmalloc(OSSIZE_T);
extern void FAR *osrealloc(void *, OSSIZE_T);

extern void FAR *osstrdup(const char *str);

#endif
