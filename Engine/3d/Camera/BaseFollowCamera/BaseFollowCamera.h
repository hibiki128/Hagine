#pragma once
#include "Camera/ViewProjection/ViewProjection.h"
#include "Transform/WorldTransform.h"

/// <summary>
/// 基本追従カメラクラス
/// ターゲットを追従するカメラの基本機能を提供する
/// </summary>
class BaseFollowCamera {
  public:
    /// ===================================================
    /// public method
    /// ===================================================

    /// <summary>
    /// 初期化
    /// </summary>
    void Init();

    /// <summary>
    /// 更新処理
    /// </summary>
    void Update();

    /// <summary>
    /// ImGui表示
    /// </summary>
    void imgui();

    /// <summary>
    /// Getter
    /// </summary>
    float GetYaw() { return yaw_; }
    ViewProjection &GetViewProjection() { return viewProjection_; }

    /// <summary>
    /// Setter
    /// </summary>
    void SetTarget(const WorldTransform *target) { target_ = target; }

  private:
    /// ===================================================
    /// private method
    /// ===================================================

    /// <summary>
    /// 移動処理
    /// </summary>
    void Move();

  private:
    /// ===================================================
    /// private varians
    /// ===================================================

    ViewProjection viewProjection_;          // ビュープロジェクション
    WorldTransform worldTransform_;          // ワールド変換
    const WorldTransform *target_ = nullptr; // 追従対象
    float yaw_;                              // ヨー角
    float distanceFromTarget_;               // ターゲットからの距離
    float heightOffset_;                     // 高さオフセット
};