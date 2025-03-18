/* img.h
 * Routines for reading and writing processed survey data files
 *
 * These routines support reading processed survey data in a variety of formats
 * - currently:
 *
 * - Survex ".3d" image files
 * - Survex ".pos" files
 * - Compass Plot files (".plt" and ".plf")
 * - CMAP XYZ files (".sht", ".adj", ".una"; ".xyz" also recognised though
 *   it seems this is a misunderstanding)
 *
 * Writing Survex ".3d" image files is supported.
 *
 * Copyright (C) Olly Betts 1993-2025
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */

#ifndef IMG_H
# define IMG_H

/* Define IMG_API_VERSION if you want more recent versions of the img API.
 *
 * 0 (default)	The old API.  date1 and date2 give the survey date as time_t.
 *		Set to 0 for "unknown".
 * 1		days1 and days2 give survey dates as days since 1st Jan 1900.
 *		Set to -1 for "unknown".
 */
#ifndef IMG_API_VERSION
# define IMG_API_VERSION 0
#elif IMG_API_VERSION > 1
# error IMG_API_VERSION > 1 too new
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <time.h> /* for time_t */

# define img_BAD   -2
# define img_STOP  -1
# define img_MOVE   0
# define img_LINE   1
/* NB: img_CROSS is never output and ignored on input.
 * Put crosses where labels are. */
/* # define img_CROSS  2 */
# define img_LABEL  3
# define img_XSECT  4
# define img_XSECT_END 5
/* Loop closure information for the *preceding* traverse (img_MOVE + one or
 * more img_LINEs). */
# define img_ERROR_INFO 6

/* Leg flags */
# define img_FLAG_SURFACE   0x01
# define img_FLAG_DUPLICATE 0x02
# define img_FLAG_SPLAY     0x04

/* Station flags */
# define img_SFLAG_SURFACE     0x01
# define img_SFLAG_UNDERGROUND 0x02
# define img_SFLAG_ENTRANCE    0x04
# define img_SFLAG_EXPORTED    0x08
# define img_SFLAG_FIXED       0x10
# define img_SFLAG_ANON        0x20
# define img_SFLAG_WALL        0x40

/* File-wide flags */
# define img_FFLAG_EXTENDED 0x80
# define img_FFLAG_SEPARATOR(CH) (((((int)CH) & 0xff) << 9) | 0x100)

/* When writing img_XSECT, img_XFLAG_END in pimg->flags means this is the last
 * img_XSECT in this tube:
 */
# define img_XFLAG_END      0x01

# define img_STYLE_UNKNOWN   -1
# define img_STYLE_NORMAL     0
# define img_STYLE_DIVING     1
# define img_STYLE_CARTESIAN  2
# define img_STYLE_CYLPOLAR   3
# define img_STYLE_NOSURVEY   4

/* 3D coordinates (in metres) */
typedef struct {
   double x, y, z;
} img_point;

