/* > cmdline.c
 * Wrapper for GNU getopt which deals with standard options
 * Copyright (C) 1998 Olly Betts
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "cmdline.h"
#include "filename.h"

/* It might be useful to be able to disable all long options on small
 * platforms like pre-386 DOS and some PDAs...
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

static const char *argv0 = "<program>";

static void
display_help(const struct help_msg *help, const struct option *opts)
{
   while (help->opt) {
      const char *longopt = 0;
      int opt = help->opt;
      const struct option *o = 0;

      if (HLP_ISLONG(opt)) {
	 o = opts + HLP_DECODELONG(opt);
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

static void
display_version(void)
{
   printf("%s - "PACKAGE" "VERSION"\n", argv0);
}

static void
display_syntax(void)
{
   printf("\nSyntax: %s [OPTION]... [FILE]...\n", argv0);
}

extern int
my_getopt_long(int argc, char *const *argv, const char *shortopts,
	       const struct option *longopts, int *longind,
	       const struct help_msg *help, int min_args)
{
   int opt;
   
   argv0 = argv[0]; /* FIXME: tidy up argv0 (remove path and extension) */
   
   opt = getopt_long(argc, argv, shortopts, longopts, longind);

   if (opt == EOF) {
      /* check minimum # of args given - if not give help message */
      if (argc - optind < min_args) opt = HLP_HELP;
   }

   switch (opt) {
    case ':': /* parameter missing */
    case '?': /* unknown opt, ambiguous match, or extraneous param */
      /* getopt displays a message for us (unless we set opterr to 0) */
      display_syntax();
      exit(0);
    case HLP_VERSION: /* --version */
      display_version();
      exit(0);
    case HLP_HELP: /* --help */
      display_version();
      display_syntax();
      putchar('\n');
      display_help(help, longopts);
      exit(0);
   }
   return opt;
}
