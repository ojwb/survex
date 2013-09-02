/* img_hosted.h
 * Build img for use in Survex code
 * Copyright (C) 2013 Olly Betts
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

#ifndef IMG_HOSTED_H
#define IMG_HOSTED_H

#ifdef __cplusplus
extern "C" {
#endif

#define IMG_API_VERSION 1

#include "img.h"

int img_error2msg(img_errcode err);

#ifdef __cplusplus
}
#endif

#endif /* IMG_HOSTED_H */
