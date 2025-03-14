/* img_for_survex.h
 * Build img for use in Survex code
 * Copyright (C) 2013,2025 Olly Betts
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

#ifndef IMG_FOR_SURVEX_H
#define IMG_FOR_SURVEX_H

#ifdef __cplusplus
extern "C" {
#endif

#define IMG_API_VERSION 1

#include "img.h"

static inline int img_error2msg(img_errcode err) {
    // We've numbered Survex's message numbers so they match up for IMG_* error
    // codes which means translating is just a cast and a range check.
    //
    // These messages aren't used anywhere else but need to exist in the source
    // code so the message handling machinery can see them and so we can add
    // TRANSLATORS comments.
#if 0
    /* TRANSLATORS: Perhaps the user tried to load a different type of file as
     * a Survex .3d file, or the .3d file was corrupted. */
    (void)msg(/*Bad 3d image file “%s”*/4);
    (void)msg(/*Error reading from file “%s”*/6);
    (void)msg(/*File “%s” has a newer format than this program can understand*/8);
#endif
    int err_int = (int)err;
    if (err_int < 0 || err_int > IMG_TOONEW) return 0;
    return err_int;
}

// Like img_open_survey() but use Survex's filename handling to try adding an
// extension, etc.
img * img_for_survex_open_survey(const char *fnm, const char *survey);

#ifdef __cplusplus
}
#endif

#endif /* IMG_FOR_SURVEX_H */
