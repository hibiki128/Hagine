#pragma once
#include "ModelStructs.h"
#include <Quaternion.h>
#include <Vector3.h>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>
class Animator {
  private:
    std::string filename_;
    std::string directorypath_;
    bool haveAnimation = false;
    float animationTime = 0.0f;
    Animation animation_;
    bool isRoop_;
    bool isAnimation_ = true;
    bool isFinish_ = false;
    static std::unordered_map<std::string, Animation> animationCache;

  public:
    void Initialize(const std::string &directorypath, const std::string &filename);

    void Update(bool roop);

    bool HaveAnimation() { return haveAnimation; }
    Animation GetAnimation() { return animation_; }
    float GetAnimationTime() { return animationTime; }
    void SetAnimationTime(float time) { animationTime = time; }
    void SetIsAnimation(bool isAnimation) { isAnimation_ = isAnimation; }
    bool IsFinish() { return isFinish_; }

    /// <summary>
    /// アニメーションファイル読み込み
    /// </summary>
    /// <param name="directoryPath"></param>
    /// <param name="filename"></param>
    /// <returns></returns>
    Animation LoadAnimationFile(const std::string &directoryPath, const std::string &filename);

    /// <summary>
    /// 値の計算(Vector3)
    /// </summary>
    /// <param name="keyframes"></param>
    /// <param name="time"></param>
    /// <returns></returns>
    static Vector3 CalculateValue(const std::vector<KeyframeVector3> &keyframes, float time);

    /// <summary>
    /// 値の計算(Quaternion)
    /// </summary>
    /// <param name="keyframes"></param>
    /// <param name="time"></param>
    /// <returns></returns>
    static Quaternion CalculateValue(const std::vector<KeyframeQuaternion> &keyframes, float time);
};
