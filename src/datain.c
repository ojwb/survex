/* datain.c
 * Reads in survey files, dealing with special characters, keywords & data
 * Copyright (C) 1991-2024 Olly Betts
 * Copyright (C) 2004 Simeon Warner
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <limits.h>
#include <stdarg.h>

#include "debug.h"
#include "cavern.h"
#include "date.h"
#include "hash.h"
#include "img.h"
#include "filename.h"
#include "message.h"
#include "filelist.h"
#include "netbits.h"
#include "netskel.h"
#include "readval.h"
#include "datain.h"
#include "commands.h"
#include "out.h"
#include "str.h"
#include "thgeomag.h"
#if PROJ_VERSION_MAJOR < 8 || \
    (PROJ_VERSION_MAJOR == 8 && PROJ_VERSION_MINOR < 2)
/* Needed for proj_factors workaround */
# include <proj_experimental.h>
#endif

#define EPSILON (REAL_EPSILON * 1000)

#define var(I) (pcs->Var[(I)])

/* Test for a not-a-number value in Compass data (999.0 or -999.0).
 *
 * Compass itself uses -999.0 but reportedly understands Karst data which used
 * 999.0 (information from Larry Fish via Simeon Warner in 2004).  However
 * testing with Compass in early 2024 it seems 999.0 is treated like any other
 * reading.
 *
 * When "corrected" backsights are specified in FORMAT, Compass seems to write
 * out -999 with the correction applied to the CLP file.
 *
 * Valid readings should be 0 to 360 for the compass and -90 to 90 for the
 * clino, and the correction should have absolute value < 360, so we test for
 * any reading with an absolute value greater than 999 - 360 = 639, which is
 * well outside the valid range.
 */
#define is_compass_NaN(x) (fabs(x) > (999.0 - 360.0))

static int
read_compass_date_as_days_since_1900(void)
{
    /* NB order is *month* *day* year */
    int month = read_uint();
    int day = read_uint();
    int year = read_uint();
    /* Note: Larry says a 2 digit year is always 19XX */
    if (year < 100) year += 1900;

    /* Compass uses 1901-01-01 when no date was specified. */
    if (year == 1901 && day == 1 && month == 1) return -1;

    return days_since_1900(year, month, day);
}
int ch;

typedef enum {
    // Clino omitted.  VAL() should be set to 0.0.
    CTYPE_OMIT,
    // An actual clino reading.
    CTYPE_READING,
    // An explicit plumb (U/D/UP/DOWN/+V/-V for reading).
    CTYPE_PLUMB,
    // An inferred plumb (+90 or -90 and *infer plumbs).
    CTYPE_INFERPLUMB,
    // An explicit horizontal leg (H/LEVEL for reading).
    CTYPE_HORIZ
} clino_type;

/* Don't explicitly initialise as we can't set the jmp_buf - this has
 * static scope so will be initialised like this anyway */
parse file /* = { NULL, NULL, 0, false, NULL } */ ;

bool f_export_ok;

static real value[Fr - 1];
#define VAL(N) value[(N)-1]
static real variance[Fr - 1];
#define VAR(N) variance[(N)-1]
static long location[Fr - 1];
#define LOC(N) location[(N)-1]
static int location_width[Fr - 1];
#define WID(N) location_width[(N)-1]

/* style functions */
static void data_normal(void);
static void data_cartesian(void);
static void data_passage(void);
static void data_nosurvey(void);
static void data_ignore(void);

void
get_pos(filepos *fp)
{
   fp->ch = ch;
   fp->offset = ftell(file.fh);
   if (fp->offset == -1)
      fatalerror_in_file(file.filename, 0, /*Error reading file*/18);
}

void
set_pos(const filepos *fp)
{
   ch = fp->ch;
   if (fseek(file.fh, fp->offset, SEEK_SET) == -1)
      fatalerror_in_file(file.filename, 0, /*Error reading file*/18);
}

static void
report_parent(parse * p) {
    if (p->parent)
	report_parent(p->parent);
    /* Force re-report of include tree for further errors in
     * parent files */
    p->reported_where = false;
    /* TRANSLATORS: %s is replaced by the filename of the parent file, and %u
     * by the line number in that file.  Your translation should also contain
     * %s:%u so that automatic parsing of error messages to determine the file
     * and line number still works. */
    fprintf(STDERR, msg(/*In file included from %s:%u:\n*/5), p->filename, p->line);
}

static void
error_list_parent_files(void)
{
   if (!file.reported_where && file.parent) {
      report_parent(file.parent);
      /* Suppress reporting of full include tree for further errors
       * in this file */
      file.reported_where = true;
   }
}

static void
show_line(int col, int width)
{
   /* Rewind to beginning of line. */
   long cur_pos = ftell(file.fh);
   int tabs = 0;
   if (cur_pos < 0 || fseek(file.fh, file.lpos, SEEK_SET) == -1)
      fatalerror_in_file(file.filename, 0, /*Error reading file*/18);

   /* Read the whole line and write it out. */
   PUTC(' ', STDERR);
   while (1) {
      int c = GETC(file.fh);
      /* Note: isEol() is true for EOF */
      if (isEol(c)) break;
      if (c == '\t') ++tabs;
      PUTC(c, STDERR);
   }
   fputnl(STDERR);

   /* If we have a location in the line for the error, indicate it. */
   if (col > 0) {
      PUTC(' ', STDERR);
      if (tabs == 0) {
	 while (--col) PUTC(' ', STDERR);
      } else {
	 /* Copy tabs from line, replacing other characters with spaces - this
	  * means that the caret should line up correctly. */
	 if (fseek(file.fh, file.lpos, SEEK_SET) == -1)
	    fatalerror_in_file(file.filename, 0, /*Error reading file*/18);
	 while (--col) {
	    int c = GETC(file.fh);
	    if (c != '\t') c = ' ';
	    PUTC(c, STDERR);
	 }
      }
      PUTC('^', STDERR);
      while (width > 1) {
	 PUTC('~', STDERR);
	 --width;
      }
      fputnl(STDERR);
   }

   /* Revert to where we were. */
   if (fseek(file.fh, cur_pos, SEEK_SET) == -1)
      fatalerror_in_file(file.filename, 0, /*Error reading file*/18);
}

char*
grab_line(void)
{
   /* Rewind to beginning of line. */
   long cur_pos = ftell(file.fh);
   string p = S_INIT;
   if (cur_pos < 0 || fseek(file.fh, file.lpos, SEEK_SET) == -1)
      fatalerror_in_file(file.filename, 0, /*Error reading file*/18);

   /* Read the whole line into a string. */
   while (1) {
      int c = GETC(file.fh);
      /* Note: isEol() is true for EOF */
      if (isEol(c)) break;
      s_catchar(&p, c);
   }

   /* Revert to where we were. */
   if (fseek(file.fh, cur_pos, SEEK_SET) == -1) {
      s_free(&p);
      fatalerror_in_file(file.filename, 0, /*Error reading file*/18);
   }

   return s_steal(&p);
}

static int caret_width = 0;

static void
compile_v_report_fpos(int severity, long fpos, int en, va_list ap)
{
   int col = 0;
   error_list_parent_files();
   if (fpos >= file.lpos)
      col = fpos - file.lpos - caret_width;
   v_report(severity, file.filename, file.line, col, en, ap);
   if (file.fh) show_line(col, caret_width);
}

static void
compile_v_report(int diag_flags, int en, va_list ap)
{
   int severity = (diag_flags & DIAG_SEVERITY_MASK);
   if (diag_flags & (DIAG_COL|DIAG_TOKEN)) {
      if (file.fh) {
	 if (diag_flags & DIAG_TOKEN) caret_width = s_len(&token);
	 compile_v_report_fpos(severity, ftell(file.fh), en, ap);
	 if (diag_flags & DIAG_TOKEN) caret_width = 0;
	 if (diag_flags & DIAG_SKIP) skipline();
	 return;
      }
   }
   error_list_parent_files();
   v_report(severity, file.filename, file.line, 0, en, ap);
   if (file.fh) {
      if (diag_flags & DIAG_TOKEN) {
	 show_line(0, s_len(&token));
      } else {
	 show_line(0, caret_width);
      }
   }
   if (diag_flags & DIAG_SKIP) skipline();
}

void
compile_diagnostic(int diag_flags, int en, ...)
{
   va_list ap;
   va_start(ap, en);
   if (diag_flags & (DIAG_DATE|DIAG_NUM|DIAG_UINT|DIAG_WORD|DIAG_TAIL)) {
      int len = 0;
      skipblanks();
      if (diag_flags & DIAG_WORD) {
	 while (!isBlank(ch) && !isEol(ch)) {
	    ++len;
	    nextch();
	 }
      } else if (diag_flags & DIAG_UINT) {
	 while (isdigit(ch)) {
	    ++len;
	    nextch();
	 }
      } else if (diag_flags & DIAG_DATE) {
	 while (isdigit(ch) || ch == '.') {
	    ++len;
	    nextch();
	 }
      } else if (diag_flags & DIAG_TAIL) {
	 int len_last_nonblank = len;
	 while (!isComm(ch) && !isEol(ch)) {
	    ++len;
	    if (!isBlank(ch)) len_last_nonblank = len;
	    nextch();
	 }
	 len = len_last_nonblank;
      } else {
	 if (isMinus(ch) || isPlus(ch)) {
	    ++len;
	    nextch();
	 }
	 while (isdigit(ch)) {
	    ++len;
	    nextch();
	 }
	 if (isDecimal(ch)) {
	    ++len;
	    nextch();
	 }
	 while (isdigit(ch)) {
	    ++len;
	    nextch();
	 }
      }
      caret_width = len;
      compile_v_report(diag_flags|DIAG_COL, en, ap);
      caret_width = 0;
   } else if (diag_flags & DIAG_STRING) {
      string p = S_INIT;
      skipblanks();
      caret_width = ftell(file.fh);
      read_string(&p);
      s_free(&p);
      /* We want to include any quotes, so can't use s_len(&p). */
      caret_width = ftell(file.fh) - caret_width;
      compile_v_report(diag_flags|DIAG_COL, en, ap);
      caret_width = 0;
   } else {
      compile_v_report(diag_flags, en, ap);
   }
   va_end(ap);
}

static void
compile_diagnostic_reading(int diag_flags, reading r, int en, ...)
{
   va_list ap;
   int severity = (diag_flags & DIAG_SEVERITY_MASK);
   va_start(ap, en);
   caret_width = WID(r);
   compile_v_report_fpos(severity, LOC(r) + caret_width, en, ap);
   caret_width = 0;
   va_end(ap);
}

static void
compile_error_reading_skip(reading r, int en, ...)
{
   va_list ap;
   va_start(ap, en);
   caret_width = WID(r);
   compile_v_report_fpos(DIAG_ERR, LOC(r) + caret_width, en, ap);
   caret_width = 0;
   va_end(ap);
   skipline();
}

void
compile_diagnostic_at(int diag_flags, const char * filename, unsigned line, int en, ...)
{
   va_list ap;
   int severity = (diag_flags & DIAG_SEVERITY_MASK);
   va_start(ap, en);
   v_report(severity, filename, line, 0, en, ap);
   va_end(ap);
}

void
compile_diagnostic_pfx(int diag_flags, const prefix * pfx, int en, ...)
{
   va_list ap;
   int severity = (diag_flags & DIAG_SEVERITY_MASK);
   va_start(ap, en);
   v_report(severity, pfx->filename, pfx->line, 0, en, ap);
   va_end(ap);
}

void
compile_diagnostic_token_show(int diag_flags, int en)
{
   string p = S_INIT;
   skipblanks();
   while (!isBlank(ch) && !isEol(ch)) {
      s_catchar(&p, (char)ch);
      nextch();
   }
   if (!s_empty(&p)) {
      caret_width = s_len(&p);
      compile_diagnostic(diag_flags|DIAG_COL, en, p);
      caret_width = 0;
      s_free(&p);
   } else {
      compile_diagnostic(DIAG_ERR|DIAG_COL, en, "");
   }
}

static void
compile_error_string(const char * s, int en, ...)
{
   va_list ap;
   va_start(ap, en);
   caret_width = strlen(s);
   compile_v_report(DIAG_ERR|DIAG_COL, en, ap);
   va_end(ap);
   caret_width = 0;
}

/* This function makes a note where to put output files */
static void
using_data_file(const char *fnm)
{
   if (!fnm_output_base) {
      /* was: fnm_output_base = base_from_fnm(fnm); */
      fnm_output_base = baseleaf_from_fnm(fnm);
   } else if (fnm_output_base_is_dir) {
      /* --output pointed to directory so use the leaf basename in that dir */
      char *lf, *p;
      lf = baseleaf_from_fnm(fnm);
      p = use_path(fnm_output_base, lf);
      osfree(lf);
      osfree(fnm_output_base);
      fnm_output_base = p;
      fnm_output_base_is_dir = 0;
   }
}

static void
skipword(void)
{
   while (!isBlank(ch) && !isEol(ch)) nextch();
}

extern void
skipblanks(void)
{
   while (isBlank(ch)) nextch();
}

extern void
skipline(void)
{
   while (!isEol(ch)) nextch();
}

static void
process_eol(void)
{
   int eolchar;

   skipblanks();

   if (!isEol(ch)) {
      if (!isComm(ch))
	 compile_diagnostic(DIAG_ERR|DIAG_TAIL, /*End of line not blank*/15);
      skipline();
   }

   eolchar = ch;
   file.line++;
   /* skip any different eol characters so we get line counts correct on
    * DOS text files and similar, but don't count several adjacent blank
    * lines as one */
   while (ch != EOF) {
      nextch();
      if (ch == eolchar || !isEol(ch)) {
	 break;
      }
      if (ch == '\n') eolchar = ch;
   }
   file.lpos = ftell(file.fh) - 1;
}

static bool
process_non_data_line(void)
{
   skipblanks();

   if (isData(ch)) return false;

   if (isKeywd(ch)) {
      nextch();
      handle_command();
   }

   process_eol();

   return true;
}

static void
read_reading(reading r, bool f_optional)
{
   int n_readings;
   q_quantity q;
   switch (r) {
      case Tape: q = Q_LENGTH; break;
      case BackTape: q = Q_BACKLENGTH; break;
      case Comp: q = Q_BEARING; break;
      case BackComp: q = Q_BACKBEARING; break;
      case Clino: q = Q_GRADIENT; break;
      case BackClino: q = Q_BACKGRADIENT; break;
      case FrDepth: case ToDepth: q = Q_DEPTH; break;
      case Dx: q = Q_DX; break;
      case Dy: q = Q_DY; break;
      case Dz: q = Q_DZ; break;
      case FrCount: case ToCount: q = Q_COUNT; break;
      case Left: q = Q_LEFT; break;
      case Right: q = Q_RIGHT; break;
      case Up: q = Q_UP; break;
      case Down: q = Q_DOWN; break;
      default:
	q = Q_NULL; /* Suppress compiler warning */;
	BUG("Unexpected case");
   }
   LOC(r) = ftell(file.fh);
   /* since we don't handle bearings in read_readings, it's never quadrant */
   VAL(r) = read_numeric_multi(f_optional, false, &n_readings);
   WID(r) = ftell(file.fh) - LOC(r);
   VAR(r) = var(q);
   if (n_readings > 1) VAR(r) /= sqrt(n_readings);
}

static void
read_bearing_or_omit(reading r)
{
   int n_readings;
   bool quadrants = false;
   q_quantity q = Q_NULL;
   switch (r) {
      case Comp:
	q = Q_BEARING;
	if (pcs->f_bearing_quadrants)
	   quadrants = true;
	break;
      case BackComp:
	q = Q_BACKBEARING;
	if (pcs->f_backbearing_quadrants)
	   quadrants = true;
	break;
      default:
	q = Q_NULL; /* Suppress compiler warning */;
	BUG("Unexpected case");
   }
   LOC(r) = ftell(file.fh);
   VAL(r) = read_bearing_multi_or_omit(quadrants, &n_readings);
   WID(r) = ftell(file.fh) - LOC(r);
   VAR(r) = var(q);
   if (n_readings > 1) VAR(r) /= sqrt(n_readings);
}

// Set up settings for reading Compass DAT or MAK.
static void
initialise_common_compass_settings(void)
{
    short *t = ((short*)osmalloc(ossizeof(short) * 257)) + 1;
    int i;
    t[EOF] = SPECIAL_EOL;
    memset(t, 0, sizeof(short) * 33);
    for (i = 33; i < 127; i++) t[i] = SPECIAL_NAMES;
    t[127] = 0;
    for (i = 128; i < 256; i++) t[i] = SPECIAL_NAMES;
    t['\t'] |= SPECIAL_BLANK;
    t[' '] |= SPECIAL_BLANK;
    t['\032'] |= SPECIAL_EOL; /* Ctrl-Z, so olde DOS text files are handled ok */
    t['\n'] |= SPECIAL_EOL;
    t['\r'] |= SPECIAL_EOL;
    t['.'] |= SPECIAL_DECIMAL;
    t['-'] |= SPECIAL_MINUS;
    t['+'] |= SPECIAL_PLUS;

    settings *pcsNew = osnew(settings);
    *pcsNew = *pcs; /* copy contents */
    pcsNew->begin_lineno = 0;
    pcsNew->Translate = t;
    pcsNew->Case = OFF;
    pcsNew->Truncate = INT_MAX;
    pcsNew->next = pcs;
    pcs = pcsNew;

    update_output_separator();
}

/* For reading Compass MAK files which have a freeform syntax */
static void
nextch_handling_eol(void)
{
   nextch();
   while (ch != EOF && isEol(ch)) {
      process_eol();
   }
}

