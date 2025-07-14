#pragma once
#include "Easing.h"
#include <Object/Base/BaseObject.h>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

enum class EasingType {
    Linear,
    EaseInSine,
    EaseOutSine,
    EaseInOutSine,
    EaseInBack,
    EaseOutBack,
    EaseInOutBack,
    EaseInQuint,
    EaseOutQuint,
    EaseInOutQuint,
    EaseInCirc,
    EaseOutCirc,
    EaseInOutCirc,
    EaseInExpo,
    EaseOutExpo,
    EaseInOutExpo,
    EaseOutCubic,
    EaseInCubic,
    EaseInOutCubic,
    EaseInQuad,
    EaseOutQuad,
    EaseInOutQuad,
    EaseInQuart,
    EaseOutQuart,
    EaseInBounce,
    EaseOutBounce,
    EaseInOutBounce,
    EaseInElastic,
    EaseOutElastic,
    EaseInOutElastic,
};

enum class MotionStatus {
    Stopped,
    Playing,
    Finished
};

struct Motion {
    BaseObject *target = nullptr;
    std::string objectName;
    float totalTime = 1.0f;
    float currentTime = 0.0f;
    MotionStatus status = MotionStatus::Stopped;

    // 既存のオフセット系（ローカル座標系）
    Vector3 startPosOffset, endPosOffset;
    Vector3 startRotOffset, endRotOffset;
    Vector3 startScaleOffset = {0, 0, 0}, endScaleOffset = {0, 0, 0};

    // 基準値（再生開始時の値）
    Vector3 basePos, baseRot, baseScale;

    // 動き始めた時の初期位置を記録（リセット用）
    Vector3 initialPos, initialRot, initialScale;
    bool hasInitialTransform = false; // 初期位置が記録されているかのフラグ

    // 実際の開始・終了値（回転・スケール用）
    Vector3 actualStartRot, actualEndRot;
    Vector3 actualStartScale, actualEndScale;

    // Catmull-Rom曲線用の制御点（ローカル座標オフセット）
    std::vector<Vector3> controlPoints;
    bool useCatmullRom = false; // イージングとCatmull-Romを切り替え

    EasingType easingType = EasingType::Linear;
    float colliderOnTime = 0.3f;
    float colliderOffTime = 0.6f;

    // 一時的なモーションかどうかのフラグ
    bool isTemporary = false;
};

class MotionEditor {
  private:
    /// ====================================
    /// private methods
    /// ====================================
    static MotionEditor *instance;
    MotionEditor() = default;
    ~MotionEditor() = default;
    MotionEditor(MotionEditor &) = delete;
    MotionEditor &operator=(MotionEditor &) = delete;

  public:
    /// ====================================
    /// public methods
    /// ====================================
    static MotionEditor *GetInstance();
    void Finalize();

    // 登録・更新・描画
    void Register(BaseObject *object);
    void Update(float deltaTime);
    void DrawImGui();
    void Save(const std::string &fileName);
    Motion Load(const std::string &fileName);
    void DrawControlPoints();
    void DrawCatmullRomCurve();
    Vector3 CatmullRomInterpolation(const std::vector<Vector3> &points, float t);

    // 再生関連
    void Play(const std::string &objectName);
    bool PlayFromFile(BaseObject *target, const std::string &fileName);
    void Stop(const std::string &objectName);
    void StopAll();

    // ステータス確認関数
    MotionStatus GetMotionStatus(const std::string &objectName);
    bool IsPlaying(const std::string &objectName);
    bool IsFinished(const std::string &objectName);

    // PlayFromFileで作成された一時的なモーション名を取得
    std::string GetTemporaryMotionName(BaseObject *target, const std::string &fileName);

    void ResetInitialPosition(const std::string &objectName);

  private:
    /// ====================================
    /// private variaus
    /// ====================================
    
    std::unordered_map<std::string, Motion> motions_;
    std::string selectedName_;
    std::string jsonName_;
    int selectedControlPoint_ = -1;

  private:
    /// ====================================
    /// private methods
    /// ====================================
    // ヘルパー関数
    Vector3 TransformLocalToWorld(const Vector3 &localOffset, const Matrix4x4 &worldMatrix);

    // 一時モーションのクリーンアップ
    void CleanupFinishedTemporaryMotions();
};