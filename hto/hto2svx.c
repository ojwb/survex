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

static void die(char *msg, char *a, int b)
{
  fprintf(stderr, "hto2svx: ");
  fprintf(stderr, msg, a, b);
  fprintf(stderr, "\n");
  exit(1);
}

static void postntcallback(HTO p, char *tag)
{
  p=p; tag=tag;
/*  fprintf(outfd, "nt: %s\n", tag); */
}

static void posttmcallback(HTO p, char *tag)
{
  p=p; tag=tag;
#if 0
  char val[100];
  HTO_GetObjectValue(p, tag, val);
  fprintf(outfd, "te: %s %s\n", tag, val);
#endif
}

static void CPcallback(HTO p, char *tag)
{
  char stn[100], x[32], y[32], z[32];
  tag=tag;
  HTO_GetObjectValue(p, "CPN", stn);
  HTO_GetObjectValue(p, "CPX", x);
  HTO_GetObjectValue(p, "CPY", y);
  HTO_GetObjectValue(p, "CPZ", z);
  fprintf(outfd, "*fix %s %s %s %s\n", stn, x, y, z);
}

static void EQVcallback(HTO p, char *tag)
{
  char a[100], b[100];
  tag=tag;
  HTO_GetObjectValue(p, "CTF", a);
  HTO_GetObjectValue(p, "CTT", b);
  fprintf(outfd, "*equate %s %s\n", a, b);
}

static void CTcallback(HTO p, char *tag)
{
  char from[100], to[100], dist[100], bearing[20], incl[20];
  tag=tag;
  HTO_GetObjectValue(p, "CTF", from);
  HTO_GetObjectValue(p, "CTT", to);
  HTO_GetObjectValue(p, "CTD", dist);
  HTO_GetObjectValue(p, "CTFA", bearing);
  HTO_GetObjectValue(p, "CTFI", incl);
  fprintf(outfd, "%s %s %s %s %s\n", from, to, dist, bearing, incl);
}

static void PFXcallback(HTO p, char *tag)
{
  char prefix[100];
  HTO_GetObjectValue(p, tag, prefix);
  fprintf(outfd, "*prefix %s\n", prefix);
}

int main(int argc, char **argv)
{
  char infn[MAXPATHLEN], outfn[MAXPATHLEN];
/*
  struct stat stb;
*/

  if (argc > 1)
    strcpy(infn, argv[1]);
  else
    infn[0] = 0;
  strcpy(outfn, infn);
  if (argc > 2)
    strcpy(outfn, argv[2]);
/*  if (infn[0])
    {
      if (stat(infn, &stb) < 0)
	{
	  strcat(infn, ".hto");
	  if (stat(infn, &stb) < 0)
	    die("unable to find input file: %s", argv[1], 0);
	}
    }
*/
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

  HTO_RegisterNonTermCallBack (in, HTO_NoCallBack, postntcallback);
  HTO_RegisterTermCallBack    (in, HTO_NoCallBack, posttmcallback);
  HTO_RegisterTagCallBack     (in, "CP", HTO_NoCallBack, CPcallback);
  HTO_RegisterTagCallBack     (in, "CT", HTO_NoCallBack, CTcallback);
  HTO_RegisterTagCallBack     (in, "EQV", HTO_NoCallBack, EQVcallback);
  HTO_RegisterTagCallBack     (in, "PFX", HTO_NoCallBack, PFXcallback);
  HTO_Process(in);
  HTO_Close(in);
  fclose(outfd);
  return 0;
}
