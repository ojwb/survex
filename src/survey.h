//
//  survey.h
//
//  Survey "document" handling
//
//  Copyright (C) 2002 Mark R. Shinwell
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//

#ifndef survey_h
#define survey_h

enum LockFlags {
    lock_NONE = 0,
    lock_X = 1,
    lock_Y = 2,
    lock_Z = 4,
    lock_POINT = lock_X | lock_Y | lock_Z,
    lock_XZ = lock_X | lock_Z,
    lock_YZ = lock_Y | lock_Z,
    lock_XY = lock_X | lock_Y
};

#endif

