/* ini.h
 * .ini file routines
 * Copyright (C) 1995-2001 Olly Betts
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

#include <stdio.h>

/* fh_list is a NULL terminated array of FILE*-s of ini files
 * section is the section of the ini file to read from
 * vars is a list of variables to read (terminated by NULL)
 * returns a list of values with NULL for "not found" (not terminated)
 */
char **ini_read(FILE **fh_list, const char *section, const char **vars);

/* very similar to ini_read, but recursively tries the section named by
 * the first read parameter until it finds the variable or finds no
 * recursive field
 */
char **ini_read_hier(FILE **fh_list, const char *section, const char **vars);