static void
data_file_compass_dat_or_clp(bool is_clp)
{
    initialise_common_compass_settings();
    default_units(pcs);
    default_calib(pcs);
    pcs->z[Q_DECLINATION] = HUGE_REAL;

    pcs->recorded_style = pcs->style = STYLE_NORMAL;
    pcs->units[Q_LENGTH] = METRES_PER_FOOT;
    pcs->infer = BIT(INFER_EQUATES) |
		 BIT(INFER_EQUATES_SELF_OK) |
		 BIT(INFER_EXPORTS) |
		 BIT(INFER_PLUMBS);
    /* We need to update separator_map so we don't pick a separator character
     * which occurs in a station name.  However Compass DAT allows everything
     * >= ASCII char 33 except 127 in station names so if we just added all
     * the valid station name characters we'd always pick space as the
     * separator for any dataset which included a DAT file, yet in practice
     * '.' is never used in any of the sample DAT files I've seen.  So
     * instead we scan the characters actually used in station names when we
     * process CompassDATFr and CompassDATTo fields.
     */

#ifdef HAVE_SETJMP_H
    /* errors in nested functions can longjmp here */
    if (setjmp(file.jbSkipLine)) {
	skipline();
	process_eol();
    }
#endif

    while (ch != EOF && !ferror(file.fh)) {
	static const reading compass_order[] = {
	    CompassDATFr, CompassDATTo, Tape, CompassDATComp, CompassDATClino,
	    CompassDATLeft, CompassDATUp, CompassDATDown, CompassDATRight,
	    CompassDATFlags, IgnoreAll
	};
	static const reading compass_order_backsights[] = {
	    CompassDATFr, CompassDATTo, Tape, CompassDATComp, CompassDATClino,
	    CompassDATLeft, CompassDATUp, CompassDATDown, CompassDATRight,
	    CompassDATBackComp, CompassDATBackClino,
	    CompassDATFlags, IgnoreAll
	};
	/* <Cave name> */
	skipline();
	process_eol();
	/* SURVEY NAME: <Short name> */
	get_token();
	get_token();
	/* if (ch != ':') ... */
	nextch();
	get_token();
	skipline();
	process_eol();
	/* SURVEY DATE: 7 10 79  COMMENT:<Long name> */
	get_token();
	get_token();
	copy_on_write_meta(pcs);
	if (ch == ':') {
	    nextch();
	    int days = read_compass_date_as_days_since_1900();
	    pcs->meta->days1 = pcs->meta->days2 = days;
	} else {
	    pcs->meta->days1 = pcs->meta->days2 = -1;
	}
	pcs->declination = HUGE_REAL;
	skipline();
	process_eol();
	/* SURVEY TEAM: */
	get_token();
	get_token();
	skipline();
	process_eol();
	/* <Survey team> */
	skipline();
	process_eol();
	/* DECLINATION: 1.00  FORMAT: DDDDLUDRADLN  CORRECTIONS: 2.00 3.00 4.00 */
	get_token();
	nextch(); /* : */
	skipblanks();
	if (pcs->dec_filename == NULL) {
	    pcs->z[Q_DECLINATION] = -read_numeric(false);
	    pcs->z[Q_DECLINATION] *= pcs->units[Q_DECLINATION];
	} else {
	    (void)read_numeric(false);
	}
	get_token();
	pcs->ordering = compass_order;
	if (S_EQ(&token, "FORMAT")) {
	    /* This documents the format in the original survey notebook - we
	     * don't need to fully parse it to be able to parse the survey data
	     * in the file, which gets converted to a fixed order and units.
	     */
	    nextch(); /* : */
	    get_token();
	    size_t token_len = s_len(&token);
	    if (token_len >= 4 && s_str(&token)[3] == 'W') {
		/* Original "Inclination Units" were "Depth Gauge". */
		pcs->recorded_style = STYLE_DIVING;
	    }
	    if (token_len >= 12) {
		char backsight_type = s_str(&token)[token_len >= 15 ? 13 : 11];
		// B means redundant backsight; C means redundant backsights
		// but displayed "corrected" (i.e. reversed to make visually
		// comparing easier).
		if (backsight_type == 'B' || backsight_type == 'C') {
		    /* We have backsights for compass and clino */
		    pcs->ordering = compass_order_backsights;
		}
	    }
	    get_token();
	}

	// CORRECTIONS and CORRECTIONS2 have already been applied to data in
	// the CLP file.
	if (!is_clp) {
	    if (S_EQ(&token, "CORRECTIONS") && ch == ':') {
		nextch(); /* : */
		pcs->z[Q_BACKBEARING] = pcs->z[Q_BEARING] = -rad(read_numeric(false));
		pcs->z[Q_BACKGRADIENT] = pcs->z[Q_GRADIENT] = -rad(read_numeric(false));
		pcs->z[Q_LENGTH] = -METRES_PER_FOOT * read_numeric(false);
		get_token();
	    }

	    /* get_token() only reads alphas so we must check for '2' here. */
	    if (S_EQ(&token, "CORRECTIONS") && ch == '2') {
		nextch(); /* 2 */
		nextch(); /* : */
		pcs->z[Q_BACKBEARING] = -rad(read_numeric(false));
		pcs->z[Q_BACKGRADIENT] = -rad(read_numeric(false));
		get_token();
	    }
	}

#if 0
	// FIXME Parse once we handle discovery dates...
	// NB: Need to skip unread CORRECTIONS* for the `is_clp` case.
	if (S_EQ(&token, "DISCOVERY") && ch == ':') {
	    // Discovery date, e.g. DISCOVERY: 2 28 2024
	    nextch(); /* : */
	    int days = read_compass_date_as_days_since_1900();
	}
#endif
	skipline();
	process_eol();
	/* BLANK LINE */
	skipline();
	process_eol();
	/* heading line */
	skipline();
	process_eol();
	/* BLANK LINE */
	skipline();
	process_eol();
	while (ch != EOF) {
	    if (ch == '\x0c') {
		nextch();
		process_eol();
		break;
	    }
	    data_normal();
	}
	clear_last_leg();
    }
    pcs->ordering = NULL; /* Avoid free() of static array. */
    pop_settings();
}

static void
data_file_compass_dat(void)
{
    data_file_compass_dat_or_clp(false);
}

static void
data_file_compass_clp(void)
{
    data_file_compass_dat_or_clp(true);
}

static void
data_file_compass_mak(void)
{
    initialise_common_compass_settings();
    short *t = pcs->Translate;
    // In a Compass MAK file a station name can't contain these three
    // characters due to how the syntax works.
    t['['] = t[','] = t[';'] = 0;

#ifdef HAVE_SETJMP_H
    /* errors in nested functions can longjmp here */
    if (setjmp(file.jbSkipLine)) {
	skipline();
	process_eol();
    }
#endif

    int datum = 0;
    int utm_zone = 0;
    real base_x = 0.0, base_y = 0.0, base_z = 0.0;
    int base_utm_zone = 0;
    unsigned int base_line = 0;
    long base_lpos = 0;
    string path = S_INIT;
    s_donate(&path, path_from_fnm(file.filename));
    struct mak_folder {
	struct mak_folder *next;
	int len;
    } *folder_stack = NULL;

    while (ch != EOF && !ferror(file.fh)) {
	switch (ch) {
	  case '#': {
	      /* include a file */
	      int ch_store;
	      string dat_fnm = S_INIT;
	      nextch_handling_eol();
	      while (ch != ',' && ch != ';' && ch != EOF) {
		  while (isEol(ch)) process_eol();
		  s_catchar(&dat_fnm, (char)ch);
		  nextch_handling_eol();
	      }
	      if (!s_empty(&dat_fnm)) {
		  if (base_utm_zone) {
		      // Process the previous @ command using the datum from &.
		      char *proj_str = img_compass_utm_proj_str(datum,
								base_utm_zone);
		      if (proj_str) {
			  // Temporarily reset line and lpos so dec_context and
			  // dec_line refer to the @ command.
			  unsigned saved_line = file.line;
			  file.line = base_line;
			  long saved_lpos = file.lpos;
			  file.lpos = base_lpos;
			  set_declination_location(base_x, base_y, base_z,
						   proj_str);
			  file.line = saved_line;
			  file.lpos = saved_lpos;
			  if (!pcs->proj_str) {
			      pcs->proj_str = proj_str;
			      if (!proj_str_out) {
				  proj_str_out = osstrdup(proj_str);
			      }
			  } else {
			      osfree(proj_str);
			  }
		      }
		  }
		  ch_store = ch;
		  data_file(s_str(&path), s_str(&dat_fnm));
		  ch = ch_store;
		  s_free(&dat_fnm);
	      }
	      while (ch != ';' && ch != EOF) {
		  nextch_handling_eol();
		  filepos fp_name;
		  get_pos(&fp_name);
		  prefix *name = read_prefix(PFX_STATION|PFX_OPT);
		  if (name) {
		      scan_compass_station_name(name);
		      skipblanks();
		      if (ch == '[') {
			  /* fixed pt */
			  node *stn;
			  real x, y, z;
			  bool in_feet = false;
			  // Compass treats these fixed points as entrances
			  // ("distance from entrance" in a .DAT file counts
			  // from 0.0 at these points) so we do too.
			  name->sflags |= BIT(SFLAGS_FIXED) |
					  BIT(SFLAGS_ENTRANCE);
			  nextch_handling_eol();
			  if (ch == 'F' || ch == 'f') {
			      in_feet = true;
			      nextch_handling_eol();
			  } else if (ch == 'M' || ch == 'm') {
			      nextch_handling_eol();
			  } else {
			      compile_diagnostic(DIAG_ERR|DIAG_COL, /*Expecting “F” or “M”*/103);
			  }
			  while (!isdigit(ch) && ch != '+' && ch != '-' &&
				 ch != '.' && ch != ']' && ch != EOF) {
			      nextch_handling_eol();
			  }
			  x = read_numeric(false);
			  while (!isdigit(ch) && ch != '+' && ch != '-' &&
				 ch != '.' && ch != ']' && ch != EOF) {
			      nextch_handling_eol();
			  }
			  y = read_numeric(false);
			  while (!isdigit(ch) && ch != '+' && ch != '-' &&
				 ch != '.' && ch != ']' && ch != EOF) {
			      nextch_handling_eol();
			  }
			  z = read_numeric(false);
			  if (in_feet) {
			      x *= METRES_PER_FOOT;
			      y *= METRES_PER_FOOT;
			      z *= METRES_PER_FOOT;
			  }
			  stn = StnFromPfx(name);
			  if (!fixed(stn)) {
			      POS(stn, 0) = x;
			      POS(stn, 1) = y;
			      POS(stn, 2) = z;
			      fix(stn);
			  } else {
			      filepos fp;
			      get_pos(&fp);
			      set_pos(&fp_name);
			      if (x != POS(stn, 0) ||
				  y != POS(stn, 1) ||
				  z != POS(stn, 2)) {
				  compile_diagnostic(DIAG_ERR|DIAG_WORD, /*Station already fixed or equated to a fixed point*/46);
			      } else {
				  compile_diagnostic(DIAG_WARN|DIAG_WORD, /*Station already fixed at the same coordinates*/55);
			      }
			      set_pos(&fp);
			  }
			  while (ch != ']' && ch != EOF) nextch_handling_eol();
			  if (ch == ']') {
			      nextch_handling_eol();
			      skipblanks();
			  }
		      } else {
			  /* FIXME: link station - ignore for now */
			  /* FIXME: perhaps issue warning?  Other station names
			   * can be "reused", which is problematic... */
		      }
		      while (ch != ',' && ch != ';' && ch != EOF)
			  nextch_handling_eol();
		  }
	      }
	      break;
	  }
	  case '$':
	    /* UTM zone */
	    nextch();
	    skipblanks();
	    utm_zone = read_int(-60, 60);
	    skipblanks();
	    if (ch == ';') nextch_handling_eol();

update_proj_str:
	    if (!pcs->next || pcs->proj_str != pcs->next->proj_str)
		osfree(pcs->proj_str);
	    pcs->proj_str = NULL;
	    if (datum && utm_zone && abs(utm_zone) <= 60) {
		/* Set up coordinate system. */
		char *proj_str = img_compass_utm_proj_str(datum, utm_zone);
		if (proj_str) {
		    pcs->proj_str = proj_str;
		    if (!proj_str_out) {
			proj_str_out = osstrdup(proj_str);
		    }
		}
	    }
	    invalidate_pj_cached();
	    break;
	  case '&': {
	      /* Datum */
	      string p = S_INIT;
	      int datum_len = 0;
	      int c = 0;
	      nextch();
	      skipblanks();
	      while (ch != ';' && !isEol(ch)) {
		  s_catchar(&p, (char)ch);
		  ++c;
		  /* Ignore trailing blanks. */
		  if (!isBlank(ch)) datum_len = c;
		  nextch();
	      }
	      if (ch == ';') nextch_handling_eol();
	      datum = img_parse_compass_datum_string(s_str(&p), datum_len);
	      s_free(&p);
	      goto update_proj_str;
	  }
	  case '[': {
	      // Enter subdirectory.
	      struct mak_folder *p = folder_stack;
	      folder_stack = osnew(struct mak_folder);
	      folder_stack->next = p;
	      folder_stack->len = s_len(&path);
	      if (!s_empty(&path))
		  s_catchar(&path, FNM_SEP_LEV);
	      nextch();
	      while (ch != ';' && !isEol(ch)) {
		  if (ch == '\\') {
		      ch = FNM_SEP_LEV;
		  }
		  s_catchar(&path, (char)ch);
		  nextch();
	      }
	      if (ch == ';') nextch_handling_eol();
	      break;
	  }
	  case ']': {
	      // Leave subdirectory.
	      struct mak_folder *p = folder_stack;
	      if (folder_stack == NULL) {
		  // FIXME: Error?  Check what Compass does.
		  break;
	      }
	      s_truncate(&path, folder_stack->len);
	      folder_stack = folder_stack->next;
	      osfree(p);
	      nextch();
	      skipblanks();
	      if (ch == ';') nextch_handling_eol();
	      break;
	  }
	  case '@': {
	      /* "Base Location" to calculate magnetic declination at:
	       * UTM East, UTM North, Elevation, UTM Zone, Convergence Angle
	       * The first three are in metres.
	       */
	      nextch();
	      real easting = read_numeric(false);
	      skipblanks();
	      if (ch != ',') break;
	      nextch();
	      real northing = read_numeric(false);
	      skipblanks();
	      if (ch != ',') break;
	      nextch();
	      real elevation = read_numeric(false);
	      skipblanks();
	      if (ch != ',') break;
	      nextch();
	      int zone = read_int(-60, 60);
	      skipblanks();
	      if (ch != ',') break;
	      nextch();
	      real convergence_angle = read_numeric(false);
	      /* We've now read them all successfully so store them.  The
	       * Compass documentation gives an example which specifies the
	       * datum *AFTER* the base location, so we need to convert lazily.
	       */
	      base_x = easting;
	      base_y = northing;
	      base_z = elevation;
	      base_utm_zone = zone;
	      base_line = file.line;
	      base_lpos = file.lpos;
	      // We ignore the stored UTM grid convergence angle since we get
	      // this from PROJ.
	      (void)convergence_angle;
	      if (ch == ';') nextch_handling_eol();
	      break;
	  }
	  default:
	    nextch_handling_eol();
	    break;
	}
    }

    while (folder_stack) {
	// FIXME: Error?  Check what Compass does.
	struct mak_folder *next = folder_stack->next;
	osfree(folder_stack);
	folder_stack = next;
    }

    pop_settings();
    s_free(&path);
}

// We don't expect a huge number of macros, and this doesn't limit how many we
// can handle, only at what point access time stops being O(1).
#define WALLS_MACRO_HASH_SIZE 0x100

typedef struct walls_macro {
    struct walls_macro *next;
    char *name;
    char *value;
} walls_macro;

// Macros set in the WPJ persist, but those set in an SRV only apply for that
// SRV so we keep a table for each and when expanding them in the SRV we use
// a definition from the SRV file in preference to one from the WPJ file.
//
// Definitions actually also get added to walls_macros, but we swap the tables
// around both before and after processing an SRV file (and we clear the
// table from the SRV file at the end of the file).
//
// Testing with Walls, macro definitions are NOT affected by SAVE, RESTORE or
// RESET.
static walls_macro **walls_macros_wpj = NULL;
static walls_macro **walls_macros = NULL;

static void
walls_swap_macro_tables()
{
    walls_macro **tmp = walls_macros_wpj;
    walls_macros_wpj = walls_macros;
    walls_macros = tmp;
}

// Takes ownership of the contents of p_name and of value.
// Passing NULL for value sets empty string.
static void
walls_set_macro(walls_macro ***table, string *p_name, char *val)
{
    //printf("MACRO: $|%s|=\"%s\":\n", name, val);
    if (!*table) {
	*table = osmalloc(WALLS_MACRO_HASH_SIZE * ossizeof(walls_macro*));
	for (size_t i = 0; i < WALLS_MACRO_HASH_SIZE; i++)
	    (*table)[i] = NULL;
    }

    unsigned h = hash_data(s_str(p_name), s_len(p_name)) &
		 (WALLS_MACRO_HASH_SIZE - 1);
    walls_macro *p = (*table)[h];
    while (p) {
	if (s_eq(p_name, p->name)) {
	    // Update existing definition of macro.
	    s_free(p_name);
	    osfree(p->value);
	    p->value = val;
	    return;
	}
	p = p->next;
    }

    walls_macro *entry = osnew(walls_macro);
    entry->name = s_steal(p_name);
    entry->value = val;
    entry->next = (*table)[h];
    (*table)[h] = entry;
}

// Returns NULL if not set.
static const char*
walls_get_macro(walls_macro ***table, const char *name, int name_len)
{
    if (!*table) return NULL;

    unsigned h = hash_data(name, name_len) & (WALLS_MACRO_HASH_SIZE - 1);
    walls_macro *p = (*table)[h];
    while (p) {
	if (strcmp(p->name, name) == 0) {
	    return p->value ? p->value : "";
	}
	p = p->next;
    }

    return NULL;
}

typedef enum {
    WALLS_CMD_DATE,
    WALLS_CMD_FLAG,
    WALLS_CMD_FIX,
    WALLS_CMD_NOTE,
    WALLS_CMD_PREFIX,
    WALLS_CMD_PREFIX2,
    WALLS_CMD_PREFIX3,
    WALLS_CMD_SEGMENT,
    WALLS_CMD_SYMBOL,
    WALLS_CMD_UNITS,
    WALLS_CMD_NULL = -1
} walls_cmd;

static const sztok walls_cmd_tab[] = {
    {"DATE",	WALLS_CMD_DATE},
    {"F",	WALLS_CMD_FLAG}, // Abbreviated form.
    {"FIX",	WALLS_CMD_FIX},
    {"FLAG",	WALLS_CMD_FLAG},
    {"N",	WALLS_CMD_NOTE}, // Abbreviated form.
    {"NOTE",	WALLS_CMD_NOTE},
    {"P",	WALLS_CMD_PREFIX}, // Abbreviated form.
    {"PREFIX",	WALLS_CMD_PREFIX},
    {"PREFIX1",	WALLS_CMD_PREFIX}, // Alias.
    {"PREFIX2",	WALLS_CMD_PREFIX2},
    {"PREFIX3",	WALLS_CMD_PREFIX3},
    {"S",	WALLS_CMD_SEGMENT}, // Abbreviated form.
    {"SEG",	WALLS_CMD_SEGMENT}, // Abbreviated form.
    {"SEGMENT",	WALLS_CMD_SEGMENT},
    {"SYM",	WALLS_CMD_SYMBOL},
    {"SYMBOL",	WALLS_CMD_SYMBOL},
    {"U",	WALLS_CMD_UNITS}, // Abbreviated form.
    {"UNITS",	WALLS_CMD_UNITS},
    {NULL,	WALLS_CMD_NULL}
};

typedef enum {
    WALLS_UNITS_OPT_A,
    WALLS_UNITS_OPT_AB,
    WALLS_UNITS_OPT_CASE,
    WALLS_UNITS_OPT_CT,
    WALLS_UNITS_OPT_D,
    WALLS_UNITS_OPT_DECL,
    WALLS_UNITS_OPT_FEET,
    WALLS_UNITS_OPT_FLAG,
    WALLS_UNITS_OPT_INCA,
    WALLS_UNITS_OPT_INCAB,
    WALLS_UNITS_OPT_INCD,
    WALLS_UNITS_OPT_INCH,
    WALLS_UNITS_OPT_INCV,
    WALLS_UNITS_OPT_INCVB,
    WALLS_UNITS_OPT_LRUD,
    WALLS_UNITS_OPT_METERS,
    WALLS_UNITS_OPT_NOTE,
    WALLS_UNITS_OPT_ORDER,
    WALLS_UNITS_OPT_PREFIX,
    WALLS_UNITS_OPT_PREFIX2,
    WALLS_UNITS_OPT_PREFIX3,
    WALLS_UNITS_OPT_RECT,
    WALLS_UNITS_OPT_RESET,
    WALLS_UNITS_OPT_RESTORE,
    WALLS_UNITS_OPT_S,
    WALLS_UNITS_OPT_SAVE,
    WALLS_UNITS_OPT_TAPE,
    WALLS_UNITS_OPT_TYPEAB,
    WALLS_UNITS_OPT_TYPEVB,
    WALLS_UNITS_OPT_UV,
    WALLS_UNITS_OPT_UVH,
    WALLS_UNITS_OPT_UVV,
    WALLS_UNITS_OPT_V,
    WALLS_UNITS_OPT_VB,
    WALLS_UNITS_OPT_NULL = -1
} walls_units_opt;

// The aliases AZIMUTH, DISTANCE and VERTICAL don't seem to be documented.
// They were found from `strings Walls32.exe` and then testing.
static const sztok walls_units_opt_tab[] = {
    {"A",		WALLS_UNITS_OPT_A},
    {"AB",		WALLS_UNITS_OPT_AB},
    {"AZIMUTH",		WALLS_UNITS_OPT_A}, // Alias (undocumented).
    {"CASE",		WALLS_UNITS_OPT_CASE},
    {"CT",		WALLS_UNITS_OPT_CT},
    {"D",		WALLS_UNITS_OPT_D},
    {"DECL",		WALLS_UNITS_OPT_DECL},
    {"DISTANCE",	WALLS_UNITS_OPT_D}, // Alias (undocumented).
    {"F",		WALLS_UNITS_OPT_FEET}, // Abbreviated form.
    {"FEET",		WALLS_UNITS_OPT_FEET},
    {"FLAG",		WALLS_UNITS_OPT_FLAG},
    // FIXME: GRID=
    {"INCA",		WALLS_UNITS_OPT_INCA},
    {"INCAB",		WALLS_UNITS_OPT_INCAB},
    {"INCD",		WALLS_UNITS_OPT_INCD},
    {"INCH",		WALLS_UNITS_OPT_INCH},
    {"INCV",		WALLS_UNITS_OPT_INCV},
    {"INCVB",		WALLS_UNITS_OPT_INCVB},
    {"LRUD",		WALLS_UNITS_OPT_LRUD},
    {"M",		WALLS_UNITS_OPT_METERS}, // Abbreviated form.
    {"METERS",		WALLS_UNITS_OPT_METERS},
    {"NOTE",		WALLS_UNITS_OPT_NOTE},
    {"O",		WALLS_UNITS_OPT_ORDER}, // Abbreviated form.
    {"ORDER",		WALLS_UNITS_OPT_ORDER},
    {"P",		WALLS_UNITS_OPT_PREFIX}, // Abbreviated form.
    {"PREFIX",		WALLS_UNITS_OPT_PREFIX},
    {"PREFIX1",		WALLS_UNITS_OPT_PREFIX}, // Alias.
    {"PREFIX2",		WALLS_UNITS_OPT_PREFIX2},
    {"PREFIX3",		WALLS_UNITS_OPT_PREFIX3},
    {"RECT",		WALLS_UNITS_OPT_RECT},
    {"RESET",		WALLS_UNITS_OPT_RESET},
    {"RESTORE",		WALLS_UNITS_OPT_RESTORE},
    {"S",		WALLS_UNITS_OPT_S},
    {"SAVE",		WALLS_UNITS_OPT_SAVE},
    {"TAPE",		WALLS_UNITS_OPT_TAPE},
    {"TYPEAB",		WALLS_UNITS_OPT_TYPEAB},
    {"TYPEVB",		WALLS_UNITS_OPT_TYPEVB},
    {"UV",		WALLS_UNITS_OPT_UV},
    {"UVH",		WALLS_UNITS_OPT_UVH},
    {"UVV",		WALLS_UNITS_OPT_UVV},
    {"V",		WALLS_UNITS_OPT_V},
    {"VB",		WALLS_UNITS_OPT_VB},
    {"VERTICAL",	WALLS_UNITS_OPT_V}, // Alias (undocumented).
    {NULL,		WALLS_UNITS_OPT_NULL}
};

