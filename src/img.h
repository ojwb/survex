/* > img.h
 * Header file for routines to read and write Survex ".3d" image files
 * Copyright (C) Olly Betts 1993,1994,1997,2001
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

#ifndef IMG_H
# define IMG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

# ifdef IMG_HOSTED
#  include "useful.h"
# endif

# define img_BAD   -2
# define img_STOP  -1
# define img_MOVE   0
# define img_LINE   1
/* NB: img_CROSS is never output and ignored on input.
 * Crosses are put where labels are. */
# define img_CROSS  2
# define img_LABEL  3

# define img_FLAG_SURFACE   0x01
# define img_FLAG_DUPLICATE 0x02
# define img_FLAG_SPLAY     0x04

# define img_SFLAG_SURFACE     0x01
# define img_SFLAG_UNDERGROUND 0x02
# define img_SFLAG_ENTRANCE    0x04
# define img_SFLAG_EXPORTED    0x08
# define img_SFLAG_FIXED       0x10

typedef struct {
   double x, y, z;
} img_point;
    
typedef struct {
   /* members you can access when reading (don't touch when writing) */
   char *label;
   int flags;
   /* all other members are for internal use only */
   FILE *fh;          /* file handle of image file */
   size_t buf_len;
# ifdef IMG_HOSTED
   bool fLinePending; /* for old style text format files */
   bool fRead;        /* fTrue for reading, fFalse for writing */
# else
   int fLinePending; /* for old style text format files */
   int fRead;        /* fTrue for reading, fFalse for writing */
# endif
   long start;
   /* version of file format (0 => 0.01 ascii, 1 => 0.01 binary,
    * 2 => byte actions and flags) */
   int version;
} img;

/* Which version of the file format to output (defaults to newest) */
extern unsigned int img_output_version;

/* Open a .3d file for reading
 * fnm is the filename
 * title_buf and date_buf should be buffers to put the title and datestamp in
 * (each of size 256 chars) or NULL if you don't want the information
 * Returns pointer to an img struct or NULL
 */
img *img_open(const char *fnm, char *title_buf, char *date_buf);

/* Open a .3d file for output
 * fnm is the filename
 * title_buf is the title
 * fBinary == 0 for ASCII .3d file
 *         != 0 for binary .3d file
 * NB only the original .3d file format has an ASCII variant
 * Returns pointer to an img struct or NULL
 */
# ifdef IMG_HOSTED
img *img_open_write(const char *fnm, char *title_buf, bool fBinary);
# else
img *img_open_write(const char *fnm, char *title_buf, int fBinary);
# endif

/* Read an item from a .3d file
 * pimg is a pointer to an img struct returned by img_open()
 * coordinates are returned in p
 * flags and label name are returned in fields in pimg
 * Returns img_XXXX as #define-d above
 */
int img_read_item(img *pimg, img_point *p);

/* Write a item to a .3d file
 * pimg is a pointer to an img struct returned by img_open_write()
 * code is one of the img_XXXX #define-d above
 * flags is the leg or station flags (meaningful for img_LINE and img_LABEL
 * s is the label (only meaningful for img_LABEL)
 * x, y, z are the coordinates
 */
void img_write_item(img *pimg, int code, int flags, const char *s,
		    double x, double y, double z);

/* rewind a .3d file opened for reading so the data can be read in
 * several passes
 * pimg is a pointer to an img struct returned by img_open()
 */
void img_rewind(img *pimg);

/* Close a .3d file
 * pimg is a pointer to an img struct returned by img_open() or
 *   img_open_write()
 */
void img_close(img *pimg);

/* Codes returned by img_error */
# ifndef IMG_HOSTED
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

#ifdef __cplusplus
}
#endif

#endif
