/*		hto2svx.c		*/

/*
1994.04.08 (OJWB) Modified not to use stat
1994.06.21 (OJWB) cured various warnings
1994.10.24 (OJWB) getopt.h unused, so removed
*/

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "hto.h"

#define MAXPATHLEN 256

static FILE *outfd;
static HTO in;

static void die(const char *msg, const char *a, int b)
{
  fprintf(stderr, "hto2svx: ");
  fprintf(stderr, msg, a, b);
  fprintf(stderr, "\n");
  exit(1);
}

int just_had_cts = 0;

static void nt_post(HTO p, const char *tag)
{
  p = p;
  tag = tag;
/*  fprintf(outfd, "nt: %s\n", tag); */
}

static void tm_post(HTO p, const char *tag)
{
  p = p;
  tag = tag;
#if 0
  char val[100];
  HTO_GetObjectValue(p, tag, val);
  fprintf(outfd, "te: %s %s\n", tag, val);
#endif
}

static void CT1_pre(HTO p, const char *tag)
{
  p = p;
  tag = tag;
  fprintf(outfd, "*begin\n");
}
  
static void CT1_post(HTO p, const char *tag)
{
  p = p;
  tag = tag;
  fprintf(outfd, "*end\n");
}

static void CT2_pre(HTO p, const char *tag)
{
  p = p;
  tag = tag;
  fprintf(outfd, "*begin\n");
  fprintf(outfd, "*data normal station newline tape compass clino\n");
  just_had_cts = 0;
}
  
static void CT2_post(HTO p, const char *tag)
{
  p = p;
  tag = tag;
  fprintf(outfd, "*end\n");
}

static void TI_post(HTO p, const char *tag)
{
  char title[100];
  tag = tag;
  HTO_GetObjectValue(p, "TI", title);
  if (title[0]) fprintf(outfd, "*title \"%s\"\n", title);
}

static void CT_post(HTO p, const char *tag)
{
  char from[100], to[100], dist[100], bearing[20], incl[20];
  tag = tag;
  HTO_GetObjectValue(p, "CTF", from);
  HTO_GetObjectValue(p, "CTT", to);
  HTO_GetObjectValue(p, "CTD", dist);
  if (!dist[0]) strcpy(dist, "0");
  HTO_GetObjectValue(p, "CTFA", bearing);
  if (!bearing[0]) strcpy(bearing, "0");
  HTO_GetObjectValue(p, "CTFI", incl);
  if (!incl[0]) strcpy(incl, "0");
  fprintf(outfd, "%s %s %s %s %s\n", from, to, dist, bearing, incl);
}

static void CTS_post(HTO p, const char *tag)
{
  char stn[100];
  tag = tag;
  HTO_GetObjectValue(p, "CTN", stn);
  if (just_had_cts) fprintf(outfd, "\n");
  fprintf(outfd, "%s\n", stn);
  just_had_cts = 1;
}

static void CTV_post(HTO p, const char *tag)
{
  char dist[100], bearing[20], incl[20];
  tag = tag;
  HTO_GetObjectValue(p, "CTD", dist);
  if (!dist[0]) strcpy(dist, "0");
  HTO_GetObjectValue(p, "CTFA", bearing);
  if (!bearing[0]) strcpy(bearing, "0");
  HTO_GetObjectValue(p, "CTFI", incl);
  if (!incl[0]) strcpy(incl, "0");
  fprintf(outfd, " %s %s %s\n", dist, bearing, incl);
  just_had_cts = 0;
}

static void CP_post(HTO p, const char *tag)
{
  char stn[100], x[32], y[32], z[32];
  tag = tag;
  HTO_GetObjectValue(p, "CPN", stn);
  HTO_GetObjectValue(p, "CPX", x);
  if (!x[0]) strcpy(x, "0");
  HTO_GetObjectValue(p, "CPY", y);
  if (!y[0]) strcpy(y, "0");
  HTO_GetObjectValue(p, "CPZ", z);
  if (!z[0]) strcpy(z, "0");
  fprintf(outfd, "*fix %s %s %s %s\n", stn, x, y, z);
}