#define WALLS_ORDER_CT(R1,R2,R3) ((R1) | ((R2) << 8) | ((R3) << 16))
#define WALLS_ORDER_RECT(R1,R2,R3) ((R1) | ((R2) << 8) | ((R3) << 16) | (1 << 24))

// Here we rely on the integer values of the reading codes used fitting in
// a byte so assert that is the case.
typedef int compiletimeassert_order_byte_encoding_ok[
    (WallsSRVTape|WallsSRVComp|WallsSRVClino|Dx|Dy|Dz) < 0x100 ? 1 : -1];

static const sztok walls_order_tab[] = {
    {"AD",	WALLS_ORDER_CT(WallsSRVComp, WallsSRVTape, 0)},
    {"ADV",	WALLS_ORDER_CT(WallsSRVComp, WallsSRVTape, WallsSRVClino)},
    {"AVD",	WALLS_ORDER_CT(WallsSRVComp, WallsSRVClino, WallsSRVTape)},
    {"DA",	WALLS_ORDER_CT(WallsSRVTape, WallsSRVComp, 0)},
    {"DAV",	WALLS_ORDER_CT(WallsSRVTape, WallsSRVComp, WallsSRVClino)},
    {"DVA",	WALLS_ORDER_CT(WallsSRVTape, WallsSRVClino, WallsSRVComp)},
    {"EN",	WALLS_ORDER_RECT(Dx, Dy, 0)},
    {"ENU",	WALLS_ORDER_RECT(Dx, Dy, Dz)},
    {"EUN",	WALLS_ORDER_RECT(Dx, Dz, Dy)},
    {"NE",	WALLS_ORDER_RECT(Dy, Dx, 0)},
    {"NEU",	WALLS_ORDER_RECT(Dy, Dx, Dz)},
    {"NUE",	WALLS_ORDER_RECT(Dy, Dz, Dx)},
    {"UEN",	WALLS_ORDER_RECT(Dz, Dx, Dy)},
    {"UNE",	WALLS_ORDER_RECT(Dz, Dy, Dx)},
    {"VAD",	WALLS_ORDER_CT(WallsSRVClino, WallsSRVComp, WallsSRVTape)},
    {"VDA",	WALLS_ORDER_CT(WallsSRVClino, WallsSRVTape, WallsSRVComp)},
    {NULL,	-1}
};

// Walls seems to only document `/` but based on real-world use also allows
// `\`.  FIXME: Check for each situation.
static inline bool isWallsSlash(int c) { return c == '/' || c == '\\'; }

// Walls-specific options.  Options which Survex has a direct equivalent of
// are stored in the `settings` struct instead.
typedef struct walls_options {
    // The current values of the three prefix levels Walls supports.
    // NULL for any level not currently set (all NULL by default).
    char* prefix[3];

    // Data order for CT legs.
    reading data_order_ct[6];

    // Data order for RECT legs (also used for #Fix coordinate order).
    reading data_order_rect[6];

    // Is this from SAVE in .OPTIONS / #Units?
    bool explicit;

    struct walls_options *next;
} walls_options;

static walls_options* p_walls_options;

static const walls_options walls_options_default = {
    // prefix[3]
    { NULL, NULL, NULL },

    // data_order_ct[6]
    {
	WallsSRVFr, WallsSRVTo, WallsSRVTape, WallsSRVComp, WallsSRVClino,
// FIXME	CompassDATLeft, CompassDATUp, CompassDATDown, CompassDATRight,
	IgnoreAll
    },

    // data_order_rect[6]
    {
	WallsSRVFr, WallsSRVTo, Dx, Dy, Dz,
// FIXME	CompassDATLeft, CompassDATUp, CompassDATDown, CompassDATRight,
	IgnoreAll
    },

    // explicit
    false,

    // next
    NULL
};

static void
push_walls_options(void)
{
    settings *pcsNew = osnew(settings);
    *pcsNew = *pcs; /* copy contents */
    pcsNew->begin_lineno = file.line;
    pcsNew->next = pcs;
    pcs = pcsNew;

    // Walls-specific settings.
    walls_options *new_options = osnew(walls_options);
    if (!p_walls_options) {
	*new_options = walls_options_default;
    } else {
	*new_options = *p_walls_options; /* copy contents */
	// There's only 3 prefix levels and typically only one seems to be
	// actually set so just copy the strings rather than trying to do
	// copy-on-write.
	for (int i = 0; i < 3; ++i) {
	    if (new_options->prefix[i])
		new_options->prefix[i] = osstrdup(new_options->prefix[i]);
	}
    }

    new_options->next = p_walls_options;
    p_walls_options = new_options;
}

static void
pop_walls_options(void)
{
    pcs->ordering = NULL; /* Avoid free() of static array. */
    pop_settings();
    walls_options *p = p_walls_options;
    p_walls_options = p_walls_options->next;
    for (int i = 0; i < 3; ++i) {
	osfree(p->prefix[i]);
    }
}

static void
walls_initialise_settings(void)
{
    push_walls_options();

    // Generic settings.
    short *t = ((short*)osmalloc(ossizeof(short) * 257)) + 1;
    // "Unprefixed names can have a maximum of eight characters and must not
    // contain any colons, semicolons, commas, pound signs (#), or embedded
    // tabs or spaces.  In order to avoid possible problems when printing or
    // when exporting data to other programs, you are encouraged to restrict
    // names in new surveys to numbers with alphabetic prefixes or suffixes
    // (e.g., BR123)."
    //
    // We assume other control characters aren't allowed either (so nothing
    // < 32 and not 127), but allow all top-bit-set characters.
    t[EOF] = SPECIAL_EOL;
    memset(t, 0, sizeof(short) * 33);
    for (int i = 33; i < 127; i++) t[i] = SPECIAL_NAMES;
    t[127] = 0;
    for (int i = 128; i < 256; i++) t[i] = SPECIAL_NAMES;
    t[':'] = 0;
    t[';'] = SPECIAL_COMMENT;
    // FIXME: `,` seems to be treated like a space almost everywhere, but right
    // after a directive name a comma instead gives a warning suggesting a
    // parse error in a different directive.
    t[','] = SPECIAL_BLANK;
    t['#'] = 0;
    t['\t'] |= SPECIAL_BLANK;
    t[' '] |= SPECIAL_BLANK;
    t['\032'] |= SPECIAL_EOL; /* Ctrl-Z, so olde DOS text files are handled ok */
    t['\n'] |= SPECIAL_EOL;
    t['\r'] |= SPECIAL_EOL;
    t['-'] |= SPECIAL_OMIT;
    t['-'] |= SPECIAL_ANON;
    t[':'] |= SPECIAL_SEPARATOR|SPECIAL_ROOT;
    t['.'] |= SPECIAL_DECIMAL;
    t['-'] |= SPECIAL_MINUS;
    t['+'] |= SPECIAL_PLUS;
    pcs->Translate = t;

    pcs->begin_lineno = 0;
    // Spec says "maximum of eight characters" - we currently allow arbitrarily
    // many.
    pcs->Truncate = INT_MAX;
    pcs->infer = BIT(INFER_EXPORTS) | // FIXME?
		 BIT(INFER_PLUMBS);
}

static void
walls_reset(void)
{
    // "[S]et all parameters (including the current name prefix) to their
    // defaults" but not the segment.  We currently ignore the segment anyway.
    pcs->Case = OFF;

    default_units(pcs);
    default_calib(pcs);
    // FIXME: pcs->z[Q_DECLINATION] = HUGE_REAL;

    pcs->recorded_style = pcs->style = STYLE_NORMAL;
    pcs->ordering = p_walls_options->data_order_ct;

    for (int i = 0; i < 3; ++i) {
	osfree(p_walls_options->prefix[i]);
    }
    *p_walls_options = walls_options_default;
}

static void
parse_options(void)
{
    skipblanks();
    while (!isEol(ch)) {
	get_token_walls();
	if (s_empty(&token) && isComm(ch)) {
	    break;
	}
	// Assign to typed variable so we get a warning if we are
	// missing a case below.
	walls_units_opt opt = match_tok(walls_units_opt_tab,
					TABSIZE(walls_units_opt_tab));
	switch (opt) {
	  case WALLS_UNITS_OPT_METERS:
	    pcs->units[Q_LENGTH] =
		pcs->units[Q_DX] =
		pcs->units[Q_DY] =
		pcs->units[Q_DZ] = 1.0;
	    break;
	  case WALLS_UNITS_OPT_FEET:
	    pcs->units[Q_LENGTH] =
		pcs->units[Q_DX] =
		pcs->units[Q_DY] =
		pcs->units[Q_DZ] = METRES_PER_FOOT;
	    break;
	  case WALLS_UNITS_OPT_D:
	    skipblanks();
	    if (ch == '=') {
		nextch();
		get_token_walls();
		// From testing it seems Walls only checks the initial letter - e.g.
		// "M", "METERS", "METRES", "F", "FEET" and even "FISH" are accepted,
		// but "X" gives an error.
		if (s_str(&uctoken)[0] == 'M') {
		    pcs->units[Q_LENGTH] = 1.0;
		} else if (s_str(&uctoken)[0] == 'F') {
		    pcs->units[Q_LENGTH] = METRES_PER_FOOT;
		} else {
		    // FIXME: Error?
		}
	    } else {
		// FIXME: Error?
	    }
	    break;
	  case WALLS_UNITS_OPT_A:
	    skipblanks();
	    if (ch == '=') {
		nextch();
		get_token_walls();
		// It seems Walls only checks the initial letter.
		if (s_str(&uctoken)[0] == 'D') {
		    // Degrees.
		    pcs->units[Q_BEARING] = M_PI / 180.0;
		} else if (s_str(&uctoken)[0] == 'G') {
		    // Grads.
		    pcs->units[Q_BEARING] = M_PI / 200.0;
		} else if (s_str(&uctoken)[0] == 'M') {
		    // Mils.
		    pcs->units[Q_BEARING] = M_PI / 3200.0;
		} else {
		    // FIXME: Error?
		}
	    } else {
		// FIXME: Error?
	    }
	    break;
	  case WALLS_UNITS_OPT_AB:
	    skipblanks();
	    if (ch == '=') {
		nextch();
		get_token_walls();
		// It seems Walls only checks the initial letter.
		if (s_str(&uctoken)[0] == 'D') {
		    // Degrees.
		    pcs->units[Q_BACKBEARING] = M_PI / 180.0;
		} else if (s_str(&uctoken)[0] == 'G') {
		    // Grads.
		    pcs->units[Q_BACKBEARING] = M_PI / 200.0;
		} else if (s_str(&uctoken)[0] == 'M') {
		    // Mils.
		    pcs->units[Q_BACKBEARING] = M_PI / 3200.0;
		} else {
		    // FIXME: Error?
		}
	    } else {
		// FIXME: Error?
	    }
	    break;
	  case WALLS_UNITS_OPT_V:
	    skipblanks();
	    if (ch == '=') {
		nextch();
		get_token_walls();
		pcs->f_clino_percent = false;
		// It seems Walls only checks the initial letter.
		if (s_str(&uctoken)[0] == 'D') {
		    // Degrees.
		    pcs->units[Q_GRADIENT] = M_PI / 180.0;
		} else if (s_str(&uctoken)[0] == 'G') {
		    // Grads.
		    pcs->units[Q_GRADIENT] = M_PI / 200.0;
		} else if (s_str(&uctoken)[0] == 'M') {
		    // Mils.
		    pcs->units[Q_GRADIENT] = M_PI / 3200.0;
		} else if (S_EQ(&uctoken, "PERCENT")) {
		    pcs->units[Q_GRADIENT] = 0.01;
		    pcs->f_clino_percent = true;
		} else {
		    // FIXME: Error?
		}
	    } else {
		// FIXME: Error?
	    }
	    break;
	  case WALLS_UNITS_OPT_VB:
	    skipblanks();
	    if (ch == '=') {
		nextch();
		get_token_walls();
		pcs->f_backclino_percent = false;
		// It seems Walls only checks the initial letter.
		if (s_str(&uctoken)[0] == 'D') {
		    // Degrees.
		    pcs->units[Q_BACKGRADIENT] = M_PI / 180.0;
		} else if (s_str(&uctoken)[0] == 'G') {
		    // Grads.
		    pcs->units[Q_BACKGRADIENT] = M_PI / 200.0;
		} else if (s_str(&uctoken)[0] == 'M') {
		    // Mils.
		    pcs->units[Q_BACKGRADIENT] = M_PI / 3200.0;
		} else if (S_EQ(&uctoken, "PERCENT")) {
		    pcs->units[Q_BACKGRADIENT] = 0.01;
		    pcs->f_backclino_percent = true;
		} else {
		    // FIXME: Error?
		}
	    } else {
		// FIXME: Error?
	    }
	    break;
	  case WALLS_UNITS_OPT_S:
	    skipblanks();
	    if (ch == '=') {
		nextch();
		get_token_walls();
		// From testing it seems Walls only checks the initial letter - e.g.
		// "M", "METERS", "METRES", "F", "FEET" and even "FISH" are accepted,
		// but "X" gives an error.
		if (s_str(&uctoken)[0] == 'M') {
		    pcs->units[Q_DX] =
		    pcs->units[Q_DY] =
		    pcs->units[Q_DZ] = 1.0;
		} else if (s_str(&uctoken)[0] == 'F') {
		    pcs->units[Q_DX] =
		    pcs->units[Q_DY] =
		    pcs->units[Q_DZ] = METRES_PER_FOOT;
		} else {
		    // FIXME: Error?
		}
	    } else {
		// FIXME: Error?
	    }
	    break;
	  case WALLS_UNITS_OPT_ORDER:
	    skipblanks();
	    if (ch == '=') {
		nextch();
		get_token_walls();
		int order = match_tok(walls_order_tab,
				      TABSIZE(walls_order_tab));
		if (order < 0) {
		    compile_diagnostic(DIAG_ERR|DIAG_TOKEN|DIAG_SKIP, /*Data style “%s” unknown*/65, s_str(&token));
		    break;
		}
		reading* p;
		if (order & (1 << 24)) {
		    order &= ((1 << 24) - 1);
		    // "RECT" order.
		    p = p_walls_options->data_order_rect + 2;
		} else {
		    // "CT" order.
		    p = p_walls_options->data_order_ct + 2;
		}
		while (order) {
		    *p++ = (order & 0xff);
		    order >>= 8;
		}
		*p = IgnoreAll;
	    } else {
		// FIXME: Error?
	    }
	    break;
	  case WALLS_UNITS_OPT_DECL:
	    skipblanks();
	    if (ch == '=') {
		nextch();
//pcs->declination = HUGE_REAL;
//	if (pcs->dec_filename == NULL) {
		pcs->z[Q_DECLINATION] = -read_numeric(false);
		pcs->z[Q_DECLINATION] *= pcs->units[Q_DECLINATION];
//	} else {
//	    (void)read_numeric(false);
//	}
	    }
	    break;
	  case WALLS_UNITS_OPT_INCA:
	    skipblanks();
	    if (ch == '=') {
		nextch();
		pcs->z[Q_BEARING] = -rad(read_numeric(false));
		// FIXME: Handle angle units
	    }
	    break;
	  case WALLS_UNITS_OPT_INCAB:
	    skipblanks();
	    if (ch == '=') {
		nextch();
		pcs->z[Q_BACKBEARING] = -rad(read_numeric(false));
		// FIXME: Handle angle units
	    }
	    break;
	  case WALLS_UNITS_OPT_INCD:
	    skipblanks();
	    if (ch == '=') {
		nextch();
		pcs->z[Q_LENGTH] = -read_numeric(false);
		// FIXME: Handle length units
	    }
	    break;
	  case WALLS_UNITS_OPT_INCH:
	    skipblanks();
	    if (ch == '=') {
		// FIXME: Actually apply this correction.
		compile_diagnostic(DIAG_WARN|DIAG_TOKEN|DIAG_SKIP, /*Unknown command “%s”*/12, s_str(&token));
		nextch();
		(void)read_numeric(false);
		// FIXME: Handle length units
	    }
	    break;
	  case WALLS_UNITS_OPT_INCV:
	    skipblanks();
	    if (ch == '=') {
		nextch();
		pcs->z[Q_GRADIENT] = -rad(read_numeric(false));
		// FIXME: Handle angle units
	    }
	    break;
	  case WALLS_UNITS_OPT_INCVB:
	    skipblanks();
	    if (ch == '=') {
		nextch();
		pcs->z[Q_BACKGRADIENT] = -rad(read_numeric(false));
		// FIXME: Handle angle units
	    }
	    break;
	  case WALLS_UNITS_OPT_RECT:
	    // There are two different RECT options, one with a
	    // parameter and one without!
	    skipblanks();
	    if (ch == '=') {
		// FIXME: This seems to be a bearing to rotate
		// cartesian data by, which we don't currently support.
		//compile_diagnostic(DIAG_WARN|DIAG_TOKEN|DIAG_SKIP, /*Unknown command “%s”*/12, s_str(&token));
		nextch();
		(void)read_numeric(false);
	    } else {
		pcs->recorded_style = pcs->style = STYLE_CARTESIAN;
		pcs->ordering = p_walls_options->data_order_rect;
	    }
	    break;
	  case WALLS_UNITS_OPT_CASE:
	    skipblanks();
	    if (ch == '=') {
		nextch();
		get_token_walls();
		switch (s_str(&uctoken)[0]) {
		  case 'L':
		    pcs->Case = LOWER;
		    break;
		  case 'U':
		    pcs->Case = UPPER;
		    break;
		  case 'M':
		    pcs->Case = OFF;
		    break;
		  default:
		    compile_diagnostic(DIAG_WARN|DIAG_TOKEN|DIAG_SKIP, /*Unknown command “%s”*/12, s_str(&token));
		    break;
		}
	    } else {
		// FIXME: Error?
	    }
	    break;
	  case WALLS_UNITS_OPT_CT:
	    pcs->recorded_style = pcs->style = STYLE_NORMAL;
	    pcs->ordering = p_walls_options->data_order_ct;
	    break;
	  case WALLS_UNITS_OPT_PREFIX:
	  case WALLS_UNITS_OPT_PREFIX2:
	  case WALLS_UNITS_OPT_PREFIX3: {
	    char *new_prefix = NULL;
	    skipblanks();
	    if (ch == '=') {
		nextch();
		new_prefix = read_walls_prefix();
	    }
	    int i = (int)WALLS_UNITS_OPT_PREFIX3 - (int)opt;
	    osfree(p_walls_options->prefix[i]);
	    p_walls_options->prefix[i] = new_prefix;
	    break;
	  }
	  case WALLS_UNITS_OPT_LRUD:
	    skipblanks();
	    if (ch == '=') {
		nextch();
		// We currently ignore LRUD, so can also ignore the
		// settings for it.
		string val = S_INIT;
		read_string(&val);
		s_free(&val);
	    } else {
		// FIXME: Anything to do?
	    }
	    break;
	  case WALLS_UNITS_OPT_TAPE:
	    skipblanks();
	    if (ch == '=') {
		nextch();
		get_token_walls();
		/* FIXME: Implement different taping methods? */
	    } else {
		// FIXME: Anything to do?
	    }
	    break;
	  case WALLS_UNITS_OPT_TYPEAB:
	    skipblanks();
	    if (ch == '=') {
		nextch();
		get_token_walls();
		if (s_str(&uctoken)[0] == 'N') {
		    pcs->z[Q_BACKBEARING] = 0.0;
		} else if (s_str(&uctoken)[0] == 'C') {
		    pcs->z[Q_BACKBEARING] = M_PI;
		} else {
		    compile_diagnostic(DIAG_WARN|DIAG_TOKEN|DIAG_SKIP, /*Unknown command “%s”*/12, s_str(&token));
		}
		if (ch == ',') {
		    nextch();
		    // FIXME: Use threshold value.
		    (void)read_numeric(false);
		    if (ch == ',') {
			nextch();
			if (toupper(ch) == 'X') {
			    nextch();
			    // FIXME: Only use foresight (but check backsight).
			}
		    }
		}
	    } else {
		// FIXME: Anything to do?
	    }
	    break;
	  case WALLS_UNITS_OPT_TYPEVB:
	    skipblanks();
	    if (ch == '=') {
		nextch();
		get_token_walls();
		if (s_str(&uctoken)[0] == 'N') {
		    pcs->sc[Q_BACKGRADIENT] = 1.0;
		} else if (s_str(&uctoken)[0] == 'C') {
		    pcs->sc[Q_BACKGRADIENT] = -1.0;
		} else {
		    compile_diagnostic(DIAG_WARN|DIAG_TOKEN|DIAG_SKIP, /*Unknown command “%s”*/12, s_str(&token));
		}
		if (ch == ',') {
		    nextch();
		    // FIXME: Use threshold value.
		    (void)read_numeric(false);
		    if (ch == ',') {
			nextch();
			if (toupper(ch) == 'X') {
			    nextch();
			    // FIXME: Only use foresight (but check backsight).
			}
		    }
		}
	    } else {
		// FIXME: Anything to do?
	    }
	    break;
	  case WALLS_UNITS_OPT_UV:
	  case WALLS_UNITS_OPT_UVH:
	  case WALLS_UNITS_OPT_UVV:
	    // Scale factors for variances (with horizontal-only and
	    // vertical-only variants).  FIXME: Actually apply these!
	    skipblanks();
	    if (ch == '=') {
		nextch();
		(void)read_numeric(false);
	    } else {
		// FIXME: Anything to do?
	    }
	    break;
	  case WALLS_UNITS_OPT_FLAG:
	  case WALLS_UNITS_OPT_NOTE:
	    // Currently ignored.
	    // FIXME: FLAG= ought to get mapped like #FLAG.
	    skipblanks();
	    if (ch == '=') {
		nextch();
		string val = S_INIT;
		read_string(&val);
		s_free(&val);
	    } else {
		// FIXME: FLAG alone clears the default flag name.
		// FIXME: Anything to do for NOTE?  Error?
	    }
	    break;
	  case WALLS_UNITS_OPT_RESET:
	    // FIXME: This should be processed before other arguments!
	    walls_reset();
	    break;
	  case WALLS_UNITS_OPT_RESTORE: {
	    // FIXME: Should this be processed before other arguments?
	    if (!p_walls_options->explicit) {
		// FIXME: RESTORE ... SAVE
		compile_diagnostic(DIAG_ERR|DIAG_SKIP, /*END with no matching BEGIN in this file*/22);
		break;
	    }
	    pop_walls_options();
	    break;
	  }
	  case WALLS_UNITS_OPT_SAVE: {
	    // FIXME: This should be processed before other arguments!
	    push_walls_options();
	    p_walls_options->explicit = true;
	    break;
	  }
	  case WALLS_UNITS_OPT_NULL:
	    if (s_str(&uctoken)[0] == '\0' && ch == '$') {
		// Macro definition.
		filepos fp;
		get_pos(&fp);
		nextch();
		string name = S_INIT;
		while (!isEol(ch) && !isBlank(ch) && ch != '=') {
		    s_catchar(&name, ch);
		    nextch();
		}
		if (!s_empty(&name)) {
		    skipblanks();
		    if (ch != '=') {
			// Set an empty value.
			walls_set_macro(&walls_macros, &name, NULL);
		    } else {
			nextch();
			string val = S_INIT;
			read_string(&val);
			walls_set_macro(&walls_macros, &name, s_steal(&val));
		    }
		    break;
		}
		s_free(&name);
		set_pos(&fp);
		s_clear(&token);
	    }
	    compile_diagnostic(DIAG_ERR|DIAG_TOKEN|DIAG_SKIP, /*Unknown command “%s”*/12, s_str(&token));
	    break;
	}
//		pcs->z[Q_BACKBEARING] = pcs->z[Q_BEARING] = -rad(read_numeric(false));
//		pcs->z[Q_BACKGRADIENT] = pcs->z[Q_GRADIENT] = -rad(read_numeric(false));
//		pcs->z[Q_LENGTH] = -METRES_PER_FOOT * read_numeric(false);

    /* Original "Inclination Units" were "Depth Gauge". */
    //pcs->recorded_style = STYLE_DIVING;
    //skipline();
	skipblanks();
    }
}

