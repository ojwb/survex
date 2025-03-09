/* readval.h
 * Routines to read a prefix or number from the current input file
 * Copyright (C) 1991-2024 Olly Betts
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
// Caller is responsible for calling osfree() on the returned value.
char *read_walls_prefix(void);

prefix *read_walls_station(char * const walls_prefix[3],
			   bool anon_allowed,
			   bool *p_new);

// Like read_numeric() but doesn't skipblanks() first and can be told to not
// allow a sign.
real read_number(bool f_optional, bool f_unsigned);

real read_quadrant(bool f_optional);

real read_numeric(bool f_optional);
real read_numeric_multi(bool f_optional, bool f_quadrants, bool f_footinches, int *p_n_readings);
real read_bearing_multi_or_omit(bool f_quadrants, int *p_n_readings);

/* Don't skip blanks, variable error code */
unsigned int read_uint_raw(int errmsg, const filepos *fp);

unsigned int read_uint(void);

int read_int(int min_val, int max_val);

void read_string(string *pstr);

void read_walls_srv_date(int *py, int *pm, int *pd);
