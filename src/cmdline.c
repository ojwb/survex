/* > cmdline.c
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "cmdline.h"
#include "filename.h"

/* It might be useful to be able to disable all long options on small
 * platforms like pre-386 DOS and perhaps some PDAs...
 */
#if 0
# define getopt_long(ARGC, ARGV, STR, OPTS, PTR) getopt(ARGC, ARGV, STR)
#endif

/*
 * bad command line give:
 * <problem>
 * 
 * <short syntax>
 * 
 * --help gives:
 * <version>
 * 
 * <short syntax>
 *
 * <table>
 * 
 * <blurb>
 *
 * --version gives:
 * <version>
 */

/*
 * want to cope with optional/required parameters on long options
 * and also parameters on short options
 */

static const char newline_tabs[] = "\n\t\t\t\t";

static int argc;
static char *const *argv;
static const char *shortopts;
static const struct option *longopts;
static int *longind;
static const struct help_msg *help;
static int min_args, max_args;

static const char *argv0 = NULL;

void
cmdline_help(void)
{
   while (help->opt) {
      const char *longopt = 0;
      int opt = help->opt;
      const struct option *o = 0;

      if (HLP_ISLONG(opt)) {
	 o = longopts + HLP_DECODELONG(opt);
	 longopt = o->name;
	 opt = o->val;
      }

      if (isalnum(opt))
	 printf("  -%c,", opt);
      else
	 fputs("     ", stdout);

      if (longopt) {
	 int len = strlen(longopt);
	 printf(" --%s", longopt);
	 if (o && o->has_arg) {
	    const char *p;
	    len += len + 1;

	    if (o->has_arg == optional_argument) {
	       putchar('[');
	       len += 2;
	    }

	    putchar('=');

	    for (p = longopt; *p ; p++) putchar(toupper(*p));

	    if (o->has_arg == optional_argument) putchar(']');
	 }
	 len = (len >> 3) + 2;
	 if (len > 4) len = 0;
	 fputs(newline_tabs + len, stdout);
      } else {
	 fputs(newline_tabs + 1, stdout);
      }

      puts(help->msg);
      help++;
   }
   /* FIXME: translate */
   puts("      --help\t\t\tdisplay this help and exit\n"
	"      --version\t\t\toutput version information and exit");
 
   exit(0);
}

void
cmdline_version(void)
{
   printf("%s - "PACKAGE" "VERSION"\n", argv0);
}

void
cmdline_syntax(void)
{  
   printf("\nSyntax: %s [OPTION]...", argv0);
   if (min_args) {
      int i = min_args;
      while (i--) fputs(" FILE", stdout);
   } else {
      if (max_args) fputs(" [FILE]", stdout);
   }
   if (max_args == -1) fputs("...", stdout);
   
   /* FIXME: not quite right - "..." means an indefinite number */
   if (max_args > min_args) fputs("...", stdout);
   putnl();
}

int
cmdline_int_arg(void)
{
   int result;
   char *endptr;
   
   errno = 0;

   result = strtol(optarg, &endptr, 10);

   if (errno == ERANGE) {
      fprintf(stderr, "%s: numeric argument `%s' out of range\n", argv0, optarg);
      cmdline_syntax();
      exit(0);
   } else if (*optarg == '\0' || *endptr != '\0') {
      fprintf(stderr, "%s: argument `%s' not an integer\n", argv0, optarg);
      cmdline_syntax();
      exit(0);
   }
   
   return result;
}

float
cmdline_float_arg(void)
{
   float result;
   char *endptr;
   
   errno = 0;

   result = strtod(optarg, &endptr);

   if (errno == ERANGE) {
      fprintf(stderr, "%s: numeric argument `%s' out of range\n", argv0, optarg);
      cmdline_syntax();
      exit(0);
   } else if (*optarg == '\0' || *endptr != '\0') {
      fprintf(stderr, "%s: argument `%s' not a number\n", argv0, optarg);
      cmdline_syntax();
      exit(0);
   }
   
   return result;
}

void
cmdline_init(int argc_, char *const *argv_, const char *shortopts_,
	     const struct option *longopts_, int *longind_,
	     const struct help_msg *help_,
	     int min_args_, int max_args_)
{
   if (!argv0) {
      argv0 = argv_[0];
      /* FIXME: tidy up argv0 (remove path and extension) */
   }

   argc = argc_;
   argv = argv_;
   shortopts = shortopts_;
   longopts = longopts_;
   longind = longind_;
   help = help_;
   min_args = min_args_;
   max_args = max_args_;
}

int
cmdline_getopt(void)
{
   int opt = getopt_long(argc, argv, shortopts, longopts, longind);

   if (opt == EOF) {
      /* check minimum # of args given - if not give syntax message */
      if (argc - optind < min_args) {
	 /* FIXME: TRANSLATE */
	 fprintf(stderr, "%s: too few arguments\n", argv0);
	 opt = '?';
      } else if (max_args >= 0 && argc - optind > max_args) {
	 /* FIXME: TRANSLATE */
	 fprintf(stderr, "%s: too many arguments\n", argv0);
	 opt = '?';
      }
   }

   switch (opt) {
    case ':': /* parameter missing */
    case '?': /* unknown opt, ambiguous match, or extraneous param */
      /* getopt displays a message for us (unless we set opterr to 0) */
      /* FIXME: set opterr to 0 so we can translate messages? */
      cmdline_syntax();
      /* FIXME: translate */
      fprintf(stderr, "Try `%s --help' for more information.\n", argv0);
      exit(0);
    case HLP_VERSION: /* --version */
      cmdline_version();
      exit(0);
    case HLP_HELP: /* --help */
      cmdline_version();
      cmdline_syntax();
      putchar('\n');
      cmdline_help();
      exit(0);
   }
   return opt;
}
