#pragma once
#include "type/Vector2.h"
#include "type/Vector3.h"

struct Easing {
	float time;
	float maxTime;
	float incrementTime_;
	float amplitude;
	float period;
};

float LerpE(const float& start, const float& end, float t);

Vector3 LerpE(const Vector3& start, const Vector3& end, float t);

Vector2 LerpE(const Vector2& start, const Vector2& end, float t);

Vector3 SLerp(const Vector3& start, const Vector3& end, float t);

float EaseInElasticAmplitude(float t, const float& totaltime, const float& amplitude, const float& period);

float EaseOutElasticAmplitude(float t, float totaltime, float amplitude, float period);

float EaseInOutElasticAmplitude(float t, float totaltime, float amplitude, float period);

template<typename T> T EaseAmplitudeScale(const T& initScale, const float& easeT, const float& totalTime, const float& amplitude, const float& period);

template<typename T> T EaseInSine(const T& start, const T& end, float x, float totalX);

template<typename T> T EaseOutSine(const T& start, const T& end, float x, float totalX);

template<typename T> T EaseInOutSine(const T& start, const T& end, float x, float totalX);
template<typename T> T EaseInBack(const T& start, const T& end, float x, float totalX);

template<typename T> T EaseOutBack(const T& start, const T& end, float x, float totalX);

template<typename T> T EaseInOutBack(const T& start, const T& end, float x, float totalX);

template<typename T> T EaseInQuint(const T& start, const T& end, float x, float totalX);

template<typename T> T EaseOutQuint(const T& start, const T& end, float x, float totalX);

template<typename T> T EaseInOutQuint(const T& start, const T& end, float x, float totalX);

template<typename T> T EaseInCirc(const T& start, const T& end, float x, float totalX);

template<typename T> T EaseOutCirc(const T& start, const T& end, float x, float totalX);

template<typename T> T EaseInOutCirc(const T& start, const T& end, float x, float totalX);

template<typename T> T EaseInExpo(const T& start, const T& end, float x, float totalX);

template<typename T> T EaseOutExpo(const T& start, const T& end, float x, float totalX);

template<typename T> T EaseInOutExpo(const T& start, const T& end, float x, float totalX);

template<typename T> T EaseOutCubic(const T& start, const T& end, float x, float totalx);

template<typename T> T EaseInCubic(const T& start, const T& end, float x, float totalx);

template<typename T> T EaseInOutCubic(const T& start, const T& end, float x, float totalx);

template<typename T> T EaseInQuad(const T& start, const T& end, float x, float totalx);

template<typename T> T EaseOutQuad(const T& start, const T& end, float x, float totalx);

template<typename T> T EaseInOutQuad(const T& start, const T& end, float x, float totalx);

template<typename T> T EaseInQuart(const T& start, const T& end, float x, float totalx);

template<typename T> T EaseOutQuart(const T& start, const T& end, float x, float totalx);

float BounceEaseOut(float x);
template<typename T> T EaseInBounce(const T& start, const T& end, float x, float totalX);
template<typename T> T EaseOutBounce(const T& start, const T& end, float x, float totalX);

template<typename T> T EaseInOutBounce(const T& start, const T& end, float x, float totalX);
template<typename T>
T EaseInElastic(const T& start, const T& end, float x, float totalX);
template<typename T>
T EaseOutElastic(const T& start, const T& end, float x, float totalX);

template<typename T>
T EaseInOutElastic(const T& start, const T& end, float x, float totalX);