static void
data_file_walls_srv(void)
{
    // FIXME: Do any of these variables need to be volatile to protect them
    // from longjmp()?  GCC isn't warning about them...

    bool standalone = (p_walls_options == NULL);
    if (standalone) {
	// We're being included standalone rather than from a WPJ file.
	walls_initialise_settings();
	walls_reset();
    }

    // FIXME: We need to update the separator_map to reflect what can be
    // SPECIAL_NAMES.  Or should we use the Compass approach and base this
    // on what's actually used?  The first approach would pick the separator
    // from {':', ';', ',', '#', space}; the latter would pick '.' if
    // the documentation station naming recommendations were followed.
    update_output_separator();

    /* We need to update separator_map so we don't pick a separator character
     * which occurs in a station name.  However Compass DAT allows everything
     * >= ASCII char 33 except 127 in station names so if we just added all
     * the valid station name characters we'd always pick space as the
     * separator for any dataset which included a DAT file, yet in practice
     * '.' is never used in any of the sample DAT files I've seen.  So
     * instead we scan the characters actually used in station names when we
     * process CompassDATFr and CompassDATTo fields. (FIXME)
     */

#ifdef HAVE_SETJMP_H
    /* errors in nested functions can longjmp here */
    if (setjmp(file.jbSkipLine)) {
	skipline();
	process_eol();
    }
#endif

    if (pcs->style == STYLE_NORMAL)
	pcs->ordering = p_walls_options->data_order_ct;
    else
	pcs->ordering = p_walls_options->data_order_rect;

    while (ch != EOF && !ferror(file.fh)) {
next_line:
	skipblanks();
	if (ch != '#') {
	    if (ch == ';' || isEol(ch)) {
		skipline();
		process_eol();
	    } else if (pcs->style == STYLE_NORMAL) {
		data_normal();
	    } else {
		// Set up Dz in case it's omitted.
		VAL(Dz) = 0.0;
		VAR(Dz) = 10.0;
		data_cartesian();
	    }
	    continue;
	}

	// Directive:
	int leading_blanks = ftell(file.fh) - file.lpos - 1;
	nextch();
	if (ch == '[') {
	    // "Commented out" data.
	    skipline();
	    process_eol();

	    int depth = 1;
	    while (depth && ch != EOF) {
		skipblanks();
		if (ch == '#') {
		    nextch();
		    depth += (ch == '[') - (ch == ']');
		}
		skipline();
		process_eol();
	    }
	    if (depth) {
		// FIXME Report unclosed `#[`.
	    }
	    goto next_line;
	}
	if (ch == ']') {
	    // FIXME Report excess `#]`.
	}
	skipblanks();
	int blanks_after_hash = ftell(file.fh) - file.lpos - leading_blanks - 2;
	get_token_walls();
	walls_cmd directive = match_tok(walls_cmd_tab, TABSIZE(walls_cmd_tab));
	parse file_store;
	volatile int ch_store;
	string line = S_INIT;
	if (directive != WALLS_CMD_FIX && directive != WALLS_CMD_NULL) {
	    bool seen_macros = false;
	    // Recreate the start of the line for error reporting.
	    // FIXME: This will change tabs to a single space...
	    if (leading_blanks)
		s_catn(&line, leading_blanks, ' ');
	    s_catchar(&line, '#');
	    if (blanks_after_hash)
		s_catn(&line, blanks_after_hash, ' ');
	    s_cats(&line, &token);

	    filepos fp_args;
	    get_pos(&fp_args);

	    // Expand macros such as $(foo) in rest of line.
	    while (!isEol(ch)) {
		if (ch != '$') {
		    s_catchar(&line, ch);
		    nextch();
		    continue;
		}
		nextch();
		if (ch != '(') {
		    s_catchar(&line, '$');
		    continue;
		}
		nextch();
		// Read name of macro onto the end of line, then replace it
		// with the value of the macro.
		int macro_start = s_len(&line);
		while (!isEol(ch) && ch != ')') {
		    s_catchar(&line, ch);
		    nextch();
		}
		nextch();
		const char *name = s_str(&line) + macro_start;
		int name_len = s_len(&line) - macro_start;
		const char *macro = walls_get_macro(&walls_macros,
						    name, name_len);
		if (!macro) {
		    macro = walls_get_macro(&walls_macros_wpj, name, name_len);
		}
		if (macro) {
		    s_truncate(&line, macro_start);
		    s_cat(&line, macro);
		} else {
		    // FIXME: report proper error.
		    printf("Macro %s not defined\n", s_str(&line) + macro_start);
		    s_truncate(&line, macro_start);
		}
		seen_macros = true;
	    }

	    if (seen_macros) {
		//printf("MACRO EXPANSION <%s>\n", line);
		// Read from the buffered macro-expanded line instead of the
		// file.
		file_store = file;
		ch_store = ch;
#ifdef HAVE_FMEMOPEN
		file.fh = fmemopen((char*)s_str(&line), s_len(&line), "rb");
#else
		file.fh = tmpfile();
		if (!file.fh) {
		    // FIXME: Report failure to create temporary file.
		}
		fwrite(s_str(&line), s_len(&line), 1, file.fh);
#endif
		fseek(file.fh, fp_args.offset - file.lpos, SEEK_SET);
		ch = (unsigned char)s_str(&line)[fp_args.offset - file.lpos - 1];
		file.lpos = 0;
	    } else {
		//printf("no macros seen in <%s>\n", line);
		// No macros seen so rewind and read directly from the file.
		s_free(&line);
		set_pos(&fp_args);
	    }
	}

	switch (directive) {
	  case WALLS_CMD_UNITS:
	    parse_options();
	    break;
	  case WALLS_CMD_DATE: {
	    int year, month, day;
	    read_walls_srv_date(&year, &month, &day);
	    copy_on_write_meta(pcs);
	    int days = days_since_1900(year, month, day);
	    pcs->meta->days1 = pcs->meta->days2 = days;
	    skipblanks();
	    if (!isEol(ch) && !isComm(ch)) {
		// Walls seems to ignore anything after the date so make this a
		// warning not an error so we can process existing Walls
		// datasets (e.g. the Mammoth dataset reportedly has `#Date
		// 1978-07-01\`.
		compile_diagnostic(DIAG_WARN|DIAG_TAIL, /*End of line not blank*/15);
		skipline();
	    }
	    break;
	  }
	  case WALLS_CMD_FIX: {
	    real coords[3];
	    prefix *name = read_walls_station(p_walls_options->prefix, false);
	    // FIXME: can be e.g. `W97:43:52.5    N31:16:45         323f`
	    // Or E/S instead of W/N.

	    enum { UNKNOWN, LATLONG, UTM } format = UNKNOWN;
	    for (int i = 0; i < 3; ++i) {
		// The order of the coordinates is specified by data_order_rect.
		int compiletimeassert_dxdydz[Dy - Dx == 1 && Dz - Dy == 1 ? 1 : -1];
		(void)compiletimeassert_dxdydz;
		int dim = p_walls_options->data_order_rect[i + 2] - Dx;
		if ((unsigned)dim > 2) {
		    // FIXME: Survex doesn't currently support horizontal-only
		    // fixes.
		    coords[2] = 0.0;
		    break;
		}

		real coord;
		skipblanks();
		int upper_ch = toupper(ch);
		if (dim == 2 || format == UTM || strchr("NWES", upper_ch) == NULL) {
		    // Read as a distance if this is the altitude, or we've
		    // already seen a distance for x or y, or if the coordinate
		    // doesn't start with a compass point letter.
		    coord = read_numeric(false);
		    if (ch == 'F' || ch == 'f') {
			coord *= METRES_PER_FOOT;
			nextch();
		    } else if (ch == 'M' || ch == 'm') {
			nextch();
		    } else {
			coord *= pcs->units[Q_LENGTH];
		    }
		    if (dim != 2) format = UTM;
		} else {
		    // Set negate if S or W.
		    bool negate = ((upper_ch & 3) == 3);
		    bool e_or_w = ((upper_ch & 5) == 5);
		    if (dim == e_or_w) {
			compile_diagnostic(DIAG_ERR|DIAG_COL,
					   e_or_w ?
					   /*Expecting “N” or “S”*/492:
					   /*Expecting “E” or “W”*/493);
		    }
		    nextch();
		    coord = read_number(false, true);
		    if (negate) coord = -coord;

		    format = LATLONG;
		}

		coords[dim] = coord;
	    }

	    skipblanks();
	    if (ch == '(') {
		// Optional variance override.  FIXME parse and use
		// E.g. `(R5,?)` specifies horizontal and vertical so:
		// `R5` means 5m RMS horizontal error
		// `?` specifies no elevations were obtained - infinite variance.
		// `*` means ... same as `?`
		// <non-negative number> means treat as compass and tape vector of that length (in length units from #units)
		// `` (empty) means don't override that one
		// no comma e.g. `(R5)` means apply to both h and v
		do {
		    nextch();
		} while (!isEol(ch) && ch != ')');
		if (ch == ')') {
		    nextch();
		    skipblanks();
		}
	    }

	    // FIXME: Convert coordinates based on the current coordinate system
	    // set by .REF in the wpj.
	    if (format == LATLONG) {
		// FIXME: Temporary hack, just scale degrees to give
		// approximate metres.
		coords[0] *= 55649.7 * cos(rad(coords[1]));
		coords[1] *= 55473.1;
	    }

	    name->sflags |= BIT(SFLAGS_FIXED);
	    node *stn = StnFromPfx(name);
	    if (!fixed(stn)) {
		POS(stn, 0) = coords[0];
		POS(stn, 1) = coords[1];
		POS(stn, 2) = coords[2];
		fix(stn);
	    } else {
		if (coords[0] != POS(stn, 0) ||
		    coords[1] != POS(stn, 1) ||
		    coords[2] != POS(stn, 2)) {
		    compile_diagnostic(DIAG_ERR, /*Station already fixed or equated to a fixed point*/46);
		} else {
		    compile_diagnostic(DIAG_WARN, /*Station already fixed at the same coordinates*/55);
		}
	    }

	    if (isWallsSlash(ch)) {
		// Station note - ignore for now.
		skipline();
	    }
	    break;
	  }
	  case WALLS_CMD_FLAG: {
	    // The flag comes after a list of stations, so to avoid having to
	    // store a list of the stations we note the position, scan ahead
	    // and parse the flag, then come back and actually parse the
	    // stations and apply the flag.
	    skipblanks();

	    if (isWallsSlash(ch)) {
		//printf("Default flag\n");
		// FIXME: Handle.
		skipline();
	    } else {
		filepos fp;
		get_pos(&fp);
		while (!isWallsSlash(ch)) {
		    if (isComm(ch) || isEol(ch)) {
			// The flag name is not required (it "can *optionally*
			// follow the list of station names" (my emphasis).
			// Elsewhere the docs say:
			//
			//   Another automatically assigned list item is
			//   "{Unnamed Flag}", which is present only if
			//   stations appear on a #Flag directive without a
			//   name parameter -- for example, "#FLAG A1 A2 A3".
			//   There's really no reason to have any of those,
			//   although Walls has always allowed them.
			//
			// These seem to occur in real data, but we ignore
			// unknown flag names, so it seems reasonable to just
			// ignore these too.  Or maybe we should warn?  FIXME
			process_eol();
			goto next_line;
		    }
		    nextch();
		}
		nextch();
		int station_flags = 0;
		bool printed = false;
		while (1) {
		    skipblanks();
		    if (isComm(ch) || isEol(ch)) break;
		    get_token_walls();
		    if (S_EQ(&uctoken, "ENTRANCE")) {
			station_flags |= BIT(SFLAGS_ENTRANCE);
		    } else if (S_EQ(&uctoken, "FIX")) {
			station_flags |= BIT(SFLAGS_FIXED);
		    } else if (S_EQ(&uctoken, "LOWER") ||
			S_EQ(&uctoken, "LEAD") ||
			S_EQ(&uctoken, "SPRING") ||
			S_EQ(&uctoken, "RESURGENCE") ||
			S_EQ(&uctoken, "RISING") ||
			S_EQ(&uctoken, "SINK") ||
			S_EQ(&uctoken, "SWALLET") ||
			S_EQ(&uctoken, "SUMP") ||
			S_EQ(&uctoken, "SHAFT") ||
			S_EQ(&uctoken, "UPPER") ||
			S_EQ(&uctoken, "CAVE") ||
			S_EQ(&uctoken, "GPS") || // -> FIXED flag?
			S_EQ(&uctoken, "SURVEYED") ||
			S_EQ(&uctoken, "BATS") ||
			S_EQ(&uctoken, "MYOTIS") ||
			S_EQ(&uctoken, "VELIFER") ||
			S_EQ(&uctoken, "GATED") ||
			S_EQ(&uctoken, "0") ||
			S_EQ(&uctoken, "LOCATION")) {
			// Walls flags seem to be arbitrary strings.  To
			// capture ones we can usefully map based on real-world
			// usage, for now we report unknown ones and maintain a
			// list of those we've seen but don't have a use for.
		    } else if (s_empty(&token)) {
			nextch();
		    } else {
			if (!printed) {
			    printed = true;
			    printf("*** Unknown flags:");
			}
			printf(" %s", s_str(&token));
		    }
		}
		if (printed) printf("\n");
		if (station_flags) {
		    // Go back and read stations and apply the flags.
		    filepos fp_end;
		    get_pos(&fp_end);
		    set_pos(&fp);
		    // It seems / and \ can't be used in #flag station names?
		    // FIXME: Need to actually test this with Walls.
		    int save_translate_slash = pcs->Translate['/'];
		    int save_translate_bslash = pcs->Translate['\\'];
		    pcs->Translate['/'] = 0;
		    pcs->Translate['\\'] = 0;
		    while (!isWallsSlash(ch)) {
			prefix *name = read_walls_station(p_walls_options->prefix, false);
			name->sflags |= station_flags;
			skipblanks();
		    }
		    pcs->Translate['/'] = save_translate_slash;
		    pcs->Translate['\\'] = save_translate_bslash;
		    set_pos(&fp_end);
		}
	    }
	    break;
	  }
	  case WALLS_CMD_PREFIX:
	  case WALLS_CMD_PREFIX2:
	  case WALLS_CMD_PREFIX3: {
	    char *new_prefix = read_walls_prefix();
	    int i = (int)WALLS_CMD_PREFIX3 - (int)directive;
	    osfree(p_walls_options->prefix[i]);
	    p_walls_options->prefix[i] = new_prefix;
	    skipblanks();
	    if (!isEol(ch) && !isComm(ch)) {
		// Walls seems to ignore anything after the prefix so make this a
		// warning not an error so we can process existing Walls
		// datasets (e.g. the Mammoth dataset reportedly has `#prefix
		// 4136 Lucys domes`).
		compile_diagnostic(DIAG_WARN|DIAG_TAIL, /*End of line not blank*/15);
		skipline();
	    }
	    break;
	  }
	  case WALLS_CMD_NOTE:
	    // A text note attached to a station - ignore for now.
	    skipline();
	    break;
	  case WALLS_CMD_SEGMENT:
	    // "Segments are optional and have no affect on the compilation of
	    // survey data" so ignore for now.
	    skipline();
	    break;
	  case WALLS_CMD_SYMBOL:
	    // Now to draw symbols.  Not really appropriate here as this is
	    // presentation information, so we just ignore it.
	    skipline();
	    break;
	  case WALLS_CMD_NULL:
	    // FIXME it's a "directive" in Walls-speak.
	    compile_diagnostic(DIAG_ERR|DIAG_TOKEN|DIAG_SKIP, /*Unknown command “%s”*/12, s_str(&token));
	    break;
	}

	if (!s_empty(&line)) {
	    // Revert to reading from the file.
	    fclose(file.fh);
	    s_free(&line);
	    file = file_store;
	    ch = ch_store;
	}

	process_eol();
    }

    clear_last_leg();

    if (walls_macros) {
	// Clear all macros set in this SRV file.
	for (unsigned i = 0; i < WALLS_MACRO_HASH_SIZE; ++i) {
	    walls_macro *p = walls_macros[i];
	    while (p) {
		walls_macro *to_free = p;
		p = p->next;
		osfree(to_free);
	    }
	    walls_macros[i] = NULL;
	}
    }

    while (p_walls_options->explicit) {
	// FIXME: Walls quietly allows SAVE without a corresponding RESTORE, but
	// probably worth at least a warning here.
	pop_walls_options();
    }
    if (standalone) pop_walls_options();
}

