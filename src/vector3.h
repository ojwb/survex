//  vector3.h
//
//  C++ class for 3-element vectors
//
//  Copyright (C) 2000-2002, Mark R. Shinwell.
//  Copyright (C) 2002-2004,2005,2006,2015 Olly Betts
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

#ifndef Vector3_h
#define Vector3_h

#include <math.h>

class Vector3 {
protected:
    double x, y, z;

public:
    Vector3() : x(0.0), y(0.0), z(0.0) { }
    Vector3(double a, double b, double c) : x(a), y(b), z(c) { }

    double GetX() const { return x; }
    double GetY() const { return y; }
    double GetZ() const { return z; }

    double magnitude() const {
	return sqrt(x*x + y*y + z*z);
    }

    // Returns a value in *radians*.
    double gradient() const {
	return atan2(z, sqrt(x*x + y*y));
    }

    void normalise();

    void assign(double a, double b, double c) {
	x = a; y = b; z = c;
    }

    void assign(const Vector3 &v) {
	*this = v;
    }

    friend Vector3 operator-(const Vector3& o) {
	return Vector3(-o.x, -o.y, -o.z);
    }
    Vector3& operator*=(double);
    Vector3& operator/=(double);
    Vector3& operator+=(const Vector3&);
    Vector3& operator-=(const Vector3&);

    friend Vector3 operator*(double, const Vector3&);
    friend Vector3 operator*(const Vector3& v, double f) {
	return f * v;
    }
    friend Vector3 operator*(const Vector3&, const Vector3&); // cross product
    friend Vector3 operator+(const Vector3&, const Vector3&);
    friend Vector3 operator-(const Vector3&, const Vector3&);
    friend bool operator==(const Vector3&, const Vector3&);
    friend bool operator<(const Vector3&, const Vector3&);
    friend double dot(const Vector3&, const Vector3&);
};

inline bool operator==(const Vector3& a, const Vector3& b) {
    return a.x == b.x && a.y == b.y && a.z == b.z;
}

// So we can use Vector3 as a key in a map...
inline bool operator<(const Vector3& a, const Vector3& b) {
    if (a.x != b.x) return a.x < b.x;
    if (a.y != b.y) return a.y < b.y;
    return a.z < b.z;
}

#endif
