/* > new3dout.c
 * .3dx writing routines
 * Copyright (C) 2000 Phil Underwood
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

/* majority of new file interface... */
#include <config.h>
#include "math.h"

#include "new3dout.h"
#ifdef NEW3DFORMAT
twig *rhizome, *limb;
char *firstfilename, *startingdir, *basesource;
char fUseNewFormat=0;
static int statcount=1; /* this is the total number of things added */

int scount(twig *twiglet) {
  twig *lib;

  if (twiglet->from) {
    return 1;
  } else {
    if (twiglet->to) {
      lib = twiglet->down;
      twiglet->count = 0;
      while (lib->right) {
	lib = lib->right;
	twiglet->count += scount(lib);
      }
      return ((twiglet->count)?1:0);
    } else {
      /* errr no - this should happen for fixed points... */
      printf("This should not happen. Ever\n");
      return 1;
    } /* we should never reach above function - it's just here to  *
       * cheer up the compiler*/
  }
}


void cave_write_title (const char *title, img *pimg)
{
  putc(TITLE_3D,pimg->fh);
  putc((uchar)strlen(title),pimg->fh);
  fputs(title,pimg->fh);
}

void cave_write_stn(node *nod)
{
}



void
cave_write_pos(pos *pid,prefix *pre)
{
uchar length;
uchar ltag;
char *tag;
  if (pid->id == 0) {
    pid->id = (w32)statcount;
    tag = pre->ident;
    ltag = strlen(tag);
    length = ltag + 12 + 4 + 1;
    /* ltag for station name, 12 for data, 4 for id, 1 for length of name */
    putc(STATION_3D,pimgOut->fh);
    putc(length,pimgOut->fh);
    put32((long)statcount, pimgOut->fh); /*station ID*/
    put32((long)((pid->p[0])*100.0), pimgOut->fh); /*X in cm*/
    put32((long)((pid->p[1])*100.0), pimgOut->fh); /*Y*/
    put32((long)((pid->p[2])*100.0), pimgOut->fh); /*Z*/
    putc(ltag,pimgOut->fh);/*length of name*/
    fputs(tag,pimgOut->fh);
    statcount++;
  } else {     /* we've already put this in the file, so just a link is needed*/
    putc(STATLINK_3D,pimgOut->fh);
    putc(0x04,pimgOut->fh);
    put32((long)(pid->id),pimgOut->fh);
  }
}

img *cave_open_write(const char *fnm,const char *title)
{
  /*   time_t tm; */
   img *pimg;
   int dummy=0;
   if (fDirectory(fnm)) {
     /*      img_errno = IMG_DIRECTORY;*/
      return NULL;
   }

   pimg = (img *)xosmalloc(ossizeof(img));
   if (pimg == NULL) {
     /* img_errno = IMG_OUTOFMEMORY; */
      return NULL;
   }

   pimg->fh = fopen(fnm, "wb");
   if (!pimg->fh) {
      osfree(pimg);
      /* img_errno = IMG_CANTOPENOUT; */
      return NULL;
   }
   fputs("SVX3d", pimg->fh);
   putc(0x00, pimg->fh); /* Major version 0 */
   putc(0x0A, pimg->fh); /* Minor version 10*/
   put32((long)dummy,pimg->fh); /*dummy number for num of stations: fill later */
   cave_write_title(title,pimg);
   return pimg;
}

int cave_error(void)
{
printf("A cave error has occured. Hmmm\n");
return 0;
}

void cave_write_leg(linkfor *leg)
{
}

void
cave_write_source(const char *source)
{
  /* create a relative path, given an absolute dir, and an absolute file */
  /* part 1. find where they differ */
  /* temp bodge */
  if (fAbsoluteFnm(source)) {
    /* strip source down to just a file name... */
    if (strncmp(source,basesource,strlen(basesource))!=0) {
      printf("Warning: source links may not work\n");
    } else {
      source = &source[strlen(basesource)];
    }
  }
  putc(SOURCE_3D,pimgOut->fh);
  putc((char)strlen(source),pimgOut->fh);
  fputs(source,pimgOut->fh);
}

