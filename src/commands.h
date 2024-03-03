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
void set_declination_location(real x, real y, real z, const char *proj_str);

void copy_on_write_meta(settings *s);

extern char *buffer;
void get_token(void);
void get_token_no_blanks(void);

typedef struct { const char *sz; int tok; } sztok;
int match_tok(const sztok *tab, int tab_size);

#define TABSIZE(T) ((sizeof(T) / sizeof(sztok)) - 1)

void scan_compass_station_name(prefix *stn);
void update_output_separator(void);
