/* > message.h
 * Function prototypes for message.c
 * Copyright (C) 1998-2001 Olly Betts
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

#ifndef MESSAGE_H /* only include once */
#define MESSAGE_H

#include <stdarg.h>

#include "useful.h"
#include "osdepend.h"
#include "osalloc.h"

#define STDERR stdout

#ifdef RISCOS_LEAKFIND
# include "Mnemosyne.mnemosyn.h"
#endif

#define CHARSET_BAD       -1
#define CHARSET_USASCII    0
#define CHARSET_ISO_8859_1 1
#define CHARSET_DOSCP850   2
#define CHARSET_RISCOS31   3
#define CHARSET_UTF8	   4

extern int msg_warnings; /* keep track of how many warnings we've given */
extern int msg_errors;   /* and how many (non-fatal) errors */

extern const char *msg_lang;

void msg_init(const char *argv0);

const char *msg_cfgpth(void);

/* Message may be overwritten by next call */
const char *msg(int en);
/* Returns persistent copy of message */
const char *msgPerm(int en);
/* Kill persistent copy of message */
#define msgFree(S) NOP

void v_report(int severity, const char *fnm, int line, int en, va_list ap);

void warning(int en, ...);
void error(int en, ...);
void fatalerror(int en, ...);

void warning_in_file(const char *fnm, int line, int en, ...);
void error_in_file(const char *fnm, int line, int en, ...);
void fatalerror_in_file(const char *fnm, int line, int en, ...);

int select_charset(int charset_code);

#endif /* MESSAGE_H */