typedef enum {
    WALLS_WPJ_CMD_BOOK,
    WALLS_WPJ_CMD_ENDBOOK,
    WALLS_WPJ_CMD_NAME,
    WALLS_WPJ_CMD_OPTIONS,
    WALLS_WPJ_CMD_PATH,
    WALLS_WPJ_CMD_REF,
    WALLS_WPJ_CMD_STATUS,
    WALLS_WPJ_CMD_SURVEY,
    WALLS_WPJ_CMD_NULL = -1
} walls_wpj_cmd;

static const sztok walls_wpj_cmd_tab[] = {
    {"BOOK",	WALLS_WPJ_CMD_BOOK},
    {"ENDBOOK",	WALLS_WPJ_CMD_ENDBOOK},
    {"NAME",	WALLS_WPJ_CMD_NAME},
    {"OPTIONS",	WALLS_WPJ_CMD_OPTIONS},
    {"PATH",	WALLS_WPJ_CMD_PATH},
    {"REF",	WALLS_WPJ_CMD_REF},
    {"STATUS",	WALLS_WPJ_CMD_STATUS},
    {"SURVEY",	WALLS_WPJ_CMD_SURVEY},
    {NULL,	WALLS_WPJ_CMD_NULL}
};

static struct {
    real x, y, z;
    int zone;
} walls_ref;

static void
data_file_walls_wpj(void)
{
    // FIXME: Do any of these variables need to be volatile to protect them
    // from longjmp()?  GCC isn't warning about them...
    char *pth = path_from_fnm(file.filename);

    walls_initialise_settings();
    walls_reset();

    // FIXME: We need to update the separator_map to reflect what can be
    // SPECIAL_NAMES.  Or should we use the Compass approach and base this
    // on what's actually used?  The first approach would pick the separator
    // from {':', ';', ',', '#', space}; the latter would pick '.' if
    // the documentation station naming recommendations were followed.
    update_output_separator();

    /* We need to update separator_map so we don't pick a separator character
     * which occurs in a station name.  However Compass DAT allows everything
     * >= ASCII char 33 except 127 in station names so if we just added all
     * the valid station name characters we'd always pick space as the
     * separator for any dataset which included a DAT file, yet in practice
     * '.' is never used in any of the sample DAT files I've seen.  So
     * instead we scan the characters actually used in station names when we
     * process CompassDATFr and CompassDATTo fields. (FIXME)
     */

#ifdef HAVE_SETJMP_H
    /* errors in nested functions can longjmp here */
    if (setjmp(file.jbSkipLine)) {
	skipline();
	process_eol();
    }
#endif

    int status = -1;
    long name_lpos = -1;
    unsigned name_lineno = 0;
    filepos fp_name;
    string name = S_INIT;
    string path = S_INIT;
    // Start from the location of this WPJ.
    s_cat(&path, pth);

    walls_ref.x = walls_ref.y = walls_ref.z = HUGE_VAL;
    walls_ref.zone = 0;

    int depth = 0;
    int detached_nest_level = 0;
    bool in_survey = false;
    while (ch != EOF && !ferror(file.fh)) {
//next_line:
	skipblanks();
	if (ch == ';') {
	    // Comment line.
	    skipline();
	    continue;
	}

	if (ch != '.') {
	    // FIXME Error?  Warning?
	    skipline();
	    process_eol();
	    continue;
	}

	nextch();
	get_token_no_blanks();
	walls_wpj_cmd tok = match_tok(walls_wpj_cmd_tab,
				      TABSIZE(walls_wpj_cmd_tab));
	if (detached_nest_level) {
	    switch (tok) {
	      case WALLS_WPJ_CMD_BOOK:
		++detached_nest_level;
		skipline();
		break;
	      case WALLS_WPJ_CMD_ENDBOOK:
		--detached_nest_level;
		break;
	      default:
		// Ignore everything else.
		skipline();
		break;
	    }
	    process_eol();
	    continue;
	}
	if (in_survey &&
	    (tok == WALLS_WPJ_CMD_SURVEY ||
	     tok == WALLS_WPJ_CMD_BOOK ||
	     tok == WALLS_WPJ_CMD_ENDBOOK)) {
	    // Process the current entry.

	    // .STATUS is a decimal integer which is a bitmap of flags.
	    // Meanings mostly cribbed from dewalls:

	    // Set if a branch is expanded in the UI - we can ignore.
#define WALLS_WPJ_STATUS_BOOK_OPEN				0x000001
	    // Detached items are not processed as part of higher level
	    // items.
#define WALLS_WPJ_STATUS_DETACHED				0x000002
	    // 0x000004 appears to be unused/no longer used.  Setting it
	    // externally in a WPJ file and then loading it into Walls and
	    // forcing saving clears it.
#define WALLS_WPJ_STATUS_UNUSED_BIT2				0x000004
	    // We ignore segments currently so ignore this too.
#define WALLS_WPJ_STATUS_NAME_DEFINES_SEGMENT			0x000008
	    // Controls the units used in reporting data - we can ignore.
#define WALLS_WPJ_STATUS_REVIEW_UNITS_FEET			0x000010
	    // Comments in dewalls-java suggests this is no longer used.
#define WALLS_WPJ_STATUS_UNUSED_BIT5				0x000020
	    // These WALLS_WPJ_STATUS_*_TRISTATE values are (shifted) binary:
	    // 00 Inherit value from parent
	    // 01 Off
	    // 10 On
	    // 11 <not used>
#define WALLS_WPJ_STATUS_USE_REFERENCE_TRISTATE			0x0000c0
#define WALLS_WPJ_STATUS_DECLINATION_AUTO_TRISTATE		0x000300
#define WALLS_WPJ_STATUS_UTM_GPS_RELATIVE_TRISTATE		0x000c00
	    // AIUI these just control distributing loop misclosure:
#define WALLS_WPJ_STATUS_PRESERVE_PLUMB_ORIENTATION_TRISTATE	0x003000
#define WALLS_WPJ_STATUS_PRESERVE_PLUMB_LENGTH_TRISTATE		0x00c000
	    // Attached file of arbitrary type (we can just ignore):
#define WALLS_WPJ_STATUS_TYPE_OTHER				0x010000
	    // We can ignore these:
#define WALLS_WPJ_STATUS_EDIT_ON_LAUNCH				0x020000
#define WALLS_WPJ_STATUS_OPEN_ON_LAUNCH				0x040000
#define WALLS_WPJ_STATUS_DEFAULT_VIEW_MASK			0x380000
#define WALLS_WPJ_STATUS_PROCESS_SVG				0x400000

	    // A quirk is that the root item is flagged with
	    // WALLS_WPJ_STATUS_DETACHED (seems the flag might be more like
	    // "don't draw a connecting line left from here").
	    if ((status & WALLS_WPJ_STATUS_DETACHED) && depth > 0) {
		// Detached survey.
		//printf("*** Detached survey\n");
		goto detached_or_not_srv;
	    }
	    if ((status & WALLS_WPJ_STATUS_TYPE_OTHER)) {
		// Attached file of arbitrary type.
		goto detached_or_not_srv;
	    }
	    if (s_empty(&name)) {
		printf("*** in_survey but no/empty NAME\n");
		goto detached_or_not_srv;
	    }

	    // Include SRV file.
#if 0
	    printf("+++ %s %s .SRV :%s:%s:%s\n", s_str(&path), s_str(&name),
		   p_walls_options->prefix[0] ? p_walls_options->prefix[0] : "",
		   p_walls_options->prefix[1] ? p_walls_options->prefix[1] : "",
		   p_walls_options->prefix[2] ? p_walls_options->prefix[2] : "");
#endif
	    char *filename;
	    FILE *fh = fopen_portable(s_str(&path), s_str(&name), "srv", "rb", &filename);
	    if (fh == NULL)
		fh = fopen_portable(s_str(&path), s_str(&name), "SRV", "rb", &filename);

	    if (fh == NULL) {
		// Report the diagnostic at the location of the ".NAME".
		unsigned save_line = file.line;
		long save_lpos = file.lpos;
		filepos fp;
		get_pos(&fp);
		set_pos(&fp_name);
		file.lpos = name_lpos;
		file.line = name_lineno;
		// Report the resolved path.  FIXME: Maybe we should use
		// full_file in the fopen_portable() call above so things
		// align better?
		string full_file = S_INIT;
		s_cats(&full_file, &path);
		s_catchar(&full_file, FNM_SEP_LEV);
		s_cats(&full_file, &name);
		s_cat(&full_file, ".SRV");
		if (!fDirectory(s_str(&path))) {
		    // Walls appears to quietly ignore file is the
		    // directory does not exist, but it seems worth
		    // warning about at least.
		    //
		    // FIXME: This should take case into account like
		    // opening the file does.
		    compile_diagnostic(DIAG_WARN|DIAG_TAIL, /*Couldn’t open file “%s”*/24, s_str(&full_file));
		} else {
		    compile_diagnostic(DIAG_ERR|DIAG_TAIL, /*Couldn’t open file “%s”*/24, s_str(&full_file));
		}
		s_free(&full_file);
		set_pos(&fp);
		file.line = save_line;
		file.lpos = save_lpos;
		goto srv_not_found;
	    }

	    {
		parse file_store = file;
		int ch_store = ch;
		if (file.fh) file.parent = &file_store;
		file.fh = fh;
		file.filename = filename;
		file.line = 1;
		file.lpos = 0;
		file.reported_where = false;
		nextch();

		using_data_file(file.filename);

		push_walls_options();
		walls_swap_macro_tables();
		data_file_walls_srv();
		walls_swap_macro_tables();
		pop_walls_options();

		if (ferror(file.fh))
		    fatalerror_in_file(file.filename, 0, /*Error reading file*/18);

		(void)fclose(file.fh);

		/* don't free this - it may be pointed to by prefix.file */
		/* osfree(file.filename); */

		file = file_store;
		ch = ch_store;
	    }

srv_not_found:
	    status = -1;
	    s_clear(&name);
	    //s_clear(&path);
	    walls_ref.x = walls_ref.y = walls_ref.z = HUGE_VAL;
	    walls_ref.zone = 0;
detached_or_not_srv:
	    in_survey = false;
	}

	switch (tok) {
	  case WALLS_WPJ_CMD_BOOK:
	    ++depth;
	    push_walls_options();
	    in_survey = false;
	    skipline();
	    break;
	  case WALLS_WPJ_CMD_SURVEY:
	    in_survey = true;
	    skipline();
	    break;
	  case WALLS_WPJ_CMD_ENDBOOK:
	    if (depth == 0) {
		// FIXME: .ENDBOOK  and .BOOK
		compile_diagnostic(DIAG_ERR|DIAG_SKIP, /*END with no matching BEGIN in this file*/22);
	    }
	    --depth;
	    pop_walls_options();
	    in_survey = false;
	    break;
	  case WALLS_WPJ_CMD_OPTIONS:
	    parse_options();
	    break;
	  case WALLS_WPJ_CMD_PATH: {
	    if (!s_empty(&path)) {
		// FIXME: PATH already set
		s_clear(&path);
	    }
	    skipblanks();
	    // Start from the location of this WPJ.
	    s_cat(&path, pth);
	    while (!isEol(ch)) {
		if (ch == '\\') {
		    ch = FNM_SEP_LEV;
		}
		s_catchar(&path, ch);
		nextch();
	    }
	    //printf("PATH: %s\n", s_str(&path));
	    break;
	  }
	  case WALLS_WPJ_CMD_REF:
	    walls_ref.y = read_numeric(false);
	    walls_ref.x = read_numeric(false);
	    walls_ref.zone = read_int(-60, 60);

	    // Ignore pre-computed convergence as we compute that for ourselves.
	    (void)read_numeric(false);

	    walls_ref.z = read_numeric(false);

	    // Ignore field which seems to be a bitmask.
	    //
	    // From experimenting with Walls32.exe and looking at the saved
	    // .wpj file contents, the bottom two bits (mask 0x03) seem to
	    // encode the format (d/dm/dms) that the Walls32.exe UI uses to
	    // display/enter lat/long.
	    //
	    // Bit 0x04 seems to be "West of meridian?" and bit 0x08 seems
	    // to be "South of equator?" (and the lat and long seem to be
	    // stored without signs).
	    //
	    // Other bits seem to be unused so it seems we can safely ignore
	    // this field.
	    (void)read_uint();

	    // Ignore same location in lat/long.
	    (void)read_numeric(false);
	    (void)read_numeric(false);
	    (void)read_numeric(false);
	    (void)read_numeric(false);
	    (void)read_numeric(false);
	    (void)read_numeric(false);

	    // Ignore integer index for the datum (e.g. 27 for "WGS 1984".  The
	    // string names seem more likely to have not changed over time.
	    (void)read_uint();

	    string datum_str = S_INIT;
	    read_string(&datum_str);
	    int datum = img_parse_compass_datum_string(s_str(&datum_str),
						       s_len(&datum_str));
	    if (datum == 0) {
		// Walls has different name strings to Compass for some datums.
		if (S_EQ(&datum_str, "NAD27 CONUS"))
		    // FIXME Assuming this is right.  Walls seems to have 6
		    // variant NAD27 datums, not sure what's going on.
		    datum = img_DATUM_NAD27;
		else if (S_EQ(&datum_str, "NAD83"))
		    datum = img_DATUM_NAD83;
		else if (S_EQ(&datum_str, "Geodetic Datum `49"))
		    datum = img_DATUM_NZGD49;
		else if (S_EQ(&datum_str, "Hu-Tzu-Shan"))
		    datum = img_DATUM_HUTZUSHAN1950;
		else
		    printf("*** FAILED TO PARSE DATUM '%s'\n", s_str(&datum_str));
	    }
	    s_free(&datum_str);

	    if (datum && walls_ref.zone && abs(walls_ref.zone) <= 60) {
		char *proj_str = img_compass_utm_proj_str(datum,
							  walls_ref.zone);
		set_declination_location(walls_ref.x, walls_ref.y, walls_ref.z,
					 proj_str);
		if (!pcs->proj_str) {
		    pcs->proj_str = proj_str;
		    if (!proj_str_out) {
			proj_str_out = osstrdup(proj_str);
		    }
		} else {
		    osfree(proj_str);
		}
	    }
	    break;
	  case WALLS_WPJ_CMD_STATUS:
	    status = read_uint();
	    // A quirk is that the root item is flagged with
	    // WALLS_WPJ_STATUS_DETACHED (seems the flag might be more like
	    // "don't draw a connecting line left from here").
	    if ((status & WALLS_WPJ_STATUS_DETACHED) && !in_survey && depth > 1) {
		//printf("Detached book (status %d = 0x%06x)\n", status, status);
		// Detached BOOK - resume at the corresponding ENDBOOK.
		s_clear(&name);
		pop_walls_options();
		detached_nest_level = 1;
	    }
	    break;
	  case WALLS_WPJ_CMD_NAME:
	    if (!s_empty(&name)) {
		// FIXME: NAME already set
		s_clear(&name);
	    }
	    skipblanks();
	    get_pos(&fp_name);
	    name_lpos = file.lpos;
	    name_lineno = file.line;
	    while (!isEol(ch)) {
		s_catchar(&name, ch);
		nextch();
	    }
	    break;
	  case WALLS_WPJ_CMD_NULL:
	    compile_diagnostic(DIAG_ERR|DIAG_TOKEN|DIAG_SKIP, /*Unknown command “%s”*/12, s_str(&token));
	}
	process_eol();
    }

    osfree(pth);

    pop_walls_options();
}

static void
data_file_survex(void)
{
    int begin_lineno_store = pcs->begin_lineno;
    pcs->begin_lineno = 0;

    if (ch == 0xef) {
	/* Maybe a UTF-8 "BOM" - skip if so. */
	if (nextch() == 0xbb && nextch() == 0xbf) {
	    nextch();
	    file.lpos = 3;
	} else {
	    rewind(file.fh);
	    ch = 0xef;
	}
    }

#ifdef HAVE_SETJMP_H
    /* errors in nested functions can longjmp here */
    if (setjmp(file.jbSkipLine)) {
	skipline();
	process_eol();
    }
#endif

    while (ch != EOF && !ferror(file.fh)) {
	if (!process_non_data_line()) {
	    f_export_ok = false;
	    switch (pcs->style) {
	      case STYLE_NORMAL:
	      case STYLE_DIVING:
	      case STYLE_CYLPOLAR:
		data_normal();
		break;
	      case STYLE_CARTESIAN:
		data_cartesian();
		break;
	      case STYLE_PASSAGE:
		data_passage();
		break;
	      case STYLE_NOSURVEY:
		data_nosurvey();
		break;
	      case STYLE_IGNORE:
		data_ignore();
		break;
	      default:
		BUG("bad style");
	    }
	}
    }
    clear_last_leg();

    /* don't allow *BEGIN at the end of a file, then *EXPORT in the
     * including file */
    f_export_ok = false;

    if (pcs->begin_lineno) {
	error_in_file(file.filename, pcs->begin_lineno,
		      /*BEGIN with no matching END in this file*/23);
	/* Implicitly close any unclosed BEGINs from this file */
	do {
	    pop_settings();
	} while (pcs->begin_lineno);
    }

    pcs->begin_lineno = begin_lineno_store;
}

#define EXT3(C1, C2, C3) (((C3) << 16) | ((C2) << 8) | (C1))

extern void
data_file(const char *pth, const char *fnm)
{
   parse file_store;
   unsigned ext = 0;

   {
      char *filename;
      FILE *fh;
      size_t len;

      if (!pth) {
	 /* file specified on command line - don't do special translation */
	 fh = fopenWithPthAndExt(pth, fnm, EXT_SVX_DATA, "rb", &filename);
      } else {
	 fh = fopen_portable(pth, fnm, EXT_SVX_DATA, "rb", &filename);
      }

      if (fh == NULL) {
	 compile_error_string(fnm, /*Couldn’t open file “%s”*/24, fnm);
	 return;
      }

      len = strlen(filename);
      if (len > 4 && filename[len - 4] == FNM_SEP_EXT) {
	  /* Read extension and pack into ext. */
	  for (int i = 1; i < 4; ++i) {
	      unsigned char ext_ch = filename[len - i];
	      ext = (ext << 8) | tolower(ext_ch);
	  }
      }

      file_store = file;
      if (file.fh) file.parent = &file_store;
      file.fh = fh;
      file.filename = filename;
      file.line = 1;
      file.lpos = 0;
      file.reported_where = false;
      nextch();
   }

   using_data_file(file.filename);

   switch (ext) {
     case EXT3('d', 'a', 't'):
       // Compass survey data.
       data_file_compass_dat();
       break;
     case EXT3('c', 'l', 'p'):
       // Compass closed data.  The format of .clp is the same as .dat,
       // but it contains loop-closed data.  This might be useful to
       // read if you want to keep existing stations at the same
       // adjusted positions, for example to be able to draw extensions
       // on an existing drawn-up survey.  Or if you managed to lose the
       // original .dat but still have the .clp.
       data_file_compass_clp();
       break;
     case EXT3('m', 'a', 'k'):
       // Compass project file.
       data_file_compass_mak();
       break;
     case EXT3('s', 'r', 'v'):
       // Walls survey data.
       data_file_walls_srv();
       break;
     case EXT3('w', 'p', 'j'):
       // Walls project file.
       data_file_walls_wpj();
       break;
     default:
       // Native Survex data.
       data_file_survex();
       break;
   }

   if (ferror(file.fh))
      fatalerror_in_file(file.filename, 0, /*Error reading file*/18);

   (void)fclose(file.fh);

   file = file_store;

   /* don't free this - it may be pointed to by prefix.file */
   /* osfree(file.filename); */
}

