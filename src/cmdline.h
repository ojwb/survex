/* > cmdline.h
 * Wrapper for GNU getopt which deals with standard options
 * Copyright (C) 1998-2000 Olly Betts
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

#include "getopt.h"

struct help_msg {
   int opt;
   char *msg;
};

/* give -1 for max_args_ if there's no limit */
void cmdline_init(int argc_, char *const *argv_, const char *shortopts_,
		  const struct option *longopts_, int *longind_,
		  const struct help_msg *help_,
		  int min_args_, int max_args_);
/* if args not NULL, use instead of auto-generated FILE args */
/* if extra not NULL, display as extra blurb at end */
void cmdline_set_syntax_message(const char *args, const char *extra);
int cmdline_getopt(void);
void cmdline_help(void);
void cmdline_version(void);
void cmdline_syntax(void);
int cmdline_int_arg(void);
float cmdline_float_arg(void);
double cmdline_double_arg(void);

#define HLP_ENCODELONG(N) (-(N + 1))
#define HLP_DECODELONG(N) (-(N + 1))
#define HLP_ISLONG(N) ((N) <= 0)
#define HLP_HELP 30000
#define HLP_VERSION 30001
