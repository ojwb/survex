/* message.h
 * Function prototypes for message.c
 * Copyright (C) 1998-2003,2005,2010,2015,2017 Olly Betts
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

#ifndef MESSAGE_H /* only include once */
#define MESSAGE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdarg.h>

#include "osdepend.h"
#include "osalloc.h"

/* Define MSG_SETUP_PROJ_SERACH_PATH before including this header to enable the
 * hooks to setup proj4's search path to be relative to the executable if this
 * is a relocatable install.
 */
#ifdef MSG_SETUP_PROJ_SERACH_PATH
/* We only support relocatable builds on these platforms. */
# if OS_WIN32 || OS_UNIX_MACOSX
#  include <proj_api.h>
#  define msg_init(ARGV) do {\
	if (msg_init_(ARGV)) pj_set_finder(msg_proj_finder_);\
    } while (0)
# endif
#endif

#ifndef msg_init
# define msg_init(ARGV) msg_init_(ARGV)
#endif

#define STDERR stdout

#define CHARSET_BAD        -1
#define CHARSET_USASCII     0
#define CHARSET_ISO_8859_1  1
#define CHARSET_DOSCP850    2
#define CHARSET_UTF8	    4
#define CHARSET_WINCP1252   5
#define CHARSET_ISO_8859_15 6
#define CHARSET_ISO_8859_2  8
#define CHARSET_WINCP1250   9

extern int msg_warnings; /* keep track of how many warnings we've given */
extern int msg_errors;   /* and how many (non-fatal) errors */

/* The language code - e.g. "en_GB" */
extern const char *msg_lang;
/* If the language code has a country specific qualification, then this will
 * be just the language code.  Otherwise it's NULL.  e.g. "en" */
extern const char *msg_lang2;

/* Not intended for direct use - use msg_init() instead, optionally defining
 * MSG_SETUP_PROJ_SERACH_PATH.
 */
int msg_init_(char *const *argv);

/* Not intended for direct use - use msg_init() instead, optionally defining
 * MSG_SETUP_PROJ_SERACH_PATH.
 */
const char * msg_proj_finder_(const char * file);

const char *msg_cfgpth(void);
const char *msg_exepth(void);
const char *msg_appname(void);

/* Return the message string corresponding to number en */
const char *msg(int en);

void v_report(int severity, const char *fnm, int line, int col, int en, va_list ap);

void warning(int en, ...);
void error(int en, ...);
void fatalerror(int en, ...);

void warning_in_file(const char *fnm, int line, int en, ...);
void error_in_file(const char *fnm, int line, int en, ...);
void fatalerror_in_file(const char *fnm, int line, int en, ...);

int select_charset(int charset_code);

#ifdef __cplusplus
}
#endif

#endif /* MESSAGE_H */
