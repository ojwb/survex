/* commands.h
 * Header file for code for directives
 * Copyright (C) 1994-2024 Olly Betts
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

#include "str.h"

/* Fix station if not already fixed.
 *
 * stn_offset gives column info - it should be the file offset (offset member
 * of a filepos) or -1 for no column info.
 *
 * Returns:
 *  0 if not already fixed
 *  1 if already fixed at the same coordinates
 * -1 if already fixed but at different coordinates
 */
int fix_station(prefix *fix_name, const double* coords, long stn_offset);

/* Fix station with variance.
 *
 * Multiple fixes for the same station are OK.
 */
void fix_station_with_variance(prefix *fix_name, const double* coords,
			       real var_x, real var_y, real var_z,
#ifndef NO_COVARIANCES
			       real cxy, real cyz, real cza
#endif
			      );

int get_length_units(int quantity);
int get_angle_units(int quantity);

extern const real factor_tab[];
extern const int units_to_msgno[];

#define get_units_factor(U) (factor_tab[(U)])
#define get_units_string(U) (msg(units_to_msgno[(U)]))

void handle_command(void);
void default_all(settings *s);

void default_units(settings *s);
void default_calib(settings *s);

void pop_settings(void);
void invalidate_pj_cached(void);
void report_declination(settings *p);

void copy_on_write_meta(settings *s);

extern string token;
extern string uctoken;

/* Read legacy token (letters only).  This only exists so we can keep reading
 * old data files which (presumably accidentally) are missing blanks between
 * a token and a number which follows.  Use get_token() in new code.
 */
void get_token_legacy(void);
void get_token_legacy_no_blanks(void);

// Issue warning if token not empty and ch not BLANK, COMM or EOL.
void do_legacy_token_warning(void);

// Read a token, comprised of a letter followed by contiguous alphanumerics.
void get_token(void);
void get_token_no_blanks(void);

// Read up to the next BLANK, COMM or end of line.
void get_word(void);

typedef struct { const char *sz; int tok; } sztok;
int match_tok(const sztok *tab, int tab_size);

#define TABSIZE(T) ((sizeof(T) / sizeof(sztok)) - 1)

void scan_compass_station_name(prefix *stn);
void update_output_separator(void);