static real
mod2pi(real a)
{
   return a - floor(a / (2 * M_PI)) * (2 * M_PI);
}

static real
handle_plumb(clino_type *p_ctype)
{
   typedef enum {
      CLINO_NULL=-1, CLINO_UP, CLINO_DOWN, CLINO_LEVEL
   } clino_tok;
   static const sztok clino_tab[] = {
      {"D",     CLINO_DOWN},
      {"DOWN",  CLINO_DOWN},
      {"H",     CLINO_LEVEL},
      {"LEVEL", CLINO_LEVEL},
      {"U",     CLINO_UP},
      {"UP",    CLINO_UP},
      {NULL,    CLINO_NULL}
   };
   static const real clinos[] = {(real)M_PI_2, (real)(-M_PI_2), (real)0.0};
   clino_tok tok;

   skipblanks();
   if (isalpha(ch)) {
      filepos fp;
      get_pos(&fp);
      get_token();
      tok = match_tok(clino_tab, TABSIZE(clino_tab));
      if (tok != CLINO_NULL) {
	 *p_ctype = (tok == CLINO_LEVEL ? CTYPE_HORIZ : CTYPE_PLUMB);
	 return clinos[tok];
      }
      set_pos(&fp);
   } else if (isSign(ch)) {
      int chOld = ch;
      nextch();
      if (toupper(ch) == 'V') {
	 nextch();
	 *p_ctype = CTYPE_PLUMB;
	 return (!isMinus(chOld) ? M_PI_2 : -M_PI_2);
      }

      if (isOmit(chOld)) {
	 *p_ctype = CTYPE_OMIT;
	 /* no clino reading, so assume 0 with large sd */
	 return (real)0.0;
      }
   } else if (isOmit(ch)) {
      /* OMIT char may not be a SIGN char too so we need to check here as
       * well as above... */
      nextch();
      *p_ctype = CTYPE_OMIT;
      /* no clino reading, so assume 0 with large sd */
      return (real)0.0;
   }
   return HUGE_REAL;
}

static void
warn_readings_differ(int msgno, real diff, int units,
		     reading r_fore, reading r_back)
{
   char buf[64];
   char *p;
   diff /= get_units_factor(units);
   snprintf(buf, sizeof(buf), "%.2f", fabs(diff));
   for (p = buf; *p; ++p) {
      if (*p == '.') {
	 char *z = p;
	 while (*++p) {
	    if (*p != '0') z = p + 1;
	 }
	 p = z;
	 break;
      }
   }
   strcpy(p, get_units_string(units));
   // FIXME: Highlight r_fore too.
   (void)r_fore;
   compile_diagnostic_reading(DIAG_WARN, r_back, msgno, buf);
}

// If one (or both) compass readings are given, return Comp or BackComp
// so we can report plumb legs with compass readings.  If neither are,
// return End.
static reading
handle_comp_units(void)
{
   reading which_comp = End;
   if (VAL(Comp) != HUGE_REAL) {
      which_comp = Comp;
      VAL(Comp) *= pcs->units[Q_BEARING];
      if (VAL(Comp) < (real)0.0 || VAL(Comp) - M_PI * 2.0 > EPSILON) {
	 /* TRANSLATORS: Suspicious means something like 410 degrees or -20
	  * degrees */
	 compile_diagnostic_reading(DIAG_WARN, Comp, /*Suspicious compass reading*/59);
	 VAL(Comp) = mod2pi(VAL(Comp));
      }
   }
   if (VAL(BackComp) != HUGE_REAL) {
      if (which_comp == End) which_comp = BackComp;
      VAL(BackComp) *= pcs->units[Q_BACKBEARING];
      if (VAL(BackComp) < (real)0.0 || VAL(BackComp) - M_PI * 2.0 > EPSILON) {
	 /* FIXME: different message for BackComp? */
	 compile_diagnostic_reading(DIAG_WARN, BackComp, /*Suspicious compass reading*/59);
	 VAL(BackComp) = mod2pi(VAL(BackComp));
      }
   }
   return which_comp;
}

static real compute_convergence(real lon, real lat) {
    // PROJ < 8.1.0 dereferences the context without a NULL check inside
    // proj_create_ellipsoidal_2D_cs() but PJ_DEFAULT_CTX is really just
    // NULL so for affected PROJ versions we create a context temporarily to
    // avoid a segmentation fault.
    PJ_CONTEXT * ctx = PJ_DEFAULT_CTX;
#if PROJ_VERSION_MAJOR < 8 || \
    (PROJ_VERSION_MAJOR == 8 && PROJ_VERSION_MINOR < 1)
    ctx = proj_context_create();
#endif

    if (!proj_str_out) {
	compile_diagnostic(DIAG_ERR, /*Output coordinate system not set*/488);
	return 0.0;
    }
    PJ * pj = proj_create(ctx, proj_str_out);
    PJ_COORD lp;
    lp.lp.lam = lon;
    lp.lp.phi = lat;
#if PROJ_VERSION_MAJOR < 8 || \
    (PROJ_VERSION_MAJOR == 8 && PROJ_VERSION_MINOR < 2)
    /* Code adapted from fix in PROJ 8.2.0 to make proj_factors() work in
     * cases we need (e.g. a CRS specified as "EPSG:<number>").
     */
    switch (proj_get_type(pj)) {
	case PJ_TYPE_PROJECTED_CRS: {
	    /* If it is a projected CRS, then compute the factors on the conversion
	     * associated to it. We need to start from a temporary geographic CRS
	     * using the same datum as the one of the projected CRS, and with
	     * input coordinates being in longitude, latitude order in radian,
	     * to be consistent with the expectations of the lp input parameter.
	     */

	    PJ * geodetic_crs = proj_get_source_crs(ctx, pj);
	    if (!geodetic_crs)
		break;
	    PJ * datum = proj_crs_get_datum(ctx, geodetic_crs);
#if PROJ_VERSION_MAJOR == 8 || \
    (PROJ_VERSION_MAJOR == 7 && PROJ_VERSION_MINOR >= 2)
	    /* PROJ 7.2.0 upgraded to EPSG 10.x which added the concept
	     * of a datum ensemble, and this version of PROJ also added
	     * an API to deal with these.
	     *
	     * If we're using PROJ < 7.2.0 then its EPSG database won't
	     * have datum ensembles, so we don't need any code to handle
	     * them.
	     */
	    if (!datum) {
		datum = proj_crs_get_datum_ensemble(ctx, geodetic_crs);
	    }
#endif
	    PJ * cs = proj_create_ellipsoidal_2D_cs(
		ctx, PJ_ELLPS2D_LONGITUDE_LATITUDE, "Radian", 1.0);
	    PJ * temp = proj_create_geographic_crs_from_datum(
		ctx, "unnamed crs", datum, cs);
	    proj_destroy(datum);
	    proj_destroy(cs);
	    proj_destroy(geodetic_crs);
	    PJ * newOp = proj_create_crs_to_crs_from_pj(ctx, temp, pj, NULL, NULL);
	    proj_destroy(temp);
	    if (newOp) {
		proj_destroy(pj);
		pj = newOp;
	    }
	    break;
	}
	default:
	    break;
    }
#endif
#if PROJ_VERSION_MAJOR < 9 || \
    (PROJ_VERSION_MAJOR == 9 && PROJ_VERSION_MINOR < 3)
    if (pj) {
	/* In PROJ < 9.3.0 proj_factors() returns a grid convergence which is
	 * off by 90° for a projected coordinate system with northing/easting
	 * axis order.  We can't copy over the fix for this in PROJ 9.3.0's
	 * proj_factors() since it uses non-public PROJ functions, but
	 * normalising the output order here works too.
	 */
	PJ* pj_norm = proj_normalize_for_visualization(PJ_DEFAULT_CTX, pj);
	proj_destroy(pj);
	pj = pj_norm;
    }
#endif
    PJ_FACTORS factors = proj_factors(pj, lp);
    proj_destroy(pj);
#if PROJ_VERSION_MAJOR < 8 || \
    (PROJ_VERSION_MAJOR == 8 && PROJ_VERSION_MINOR < 1)
    proj_context_destroy(ctx);
#endif
    return factors.meridian_convergence;
}

static real
handle_compass(real *p_var)
{
   real compvar = VAR(Comp);
   real comp = VAL(Comp);
   real backcomp = VAL(BackComp);
   real declination;
   if (pcs->z[Q_DECLINATION] != HUGE_REAL) {
      declination = -pcs->z[Q_DECLINATION];
   } else if (pcs->declination != HUGE_REAL) {
      /* Cached value calculated for a previous compass reading taken on the
       * same date (by the 'else' just below).
       */
      declination = pcs->declination;
   } else {
      if (!pcs->meta || pcs->meta->days1 == -1) {
	  compile_diagnostic(DIAG_WARN, /*No survey date specified - using 0 for magnetic declination*/304);
	  declination = 0;
      } else {
	  int avg_days = (pcs->meta->days1 + pcs->meta->days2) / 2;
	  double dat = julian_date_from_days_since_1900(avg_days);
	  /* thgeomag() takes (lat, lon, h, dat) - i.e. (y, x, z, date). */
	  declination = thgeomag(pcs->dec_lat, pcs->dec_lon, pcs->dec_alt, dat);
	  if (declination < pcs->min_declination) {
	      pcs->min_declination = declination;
	      pcs->min_declination_days = avg_days;
	  }
	  if (declination > pcs->max_declination) {
	      pcs->max_declination = declination;
	      pcs->max_declination_days = avg_days;
	  }
      }
      if (pcs->convergence == HUGE_REAL) {
	  /* Compute the convergence lazily.  It only depends on the output
	   * coordinate system so we can cache it for reuse to apply to
	   * a declination value for a different date.
	   */
	  pcs->convergence = compute_convergence(pcs->dec_lon, pcs->dec_lat);
      }
      declination -= pcs->convergence;
      /* We cache the calculated declination as the calculation is relatively
       * expensive.  We also cache an "assumed 0" answer so that we only
       * warn once per such survey rather than for every line with a compass
       * reading. */
      pcs->declination = declination;
   }
   if (comp != HUGE_REAL) {
      comp = (comp - pcs->z[Q_BEARING]) * pcs->sc[Q_BEARING];
      comp += declination;
   }
   if (backcomp != HUGE_REAL) {
      backcomp = (backcomp - pcs->z[Q_BACKBEARING])
	      * pcs->sc[Q_BACKBEARING];
      backcomp += declination;
      backcomp -= M_PI;
      if (comp != HUGE_REAL) {
	 real diff = comp - backcomp;
	 real adj = fabs(diff) > M_PI ? M_PI : 0;
	 diff -= floor((diff + M_PI) / (2 * M_PI)) * 2 * M_PI;
	 if (sqrd(diff / 3.0) > compvar + VAR(BackComp)) {
	    /* fore and back readings differ by more than 3 sds */
	    /* TRANSLATORS: %s is replaced by the amount the readings disagree
	     * by, e.g. "2.5°" or "3ᵍ". */
	    warn_readings_differ(/*COMPASS reading and BACKCOMPASS reading disagree by %s*/98,
				 diff, get_angle_units(Q_BEARING), Comp, BackComp);
	 }
	 comp = (comp / compvar + backcomp / VAR(BackComp));
	 compvar = (compvar + VAR(BackComp)) / 4;
	 comp *= compvar;
	 comp += adj;
      } else {
	 comp = backcomp;
	 compvar = VAR(BackComp);
      }
   }
   *p_var = compvar;
   return comp;
}

static real
handle_clino(q_quantity q, reading r, real val, bool percent, clino_type *p_ctype)
{
   bool range_0_180;
   real z;
   real diff_from_abs90;
   val *= pcs->units[q];
   /* percentage scale */
   if (percent) val = atan(val);
   /* We want to warn if there's a reading which it would be impossible
    * to have read from the instrument (e.g. on a -90 to 90 degree scale
    * you can't read "96" (it's probably a typo for "69").  However, the
    * gradient reading from a topofil is typically in the range 0 to 180,
    * with 90 being horizontal.
    *
    * Really we should allow the valid range to be specified, but for now
    * we infer it from the zero error - if this is within 45 degrees of
    * 90 then we assume the range is 0 to 180.
    */
   z = pcs->z[q];
   range_0_180 = (z > M_PI_4 && z < 3*M_PI_4);
   diff_from_abs90 = fabs(val) - M_PI_2;
   if (diff_from_abs90 > EPSILON) {
      if (!range_0_180) {
	 int clino_units = get_angle_units(q);
	 const char * units = get_units_string(clino_units);
	 real right_angle = M_PI_2 / get_units_factor(clino_units);
	 /* FIXME: different message for BackClino? */
	 /* TRANSLATORS: %.f%s will be replaced with a right angle in the
	  * units currently in use, e.g. "90°" or "100ᵍ".  And "absolute
	  * value" means the reading ignoring the sign (so it might be
	  * < -90° or > 90°. */
	 compile_diagnostic_reading(DIAG_WARN, r, /*Clino reading over %.f%s (absolute value)*/51,
				    right_angle, units);
      }
   } else if (TSTBIT(pcs->infer, INFER_PLUMBS) &&
	      diff_from_abs90 >= -EPSILON) {
      *p_ctype = CTYPE_INFERPLUMB;
   }
   if (range_0_180 && *p_ctype != CTYPE_INFERPLUMB) {
      /* FIXME: Warning message not ideal... */
      if (val < 0.0 || val - M_PI > EPSILON) {
	 int clino_units = get_angle_units(q);
	 const char * units = get_units_string(clino_units);
	 real right_angle = M_PI_2 / get_units_factor(clino_units);
	 compile_diagnostic_reading(DIAG_WARN, r, /*Clino reading over %.f%s (absolute value)*/51,
				    right_angle, units);
      }
   }
   return val;
}

static int
process_normal(prefix *fr, prefix *to, bool fToFirst,
	       clino_type ctype, clino_type backctype)
{
   real tape = VAL(Tape);
   real clin = VAL(Clino);
   real backclin = VAL(BackClino);

   real dx, dy, dz;
   real vx, vy, vz;
#ifndef NO_COVARIANCES
   real cxy, cyz, czx;
#endif

   /* adjusted tape is negative -- probably the calibration is wrong */
   if (tape < (real)0.0) {
      /* TRANSLATE different message for topofil? */
      compile_diagnostic_reading(DIAG_WARN, Tape, /*Negative adjusted tape reading*/79);
   }

   reading comp_given = handle_comp_units();

   if (ctype == CTYPE_READING) {
      clin = handle_clino(Q_GRADIENT, Clino, clin,
			  pcs->f_clino_percent, &ctype);
   }

   if (backctype == CTYPE_READING) {
      backclin = handle_clino(Q_BACKGRADIENT, BackClino, backclin,
			      pcs->f_backclino_percent, &backctype);
   }

   /* un-infer the plumb if the backsight was just a reading */
   if (ctype == CTYPE_INFERPLUMB && backctype == CTYPE_READING) {
       ctype = CTYPE_READING;
   }

   if (ctype != CTYPE_OMIT && backctype != CTYPE_OMIT && ctype != backctype) {
      /* TRANSLATORS: In data with backsights, the user has tried to give a
       * plumb for the foresight and a clino reading for the backsight, or
       * something similar. */
      compile_error_reading_skip(Clino, /*CLINO and BACKCLINO readings must be of the same type*/84);
      return 0;
   }

   if (ctype == CTYPE_PLUMB || ctype == CTYPE_INFERPLUMB ||
       backctype == CTYPE_PLUMB || backctype == CTYPE_INFERPLUMB) {
      /* plumbed */
      if (comp_given != End) {
	 if (ctype == CTYPE_PLUMB ||
	     (ctype == CTYPE_INFERPLUMB && VAL(Comp) != 0.0) ||
	     backctype == CTYPE_PLUMB ||
	     (backctype == CTYPE_INFERPLUMB && VAL(BackComp) != 0.0)) {
	    /* TRANSLATORS: A "plumbed leg" is one measured using a plumbline
	     * (a weight on a string).  So the problem here is that the leg is
	     * vertical, so a compass reading has no meaning! */
	    compile_diagnostic_reading(DIAG_WARN, comp_given, /*Compass reading given on plumbed leg*/21);
	 }
      }

      dx = dy = (real)0.0;
      if (ctype != CTYPE_OMIT) {
	 if (backctype != CTYPE_OMIT && (clin > 0) == (backclin > 0)) {
	    /* TRANSLATORS: We've been told the foresight and backsight are
	     * both "UP", or that they're both "DOWN". */
	    compile_error_reading_skip(Clino, /*Plumbed CLINO and BACKCLINO readings can't be in the same direction*/92);
	    return 0;
	 }
	 dz = (clin > (real)0.0) ? tape : -tape;
      } else {
	 dz = (backclin < (real)0.0) ? tape : -tape;
      }
      vx = vy = var(Q_POS) / 3.0 + dz * dz * var(Q_PLUMB);
      vz = var(Q_POS) / 3.0 + VAR(Tape);
#ifndef NO_COVARIANCES
      /* Correct values - no covariances in this case! */
      cxy = cyz = czx = (real)0.0;
#endif
   } else {
      /* Each of ctype and backctype are either CTYPE_READING/CTYPE_HORIZ
       * or CTYPE_OMIT */
      /* clino */
      real L2, cosG, LcosG, cosG2, sinB, cosB, dx2, dy2, dz2, v, V;
      if (comp_given == End) {
	 /* TRANSLATORS: Here "legs" are survey legs, i.e. measurements between
	  * survey stations. */
	 compile_error_reading_skip(Comp, /*Compass reading may not be omitted except on plumbed legs*/14);
	 return 0;
      }
      if (tape == (real)0.0) {
	 dx = dy = dz = (real)0.0;
	 vx = vy = vz = (real)(var(Q_POS) / 3.0); /* Position error only */
#ifndef NO_COVARIANCES
	 cxy = cyz = czx = (real)0.0;
#endif
#if DEBUG_DATAIN_1
	 printf("Zero length leg: vx = %f, vy = %f, vz = %f\n", vx, vy, vz);
#endif
      } else {
	 real sinGcosG;
	 /* take into account variance in LEVEL case */
	 real var_clin = var(Q_LEVEL);
	 real var_comp;
	 real comp = handle_compass(&var_comp);
	 /* ctype != CTYPE_READING is LEVEL case */
	 if (ctype == CTYPE_READING) {
	    clin = (clin - pcs->z[Q_GRADIENT]) * pcs->sc[Q_GRADIENT];
	    var_clin = VAR(Clino);
	 }
	 if (backctype == CTYPE_READING) {
	    backclin = (backclin - pcs->z[Q_BACKGRADIENT])
	       * pcs->sc[Q_BACKGRADIENT];
	    if (ctype == CTYPE_READING) {
	       if (sqrd((clin + backclin) / 3.0) > var_clin + VAR(BackClino)) {
		  /* fore and back readings differ by more than 3 sds */
		  /* TRANSLATORS: %s is replaced by the amount the readings disagree
		   * by, e.g. "2.5°" or "3ᵍ". */
		  warn_readings_differ(/*CLINO reading and BACKCLINO reading disagree by %s*/99,
				       clin + backclin, get_angle_units(Q_GRADIENT), Clino, BackClino);
	       }
	       clin = (clin / var_clin - backclin / VAR(BackClino));
	       var_clin = (var_clin + VAR(BackClino)) / 4;
	       clin *= var_clin;
	    } else {
	       clin = -backclin;
	       var_clin = VAR(BackClino);
	    }
	 }

#if DEBUG_DATAIN
	 printf("    %4.2f %4.2f %4.2f\n", tape, comp, clin);
#endif
	 cosG = cos(clin);
	 LcosG = tape * cosG;
	 sinB = sin(comp);
	 cosB = cos(comp);
#if DEBUG_DATAIN_1
	 printf("sinB = %f, cosG = %f, LcosG = %f\n", sinB, cosG, LcosG);
#endif
	 dx = LcosG * sinB;
	 dy = LcosG * cosB;
	 dz = tape * sin(clin);
/*      printf("%.2f\n",clin); */
#if DEBUG_DATAIN_1
	 printf("dx = %f\ndy = %f\ndz = %f\n", dx, dy, dz);
#endif
	 dx2 = dx * dx;
	 L2 = tape * tape;
	 V = VAR(Tape) / L2;
	 dy2 = dy * dy;
	 cosG2 = cosG * cosG;
	 sinGcosG = sin(clin) * cosG;
	 dz2 = dz * dz;
	 v = dz2 * var_clin;
#ifdef NO_COVARIANCES
	 vx = (var(Q_POS) / 3.0 + dx2 * V + dy2 * var_comp +
	       (.5 + sinB * sinB * cosG2) * v);
	 vy = (var(Q_POS) / 3.0 + dy2 * V + dx2 * var_comp +
	       (.5 + cosB * cosB * cosG2) * v);
	 if (ctype == CTYPE_OMIT && backctype == CTYPE_OMIT) {
	    /* if no clino, assume sd=tape/sqrt(10) so 3sds = .95*tape */
	    vz = var(Q_POS) / 3.0 + L2 * (real)0.1;
	 } else {
	    vz = var(Q_POS) / 3.0 + dz2 * V + L2 * cosG2 * var_clin;
	 }
	 /* for Surveyor87 errors: vx=vy=vz=var(Q_POS)/3.0; */
#else
	 vx = var(Q_POS) / 3.0 + dx2 * V + dy2 * var_comp +
	    (sinB * sinB * v);
	 vy = var(Q_POS) / 3.0 + dy2 * V + dx2 * var_comp +
	    (cosB * cosB * v);
	 if (ctype == CTYPE_OMIT && backctype == CTYPE_OMIT) {
	    /* if no clino, assume sd=tape/sqrt(10) so 3sds = .95*tape */
	    vz = var(Q_POS) / 3.0 + L2 * (real)0.1;
	 } else {
	    vz = var(Q_POS) / 3.0 + dz2 * V + L2 * cosG2 * var_clin;
	 }
	 /* usual covariance formulae are fine in no clino case since
	  * dz = 0 so value of var_clin is ignored */
	 cxy = sinB * cosB * (VAR(Tape) * cosG2 + var_clin * dz2)
	       - var_comp * dx * dy;
	 czx = VAR(Tape) * sinB * sinGcosG - var_clin * dx * dz;
	 cyz = VAR(Tape) * cosB * sinGcosG - var_clin * dy * dz;
#if 0
	 printf("vx = %6.3f, vy = %6.3f, vz = %6.3f\n", vx, vy, vz);
	 printf("cxy = %6.3f, cyz = %6.3f, czx = %6.3f\n", cxy, cyz, czx);
#endif
#endif
#if DEBUG_DATAIN_1
	 printf("In DATAIN.C, vx = %f, vy = %f, vz = %f\n", vx, vy, vz);
#endif
      }
   }
#if DEBUG_DATAIN_1
   printf("Just before addleg, vx = %f\n", vx);
#endif
   /*printf("dx,dy,dz = %.2f %.2f %.2f\n\n", dx, dy, dz);*/
   addlegbyname(fr, to, fToFirst, dx, dy, dz, vx, vy, vz
#ifndef NO_COVARIANCES
		, cyz, czx, cxy
#endif
		);
   return 1;
}

