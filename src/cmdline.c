/* cmdline.c
 * Wrapper for GNU getopt which deals with standard options
 * Copyright (C) 1998-2001,2003,2004,2011,2012,2014 Olly Betts
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <ctype.h>
#include <errno.h>
#include <float.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

#include "getopt.h"

#include "cmdline.h"
#include "debug.h"
#include "filename.h"

#include "message.h"

/* It might be useful to be able to disable all long options on small
 * platforms like older PDAs.
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
static char * const *argv;
static const char *shortopts;
static const struct option *longopts;
static int *longind;
static const struct help_msg *help;
static int min_args, max_args;
static int msg_args, msg_extra;
static const char * msg_extra_arg;

void
cmdline_help(void)
{
   while (help && help->opt) {
      const char *longopt = 0;
      int opt = help->opt;
      const struct option *o = 0;

      if (HLP_ISLONG(opt)) {
	 o = longopts + HLP_DECODELONG(opt);
	 longopt = o->name;
	 opt = o->val;
      }

      if (isalnum((unsigned char)opt))
	 printf("  -%c%c", opt, longopt ? ',' : ' ');
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

      if (help->arg) {
	  SVX_ASSERT(strstr(msg(help->msg_no), "%s") != NULL);
	  printf(msg(help->msg_no), help->arg);
	  putnl();
      } else {
	  SVX_ASSERT(strstr(msg(help->msg_no), "%s") == NULL);
	  puts(msg(help->msg_no));
      }
      help++;
   }
   fputs("      --help\t\t\t", stdout);
   /* TRANSLATORS: description of --help option */
   puts(msg(/*display this help and exit*/150));
   fputs("      --version\t\t\t", stdout);
   /* TRANSLATORS: description of --version option */
   puts(msg(/*output version information and exit*/151));

   if (msg_extra) {
      putnl();
      if (msg_extra_arg) {
	  SVX_ASSERT(strstr(msg(msg_extra), "%s") != NULL);
	  printf(msg(msg_extra), msg_extra_arg);
	  putnl();
      } else {
	  SVX_ASSERT(strstr(msg(msg_extra), "%s") == NULL);
	  puts(msg(msg_extra));
      }
   }

   exit(0);
}

void
cmdline_version(void)
{
   printf("%s - "PRETTYPACKAGE" "VERSION"\n", msg_appname());
}

void
cmdline_syntax(void)
{
   /* TRANSLATORS: as in: Usage: cavern … */
   printf("\n%s: %s", msg(/*Usage*/49), msg_appname());
   /* TRANSLATORS: in command line usage messages e.g. Usage: cavern [OPTION]… */
   if (help && help->opt) printf(" [%s]...", msg(/*OPTION*/153));
   if (msg_args) {
      putchar(' ');
      puts(msg(msg_args));
      return;
   }
   if (min_args) {
      int i = min_args;
      while (i--) printf(" %s", msg(/*FILE*/124));
   }
   if (max_args == -1) {
      if (!min_args) printf(" [%s]", msg(/*FILE*/124));
      fputs("...", stdout);
   } else if (max_args > min_args) {
      int i = max_args - min_args;
      while (i--) printf(" [%s]", msg(/*FILE*/124));
   }
   putnl();
}

static void
syntax_and_help_pointer(void)
{
   cmdline_syntax();
   fprintf(stderr, msg(/*Try “%s --help” for more information.\n*/157),
	   msg_appname());
   exit(1);
}

static void
moan_and_die(int msgno)
{
   fprintf(stderr, "%s: ", msg_appname());
   fprintf(stderr, msg(msgno), optarg);
   fputnl(stderr);
   cmdline_syntax();
   exit(1);
}

void
cmdline_too_few_args(void)
{
   fprintf(stderr, "%s: %s\n", msg_appname(), msg(/*too few arguments*/122));
   syntax_and_help_pointer();
}

void
cmdline_too_many_args(void)
{
   fprintf(stderr, "%s: %s\n", msg_appname(), msg(/*too many arguments*/123));
   syntax_and_help_pointer();
}

void
cmdline_set_syntax_message(int msg_args_, int msg_extra_, const char * arg)
{
   msg_args = msg_args_;
   msg_extra = msg_extra_;
   msg_extra_arg = arg;
}

int
cmdline_int_arg(void)
{
   long result;
   char *endptr;

   errno = 0;

   result = strtol(optarg, &endptr, 10);

   if (errno == ERANGE || result > INT_MAX || result < INT_MIN) {
      moan_and_die(/*numeric argument “%s” out of range*/185);
   } else if (*optarg == '\0' || *endptr != '\0') {
      moan_and_die(/*argument “%s” not an integer*/186);
   }

   return (int)result;
}

double
cmdline_double_arg(void)
{
   double result;
   char *endptr;

   errno = 0;

   result = strtod(optarg, &endptr);

   if (errno == ERANGE) {
      moan_and_die(/*numeric argument “%s” out of range*/185);
   } else if (*optarg == '\0' || *endptr != '\0') {
      moan_and_die(/*argument “%s” not a number*/187);
   }

   return result;
}

void
cmdline_init(int argc_, char *const *argv_, const char *shortopts_,
	     const struct option *longopts_, int *longind_,
	     const struct help_msg *help_,
	     int min_args_, int max_args_)
{
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

   switch (opt) {
    case EOF:
      /* check valid # of args given - if not give syntax message */
      if (argc - optind < min_args) {
	 cmdline_too_few_args();
      } else if (max_args >= 0 && argc - optind > max_args) {
	 cmdline_too_many_args();
      }
      break;
    case ':': /* parameter missing */
    case '?': /* unknown opt, ambiguous match, or extraneous param */
      /* getopt displays a message for us */
      syntax_and_help_pointer();
      break;
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
