//
//  vector3.cc
//
//  C++ class for 3-element vectors
//
//  Copyright (C) 2000-2001, Mark R. Shinwell.
//  Copyright (C) 2002-2003 Olly Betts
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "vector3.h"

#include <math.h>

void Vector3::normalise()
{
    double mag = magnitude();
    if (mag != 0.0) {
	x /= mag;
	y /= mag;
	z /= mag;
    }
}

double dot(const Vector3& left, const Vector3& right)
{
    return left.x*right.x + left.y*right.y + left.z*right.z;
}

Vector3& Vector3::operator*=(const double f)
{
    x *= f;
    y *= f;
    z *= f;

    return *this;
}

Vector3& Vector3::operator/=(const double f)
{
    x /= f;
    y /= f;
    z /= f;

    return *this;
}

Vector3& Vector3::operator+=(const Vector3 &v)
{
    x += v.x;
    y += v.y;
    z += v.z;

    return *this;
}

Vector3& Vector3::operator-=(const Vector3 &v)
{
    x -= v.x;
    y -= v.y;
    z -= v.z;

    return *this;
}

Vector3 operator*(const double f, const Vector3& v)
{
    Vector3 o;
    o.x = v.x * f;
    o.y = v.y * f;
    o.z = v.z * f;

    return o;
}

Vector3 operator*(const Vector3& v1, const Vector3& v2)
{
    // Cross product.

    Vector3 o;
    o.x = v1.y*v2.z - v1.z*v2.y;
    o.y = v1.z*v2.x - v1.x*v2.z;
    o.z = v1.x*v2.y - v1.y*v2.x;

    return o;
}

Vector3 operator+(const Vector3& v1, const Vector3& v2)
{
    Vector3 o;
    o.x = v1.x + v2.x;
    o.y = v1.y + v2.y;
    o.z = v1.z + v2.z;

    return o;
}

Vector3 operator-(const Vector3& v1, const Vector3& v2)
{
    Vector3 o;
    o.x = v1.x - v2.x;
    o.y = v1.y - v2.y;
    o.z = v1.z - v2.z;

    return o;
}
