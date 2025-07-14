#pragma once
#include "Model/ModelStructs.h"
#include <map>
#include <string>
#include <type/Quaternion.h>
#include <type/Vector3.h>
#include <unordered_map>
#include <vector>

// アニメーション補間の状態を管理する構造体
struct AnimationBlendState {
    Animation fromAnimation;        // 補間元のアニメーション
    Animation toAnimation;          // 補間先のアニメーション
    float blendFactor = 0.0f;       // 補間係数 (0.0 ~ 1.0)
    float blendDuration = 0.5f;     // 補間にかける時間
    float blendTimer = 0.0f;        // 補間の経過時間
    bool isBlending = false;        // 補間中かどうか
    float fromAnimationTime = 0.0f; // 補間元のアニメーション時間
    float toAnimationTime = 0.0f;   // 補間先のアニメーション時間
    std::string toDirectoryPath;
    std::string toFilename;
};

class Animator {
  private:
    std::string filename_;
    std::string directorypath_;
    bool haveAnimation = false;
    float animationTime = 0.0f;
    Animation currentAnimation_;
    AnimationBlendState blendState_;
    bool isAnimation_ = true;
    bool isFinish_ = false;
    static std::unordered_map<std::string, Animation> animationCache;

  public:
    void Initialize(const std::string &directorypath, const std::string &filename);

    /// <summary>
    /// アニメーション更新（補間対応）
    /// </summary>
    void Update(bool loop);

    /// <summary>
    /// アニメーション切り替え（補間付き）
    /// </summary>
    /// <param name="newAnimation">切り替え先のアニメーション</param>
    /// <param name="blendDuration">補間時間（秒）</param>
    void BlendToAnimation(const Animation &newAnimation, float blendDuration = 0.5f);

    /// <summary>
    /// アニメーション切り替え（ファイルパス指定）
    /// </summary>
    /// <param name="directoryPath">ディレクトリパス</param>
    /// <param name="filename">アニメーションファイル名</param>
    /// <param name="blendDuration">補間時間（秒）</param>
    void BlendToAnimation(const std::string &directoryPath, const std::string &filename, float blendDuration = 0.5f);

    /// <summary>
    /// 補間中かどうかを取得
    /// </summary>
    bool IsBlending() const { return blendState_.isBlending; }

    /// <summary>
    /// 現在のアニメーションデータを取得（補間済み）
    /// </summary>
    Animation GetCurrentAnimation() const;

    void UpdateCurrentFileInfo(const std::string &directoryPath, const std::string &filename);

    /// <summary>
    /// 補間されたノードアニメーションを取得
    /// </summary>
    std::map<std::string, NodeAnimation> GetBlendedNodeAnimations() const;

    // Getter/Setter
    Animation GetAnimation() const { return currentAnimation_; }
    void SetIsAnimation(bool isAnimation) { isAnimation_ = isAnimation; }
    void SetAnimationTime(float time) { animationTime = time; }
    bool HaveAnimation() const { return haveAnimation; }
    bool IsFinish() const { return isFinish_; }
    float GetAnimationTime() const { return animationTime; }
    std::string GetCurrentFilename() const { return filename_; }
    std::string GetCurrentDirectoryPath() const { return directorypath_; }

     /// <summary>
    /// 値の計算(Vector3)
    /// </summary>
    static Vector3 CalculateValue(const std::vector<KeyframeVector3> &keyframes, float time);

    /// <summary>
    /// 値の計算(Quaternion)
    /// </summary>
    static Quaternion CalculateValue(const std::vector<KeyframeQuaternion> &keyframes, float time);

    // アニメーションが終了したかどうかを返す
    bool IsFinished() const {
        return isFinish_;
    }

    // アニメーションが再生中かどうかを返す
    bool IsPlaying() const {
        return isAnimation_;
    }

  private:
    /// <summary>
    /// アニメーションファイル読み込み
    /// </summary>
    Animation LoadAnimationFile(const std::string &directoryPath, const std::string &filename);

    /// <summary>
    /// 補間更新処理
    /// </summary>
    void UpdateBlend(bool loop);

    /// <summary>
    /// 通常のアニメーション更新処理
    /// </summary>
    void UpdateSingle(bool loop);

    /// <summary>
    /// 補間された値を計算（Vector3用）
    /// </summary>
    Vector3 CalculateBlendedValue(
        const std::vector<KeyframeVector3> &fromKeyframes,
        const std::vector<KeyframeVector3> &toKeyframes,
        float fromTime, float toTime, float blendFactor) const;

    /// <summary>
    /// 補間された値を計算（Quaternion用）
    /// </summary>
    Quaternion CalculateBlendedValue(
        const std::vector<KeyframeQuaternion> &fromKeyframes,
        const std::vector<KeyframeQuaternion> &toKeyframes,
        float fromTime, float toTime, float blendFactor) const;
};
