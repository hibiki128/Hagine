#include "Vector3.h"

Vector3 Vector3::operator+(const Vector3& obj) {
	Vector3 v3;
	v3.x = this->x + obj.x;
	v3.y = this->y + obj.y;
	v3.z = this->z + obj.z;
	return v3;
}

Vector3 Vector3::operator-(const Vector3& obj) {
	Vector3 v3;
	v3.x = this->x - obj.x;
	v3.y = this->y - obj.y;
	v3.z = this->z - obj.z;
	return v3;
}

Vector3 Vector3::operator*(const Vector3& obj) {
	Vector3 v3;
	v3.x = this->x * obj.x;
	v3.y = this->y * obj.y;
	v3.z = this->z * obj.z;
	return v3;
}

Vector3 Vector3::operator*(const float& scalar) {
	Vector3 v3;
	v3.x = this->x * scalar;
	v3.y = this->y * scalar;
	v3.z = this->z * scalar;
	return v3;
}


Vector3& Vector3::operator+=(const Vector3& obj) {
	this->x += obj.x;
	this->y += obj.y;
	this->z += obj.z;
	return *this;
}

float Vector3::Dot(const Vector3& v1, const Vector3& v2) { return {v1.x * v2.x + v1.y * v2.y + v1.z * v2.z}; }

float Vector3::Length(const Vector3& v) {
	return {
	    sqrtf(v.x * v.x + v.y * v.y + v.z * v.z),
	};
}

Vector3 Vector3::Normalize(const Vector3& v) { return {v.x / Length(v), v.y / Length(v), v.z / Length(v)}; }

Vector3 Vector3::Cross(const Vector3& v1, const Vector3& v2) { return {v1.y * v2.z - v1.z * v2.y, v1.z * v2.x - v1.x * v2.z, v1.x * v2.y - v1.y * v2.x}; };
