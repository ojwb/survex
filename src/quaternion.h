//
//  quaternion.h
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

#ifndef Quaternion_h
#define Quaternion_h

#include "vector3.h"
#include "matrix4.h"

class Quaternion {
    double w;
    Vector3 v;

public:
    Quaternion();
    Quaternion(double pan, double tilt, double rotation_amount);
    Quaternion(Vector3 v, double rotation_amount);

    ~Quaternion();

    double magnitude();
    void normalise();

    Matrix4 asMatrix();
    Matrix4 asInverseMatrix();

    Quaternion& operator=(const Quaternion&);
    friend Quaternion operator*(const Quaternion&, const Quaternion&);

    Vector3 getVector();
    double getScalar();

    void setVector(const Vector3&);
    void setScalar(const double);

    void setFromEulerAngles(double rotation_x, double rotation_y, double rotation_z);
    void setFromSphericalPolars(double pan, double tilt, double rotation_amount);
    void setFromVectorAndAngle(Vector3 v, double rotation_amount);

    //    static void axisAndAngleForInterpolation(const RotMatrix& start, const RotMatrix& end,
    //                                             Vector3& axis_return, double* angle) const;
};

#endif
