/* img_hosted.c
 * Build img for use in Survex code
 * Copyright (C) 1997,1999,2000,2001,2011,2013,2014,2015,2016 Olly Betts
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

static const int img_error2msg_map[] = {
    /* IMG_NONE */         0,
    /* IMG_FILENOTFOUND */ /*Couldn’t open file “%s”*/24,
    /* TRANSLATORS: %s will be replaced by the filename that we were trying
     * to read when we ran out of memory.
     */
    /* IMG_OUTOFMEMORY */  /*Out of memory trying to read file “%s”*/38,
    /* IMG_CANTOPENOUT */  /*Failed to open output file “%s”*/47,
    /* TRANSLATORS: Perhaps the user tried to load a different type of file as
     * a Survex .3d file, or the .3d file was corrupted. */
    /* IMG_BADFORMAT */    /*Bad 3d image file “%s”*/106,
    /* IMG_DIRECTORY */    /*Filename “%s” refers to directory*/44,
    /* IMG_READERROR */    /*Error reading from file “%s”*/109,
    /* IMG_WRITEERROR */   /*Error writing to file “%s”*/110,
    /* IMG_TOONEW */       /*File “%s” has a newer format than this program can understand*/114
};

int
img_error2msg(img_errcode err)
{
    int err_int = (int)err;
    if (err_int < 0 || err_int > IMG_TOONEW) return 0;
    return img_error2msg_map[err_int];
}
