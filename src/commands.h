/* > commands.h
 * Header file for code for directives
 * Copyright (C) 1994,1995,1996 Olly Betts
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

extern void handle_command(void);
extern void default_all(settings *s);

extern void get_token(void);

typedef struct { const char *sz; int tok; } sztok;
extern int match_tok(const sztok *tab, int tab_size);

#define TABSIZE(T) ((sizeof(T) / sizeof(sztok)) - 1)