static int
process_diving(prefix *fr, prefix *to, bool fToFirst, bool fDepthChange)
{
   real tape = VAL(Tape);

   real dx, dy, dz;
   real vx, vy, vz;
#ifndef NO_COVARIANCES
   real cxy = 0, cyz = 0, czx = 0;
#endif

   handle_comp_units();

   /* depth gauge readings increase upwards with default calibration */
   if (fDepthChange) {
      SVX_ASSERT(VAL(FrDepth) == 0.0);
      dz = VAL(ToDepth) * pcs->units[Q_DEPTH] - pcs->z[Q_DEPTH];
      dz *= pcs->sc[Q_DEPTH];
   } else {
      dz = VAL(ToDepth) - VAL(FrDepth);
      dz *= pcs->units[Q_DEPTH] * pcs->sc[Q_DEPTH];
   }

   /* adjusted tape is negative -- probably the calibration is wrong */
   if (tape < (real)0.0) {
      compile_diagnostic_reading(DIAG_WARN, Tape, /*Negative adjusted tape reading*/79);
   }

   /* check if tape is less than depth change */
   if (tape < fabs(dz)) {
      /* FIXME: allow margin of error based on variances? */
      /* TRANSLATORS: This means that the data fed in said this.
       *
       * It could be a gross error (e.g. the decimal point is missing from the
       * depth gauge reading) or it could just be due to random error on a near
       * vertical leg */
      compile_diagnostic_reading(DIAG_WARN, Tape, /*Tape reading is less than change in depth*/62);
   }

   if (tape == (real)0.0 && dz == 0.0) {
      dx = dy = dz = (real)0.0;
      vx = vy = vz = (real)(var(Q_POS) / 3.0); /* Position error only */
   } else if (VAL(Comp) == HUGE_REAL &&
	      VAL(BackComp) == HUGE_REAL) {
      /* plumb */
      dx = dy = (real)0.0;
      if (dz < 0) tape = -tape;
      /* FIXME: Should use FrDepth sometimes... */
      dz = (dz * VAR(Tape) + tape * 2 * VAR(ToDepth))
	 / (VAR(Tape) * 2 * VAR(ToDepth));
      vx = vy = var(Q_POS) / 3.0 + dz * dz * var(Q_PLUMB);
      /* FIXME: Should use FrDepth sometimes... */
      vz = var(Q_POS) / 3.0 + VAR(Tape) * 2 * VAR(ToDepth)
			      / (VAR(Tape) + VAR(ToDepth));
   } else {
      real L2, sinB, cosB, dz2, D2;
      real var_comp;
      real comp = handle_compass(&var_comp);
      sinB = sin(comp);
      cosB = cos(comp);
      L2 = tape * tape;
      dz2 = dz * dz;
      D2 = L2 - dz2;
      if (D2 <= (real)0.0) {
	 /* FIXME: Should use FrDepth sometimes... */
	 real vsum = VAR(Tape) + 2 * VAR(ToDepth);
	 dx = dy = (real)0.0;
	 vx = vy = var(Q_POS) / 3.0;
	 /* FIXME: Should use FrDepth sometimes... */
	 vz = var(Q_POS) / 3.0 + VAR(Tape) * 2 * VAR(ToDepth) / vsum;
	 if (dz > 0) {
	    /* FIXME: Should use FrDepth sometimes... */
	    dz = (dz * VAR(Tape) + tape * 2 * VAR(ToDepth)) / vsum;
	 } else {
	    dz = (dz * VAR(Tape) - tape * 2 * VAR(ToDepth)) / vsum;
	 }
      } else {
	 real D = sqrt(D2);
	 /* FIXME: Should use FrDepth sometimes... */
	 real F = VAR(Tape) * L2 + 2 * VAR(ToDepth) * D2;
	 dx = D * sinB;
	 dy = D * cosB;

	 vx = var(Q_POS) / 3.0 +
	    sinB * sinB * F / D2 + var_comp * dy * dy;
	 vy = var(Q_POS) / 3.0 +
	    cosB * cosB * F / D2 + var_comp * dx * dx;
	 /* FIXME: Should use FrDepth sometimes... */
	 vz = var(Q_POS) / 3.0 + 2 * VAR(ToDepth);

#ifndef NO_COVARIANCES
	 cxy = sinB * cosB * (F / D2 + var_comp * D2);
	 /* FIXME: Should use FrDepth sometimes... */
	 cyz = -2 * VAR(ToDepth) * dy / D;
	 czx = -2 * VAR(ToDepth) * dx / D;
#endif
      }
      /* FIXME: If there's a clino reading, check it against the depth reading,
       * and average.
       * if (VAL(Clino) != HUGE_REAL || VAL(BackClino) != HUGE_REAL) { ... }
       */
   }
   addlegbyname(fr, to, fToFirst, dx, dy, dz, vx, vy, vz
#ifndef NO_COVARIANCES
		, cxy, cyz, czx
#endif
		);
   return 1;
}

static int
process_cartesian(prefix *fr, prefix *to, bool fToFirst)
{
   real dx = (VAL(Dx) * pcs->units[Q_DX] - pcs->z[Q_DX]) * pcs->sc[Q_DX];
   real dy = (VAL(Dy) * pcs->units[Q_DY] - pcs->z[Q_DY]) * pcs->sc[Q_DY];
   real dz = (VAL(Dz) * pcs->units[Q_DZ] - pcs->z[Q_DZ]) * pcs->sc[Q_DZ];

   addlegbyname(fr, to, fToFirst, dx, dy, dz, VAR(Dx), VAR(Dy), VAR(Dz)
#ifndef NO_COVARIANCES
		, 0, 0, 0
#endif
		);
   return 1;
}

static void
data_cartesian(void)
{
   prefix *fr = NULL, *to = NULL;

   bool fMulti = false;

   reading first_stn = End;

   const reading *ordering;

   again:

   for (ordering = pcs->ordering ; ; ordering++) {
      skipblanks();
      switch (*ordering) {
       case Fr:
	 fr = read_prefix(PFX_STATION|PFX_ALLOW_ROOT);
	 if (first_stn == End) first_stn = Fr;
	 break;
       case To:
	 to = read_prefix(PFX_STATION|PFX_ALLOW_ROOT);
	 if (first_stn == End) first_stn = To;
	 break;
       case Station:
	 fr = to;
	 to = read_prefix(PFX_STATION);
	 first_stn = To;
	 break;
       case Dx: case Dy: case Dz:
	 read_reading(*ordering, false);
	 break;
       case WallsSRVFr:
	  // Walls SRV is always From then To.
	  first_stn = Fr;
	  fr = read_walls_station(p_walls_options->prefix, true);
	  skipblanks();
	  if (ch == '*' || ch == '<') {
	      // Isolated LRUD.  Ignore for now.
	      skipline();
	      process_eol();
	      return;
	  }
	  break;
       case WallsSRVTo:
	  to = read_walls_station(p_walls_options->prefix, true);
	  skipblanks();
	  if (ch == '*' || ch == '<') {
	      // Odd apparently undocumented variant of isolated LRUD.  Ignore
	      // for now.
	      skipline();
	      process_eol();
	      return;
	  }
	  break;
       case Ignore:
	 skipword(); break;
       case IgnoreAllAndNewLine:
	 skipline();
	 /* fall through */
       case Newline:
	 if (fr != NULL) {
	    if (!process_cartesian(fr, to, first_stn == To))
	       skipline();
	 }
	 fMulti = true;
	 while (1) {
	    process_eol();
	    skipblanks();
	    if (isData(ch)) break;
	    if (!isComm(ch)) {
	       return;
	    }
	 }
	 break;
       case IgnoreAll:
	 skipline();
	 /* fall through */
       case End:
	 if (!fMulti) {
	    process_cartesian(fr, to, first_stn == To);
	    process_eol();
	    return;
	 }
	 do {
	    process_eol();
	    skipblanks();
	 } while (isComm(ch));
	 goto again;
       default: BUG("Unknown reading in ordering");
      }
   }
}

static int
process_cylpolar(prefix *fr, prefix *to, bool fToFirst, bool fDepthChange)
{
   real tape = VAL(Tape);

   real dx, dy, dz;
   real vx, vy, vz;
#ifndef NO_COVARIANCES
   real cxy = 0;
#endif

   handle_comp_units();

   /* depth gauge readings increase upwards with default calibration */
   if (fDepthChange) {
      SVX_ASSERT(VAL(FrDepth) == 0.0);
      dz = VAL(ToDepth) * pcs->units[Q_DEPTH] - pcs->z[Q_DEPTH];
      dz *= pcs->sc[Q_DEPTH];
   } else {
      dz = VAL(ToDepth) - VAL(FrDepth);
      dz *= pcs->units[Q_DEPTH] * pcs->sc[Q_DEPTH];
   }

   /* adjusted tape is negative -- probably the calibration is wrong */
   if (tape < (real)0.0) {
      compile_diagnostic_reading(DIAG_WARN, Tape, /*Negative adjusted tape reading*/79);
   }

   if (VAL(Comp) == HUGE_REAL && VAL(BackComp) == HUGE_REAL) {
      /* plumb */
      dx = dy = (real)0.0;
      vx = vy = var(Q_POS) / 3.0 + dz * dz * var(Q_PLUMB);
      /* FIXME: Should use FrDepth sometimes... */
      vz = var(Q_POS) / 3.0 + 2 * VAR(ToDepth);
   } else {
      real sinB, cosB;
      real var_comp;
      real comp = handle_compass(&var_comp);
      sinB = sin(comp);
      cosB = cos(comp);

      dx = tape * sinB;
      dy = tape * cosB;

      vx = var(Q_POS) / 3.0 +
	 VAR(Tape) * sinB * sinB + var_comp * dy * dy;
      vy = var(Q_POS) / 3.0 +
	 VAR(Tape) * cosB * cosB + var_comp * dx * dx;
      /* FIXME: Should use FrDepth sometimes... */
      vz = var(Q_POS) / 3.0 + 2 * VAR(ToDepth);

#ifndef NO_COVARIANCES
      cxy = (VAR(Tape) - var_comp * tape * tape) * sinB * cosB;
#endif
   }
   addlegbyname(fr, to, fToFirst, dx, dy, dz, vx, vy, vz
#ifndef NO_COVARIANCES
		, cxy, 0, 0
#endif
		);
   return 1;
}

/* Process tape/compass/clino, diving, and cylpolar styles of survey data
 * Also handles topofil (fromcount/tocount or count) in place of tape */
