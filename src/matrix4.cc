//
//  matrix4.cc
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
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "matrix4.h"

#ifdef DEBUG
#include <stdio.h>
#endif
#include <assert.h>

Matrix4::Matrix4()
{
}

Matrix4::~Matrix4()
{
}

void Matrix4::setRow(int row, double a, double b, double c, double d)
{
    assert(row >= 0 && row <= 3);

    double* p = &data[row * 4];
    *p++ = a;
    *p++ = b;
    *p++ = c;
    *p = d;
}

Matrix4 Matrix4::transpose() const
{
    Matrix4 r;
    const double* d = data;

    for (int row = 0; row < 4; row++) {
	for (int col = 0; col < 4; col++) {
	    r.data[row*4 + col] = *d++;
	}
    }

    return r;
}

Matrix4 operator*(const Matrix4& left, const Matrix4& right)
{
    Matrix4 m;

    const double* col_start_r = right.data;
    const double* row_start_l;
    const double* posn_l;
    const double* posn_r;
    double* result;
    double total = 0.0;

    // for a particular column in the right matrix...
    for (int col = 0; col < 4; col++) {
	row_start_l = left.data;

	// find the top of the column in the output matrix for this column...
	result = &m.data[col];

	// and iterate over the rows in the left matrix.
	for (int row = 0; row < 4; row++) {
	    posn_l = row_start_l;
	    posn_r = col_start_r;

	    // for the row in the left matrix, multiply it by the column in the right matrix.
	    for (int elem = 0; elem < 4; elem++) {
		total += (*posn_l++) * (*posn_r);
		posn_r += 4;
	    }

	    // store this value in the correct place in the output matrix.
	    *result = total;
	    result += 4;

	    // reset the total
	    total = 0.0;

	    // get to the start of the next row in the left matrix
	    row_start_l += 4;
	}

	// get to the top of the next column in the right matrix
	col_start_r++;
    }

    return m;
}

#ifdef DEBUG
void Matrix4::print() const
{
    const double* d = data;
    for (int row = 0; row < 4; row++) {
	printf("[ ");
	for (int col = 0; col < 4; col++) {
	    printf("%02.2g ", *d++);
	}
	printf("]\n");
    }
}
#endif