static void EQV_pre(HTO p, const char *tag)
{
  p = p;
  tag = tag;
  fprintf(outfd, "*equate");
}

static void EQV_post(HTO p, const char *tag)
{
  p = p;
  tag = tag;
  fprintf(outfd, "\n");
}

static void EQN_post(HTO p, const char *tag)
{
  char stn[100];
  tag = tag;
  HTO_GetObjectValue(p, "EQN", stn);
  fprintf(outfd, " %s", stn);
}

static void PFX_post(HTO p, const char *tag)
{
  char prefix[100];
  HTO_GetObjectValue(p, tag, prefix);
  fprintf(outfd, "*prefix %s\n", prefix);
}

int main(int argc, char **argv)
{
  char infn[MAXPATHLEN], outfn[MAXPATHLEN];
#if 0
  struct stat stb;
#endif

  if (argv[1]) {
    if (strcmp(argv[1], "--version") == 0) {
      printf("%s - "PACKAGE" "VERSION"\n", argv[0]);
      exit(0);
    }

    if (strcmp(argv[1], "--help") == 0) {
      printf("%s - "PACKAGE" "VERSION"\n\n"
	     "Syntax: %s [HTO_FILE [SVX_FILE]]\n\n", argv[0], argv[0]);
      fputs(
"HTO_FILE defaults to stdin\n"
"SVX_FILE defaults to HTO_FILE but with extension changed to .svx, or if\n"
"HTO_FILE omitted, to stdout\n"
"SVX_FILE = `-' means write to stdout\n", stdout);
      exit(0);
    }
  }

  if (argc > 3) {
    printf("Syntax: %s [HTO_FILE [SVX_FILE]]\n", argv[0]);
    exit(1);
  }
    
  if (argc > 1)
    strcpy(infn, argv[1]);
  else
    infn[0] = 0;
  strcpy(outfn, infn);
  if (argc > 2)
    strcpy(outfn, argv[2]);
#if 0
  if (infn[0])
    {
      if (stat(infn, &stb) < 0)
	{
	  strcat(infn, ".hto");
	  if (stat(infn, &stb) < 0)
	    die("unable to find input file: %s", argv[1], 0);
	}
    }
#endif
  in = HTO_Open(infn);

  if (in == NULL)
  {
    strcat(infn, ".hto");
    in = HTO_Open(infn);
  }

  if (in == NULL)
    die("unable to open input file: %s", infn, 0);
  outfd = stdout;
  if (outfn[0] && strcmp(outfn, "-"))
    {
      char *p = strrchr(outfn, '.');
      if (p) *p = 0;
      strcat(outfn, ".svx");
      outfd = fopen(outfn, "w");
      if (outfd == NULL)
	die("unable to open output file: %s", outfn, 0);
    }

  HTO_RegisterNonTermCallBack (in, HTO_NoCallBack, nt_post);
  HTO_RegisterTermCallBack    (in, HTO_NoCallBack, tm_post);
  HTO_RegisterTagCallBack     (in, "CT1", CT1_pre, CT1_post);
  HTO_RegisterTagCallBack     (in, "CT2", CT2_pre, CT2_post);
  HTO_RegisterTagCallBack     (in, "TI", HTO_NoCallBack, TI_post);
  HTO_RegisterTagCallBack     (in, "CT", HTO_NoCallBack, CT_post);
  HTO_RegisterTagCallBack     (in, "CTS", HTO_NoCallBack, CTS_post);
  HTO_RegisterTagCallBack     (in, "CTV", HTO_NoCallBack, CTV_post);
  HTO_RegisterTagCallBack     (in, "CP", HTO_NoCallBack, CP_post);
  HTO_RegisterTagCallBack     (in, "EQV", EQV_pre, EQV_post);
  HTO_RegisterTagCallBack     (in, "EQN", HTO_NoCallBack, EQN_post);
  HTO_RegisterTagCallBack     (in, "PFX", HTO_NoCallBack, PFX_post);
  HTO_Process(in);
  HTO_Close(in);
  fclose(outfd);
  return 0;
}
