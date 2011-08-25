// Copyright 2006 Dietrich Epp <depp@zdome.net>
// $Id: vector.h 51 2006-08-16 15:32:33Z depp $
#ifndef VECTOR_H
#define VECTOR_H
#include <math.h>
namespace sparks {

class vector {
	public:
		float v[2];
		vector() {
			v[0] = 0.0f;
			v[1] = 0.0f;
		}
		vector(float a, float b) {
			v[0] = a;
			v[1] = b;
		}
		
		vector operator+(const vector& a) const {
			return vector(v[0] + a.v[0], v[1] + a.v[1]);
		}
		vector operator-(const vector& a) const {
			return vector(v[0] - a.v[0], v[1] - a.v[1]);
		}
		vector operator-() const {
			return vector(-v[0], -v[1]);
		}
		vector operator*(const float a) const {
			return vector(v[0]*a, v[1]*a);
		}
		vector operator/(const float a) const {
			return vector(v[0]/a, v[1]/a);
		}
		vector& operator= (const vector& a) {
			v[0] = a.v[0];
			v[1] = a.v[1];
			return *this;
		}
		vector& operator*= (const float a) {
			v[0] *= a;
			v[1] *= a;
			return *this;
		}
		vector& operator/= (const float a) {
			v[0] /= a;
			v[1] /= a;
			return *this;
		}
		vector& operator+= (const vector& a) {
			v[0] += a.v[0];
			v[1] += a.v[1];
			return *this;
		}
		vector& operator-= (const vector& a) {
			v[0] -= a.v[0];
			v[1] -= a.v[1];
			return *this;
		}
		float squared() const {
			return v[0] * v[0] + v[1] * v[1];
		}
		vector& normalize() {
			*this /= sqrtf(squared());
			return *this;
		}
		vector normalized() const {
			return *this / sqrtf(squared());
		}
};

} // namespace sparks
#endif
