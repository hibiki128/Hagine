#include "Quaternion.h"
#include"numbers"

void Quaternion::SetFromTo(const Vector3& from, const Vector3& to)
{
	Vector3 f = from.Normalize(); // 正規化したfromベクトル
	Vector3 t = to.Normalize();   // 正規化したtoベクトル

	Vector3 cross = f.Cross(t);    // fromとtoのクロス積
	float dot = f.Dot(t);          // fromとtoの内積

	// 回転角をクォータニオンに変換
	w = sqrt((1.0f + dot) * 0.5f); // 回転のスカラー成分
	float s = 0.5f / w;

	x = cross.x * s;
	y = cross.y * s;
	z = cross.z * s;
}

Quaternion Quaternion::FromEulerAngles(const Vector3& eulerAngles)
{
	float pitch = eulerAngles.x * 0.5f;
	float yaw = eulerAngles.y * 0.5f;
	float roll = eulerAngles.z * 0.5f;

	float sinPitch = sinf(pitch);
	float cosPitch = cosf(pitch);
	float sinYaw = sinf(yaw);
	float cosYaw = cosf(yaw);
	float sinRoll = sinf(roll);
	float cosRoll = cosf(roll);

	return Quaternion(
		cosYaw * cosPitch * cosRoll + sinYaw * sinPitch * sinRoll, // w成分
		sinYaw * cosPitch * cosRoll - cosYaw * sinPitch * sinRoll, // x成分
		cosYaw * sinPitch * cosRoll + sinYaw * cosPitch * sinRoll, // y成分
		cosYaw * cosPitch * sinRoll - sinYaw * sinPitch * cosRoll  // z成分
	);
}

Vector3 Quaternion::ToEulerAngles() const
{
	Vector3 angles;

	// ピッチ（X軸）
	float sinPitch = 2.0f * (w * x + y * z);
	float cosPitch = 1.0f - 2.0f * (x * x + y * y);
	angles.x = atan2(sinPitch, cosPitch);

	// ヨー（Y軸）
	float sinYaw = 2.0f * (w * y - z * x);
	angles.y = fabs(sinYaw) >= 1.0f ? copysign(std::numbers::pi_v<float> / 2, sinYaw) : asin(sinYaw); // 特別なケース

	// ロール（Z軸）
	float sinRoll = 2.0f * (w * z + x * y);
	float cosRoll = 1.0f - 2.0f * (y * y + z * z);
	angles.z = atan2(sinRoll, cosRoll);

	return angles;
}

Quaternion Quaternion::Conjugate() const
{
	return Quaternion(-x, -y, -z, w);
}

Quaternion Quaternion::Normalize() const
{
	float length = sqrtf(x * x + y * y + z * z + w * w);
	return Quaternion(x / length, y / length, z / length, w / length);
}

Quaternion Quaternion::FromLookRotation(const Vector3& direction, const Vector3& up)
{
	Vector3 forward = direction.Normalize();
	Vector3 right = up.Cross(forward).Normalize();
	Vector3 newUp = forward.Cross(right);

	// 回転行列の要素からクォータニオンを計算
	float w = sqrtf(1.0f + right.x + newUp.y + forward.z) * 0.5f;
	float x = (newUp.z - forward.y) / (4.0f * w);
	float y = (forward.x - right.z) / (4.0f * w);
	float z = (right.y - newUp.x) / (4.0f * w);

	return Quaternion(w, x, y, z).Normalize();
}

Quaternion Quaternion::operator*(const Quaternion& q) const
{
	return Quaternion(
		w * q.w - x * q.x - y * q.y - z * q.z,                      // スカラー成分
		w * q.x + x * q.w + y * q.z - z * q.y,                      // x成分
		w * q.y - x * q.z + y * q.w + z * q.x,                      // y成分
		w * q.z + x * q.y - y * q.x + z * q.w                       // z成分
	);
}

Quaternion Quaternion::operator+(const Quaternion& other) const
{
	return { x + other.x, y + other.y, z + other.z, w + other.w };
}

Quaternion Quaternion::operator-(const Quaternion& other) const
{
	return { x - other.x, y - other.y, z - other.z, w - other.w };
}

Quaternion Quaternion::operator/(const Quaternion& other) const
{
	Quaternion inverse = other.Inverse();
	return *this * inverse;
}

Quaternion Quaternion::operator*(const float& scalar) const
{
	return Quaternion(x * scalar, y * scalar, z * scalar, w * scalar);
}

Quaternion Quaternion::IdentityQuaternion()
{
	return { 0.0f, 0.0f, 0.0f, 1.0f };
}

float Quaternion::Norm() const
{
	return sqrtf(x * x + y * y + z * z + w * w);
}

float Quaternion::Dot(const Quaternion& other) const
{
	return x * other.x + y * other.y + z * other.z + w * other.w;
}


Quaternion Quaternion::Inverse() const
{
	float normSquared = Norm();
	normSquared *= normSquared;
	if (normSquared == 0.0f) {
		return IdentityQuaternion();
	}
	Quaternion conjugate = Conjugate();
	return { conjugate.x / normSquared, conjugate.y / normSquared, conjugate.z / normSquared, conjugate.w / normSquared };
}

Quaternion Quaternion::Sleap(Quaternion q1, Quaternion q2, float t)
{
	// クォータニオンの内積を計算
	float dot = q1.x * q2.x + q1.y * q2.y + q1.z * q2.z + q1.w * q2.w;

	// ドット積が負の場合、逆の方向に補間するために q2 を反転
	if (dot < 0.0f) {
		q2.x = -q2.x;
		q2.y = -q2.y;
		q2.z = -q2.z;
		q2.w = -q2.w;
		dot = -dot;
	}

	// 補間係数を使った係数の計算
	const float threshold = 0.9995f;
	if (dot > threshold) {
		// ドット積が閾値を超えた場合、線形補間を実行（角度が小さいため）
		Quaternion result = {
			q1.x + t * (q2.x - q1.x),
			q1.y + t * (q2.y - q1.y),
			q1.z + t * (q2.z - q1.z),
			q1.w + t * (q2.w - q1.w)
		};
		return result.Normalize(); // 結果を正規化
	}

	// 角度の計算
	float theta_0 = std::acos(dot);        // θ0 = q1 と q2 間の角度
	float theta = theta_0 * t;             // θ = t に対応する角度

	// 係数の計算
	float sin_theta = std::sin(theta);
	float sin_theta_0 = std::sin(theta_0);

	float s1 = std::cos(theta) - dot * sin_theta / sin_theta_0;
	float s2 = sin_theta / sin_theta_0;

	// 補間結果の計算
	Quaternion result = {
		s1 * q1.x + s2 * q2.x,
		s1 * q1.y + s2 * q2.y,
		s1 * q1.z + s2 * q2.z,
		s1 * q1.w + s2 * q2.w
	};
	return result;
}
