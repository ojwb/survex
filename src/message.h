/* > message.h
 * Function prototypes for message.c
 * Copyright (C) 1998 Olly Betts
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
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

/* name of current application */
extern const char *szAppNameCopy;

extern void msg_init(const char *argv0);

extern const char *msg_cfgpth(void);

/* report total warnings and non-fatal errors */
extern int error_summary(void);

/* Message may be overwritten by next call */
extern const char *msg(int en);
/* Returns persistent copy of message */
extern const char *msgPerm(int en);
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
