/* > cmdline.h
 * Wrapper for GNU getopt which deals with standard options
 * Copyright (C) 1998 Olly Betts
 */

#include "getopt.h"

struct help_msg { int opt; char *msg; };

extern int my_getopt_long(int argc, char *const *argv, const char *shortopts,
			  const struct option *longopts, int *longind,
			  const struct help_msg *help, int min_args);

#define HLP_ENCODELONG(N) (-(N + 1))
#define HLP_DECODELONG(N) (-(N + 1))
#define HLP_ISLONG(N) ((N) <= 0)
#define HLP_HELP 30000
#define HLP_VERSION 30001