void save3d(twig *sticky)
{
  char ltag;
  short unsigned int stubcount;
  double error, length, offset;
  twig *twiglet;
  twiglet = sticky->right;
  for(;(twiglet);twiglet = twiglet->right)
    {
      if (twiglet->from) {
	if (!twiglet->to) { /* fixed point */
	  cave_write_pos(twiglet->from->pos,twiglet->from);
	} else { /*leg */
	  /* calculate an average percentage error, based on %age change of polars */
	  error = 0.0;
	  /* offset is how far the _to_ point is from where we thought it would be */
	  offset = hypot(twiglet->to->pos->p[2] - (twiglet->from->pos->p[2] +
						   twiglet->delta[2]),
			 hypot(twiglet->to->pos->p[0] - (twiglet->from->pos->p[0] +
							 twiglet->delta[0]),
			       twiglet->to->pos->p[1] - (twiglet->from->pos->p[1] +
							 twiglet->delta[1])));
	  /* length is our reconstituted expected length */
	  length = hypot(hypot(twiglet->delta[0],twiglet->delta[1]),
			 twiglet->delta[2]);
#ifdef DEBUG
	  if (strcmp(twiglet->from->ident,"86")==0)
	    printf("deltas... %g %g %g length %g offset %g",
		   twiglet->delta[0],
		   twiglet->delta[1],
		   twiglet->delta[2],
		   length,
		   offset);
#endif
	  if (fabs(length)<0.01) {
	    error = 0; /*what is the error for a leg with zero length?
			*Phil -  he say none. */
	  } else {
	    error =10000.0*offset/length;
	  }
	  putc(LEG_3D,pimgOut->fh);
	  put16((short)0x02,pimgOut->fh);
	  putc(0x04,pimgOut->fh);
	  put32((long)(error),pimgOut->fh); /* output error in %*100 */
	  cave_write_pos(twiglet->from->pos,twiglet->from);
	  cave_write_pos(twiglet->to->pos,twiglet->to);
	}
      } else {
	if (twiglet->count) {
	  putc(BRANCH_3D,pimgOut->fh);
	  /* number of records  - legs + values */
	  stubcount = 0;
	  if (twiglet->source) stubcount++;
	  if (twiglet->drawings) stubcount++;
	  if (twiglet->instruments) stubcount++;
	  if (twiglet->tape) stubcount++;
	  if (twiglet->date) stubcount++;
	  stubcount += twiglet->count;
	  put16((unsigned short)stubcount,pimgOut->fh);
	  ltag=(uchar)strlen(twiglet->to->ident);

	  putc(ltag+1,pimgOut->fh);
	  putc(ltag,pimgOut->fh);
	  fputs(twiglet->to->ident,pimgOut->fh);
	  if (twiglet->source) {
	    cave_write_source(twiglet->source);
	  }
	  if (twiglet->date) {
	    putc(DATE_3D,pimgOut->fh);
	    putc((uchar)strlen(twiglet->date),pimgOut->fh);
	    fputs(twiglet->date,pimgOut->fh);
	  }
	  if (twiglet->drawings) {
	    putc(DRAWINGS_3D,pimgOut->fh);
	    putc((uchar)strlen(twiglet->drawings),pimgOut->fh);
	    fputs(twiglet->drawings,pimgOut->fh);
	  }
	  if (twiglet->instruments) {
	    putc(INSTRUMENTS_3D,pimgOut->fh);
	    putc((uchar)strlen(twiglet->instruments),pimgOut->fh);
	    fputs(twiglet->instruments,pimgOut->fh);
	  }
	  if (twiglet->tape) {
	    putc(TAPE_3D,pimgOut->fh);
	    putc((uchar)strlen(twiglet->tape),pimgOut->fh);
	    fputs(twiglet->tape,pimgOut->fh);
	  }
	  save3d(twiglet->down);
	}
      }
    }
}
void
cave_write_base_source()
{
  char *temp, *temp2;
  uchar length;
  /* is it an absolute? */
  if (fAbsoluteFnm(firstfilename)) {
    basesource = path_from_fnm(firstfilename);
  } else {
    temp = use_path(startingdir,firstfilename);
	basesource = path_from_fnm(temp);
	osfree (temp);
  }

  temp = osstrdup(basesource);
  /* remove any ./../. etcs but not today*/
  /* realpath(temp,basesource); */
  putc(BASE_SOURCE_3D,pimgOut->fh);
  length = strlen(basesource);
  putc((uchar)length,pimgOut->fh);
  fputs(basesource,pimgOut->fh);
  /* get the actual file name */

  temp2 = leaf_from_fnm(firstfilename);
  putc(BASE_FILE_3D,pimgOut->fh);
  length = strlen(temp2);
  putc(length,pimgOut->fh);
  fputs(temp2,pimgOut->fh);
  osfree (temp2);
}

void cave_close(img *pimg)
{
  twig *twiglet;
/*lets do the twiglet traverse!*/

  twiglet = rhizome->down;
  cave_write_base_source();
  scount(rhizome);
  save3d(twiglet);
   if (pimg->fh) {
      /* If writing a binary file, write end of data marker */
      putc(END_3D,pimg->fh);
      /* and finally write how many stations there are */
      fseek (pimg->fh,7L,SEEK_SET);
      put32((long)statcount,pimg->fh);
      fclose(pimg->fh);
   }
   /*   memory leak, but it doesn't like this for soem reason */
   osfree(pimg);
}

void create_twig(prefix *pre,const char * fname) {
  twig *twiglet;
  twig *lib;
  lib = get_twig(pre->up); /* get the active twig for parents prefix*/
  twiglet = osnew(twig);
  twiglet->from = NULL;
  twiglet->to = pre;
  twiglet->sourceval=0; /* lowest priority */
  pre->twig_link = twiglet; /* connect both ways */
  twiglet->right = NULL;
  twiglet->drawings = twiglet->date = twiglet->tape = twiglet->instruments =NULL;
  twiglet->source = osstrdup(fname);
  if (lib) {
    twiglet->count = lib->count+1;
    if (lib==limb) /* ie we are changing the global attachment update...*/
      limb = twiglet;
    lib->right = twiglet;
    twiglet->up = lib->up;
  } else {
    twiglet->up = NULL;
  }
  lib = twiglet;
  twiglet = osnew(twig);
  twiglet->to = twiglet->from = NULL;
  twiglet->up = lib;
  twiglet->sourceval = 0;
  twiglet->drawings = twiglet->source = twiglet->instruments \
    = twiglet->date = twiglet->tape = NULL;
  twiglet->down = twiglet->right = NULL;
  twiglet->count=0;
  lib->down = twiglet;
}

twig *get_twig(prefix *pre) {
  twig *temp;
  if (pre && pre->twig_link) {
    temp = pre->twig_link->down;
    if (temp) while (temp->right) temp = temp->right;
    return temp;
  } else {
    return NULL;
  }
}
#endif
