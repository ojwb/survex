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

#include <math.h>

class Quaternion {
    double w;
    Vector3 v;

public:
    Quaternion() : w(0.0) {}
    Quaternion(double pan, double tilt, double rotation_amount) {
        setFromSphericalPolars(pan, tilt, rotation_amount);
    }
    Quaternion(Vector3 v, double rotation_amount) {
        setFromVectorAndAngle(v, rotation_amount);
    }

    ~Quaternion() {}

    double magnitude() {
        double mv = v.magnitude();
        return sqrt(mv*mv + w*w);
    }

    void normalise() {
        double mag = magnitude();

	if (mag != 0.0) {
	    w /= mag;
	    v /= mag;
	}
    }

    Matrix4 asMatrix() {
        static Matrix4 m;

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

    Matrix4 asInverseMatrix() {
        static Quaternion q;
	q.v.set(-v.getX(), -v.getY(), -v.getZ());
	q.w = w;
	return q.asMatrix();
    }

    Quaternion& operator=(const Quaternion& q) {
        v = q.v;
	w = q.w;

	return *this;
    }

    friend Quaternion operator*(const Quaternion& qa, const Quaternion& qb) {
        static Quaternion q;

	q.w = (qa.w * qb.w) - dot(qa.v, qb.v);
	q.v = (qa.w * qb.v) + (qa.v * qb.w) + (qa.v * qb.v);

	q.normalise();
	
	return q;
    }

    Vector3 getVector() { return v; }
    double getScalar() { return w; }

    void setVector(const Vector3& v) { this->v = v; }
    void setScalar(const double w) { this->w = w; }

    void setFromEulerAngles(double rotation_x, double rotation_y, double rotation_z) {
        static double cr, cp, cy, sr, sp, sy, cpcy, spsy;

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

    void setFromSphericalPolars(double pan, double tilt, double rotation_amount);
    void setFromVectorAndAngle(Vector3 v, double rotation_amount);

    //    static void axisAndAngleForInterpolation(const RotMatrix& start, const RotMatrix& end,
    //                                             Vector3& axis_return, double* angle) const;
};

#endif
