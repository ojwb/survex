//
//  quaternion.cc
//
//  C++ class for handling quarternions.
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "matrix4.h"
#include "quaternion.h"

#include <math.h>
#include <assert.h>
//#include <stdio.h>

void Quaternion::setFromSphericalPolars(double pan, double tilt, double rotation_amount)
{
    double sin_rot = sin(rotation_amount);

    double vx = sin_rot * cos(tilt) * sin(pan);
    double vy = sin_rot * sin(tilt);
    double vz = vy * cos(pan);

    w = cos(rotation_amount);
    //    printf("w is %.2g\n", w);
    //    printf("v is [%.02g %.02g %.02g]\n", v.getX(), v.getY(), v.getZ());

    v.set(vx, vy, vz);
    normalise();
}

void Quaternion::setFromVectorAndAngle(Vector3 vec, double rotation_amount)
{
    double sin_rot = sin(rotation_amount / 2.0);

    v = vec;
    v *= sin_rot;
    w = cos(rotation_amount / 2.0);
    normalise();
}

#if 0
void Quaternion::axisAndAngleForInterpolation(const Matrix4& m1, const Matrix4& m2,
					      Vector3& axis, double& angle)
{
    assert(t >= 0.0);
    assert(t <= 1.0);

    Matrix4 tr = m1.transpose();
    Matrix4 m = tr * m2;

    m.toAxisAndAngle(axis, angle);
}
#endif
