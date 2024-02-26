/* message.h
 * Function prototypes for message.c
 * Copyright (C) 1998-2022 Olly Betts
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

#include "osalloc.h"

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

void msg_init(char *const *argv);

const char *msg_cfgpth(void);
const char *msg_exepth(void);
const char *msg_appname(void);

/* Return the message string corresponding to number en */
const char *msg(int en);

/* These need to fit within DIAG_SEVERITY_MASK defined in datain.h. */
#define DIAG_INFO	0x00
#define DIAG_WARN	0x01
#define DIAG_ERR	0x02
#define DIAG_FATAL	0x03

void v_report(int severity, const char *fnm, int line, int col, int en, va_list ap);

void diag(int severity, int en, ...);

#define information(...) diag(DIAG_INFO, __VA_ARGS__)
#define warning(...) diag(DIAG_WARN, __VA_ARGS__)
#define error(...) diag(DIAG_ERR, __VA_ARGS__)
#define fatalerror(...) diag(DIAG_FATAL, __VA_ARGS__)

void diag_in_file(int severity, const char *fnm, int line, int en, ...);

#define information_in_file(...) diag_in_file(DIAG_INFO, __VA_ARGS__)
#define warning_in_file(...) diag_in_file(DIAG_WARN, __VA_ARGS__)
#define error_in_file(...) diag_in_file(DIAG_ERR, __VA_ARGS__)
#define fatalerror_in_file(...) diag_in_file(DIAG_FATAL, __VA_ARGS__)

int select_charset(int charset_code);

#ifdef __cplusplus
}
#endif

/* Define MSG_SETUP_PROJ_SEARCH_PATH before including this header to enable the
 * hooks to setup proj's search path to be relative to the executable if this
 * is a relocatable install.
 */
#ifdef MSG_SETUP_PROJ_SEARCH_PATH
/* We only need this on these platforms. */
# if OS_WIN32
#  include <proj.h>
#  define msg_init(ARGV) do {\
	(msg_init)(ARGV); \
	const char* msg_init_proj_path = msg_cfgpth();\
	proj_context_set_search_paths(PJ_DEFAULT_CTX, 1, &msg_init_proj_path);\
    } while (0)
# endif
#endif

#endif /* MESSAGE_H */
