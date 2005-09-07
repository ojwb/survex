//
//  vector3.h
//
//  C++ class for 3-element vectors
//
//  Copyright (C) 2000-2002, Mark R. Shinwell.
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

#ifndef Vector3_h
#define Vector3_h

#include <stdio.h>
#include <math.h>

class Vector3 {
    double x; //--FIXME
    double y;
    double z;

public:
    Vector3();
    Vector3(double, double, double);
    ~Vector3();

    double getX() const { return x; }
    double getY() const { return y; }
    double getZ() const { return z; }

    double magnitude() const {
	return sqrt(x*x + y*y + z*z);
    }

    void normalise();

    void set(double, double, double);

    friend Vector3 operator-(const Vector3& v) {
	Vector3 o;
	o.x = -v.x;
	o.y = -v.y;
	o.z = -v.z;
	return o;
    }
    Vector3& operator*=(const double);
    Vector3& operator/=(const double);
    Vector3& operator=(const Vector3&);

    friend Vector3 operator*(const double, const Vector3&);
    friend Vector3 operator*(const Vector3& v, const double f) {
	return f * v;
    }
    friend Vector3 operator*(const Vector3&, const Vector3&); // cross product
    friend Vector3 operator+(const Vector3&, const Vector3&);
    friend Vector3 operator-(const Vector3&, const Vector3&);
    friend double dot(const Vector3&, const Vector3&);
};

#endif
