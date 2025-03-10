/* img_hosted.c
 * Build img for use in Survex code
 * Copyright (C) 1997-2025 Olly Betts
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

#include <config.h>

#include "img_hosted.h"

#define IMG_HOSTED 1

#include "img.c"

#include "filename.h"

img *
img_hosted_open_survey(const char *fnm, const char *survey)
{
   if (fDirectory(fnm)) {
      img_errno = IMG_DIRECTORY;
      return NULL;
   }

   char *filename_opened = NULL;
   FILE *fh = fopenWithPthAndExt("", fnm, "3d", "rb", &filename_opened);
#ifdef ENOMEM
   if (!fh && errno == ENOMEM) {
       img_errno = IMG_OUTOFMEMORY;
       return NULL;
   }
#endif
   img *pimg = img_read_stream_survey(fh, fclose,
				      filename_opened ? filename_opened : fnm,
				      survey);
   free(filename_opened);
   return pimg;
}
