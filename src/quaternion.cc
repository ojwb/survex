//
//  quaternion.cxx
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

#include "matrix4.h"
#include "quaternion.h"

#include <stdio.h>
#include <math.h>
#include <assert.h>

Quaternion::Quaternion() : w(0.0)
{
    // no further action
}

Quaternion::Quaternion(Vector3 v, double rotation_amount)
{
    setFromVectorAndAngle(v, rotation_amount);
}

Quaternion::Quaternion(double pan, double tilt, double rotation_amount)
{
    setFromSphericalPolars(pan, tilt, rotation_amount);
}

Quaternion::~Quaternion()
{
    // no action
}

double Quaternion::magnitude()
{
    double mv = v.magnitude();
    return sqrt(mv*mv + w*w);
}

void Quaternion::normalise()
{
    double mag = magnitude();

    if (mag != 0.0) {
        w /= mag;
	v /= mag;
    }
}

Quaternion& Quaternion::operator=(const Quaternion& q)
{
    v = q.v;
    w = q.w;

    return *this;
}

Quaternion operator*(const Quaternion& qa, const Quaternion& qb)
{
    Quaternion q;

    q.w = (qa.w * qb.w) - dot(qa.v, qb.v);
    q.v = (qa.w * qb.v) + (qa.v * qb.w) + (qa.v * qb.v);

    q.normalise();

    return q;
}

Vector3 Quaternion::getVector()
{
    return v;
}

double Quaternion::getScalar()
{
    return w;
}

void Quaternion::setVector(const Vector3& v)
{
    this->v = v;
}

void Quaternion::setScalar(const double w)
{
    this->w = w;
}

void Quaternion::setFromEulerAngles(double rotation_x, double rotation_y, double rotation_z)
{
    double cr, cp, cy, sr, sp, sy, cpcy, spsy;

    cr = cos(rotation_x / 2.0);
    cp = cos(rotation_y / 2.0);
    cy = cos(rotation_z / 2.0);

    sr = sin(rotation_x / 2.0);
    sp = sin(rotation_y / 2.0);
    sy = sin(rotation_z / 2.0);

    cpcy = cp * cy;
    spsy = sp * sy;

    w = cr * cpcy + sr * spsy;
    v.set(sr * cpcy - cr * spsy, cr * sp * cy + sr * cp * sy, cr * cp * sy - sr * sp * cy);
    normalise();
}

void Quaternion::setFromSphericalPolars(double pan, double tilt, double rotation_amount)
{
    double sin_rot = sin(rotation_amount);
    double sin_tilt = sin(tilt);

    double vx = sin_rot * cos(tilt) * sin(pan);
    double vy = sin_rot * sin_tilt;
    double vz = sin_rot * sin_tilt * cos(pan);

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

Matrix4 Quaternion::asInverseMatrix()
{
    Quaternion q;
    q.v.set(-v.getX(), -v.getY(), -v.getZ());
    q.w = w;
    return q.asMatrix();
}

/*
void Quaternion::axisAndAngleForInterpolation(const Matrix4& m1, const Matrix4& m2,
					      Vector3& axis, double& angle)
{
    assert(t >= 0.0);
    assert(t <= 1.0);

    Matrix4 tr = m1.transpose();
    Matrix4 m = tr * m2;
    
    m.toAxisAndAngle(axis, angle);
}
*/

Matrix4 Quaternion::asMatrix()
{
    Matrix4 m;

    double xx = v.getX() * v.getX();
    double xy = v.getX() * v.getY();
    double xz = v.getX() * v.getZ();
    double xw = v.getX() * w;

    double yy = v.getY() * v.getY();
    double yz = v.getY() * v.getZ();
    double yw = v.getY() * w;

    double zz = v.getZ() * v.getZ();
    double zw = v.getZ() * w;

    m.setRow(0, 1 - 2*(yy + zz),     2 * (xy - zw),     2 * (xz + yw), 0.0);
    m.setRow(1,     2*(xy + zw), 1 - 2 * (xx + zz),     2 * (yz - xw), 0.0);
    m.setRow(2,     2*(xz - yw),     2 * (yz + xw), 1 - 2 * (xx + yy), 0.0);
    m.setRow(3,             0.0,               0.0,               0.0, 1.0);

    return m;
}
