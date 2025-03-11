/* dynamic string handling */
/* Copyright (c) Olly Betts 1999, 2001, 2012, 2014, 2024
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

#ifndef SURVEX_INCLUDED_STR_H
#define SURVEX_INCLUDED_STR_H

#include "osalloc.h"
#include <string.h>

typedef struct {
    char *s;
    int len;
    int capacity; /* One less than the allocated size! */
} string;

// string foo = S_INIT;
#define S_INIT { NULL, 0, 0 }

void s_expand_(string *pstr, int addition);

/* Append a block of text with given length. */
void s_appendlen(string *pstr, const char *s, int s_len);

void s_appendn(string *pstr, int n, char c);

/* Append another string. */
static inline void s_appends(string *pstr, const string *s) {
    s_appendlen(pstr, s->s, s->len);
}

/* Append a C string. */
static inline void s_append(string *pstr, const char *s) {
    s_appendlen(pstr, s, strlen(s));
}

/* Append a character */
static inline void s_appendch(string *pstr, char c) {
    if (pstr->capacity == pstr->len) s_expand_(pstr, 1);
    pstr->s[pstr->len++] = c;
}

/* Truncate string to zero length (and ensure it isn't NULL). */
static inline void s_clear(string *pstr) {
    if (pstr->s == NULL) s_expand_(pstr, 0);
    pstr->len = 0;
}

/* Truncate string */
static inline void s_truncate(string *pstr, int new_len) {
    if (new_len < pstr->len) {
	pstr->len = new_len;
	pstr->s[new_len] = '\0';
    }
}

/* Release allocated memory. */
static inline void s_free(string *pstr) {
    free(pstr->s);
    pstr->s = NULL;
    pstr->len = 0;
    pstr->capacity = 0;
}

/* Steal the C string. */
static inline char *s_steal(string *pstr) {
    char *s = pstr->s;
    s[pstr->len] = '\0';
    pstr->s = NULL;
    pstr->len = 0;
    pstr->capacity = 0;
    return s;
}

/* Donate a malloc-ed C string. */
static inline void s_donate(string *pstr, char *s) {
    free(pstr->s);
    pstr->s = s;
    pstr->capacity = pstr->len = strlen(s);
}

static inline int s_len(const string *pstr) { return pstr->len; }

static inline bool s_empty(const string *pstr) { return pstr->len == 0; }

static inline const char *s_str(string *pstr) {
    char *s = pstr->s;
    if (s) s[pstr->len] = '\0';
    return s;
}

static inline bool s_eqlen(const string *pstr, const char *s, int s_len) {
    return pstr->len == s_len && memcmp(pstr->s, s, s_len) == 0;
}

static inline bool s_eq(const string *pstr, const char *s) {
    return strncmp(pstr->s, s, pstr->len) == 0 && s[pstr->len] == '\0';
}

static inline char s_back(const string *pstr) {
    return pstr->s[pstr->len - 1];
}

#define S_EQ(PSTR, LITERAL) s_eqlen((PSTR), LITERAL, sizeof(LITERAL "") - 1)

#endif // SURVEX_INCLUDED_STR_H