typedef struct {
   /* Members you can access when reading (don't touch when writing): */
   char *label;
   int flags;
   char *title;
   /* This contains a string describing the coordinate system which the data is
    * in, suitable for passing to PROJ.  For a coordinate system with an EPSG
    * code this will typically be "EPSG:" followed by the code number.
    *
    * If no coordinate system was specified then this member will be NULL.
    */
   char *cs;
   /* Older .3d format versions stored a human readable datestamp string.
    * Format versions >= 8 versions store a string consisting of "@" followed
    * by the number of seconds since midnight UTC on 1/1/1970.  Some foreign
    * formats contain a human readable string, others no date information
    * (which results in "?" being returned).
    */
   char *datestamp;
   /* The datestamp as a time_t (or (time_t)-1 if not available).
    *
    * For 3d format versions >= 8, this is a reliable value and in UTC.  Older
    * 3d format versions store a human readable time, which img will attempt
    * to decode, but it may fail, particularly with handling timezones.  Even
    * if it does work, beware that times in local time where DST applies are
    * inherently ambiguous around when the clocks go back.
    *
    * CMAP XYZ files contain a timestamp.  It's probably in localtime (but
    * without any timezone information) and the example files are all pre-2000
    * and have two digit years.  We do our best to turn these into a useful
    * time_t value.
    */
   time_t datestamp_numeric;
   char separator; /* character used to separate survey levels ('.' usually) */

   /* Members that can be set when writing: */
#if IMG_API_VERSION == 0
   time_t date1, date2;
#else /* IMG_API_VERSION == 1 */
   int days1, days2;
#endif
   double l, r, u, d;

   /* Error information - valid when img_ERROR_INFO is returned: */
   int n_legs;
   double length;
   double E, H, V;

   /* This member was documented as being set to the filename of the file
    * loaded but was actually always set to NULL when using img outside the
    * Survex code.
    *
    * It has now been removed, so if you are referencing it in your code
    * then instead use the filename you passed to img_open_survey() when
    * opening the file.
    */
   /* char * filename_opened; */

   /* Non-zero if reading an extended elevation: */
   int is_extended_elevation;

   /* Members that can be set when writing: */
   /* The style of the data - one of the img_STYLE_* constants above */
   int style;

   /* All other members are for internal use only: */
   FILE *fh;          /* file handle of image file */
   int (*close_func)(FILE*);
   char *label_buf;
   size_t buf_len;
   size_t label_len;
   int fRead;        /* 1 for reading, 0 for writing */
   long start;
   /* Version of file format.
    *
    * Positive values are .3d file format versions:
    *   0 => 0.01 ascii
    *   1 => 0.01 binary,
    *   2 => byte actions and flags
    *   3 => prefixes for legs; compressed prefixes
    *   4 => survey date
    *   5 => LRUD info
    *   6 => error info
    *   7 => more compact dates with wider range
    *   8 => lots of changes
    *
    * Negative values are other formats which img can read:
    *  IMG_VERSION_CMAP_SHOT => CMAP XYZ file, shot variant (.sht)
    *  IMG_VERSION_CMAP_STATION => CMAP XYZ file, station variant (.adj, .una)
    *  IMG_VERSION_COMPASS_PLT => Compass .plt (or .plf) file
    *  IMG_VERSION_SURVEX_POS => .pos file
    */
   int version;
   char *survey;
   size_t survey_len;
   /* Used to track state in various ways depending on the format, and also for
    * filtering by survey. */
   int pending;
   img_point mv;
#if IMG_API_VERSION == 0
   time_t olddate1, olddate2;
#else /* IMG_API_VERSION == 1 */
   int olddays1, olddays2;
#endif
   int oldstyle;
   /* Pointer to extra data reading some formats requires. */
   void *data;
} img;

/* Fake "version numbers" for non-3d formats we can read, used in
 * the version member of the img struct.  These are not valid in
 * img_output_version.
 */
#define IMG_VERSION_CMAP_SHOT		-4
#define IMG_VERSION_CMAP_STATION	-3
#define IMG_VERSION_COMPASS_PLT		-2
#define IMG_VERSION_SURVEX_POS		-1

/* Which version of the file format to output (defaults to newest) */
extern unsigned int img_output_version;

/* Minimum supported value for img_output_version: */
#define IMG_VERSION_MIN 1

/* Maximum supported value for img_output_version: */
#define IMG_VERSION_MAX 8

/* Open a processed survey data file for reading
 *
 * fnm is the filename
 *
 * Returns pointer to an img struct or NULL
 */
#define img_open(F) img_open_survey((F), NULL)

/* Open a processed survey data file for reading
 *
 * fnm is the filename
 *
 * survey points to a survey name to restrict reading to (or NULL for all
 * survey data in the file)
 *
 * Returns pointer to an img struct or NULL
 */
img *img_open_survey(const char *fnm, const char *survey);

/* Read processed survey data from an existing stream.
 *
 * stream is a FILE* open on the stream (can be NULL which will give error
 * IMG_FILENOTFOUND so you don't need to handle that case specially).  The
 * stream should be opened for reading in binary mode and positioned at the
 * start of the survey data file.
 *
 * close_func is a function to call to close the stream (most commonly
 * fclose, or pclose if the stream was opened using popen()) or NULL if
 * the caller wants to take care of closing the stream.
 *
 * filename is used to determine the format based on the file extension,
 * and also the leafname with the extension removed is used for the survey
 * title for formats which don't support a title or when no title is
 * specified.  If you're not interested in default titles, you can just
 * pass the extension including a leading "." - e.g. ".3d".  May not be
 * NULL.
 *
 * Returns pointer to an img struct or NULL on error.  Any close function
 * specified is called on error (unless stream is NULL).
 */
