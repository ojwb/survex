/* svx2hto.c */

/*
1994.04.08 (OJWB) Modified not to use stat
           tm is a time_t not a long (check)
1994.06.03 (OJWB) errors reported as from svx2hto and not svx3hto
1994.06.21 (OJWB) cured various warnings
1994.07.08 (OJWB) fixed minor problem for BCC compile
1994.08.10 (OJWB) fixed warning of static with extern prototype
1994.09.12 (OJWB) above fix gives error with BC, changed back
1994.10.04 (OJWB) yet another go, as GCC gives warning
1994.11.13 (OJWB) complain if too many command line args
*/

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>
#include <time.h>
#ifdef TM_IN_SYS_TIME
# include <sys/time.h>
#endif
#include <string.h>
#include <stdlib.h>
#include "hto.h"

#define MAXPATHLEN 256
#define HTO_VERSION "1.0"
#define PREFIX

static HTO outfd;
#ifndef PREFIX
static char prefix[256];
#endif
static int marked = 0;
static char otag[16];
static int linect = 0;

static void die(const char *msg, const char *ins1, int ins2)
{
  fprintf(stderr, "svx2hto: ");
  fprintf(stderr, msg, ins1, ins2);
  fprintf(stderr, "\n");
  exit(1);
}

static char *skipblanks(char *p)
{
  while (*p && *p <= ' ')
    p++;
  return p;
}

static void trashNL(char *p)
{
  while (*p)
    {
      if (*p < ' ') *p = ' ';
      p++;
    }
}

static char *sort_prefix(char *p)
{
#ifdef PREFIX
  return p;
#else
  static char buff[256];

  if (*p == '\\')
    return p+1;
  strcpy(buff, prefix);
  if (prefix[0])
    strcat(buff, ".");
  strcat(buff, p);
  return buff;
#endif
}

static void wv(const char *tag, const char *val)
{
  if (marked == 0)
    {
      char lnbuff[16];
      sprintf(lnbuff, "%d", linect);
      HTO_BeginObject(outfd, otag);
      if (linect) HTO_WriteValue(outfd, "LI", lnbuff);
      marked = 1;
    }
  HTO_WriteValue(outfd, tag, val);
}

static void bo(const char *tag, int lno)
{
  strcpy(otag, tag);
  linect = lno;
  marked = 0;
}

static void eo(void)
{
  if (marked)
    HTO_EndObject(outfd);
}

static void process(char *infn);

static void command(char *p, char *fn, int lno)
{
  p++;
  if (strncmp(p, "prefix", 6) == 0)
    {
      p = skipblanks(p+6);
#ifdef PREFIX
      wv("PFX", p);
#else
      if (*p == '\\')
	{
	  p++;
	  strcpy(prefix, p);
	}
      else
	{
	  strcat(prefix, '.');
	  strcat(prefix, p);
	}
      p = strchr(prefix, ' ');
      if (p) *p = 0;
#endif
    }
  else if (strncmp(p, "include", 7) == 0)
    {
      char infn[MAXPATHLEN];
/*      FILE *infd; */

      p = skipblanks(p+7);
      strcpy(infn, p);
      p = strchr(infn, ' ');
      if (p) *p = 0;
      process(infn);
    }
  else if (strncmp(p, "equate", 6) == 0)
    {
      char from[64], to[64], *q;

      p = skipblanks(p+6);
      q = strchr(p, ' ');
      if (q == NULL) die("bad syntax in file %s at line %d", fn, lno);
      *q = 0;
      strcpy(from, p);
      p = skipblanks(q+1);
      q = strchr(p, ' ');
      if (q) *q = 0;
      strcpy(to, p);
      bo("EQV", lno);
      wv("CTF", sort_prefix(from));
      wv("CTT", sort_prefix(to));
      eo();
    }
  else if (strncmp(p, "fix", 3) == 0)
    {
      char stn[64], x[32], y[32], z[32];
      int k = sscanf(p+3, "%s %s %s %s", stn, x, y, z);
      if (k != 4) die("bad syntax in file %s at line %d", fn, lno);
      bo("CP", lno);
      wv("CPN", sort_prefix(stn));
      wv("CPX", x);
      wv("CPY", y);
      wv("CPZ", z);
      eo();
    }
  else
    {
      die("unrecogised command in file %s at line %d", fn, lno);
    }
}

static void data(char *p, char *fn, int lno)
{
  char from[64], to[64], dist[32], bearing[32], incl[32];
  int k = sscanf(p, "%s%s%s%s%s", from, to, dist, bearing, incl);
  if (k != 5)
	 die("incorrect format in data file %s, line %d\n", fn, lno);
  bo("CT", lno);
  wv("CTF", sort_prefix(from));
  wv("CTT", sort_prefix(to));
  wv("CTD", dist);
  wv("CTFA", bearing);
  wv("CTFI", incl);
  eo();
}

static void process(char *infn)
{
  FILE *infd = fopen(infn, "r");
  char buff[256], *p;
  int lineno = 0;

  if (infd == NULL)
    {
      strcpy(buff, infn);
      strcat(buff, ".svx");
      infd = fopen(buff, "r");
    }
  if (infd == NULL)
    die("unable to open input file %s", infn, 0);
  strcpy(buff, infn);
  p = strchr(buff, '.');
  if (p) *p = 0;
  bo("GR", lineno);
  wv("TI", buff);
  while (fgets(buff, 256, infd))
    {
      char lnobuff[32];

      lineno++;
      p = skipblanks(buff);
      if (*p)
	{
	  sprintf(lnobuff, "%d", lineno);
	  trashNL(buff);
	  if (*p == ';')
	    HTO_WriteValue(outfd, "COM", buff+1);
	  else
	    {
	      if (*p == '*')
		command(p, infn, lineno);
	      else
		data(p, infn, lineno);
	    }
	}
    }
  eo();
  fclose(infd);
}

int main(int argc, char **argv)
{
  char infn[MAXPATHLEN], outfn[MAXPATHLEN], buff[32], *p;
  time_t tm;
  struct tm *clk;

  if (argv[1]) {
    if (strcmp(argv[1], "--version") == 0) {
      printf("%s - "PACKAGE" "VERSION"\n", argv[0]);
      exit(0);
    }

    if (strcmp(argv[1], "--help") == 0) {
      printf("%s - "PACKAGE" "VERSION"\n\n"
	     "Syntax: %s SVX_FILE [HTO_FILE]\n\n", argv[0], argv[0]);
      puts("HTO_FILE defaults to SVX_FILE but with extension changed to .hto");
      exit(0);
    }
  }

  if (argc < 2 || argc > 3) {
    printf("Syntax: %s SVX_FILE [HTO_FILE]\n", argv[0]);
    exit(1);
  }
  strcpy(infn, argv[1]);
  if (argc == 2)
    {
      strcpy(outfn, infn);
      p = strrchr(outfn, '.');
      if (p) *p = 0;
      strcat(outfn, ".hto");
    }
  else
    strcpy(outfn, argv[2]);
  outfd = HTO_Create(outfn);
  if (outfd == NULL)
    die("unable to create output file: %s", outfn, 0);
  bo("CT1", 0);
  wv("HTO", HTO_VERSION);
  wv("COUNTRY", "United_Kingdom");
  wv("PROG", "SURVEX");
  tm = time(0);
  clk = localtime(&tm);
  strftime(buff, 16, "%Y%m%d", clk);
  wv("CDATE", buff);
  process(infn);
  HTO_Finish(outfd);
  return 0;
}