static void
data_normal(void)
{
   prefix *fr = NULL, *to = NULL;
   reading first_stn = End;

   bool fTopofil = false, fMulti = false;
   bool fRev;
   clino_type ctype, backctype;
   bool fDepthChange;
   unsigned long compass_dat_flags = 0;

   const reading *ordering;

   VAL(Tape) = VAL(BackTape) = HUGE_REAL;
   VAL(Comp) = VAL(BackComp) = HUGE_REAL;
   VAL(FrCount) = VAL(ToCount) = 0;
   VAL(FrDepth) = VAL(ToDepth) = 0;
   VAL(Left) = VAL(Right) = VAL(Up) = VAL(Down) = HUGE_REAL;

   fRev = false;
   ctype = backctype = CTYPE_OMIT;
   fDepthChange = false;

   /* ordering may omit clino reading, so set up default here */
   /* this is also used if clino reading is the omit character */
   VAL(Clino) = VAL(BackClino) = 0;

   again:

   /* We clear these flags in the normal course of events, but if there's an
    * error in a reading, we might not, so make sure it has been cleared here.
    */
   pcs->flags &= ~(BIT(FLAGS_ANON_ONE_END) | BIT(FLAGS_IMPLICIT_SPLAY));
   for (ordering = pcs->ordering; ; ordering++) {
      skipblanks();
      switch (*ordering) {
       case Fr:
	  fr = read_prefix(PFX_STATION|PFX_ALLOW_ROOT|PFX_ANON);
	  if (first_stn == End) first_stn = Fr;
	  break;
       case To:
	  to = read_prefix(PFX_STATION|PFX_ALLOW_ROOT|PFX_ANON);
	  if (first_stn == End) first_stn = To;
	  break;
       case CompassDATFr:
	  // Compass DAT is always From then To.
	  first_stn = Fr;
	  fr = read_prefix(PFX_STATION);
	  scan_compass_station_name(fr);
	  break;
       case CompassDATTo:
	  to = read_prefix(PFX_STATION);
	  scan_compass_station_name(to);
	  break;
       case Station:
	  fr = to;
	  to = read_prefix(PFX_STATION);
	  first_stn = To;
	  break;
       case Dir: {
	  typedef enum {
	     DIR_NULL=-1, DIR_FORE, DIR_BACK
	  } dir_tok;
	  static const sztok dir_tab[] = {
	     {"B",     DIR_BACK},
	     {"F",     DIR_FORE},
	  };
	  dir_tok tok;
	  get_token();
	  tok = match_tok(dir_tab, TABSIZE(dir_tab));
	  switch (tok) {
	   case DIR_FORE:
	     break;
	   case DIR_BACK:
	     fRev = true;
	     break;
	   default:
	     compile_diagnostic(DIAG_ERR|DIAG_TOKEN|DIAG_SKIP, /*Found “%s”, expecting “F” or “B”*/131, s_str(&token));
	     process_eol();
	     return;
	  }
	  break;
       }
       case Tape: case BackTape: {
	  reading r = *ordering;
	  read_reading(r, true);
	  if (VAL(r) == HUGE_REAL) {
	     if (!isOmit(ch)) {
		compile_diagnostic_token_show(DIAG_ERR, /*Expecting numeric field, found “%s”*/9);
		/* Avoid also warning about omitted tape reading. */
		VAL(r) = 0;
	     } else {
		nextch();
	     }
	  } else if (VAL(r) < (real)0.0) {
	     compile_diagnostic_reading(DIAG_WARN, r, /*Negative tape reading*/60);
	  }
	  break;
       }
       case Count:
	  VAL(FrCount) = VAL(ToCount);
	  LOC(FrCount) = LOC(ToCount);
	  WID(FrCount) = WID(ToCount);
	  read_reading(ToCount, false);
	  fTopofil = true;
	  break;
       case FrCount:
	  read_reading(FrCount, false);
	  break;
       case ToCount:
	  read_reading(ToCount, false);
	  fTopofil = true;
	  break;
       case Comp: case BackComp:
	  read_bearing_or_omit(*ordering);
	  break;
       case Clino: case BackClino: {
	  reading r = *ordering;
	  clino_type * p_ctype = (r == Clino ? &ctype : &backctype);
	  read_reading(r, true);
	  if (VAL(r) == HUGE_REAL) {
	     VAL(r) = handle_plumb(p_ctype);
	     if (VAL(r) != HUGE_REAL) break;
	     compile_diagnostic_token_show(DIAG_ERR, /*Expecting numeric field, found “%s”*/9);
	     skipline();
	     process_eol();
	     return;
	  }
	  *p_ctype = CTYPE_READING;
	  break;
       }
       case FrDepth: case ToDepth:
	  read_reading(*ordering, false);
	  break;
       case Depth:
	  VAL(FrDepth) = VAL(ToDepth);
	  LOC(FrDepth) = LOC(ToDepth);
	  WID(FrDepth) = WID(ToDepth);
	  read_reading(ToDepth, false);
	  break;
       case DepthChange:
	  fDepthChange = true;
	  VAL(FrDepth) = 0;
	  read_reading(ToDepth, false);
	  break;
       case CompassDATComp:
	  read_bearing_or_omit(Comp);
	  if (is_compass_NaN(VAL(Comp))) VAL(Comp) = HUGE_REAL;
	  break;
       case CompassDATBackComp:
	  read_bearing_or_omit(BackComp);
	  if (is_compass_NaN(VAL(BackComp))) VAL(BackComp) = HUGE_REAL;
	  break;
       case CompassDATClino: case CompassDATBackClino: {
	  reading r;
	  clino_type * p_ctype;
	  if (*ordering == CompassDATClino) {
	     r = Clino;
	     p_ctype = &ctype;
	  } else {
	     r = BackClino;
	     p_ctype = &backctype;
	  }
	  read_reading(r, false);
	  if (is_compass_NaN(VAL(r))) {
	     VAL(r) = 0;
	     *p_ctype = CTYPE_OMIT;
	  } else {
	     *p_ctype = CTYPE_READING;
	  }
	  break;
       }
       case CompassDATLeft: case CompassDATRight:
       case CompassDATUp: case CompassDATDown: {
	  /* FIXME: need to actually make use of these entries! */
	  reading actual = Left + (*ordering - CompassDATLeft);
	  read_reading(actual, false);
	  if (VAL(actual) < 0) VAL(actual) = HUGE_REAL;
	  break;
       }
       case CompassDATFlags:
	  if (ch == '#') {
	     filepos fp;
	     get_pos(&fp);
	     nextch();
	     if (ch == '|') {
		nextch();
		while (ch >= 'A' && ch <= 'Z') {
		   compass_dat_flags |= BIT(ch - 'A');
		   /* Flags we handle:
		    *   L (exclude from length)
		    *   S (splay)
		    *   P (no plot) (mapped to FLAG_SURFACE)
		    *   X (exclude data)
		    * FIXME: Defined flags we currently ignore:
		    *   C (no adjustment) (set all (co)variances to 0?  Then
		    *	  we need to handle a loop of such legs or a traverse
		    *	  of such legs between two fixed points...)
		    */
		   nextch();
		}
		if (ch == '#') {
		   nextch();
		} else {
		   compass_dat_flags = 0;
		   set_pos(&fp);
		}
	     } else {
		set_pos(&fp);
	     }
	  }
	  break;
       case WallsSRVFr:
	  // Walls SRV is always From then To.
	  first_stn = Fr;
	  fr = read_walls_station(p_walls_options->prefix, true);
	  skipblanks();
	  if (ch == '*' || ch == '<') {
	      // Isolated LRUD.  Ignore for now.
	      skipline();
	      process_eol();
	      return;
	  }
	  break;
       case WallsSRVTo:
	  to = read_walls_station(p_walls_options->prefix, true);
	  skipblanks();
	  if (ch == '*' || ch == '<') {
	      // Odd apparently undocumented variant of isolated LRUD.  Ignore
	      // for now.
	      skipline();
	      process_eol();
	      return;
	  }
	  break;
       case WallsSRVTape:
	  LOC(Tape) = ftell(file.fh);
	  VAL(Tape) = read_numeric(true);
	  if (VAL(Tape) == HUGE_REAL) {
	      if (ch == 'i' || ch == 'I') {
		  // Length specified in inches only, e.g. `i6` is 6 inches.
		  VAL(Tape) = 0.0;
		  goto inches_only;
	      }
	      // Walls expects 2 or more - for an omitted value.
	      if (ch != '-' || nextch() != '-') {
		  compile_diagnostic_token_show(DIAG_ERR, /*Expecting numeric field, found “%s”*/9);
		  /* Avoid also warning about omitted tape reading. */
		  VAL(Tape) = 0;
	      } else {
		  while (nextch() == '-') { }
	      }
	  } else {
	      if (VAL(Tape) < (real)0.0)
		  compile_diagnostic_reading(DIAG_WARN, Tape, /*Negative tape reading*/60);
	      switch (ch) {
		case 'I': case 'i':
inches_only:
		  nextch();
		  if (isdigit(ch)) {
		      real inches = read_numeric(false);
		      VAL(Tape) += inches / 12.0;
		  }
		  /* FALLTHRU */
		case 'F': case 'f':
		  VAL(Tape) *= METRES_PER_FOOT;
		  /* FALLTHRU */
		case 'M': case 'm':
		  VAL(Tape) /= pcs->units[Q_LENGTH];
		  nextch();
	      }
	  }
	  WID(Tape) = ftell(file.fh) - LOC(Tape);
	  VAR(Tape) = var(Q_LENGTH);
	  break;
       case WallsSRVComp: {
	  skipblanks();
	  LOC(Comp) = ftell(file.fh);
	  if (ch != '/') {
	      if (isalpha(ch)) {
		  VAL(Comp) = read_quadrant(false);
	      } else {
		  VAL(Comp) = read_number(true, false);
		  if (VAL(Comp) == HUGE_REAL) {
		      // Walls documents two or more `-` for an omitted
		      // reading, but actually just one works too!
		      while (nextch() == '-') { }
		  }
	      }
	      WID(Comp) = ftell(file.fh) - LOC(Comp);
	      VAR(Comp) = var(Q_BEARING);
	  } else {
	      // Omitted foresight, e.g. `/123` or `/` (both omitted).
	      WID(Comp) = 0;
	      VAL(Comp) = HUGE_REAL;
	  }
	  if (ch == '/' && !isBlank(nextch())) {
	      LOC(BackComp) = ftell(file.fh);
	      if (isalpha(ch)) {
		  VAL(BackComp) = read_quadrant(false);
	      } else {
		  VAL(BackComp) = read_number(true, false);
		  if (VAL(BackComp) == HUGE_REAL) {
		      // Walls documents two or more `-` for an omitted
		      // reading, but actually just one works too!
		      while (nextch() == '-') { }
		  }
	      }
	      WID(BackComp) = ftell(file.fh) - LOC(BackComp);
	      VAR(BackComp) = var(Q_BACKBEARING);
	  } else {
	      // Omitted backsight, e.g. `123/` or `/` (both omitted).
	      LOC(BackComp) = ftell(file.fh);
	      WID(BackComp) = 0;
	      VAL(BackComp) = HUGE_REAL;
	  }
	  break;
       }
       case WallsSRVClino: {
	  skipblanks();
	  LOC(Clino) = ftell(file.fh);
	  if (ch != '/') {
	      real clin = read_number(true, false);
	      if (clin == HUGE_REAL) {
		  // Walls documents two or more `-` for an omitted
		  // reading, but actually just one works too!
		  while (nextch() == '-') { }
	      } else {
		  VAL(Clino) = clin;
		  ctype = CTYPE_READING;
	      }
	      WID(Clino) = ftell(file.fh) - LOC(Clino);
	      VAR(Clino) = var(Q_GRADIENT);
	  } else {
	      // Omitted foresight, e.g. `/12` or `/` (both omitted).
	      WID(Clino) = 0;
	  }
	  if (ch == '/' && !isBlank(nextch())) {
	      LOC(BackClino) = ftell(file.fh);
	      real backclin = read_number(true, false);
	      if (backclin == HUGE_REAL) {
		  // Walls documents two or more `-` for an omitted
		  // reading, but actually just one works too!
		  while (nextch() == '-') { }
	      } else {
		  VAL(BackClino) = backclin;
		  backctype = CTYPE_READING;
	      }
	      WID(BackClino) = ftell(file.fh) - LOC(BackClino);
	      VAR(BackClino) = var(Q_BACKGRADIENT);
	  } else {
	      // Omitted backsight, e.g. `12/` or `/` (both omitted).
	      LOC(BackClino) = ftell(file.fh);
	      WID(BackClino) = 0;
	  }
	  break;
       }
       case Ignore:
	  skipword(); break;
       case IgnoreAllAndNewLine:
	  skipline();
	  /* fall through */
       case Newline:
	  if (fr != NULL) {
	     int r;
	     int save_flags;
	     int implicit_splay;
	     if (fTopofil) {
		VAL(Tape) = VAL(ToCount) - VAL(FrCount);
		LOC(Tape) = LOC(ToCount);
		WID(Tape) = WID(ToCount);
	     }
	     /* Note: frdepth == todepth test works regardless of fDepthChange
	      * (frdepth always zero, todepth is change of depth) and also
	      * works for STYLE_NORMAL (both remain 0) */
	     if (TSTBIT(pcs->infer, INFER_EQUATES) &&
		 (VAL(Tape) == (real)0.0 || VAL(Tape) == HUGE_REAL) &&
		 (VAL(BackTape) == (real)0.0 || VAL(BackTape) == HUGE_REAL) &&
		 VAL(FrDepth) == VAL(ToDepth)) {
		if (!TSTBIT(pcs->infer, INFER_EQUATES_SELF_OK) || fr != to)
		   process_equate(fr, to);
		goto inferred_equate;
	     }
	     if (fRev) {
		prefix *t = fr;
		fr = to;
		to = t;
	     }
	     if (fTopofil) {
		VAL(Tape) *= pcs->units[Q_COUNT] * pcs->sc[Q_COUNT];
	     } else if (VAL(Tape) != HUGE_REAL) {
		VAL(Tape) *= pcs->units[Q_LENGTH];
		VAL(Tape) -= pcs->z[Q_LENGTH];
		VAL(Tape) *= pcs->sc[Q_LENGTH];
	     }
	     if (VAL(BackTape) != HUGE_REAL) {
		VAL(BackTape) *= pcs->units[Q_BACKLENGTH];
		VAL(BackTape) -= pcs->z[Q_BACKLENGTH];
		VAL(BackTape) *= pcs->sc[Q_BACKLENGTH];
		if (VAL(Tape) != HUGE_REAL) {
		   real diff = VAL(Tape) - VAL(BackTape);
		   if (sqrd(diff / 3.0) > VAR(Tape) + VAR(BackTape)) {
		      /* fore and back readings differ by more than 3 sds */
		      /* TRANSLATORS: %s is replaced by the amount the readings disagree
		       * by, e.g. "0.12m" or "0.2ft". */
		      warn_readings_differ(/*TAPE reading and BACKTAPE reading disagree by %s*/97,
					   diff, get_length_units(Q_LENGTH), Tape, BackTape);
		   }
		   VAL(Tape) = VAL(Tape) / VAR(Tape) + VAL(BackTape) / VAR(BackTape);
		   VAR(Tape) = (VAR(Tape) + VAR(BackTape)) / 4;
		   VAL(Tape) *= VAR(Tape);
		} else {
		   VAL(Tape) = VAL(BackTape);
		   VAR(Tape) = VAR(BackTape);
		}
	     } else if (VAL(Tape) == HUGE_REAL) {
		compile_diagnostic_reading(DIAG_ERR, Tape, /*Tape reading may not be omitted*/94);
		goto inferred_equate;
	     }
	     implicit_splay = TSTBIT(pcs->flags, FLAGS_IMPLICIT_SPLAY);
	     pcs->flags &= ~(BIT(FLAGS_ANON_ONE_END) | BIT(FLAGS_IMPLICIT_SPLAY));
	     save_flags = pcs->flags;
	     if (implicit_splay) {
		pcs->flags |= BIT(FLAGS_SPLAY);
	     }
	     switch (pcs->style) {
	      case STYLE_NORMAL:
		r = process_normal(fr, to, (first_stn == To) ^ fRev,
				   ctype, backctype);
		break;
	      case STYLE_DIVING:
		/* FIXME: Handle any clino readings */
		r = process_diving(fr, to, (first_stn == To) ^ fRev,
				   fDepthChange);
		break;
	      case STYLE_CYLPOLAR:
		r = process_cylpolar(fr, to, (first_stn == To) ^ fRev,
				     fDepthChange);
		break;
	      default:
		r = 0; /* avoid warning */
		BUG("bad style");
	     }
	     pcs->flags = save_flags;
	     if (!r) skipline();

	     /* Swap fr and to back to how they were for next line */
	     if (fRev) {
		prefix *t = fr;
		fr = to;
		to = t;
	     }
	  }

	  fRev = false;
	  ctype = backctype = CTYPE_OMIT;
	  fDepthChange = false;

	  /* ordering may omit clino reading, so set up default here */
	  /* this is also used if clino reading is the omit character */
	  VAL(Clino) = VAL(BackClino) = 0;
	  LOC(Clino) = LOC(BackClino) = -1;
	  WID(Clino) = WID(BackClino) = 0;

	  inferred_equate:

	  fMulti = true;
	  while (1) {
	      process_eol();
	      skipblanks();
	      if (isData(ch)) break;
	      if (!isComm(ch)) {
		 return;
	      }
	  }
	  break;
       case IgnoreAll:
	  skipline();
	  /* fall through */
       case End:
	  if (!fMulti) {
	     int save_flags;
	     int implicit_splay;
	     /* Compass ignore flag is 'X' */
	     if ((compass_dat_flags & BIT('X' - 'A'))) {
		process_eol();
		return;
	     }
	     if (fRev) {
		prefix *t = fr;
		fr = to;
		to = t;
	     }
	     if (fTopofil) {
		VAL(Tape) = VAL(ToCount) - VAL(FrCount);
		LOC(Tape) = LOC(ToCount);
		WID(Tape) = WID(ToCount);
	     }
	     /* Note: frdepth == todepth test works regardless of fDepthChange
	      * (frdepth always zero, todepth is change of depth) and also
	      * works for STYLE_NORMAL (both remain 0) */
	     if (TSTBIT(pcs->infer, INFER_EQUATES) &&
		 (VAL(Tape) == (real)0.0 || VAL(Tape) == HUGE_REAL) &&
		 (VAL(BackTape) == (real)0.0 || VAL(BackTape) == HUGE_REAL) &&
		 VAL(FrDepth) == VAL(ToDepth)) {
		if (!TSTBIT(pcs->infer, INFER_EQUATES_SELF_OK) || fr != to)
		   process_equate(fr, to);
		process_eol();
		return;
	     }
	     if (fTopofil) {
		VAL(Tape) *= pcs->units[Q_COUNT] * pcs->sc[Q_COUNT];
	     } else if (VAL(Tape) != HUGE_REAL) {
		VAL(Tape) *= pcs->units[Q_LENGTH];
		VAL(Tape) -= pcs->z[Q_LENGTH];
		VAL(Tape) *= pcs->sc[Q_LENGTH];
	     }
	     if (VAL(BackTape) != HUGE_REAL) {
		VAL(BackTape) *= pcs->units[Q_BACKLENGTH];
		VAL(BackTape) -= pcs->z[Q_BACKLENGTH];
		VAL(BackTape) *= pcs->sc[Q_BACKLENGTH];
		if (VAL(Tape) != HUGE_REAL) {
		   real diff = VAL(Tape) - VAL(BackTape);
		   if (sqrd(diff / 3.0) > VAR(Tape) + VAR(BackTape)) {
		      /* fore and back readings differ by more than 3 sds */
		      /* TRANSLATORS: %s is replaced by the amount the readings disagree
		       * by, e.g. "0.12m" or "0.2ft". */
		      warn_readings_differ(/*TAPE reading and BACKTAPE reading disagree by %s*/97,
					   diff, get_length_units(Q_LENGTH), Tape, BackTape);
		   }
		   VAL(Tape) = VAL(Tape) / VAR(Tape) + VAL(BackTape) / VAR(BackTape);
		   VAR(Tape) = (VAR(Tape) + VAR(BackTape)) / 4;
		   VAL(Tape) *= VAR(Tape);
		} else {
		   VAL(Tape) = VAL(BackTape);
		   VAR(Tape) = VAR(BackTape);
		}
	     } else if (VAL(Tape) == HUGE_REAL) {
		compile_diagnostic_reading(DIAG_ERR, Tape, /*Tape reading may not be omitted*/94);
		process_eol();
		return;
	     }
	     implicit_splay = TSTBIT(pcs->flags, FLAGS_IMPLICIT_SPLAY);
	     pcs->flags &= ~(BIT(FLAGS_ANON_ONE_END) | BIT(FLAGS_IMPLICIT_SPLAY));
	     save_flags = pcs->flags;
	     if (implicit_splay) {
		pcs->flags |= BIT(FLAGS_SPLAY);
	     }
	     if ((compass_dat_flags & BIT('S' - 'A'))) {
		/* 'S' means "splay". */
		pcs->flags |= BIT(FLAGS_SPLAY);
	     }
	     if ((compass_dat_flags & BIT('P' - 'A'))) {
		/* 'P' means "Exclude this shot from plotting", but the use
		 * suggested in the Compass docs is for surface data, and legs
		 * with this flag "[do] not support passage modeling".
		 *
		 * Even if it's actually being used for a different
		 * purpose, Survex programs don't show surface legs
		 * by default so FLAGS_SURFACE matches fairly well.
		 */
		pcs->flags |= BIT(FLAGS_SURFACE);
	     }
	     if ((compass_dat_flags & BIT('L' - 'A'))) {
		/* 'L' means "exclude from length" - map this to Survex's
		 * FLAGS_DUPLICATE. */
		pcs->flags |= BIT(FLAGS_DUPLICATE);
	     }
	     switch (pcs->style) {
	      case STYLE_NORMAL:
		process_normal(fr, to, (first_stn == To) ^ fRev,
			       ctype, backctype);
		break;
	      case STYLE_DIVING:
		/* FIXME: Handle any clino readings */
		process_diving(fr, to, (first_stn == To) ^ fRev,
			       fDepthChange);
		break;
	      case STYLE_CYLPOLAR:
		process_cylpolar(fr, to, (first_stn == To) ^ fRev,
				 fDepthChange);
		break;
	      default:
		BUG("bad style");
	     }
	     pcs->flags = save_flags;

	     process_eol();
	     return;
	  }
	  do {
	     process_eol();
	     skipblanks();
	  } while (isComm(ch));
	  goto again;
       default:
	  BUG("Unknown reading in ordering");
      }
   }
}

static int
process_lrud(prefix *stn)
{
   SVX_ASSERT(next_lrud);
   lrud * xsect = osnew(lrud);
   xsect->stn = stn;
   xsect->l = (VAL(Left) * pcs->units[Q_LEFT] - pcs->z[Q_LEFT]) * pcs->sc[Q_LEFT];
   xsect->r = (VAL(Right) * pcs->units[Q_RIGHT] - pcs->z[Q_RIGHT]) * pcs->sc[Q_RIGHT];
   xsect->u = (VAL(Up) * pcs->units[Q_UP] - pcs->z[Q_UP]) * pcs->sc[Q_UP];
   xsect->d = (VAL(Down) * pcs->units[Q_DOWN] - pcs->z[Q_DOWN]) * pcs->sc[Q_DOWN];
   xsect->meta = pcs->meta;
   if (pcs->meta) ++pcs->meta->ref_count;
   xsect->next = NULL;
   *next_lrud = xsect;
   next_lrud = &(xsect->next);

   return 1;
}

static void
data_passage(void)
{
   prefix *stn = NULL;
   const reading *ordering;

   for (ordering = pcs->ordering ; ; ordering++) {
      skipblanks();
      switch (*ordering) {
       case Station:
	 stn = read_prefix(PFX_STATION);
	 break;
       case Left: case Right: case Up: case Down: {
	 reading r = *ordering;
	 read_reading(r, true);
	 if (VAL(r) == HUGE_REAL) {
	    if (!isOmit(ch)) {
	       compile_diagnostic_token_show(DIAG_ERR, /*Expecting numeric field, found “%s”*/9);
	    } else {
	       nextch();
	    }
	    VAL(r) = -1;
	 }
	 break;
       }
       case Ignore:
	 skipword(); break;
       case IgnoreAll:
	 skipline();
	 /* fall through */
       case End: {
	 process_lrud(stn);
	 process_eol();
	 return;
       }
       default: BUG("Unknown reading in ordering");
      }
   }
}

static int
process_nosurvey(prefix *fr, prefix *to, bool fToFirst)
{
   nosurveylink *link;

   /* Suppress "unused fixed point" warnings for these stations */
   fr->sflags |= BIT(SFLAGS_USED);
   to->sflags |= BIT(SFLAGS_USED);

   /* add to linked list which is dealt with after network is solved */
   link = osnew(nosurveylink);
   if (fToFirst) {
      link->to = StnFromPfx(to);
      link->fr = StnFromPfx(fr);
   } else {
      link->fr = StnFromPfx(fr);
      link->to = StnFromPfx(to);
   }
   link->flags = pcs->flags | (STYLE_NOSURVEY << FLAGS_STYLE_BIT0);
   link->meta = pcs->meta;
   if (pcs->meta) ++pcs->meta->ref_count;
   link->next = nosurveyhead;
   nosurveyhead = link;
   return 1;
}

static void
data_nosurvey(void)
{
   prefix *fr = NULL, *to = NULL;

   bool fMulti = false;

   reading first_stn = End;

   const reading *ordering;

   again:

   for (ordering = pcs->ordering ; ; ordering++) {
      skipblanks();
      switch (*ordering) {
       case Fr:
	  fr = read_prefix(PFX_STATION|PFX_ALLOW_ROOT);
	  if (first_stn == End) first_stn = Fr;
	  break;
       case To:
	  to = read_prefix(PFX_STATION|PFX_ALLOW_ROOT);
	  if (first_stn == End) first_stn = To;
	  break;
       case Station:
	  fr = to;
	  to = read_prefix(PFX_STATION);
	  first_stn = To;
	  break;
       case Ignore:
	 skipword(); break;
       case IgnoreAllAndNewLine:
	 skipline();
	 /* fall through */
       case Newline:
	 if (fr != NULL) {
	    if (!process_nosurvey(fr, to, first_stn == To))
	       skipline();
	 }
	 if (ordering[1] == End) {
	    do {
	       process_eol();
	       skipblanks();
	    } while (isComm(ch));
	    if (!isData(ch)) {
	       return;
	    }
	    goto again;
	 }
	 fMulti = true;
	 while (1) {
	    process_eol();
	    skipblanks();
	    if (isData(ch)) break;
	    if (!isComm(ch)) {
	       return;
	    }
	 }
	 break;
       case IgnoreAll:
	 skipline();
	 /* fall through */
       case End:
	 if (!fMulti) {
	    (void)process_nosurvey(fr, to, first_stn == To);
	    process_eol();
	    return;
	 }
	 do {
	    process_eol();
	    skipblanks();
	 } while (isComm(ch));
	 goto again;
       default: BUG("Unknown reading in ordering");
      }
   }
}

/* totally ignore a line of survey data */
static void
data_ignore(void)
{
   skipline();
   process_eol();
}