#define img_read_stream(S, C, F) img_read_stream_survey((S), (C), (F), NULL)

/* Read processed survey data from an existing stream.
 *
 * stream is a FILE* open on the stream (can be NULL which will give error
 * IMG_FILENOTFOUND so you don't need to handle that case specially).  The
 * stream should be opened for reading in binary mode and positioned at the
 * start of the survey data file.
 *
 * close_func is a function to call to close the stream (most commonly
 * fclose, or pclose if the stream was opened using popen()) or NULL if
 * the caller wants to take care of closing the stream.
 *
 * filename is used to determine the format based on the file extension,
 * and also the leafname with the extension removed is used for the survey
 * title for formats which don't support a title or when no title is
 * specified.  If you're not interested in default titles, you can just
 * pass the extension including a leading "." - e.g. ".3d".  filename must
 * not be NULL.
 *
 * survey points to a survey name to restrict reading to (or NULL for all
 * survey data in the file)
 *
 * Returns pointer to an img struct or NULL on error.  Any close function
 * specified is called on error.
 */
img *img_read_stream_survey(FILE *stream, int (*close_func)(FILE*),
			    const char *filename,
			    const char *survey);

/* Open a .3d file for output with no specified coordinate system
 *
 * This is a very thin wrapper around img_open_write_cs() which passes NULL for
 * cs, provided for compatibility with the API provided before support for
 * coordinate systems was added.
 *
 * See img_open_write_cs() for documentation.
 */
#define img_open_write(F, T, S) img_open_write_cs(F, T, NULL, S)

/* Open a .3d file for output in a specified coordinate system
 *
 * fnm is the filename
 *
 * title is the title
 *
 * cs is a string describing the coordinate system, suitable for passing to
 * PROJ (or NULL to not specify a coordinate system).  For a coordinate system
 * with an assigned EPSG code number, "EPSG:" followed by the code number is
 * the recommended way to specify this.
 *
 * flags contains a bitwise-or of any file-wide flags - currently these are
 * available:
 *
 * img_FFLAG_EXTENDED : this is an extended elevation
 * img_FFLAG_SEPARATOR(CHARACTER) : specify the separator character
 *		(default: '.')
 *
 * Returns pointer to an img struct or NULL for error (check img_error()
 * for details)
 */
img *img_open_write_cs(const char *fnm, const char *title, const char * cs,
		       int flags);

/* Write a .3d file to a stream
 *
 * stream is a FILE* open on the stream (can be NULL which will give error
 * IMG_FILENOTFOUND so you don't need to handle that case specially).  The
 * stream should be opened for writing in binary mode.
 *
 * close_func is a function to call to close the stream (most commonly
 * fclose, or pclose if the stream was opened using popen()) or NULL if
 * the caller wants to take care of closing the stream.
 *
 * title is the title
 *
 * cs is a string describing the coordinate system, suitable for passing to
 * PROJ (or NULL to not specify a coordinate system).  For a coordinate system
 * with an assigned EPSG code number, "EPSG:" followed by the code number is
 * the recommended way to specify this.
 *
 * flags contains a bitwise-or of any file-wide flags - currently these are
 * available:
 *
 * img_FFLAG_EXTENDED : this is an extended elevation
 * img_FFLAG_SEPARATOR(CHARACTER) : specify the separator character
 *		(default: '.')
 *
 * Returns pointer to an img struct or NULL for error (check img_error()
 * for details).  Any close function specified is called on error (unless
 * stream is NULL).
 */
img *img_write_stream(FILE *stream, int (*close_func)(FILE*),
		      const char *title, const char * cs, int flags);

/* Read an item from a processed survey data file
 *
 * pimg is a pointer to an img struct returned by img_open()
 *
 * coordinates are returned in p
 *
 * flags and label name are returned in fields in pimg
 *
 * Returns img_XXXX as #define-d above
 */
int img_read_item(img *pimg, img_point *p);

/* Write a item to a .3d file
 *
 * pimg is a pointer to an img struct returned by img_open_write()
 *
 * code is one of the img_XXXX #define-d above
 *
 * flags is the leg, station, or xsect flags
 * (meaningful for img_LINE, img_LABEL, and img_XSECT respectively)
 *
 * s is the label (only meaningful for img_LABEL)
 *
 * x, y, z are the coordinates
 */
