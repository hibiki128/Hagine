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
    Vector3 startRotOffset, endRotOffset; // オイラー角のオフセット（保存用）
    Vector3 startScaleOffset = {0, 0, 0}, endScaleOffset = {0, 0, 0};

    // 基準値（再生開始時の値）
    Vector3 basePos, baseRot, baseScale; // baseRotはオイラー角として保持

    // 動き始めた時の初期位置を記録（リセット用）
    Vector3 initialPos, initialRot, initialScale; // initialRotはオイラー角として保持
    bool hasInitialTransform = false;             // 初期位置が記録されているかのフラグ

    // 実際の開始・終了値（回転・スケール用）
    Quaternion actualStartRot, actualEndRot; // クォータニオンとして計算
    Vector3 actualStartScale, actualEndScale;

    // Catmull-Rom曲線用の制御点（ローカル座標オフセット）
    std::vector<Vector3> controlPoints;
    bool useCatmullRom = false; // イージングとCatmull-Romを切り替え

    EasingType easingType = EasingType::Linear;
    float colliderOnTime = 0.3f;
    float colliderOffTime = 0.6f;

    // 一時的なモーションかどうかのフラグ
    bool isTemporary = false;

    // 元の位置に戻すかどうかのフラグ
    bool returnToOriginal = false;

    // 最初のモーション開始時の位置（コンボ全体の基準位置）
    Vector3 comboStartPos, comboStartRot, comboStartScale;
    bool hasComboStartTransform = false;
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
    void Play(const std::string &jsonName);
    void Stop(const std::string &objectName);
    void StopAll();

    // ステータス確認関数
    MotionStatus GetMotionStatus(const std::string &objectName);
    bool IsPlaying(const std::string &objectName);
    bool IsFinished(const std::string &objectName);
     // 元の位置に戻すフラグ付きでモーションを再生
    bool PlayFromFile(BaseObject *target, const std::string &fileName, bool returnToOriginal = false);

    // PlayFromFileで作成された一時的なモーション名を取得
    std::string GetTemporaryMotionName(BaseObject *target, const std::string &fileName);

    void ResetInitialPosition(const std::string &objectName);
    // コンボの開始位置を設定
    void SetComboStartPosition(BaseObject *target);
    // コンボ終了時に元の位置に戻す
    void ReturnToComboStart(BaseObject *target);

    void ClearComboStartPosition(BaseObject *target);

    void ClearAllComboStartPositions();

   bool IsAttackFinished(BaseObject *target);
    bool IsAttackFinishedWithInterval(BaseObject *target);
    void SetAttackEndInterval(BaseObject *target, float interval = 0.3f);
    void ClearAttackEndInterval(BaseObject *target);

    // 一時的なモーションの状態チェック
    bool IsTemporaryMotionFinished(BaseObject *target, const std::string &fileName);


  private:
    /// ====================================
    /// private variaus
    /// ====================================

    std::unordered_map<BaseObject *, Vector3> comboStartPositions_;
    std::unordered_map<BaseObject *, Vector3> comboStartRotations_;
    std::unordered_map<BaseObject *, Vector3> comboStartScales_;
    std::unordered_map<std::string, Motion> motions_;
    std::unordered_map<BaseObject *, float> attackEndIntervals_;
    static const float ATTACK_END_INTERVAL; // 攻撃終了後のインターバル時間
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

    // 親子関係対応のヘルパー関数
    Matrix4x4 GetParentInverseWorldMatrix(BaseObject *object);
    Vector3 GetLocalControlPointPosition(BaseObject *object, const Vector3 &worldPos);
    Vector3 TransformLocalControlPointToWorld(BaseObject *object, const Vector3 &localPos);
};

