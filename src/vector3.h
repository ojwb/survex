//
//  vector3.h
//
//  C++ class for 3-element vectors
//
//  Copyright (C) 2000-2001, Mark R. Shinwell.
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

class Vector3 {
    double x;
    double y;
    double z;

public:
    Vector3();
    Vector3(double, double, double);
    ~Vector3();

    double getX() { return x; }
    double getY() { return y; }
    double getZ() { return z; }

    double magnitude();
    void normalise();

    void set(double, double, double);

    Vector3& operator*=(const double);
    Vector3& operator/=(const double);
    Vector3& operator=(const Vector3&);

    friend Vector3 operator*(const Vector3&, const double);
    friend Vector3 operator*(const double, const Vector3&);
    friend Vector3 operator*(const Vector3&, const Vector3&);
    friend Vector3 operator+(const Vector3&, const Vector3&);
    friend double dot(const Vector3&, const Vector3&);
};

#endif
