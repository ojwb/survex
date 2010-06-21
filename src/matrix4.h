//
//  matrix4.h
//
//  C++ class for handling 4x4 matrices
//
//  Copyright (C) 2000-2001, Mark R. Shinwell.
//  Copyright (C) 2002 Olly Betts
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
//  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
//

#ifndef Matrix4_h
#define Matrix4_h

#include "vector3.h"

class Matrix4 {
    double data[16];

public:
    Matrix4();
    ~Matrix4();

    void setRow(int row, double a, double b, double c, double d);
    Matrix4 transpose() const;
    double get(int r, int c) const { return data[r*4 + c]; }

#if 0
    void print() const;
#endif

    friend Matrix4 operator*(const Matrix4& left, const Matrix4& right);
};

#endif
