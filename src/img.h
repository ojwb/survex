/* > img.h
 * Header file for routines to read and write Survex ".3d" image files
 * Copyright (C) Olly Betts 1993,1994,1997
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef IMG_H
# define IMG_H

# ifndef STANDALONE
#  include "useful.h"
# endif

# define img_BAD   -2
# define img_STOP  -1
# define img_MOVE   0
# define img_LINE   1
# define img_CROSS  2
# define img_LABEL  3
# define img_SCALE  4 /* ignore this one for now */

/* internal structure -- don't access members directly */
typedef struct {
   FILE *fh;          /* file handle of image file */
# ifndef STANDALONE
   bool fBinary;      /* fTrue for binary file format, fFalse for ASCII */
   bool fLinePending; /* for old style text format files */
   bool fRead;        /* fTrue for reading, fFalse for writing */
# else
   int fBinary;      /* fTrue for binary file format, fFalse for ASCII */
   int fLinePending; /* for old style text format files */
   int fRead;        /* fTrue for reading, fFalse for writing */
# endif
} img;

/* Open a .3d file for input
 * fnm is the filename
 * szTitle and szDateStamp should be buffers to read the title and
 *   datestamp into
 * Returns pointer to an img struct or NULL
 */
img *img_open(const char *fnm, char *szTitle, char *szDateStamp);

/* Open a .3d file for output
 * fnm is the filename
 * szTitle is the title
 * fBinary == 0 for ASCII .3d file
 *         != 0 for binary .3d file
 * Returns pointer to an img struct or NULL
 */
# ifndef STANDALONE
img *img_open_write(const char *fnm, char *szTitle, bool fBinary);
# else
img *img_open_write(const char *fnm, char *szTitle, int fBinary);
# endif

/* Read a datum from a .3d file
 * pimg is a pointer to an img struct returned by img_open()
 * sz is a buffer for a label (only meaningful for img_LABEL)
 * px,py,pz are pointers to floats for the x,y and z coordinates
 * Returns img_XXXX as #define-d above
 */
int img_read_datum(img *pimg, char *sz, float *px, float *py, float *pz);

/* Write a datum to a .3d file
 * pimg is a pointer to an img struct returned by img_open_write()
 * code is one of the img_XXXX #define-d above
 * sz is the label (only meaningful for img_LABEL)
 * x,y,z are the x,y and z coordinates
 */
void img_write_datum(img *pimg, int code, const char *sz,
		     float x, float y, float z);
/* Close a .3d file
 * pimg is a pointer to an img struct returned by img_open() or
 *   img_open_write()
 */
void img_close(img *pimg);

/* Codes returned by img_error */
# ifdef STANDALONE
typedef enum {
   IMG_NONE = 0, IMG_FILENOTFOUND, IMG_OUTOFMEMORY,
   IMG_CANTOPENOUT, IMG_BADFORMAT, IMG_DIRECTORY
} img_errcode;

/* Read the error code
 * if img_open() or img_open_write() returns NULL, you can call this
 * to discover why */
img_errcode img_error(void);
# else
int img_error(void);
# endif

#endif