void img_write_item(img *pimg, int code, int flags, const char *s,
		    double x, double y, double z);

/* Write error information for the current traverse
 *
 * n_legs is the number of legs in the traverse
 *
 * length is the traverse length (in m)
 *
 * E is the ratio of the observed misclosure to the theoretical one
 *
 * H is the ratio of the observed horizontal misclosure to the theoretical one
 *
 * V is the ratio of the observed vertical misclosure to the theoretical one
 */
void img_write_errors(img *pimg, int n_legs, double length,
		      double E, double H, double V);

/* rewind a processed survey data file opened for reading
 *
 * This is useful if you want to read the data in several passes.
 *
 * pimg is a pointer to an img struct returned by img_open()
 *
 * Returns: non-zero for success, zero for error (check img_error() for
 *   details)
 */
int img_rewind(img *pimg);

/* Close a processed survey data file
 *
 * pimg is a pointer to an img struct returned by img_open() or
 *   img_open_write()
 *
 * Returns: non-zero for success, zero for error (check img_error() for
 *   details)
 */
int img_close(img *pimg);

/* Codes returned by img_error */
typedef enum {
   IMG_NONE = 0, IMG_FILENOTFOUND, IMG_OUTOFMEMORY,
   IMG_CANTOPENOUT, IMG_BADFORMAT, IMG_DIRECTORY,
   IMG_READERROR, IMG_WRITEERROR, IMG_TOONEW
} img_errcode;

/* Read the error code
 *
 * If img_open(), img_open_survey() or img_open_write() returns NULL, or
 * img_rewind() or img_close() returns 0, or img_read_item() returns img_BAD
 * then you can call this function to discover why.
 */
img_errcode img_error(void);

/* Datum codes returned by img_parse_compass_datum_string().
 *
 * We currently don't handle the following, which appear in the datum list
 * in Compass, but there don't seem to be any EPSG codes for UTM with any
 * of these:
 *
 *   Australian 1966
 *   Australian 1984
 *   Camp Area Astro (Antarctica only)
 *   European 1979
 *   Hong Kong 1963
 *   Oman
 *   Ordnance Survey 1936
 *   Pulkovo 1942
 *   South American 1956
 *   South American 1969
 */
typedef enum {
    img_DATUM_UNKNOWN = 0,
    img_DATUM_ADINDAN,
    img_DATUM_ARC1950,
    img_DATUM_ARC1960,
    img_DATUM_CAPE,
    img_DATUM_EUROPEAN1950,
    img_DATUM_NZGD49,
    img_DATUM_HUTZUSHAN1950,
    img_DATUM_INDIAN1960,
    img_DATUM_NAD27,
    img_DATUM_NAD83,
    img_DATUM_TOKYO,
    img_DATUM_WGS72,
    img_DATUM_WGS84
} img_datum;

/* Parse a Compass datum string and return an img_datum code. */
img_datum img_parse_compass_datum_string(const char *s, size_t len);

/* Return a CRS string to pass to PROJ from an img_datum and UTM zone.
 *
 * utm_zone can be between -60 and -1 (Southern Hemisphere), or 1 and 60
 * (Northern Hemisphere).
 *
 * Where possible a string of the form "EPSG:1234" is returned.
 *
 * Example Compass files we've seen use "North American 1927" outside of its
 * defined area of use, presumably because some users fail to change the datum
 * from Compass' default.  To enable reading such files we return a PROJ4
 * string of the form "+proj=utm ..." for "North American 1927" and "North
 * American 1983" when there's no EPSG code for the specified datum and
 * utm_zone combination.
 *
 * If no mapping is known NULL is returned.
 *
 * The returned value is allocated with malloc() and the caller is responsible
 * for calling free().
 */
char *img_compass_utm_proj_str(img_datum datum, int utm_zone);

/* Return EPSG code for geodetic CRS (i.e. long/lat) with datum img_datum.
 *
 * Returns -1 for img_DATUM_UNKNOWN.
 */
int img_compass_longlat_epsg_code(img_datum datum);

#ifdef __cplusplus
}
#endif

#endif
