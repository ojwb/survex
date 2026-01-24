/* readval.h
 * Routines to read a prefix or number from the current input file
 * Copyright (C) 1991-2025 Olly Betts
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
 * along with this program; if not, see
 * <https://www.gnu.org/licenses/>.
 */

#include "datain.h"

extern int root_depr_count;

enum {
    /* Can the prefix be omitted?  If it is, read_prefix() returns NULL. */
    PFX_OPT = 1,
    /* Read a survey? */
    PFX_SURVEY = 2,
    /* Make implicit checks */
    PFX_SUSPECT_TYPO = 4,
    /* Can the deprecated "root" be used? */
    PFX_ALLOW_ROOT = 8,
    /* Warn if the read prefix contains a separator? */
    PFX_WARN_SEPARATOR = 16,
    /* Anonymous stations OK? */
    PFX_ANON = 32,
    /* Read a station? */
    PFX_STATION = 0
};

prefix *read_prefix(unsigned flags);

// Read a sequence of NAMES characters.  Returns NULL if none.
// Caller is responsible for calling free() on the returned value.
char *read_walls_prefix(void);

prefix *read_walls_station(char * const walls_prefix[3],
			   bool anon_allowed,
			   bool *p_new);

// Like read_number() but reports if the number contains a decimal point or
// not.
//
// pf_decimal_point can be NULL (but use read_numeric() instead if always
// NULL).
real read_number_or_int(bool f_optional, bool f_unsigned,
			bool *pf_decimal_point);

// Like read_numeric() but doesn't skipblanks() first and can be told to not
// allow a sign.
static inline real read_number(bool f_optional, bool f_unsigned)
{
    return read_number_or_int(f_optional, f_unsigned, NULL);
}

real read_quadrant(bool f_optional);

real read_numeric(bool f_optional);
real read_numeric_multi(bool f_optional, bool f_quadrants, int *p_n_readings);
real read_bearing_multi_or_omit(bool f_quadrants, int *p_n_readings);

/* Don't skip blanks; can specify diagnostic type and error code. */
unsigned int read_uint_raw(int diag_type, int errmsg, const filepos *fp);

unsigned int read_uint(void);

int read_int(int min_val, int max_val);

void read_string(string *pstr);

bool read_string_warning(string *pstr);

void read_walls_srv_date(int *py, int *pm, int *pd);
