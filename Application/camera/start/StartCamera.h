#pragma once
#include <camera/projection/ViewProjection.h>
#include <transform/WorldTransform.h>

// 前方宣言
namespace Hagine {
class Input;
}
namespace Hagine {
class GamePad;
}

/// <summary>
/// スタート時のカメラの動きを行うカメラクラス
/// </summary>
class StartCamera
{
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
    /// デバッグ用のImGui表示
    /// </summary>
    void imgui();

    /// <summary>
    /// カメラの入力移動処理
    /// </summary>
    void Move();

    /// ===================================================
    /// Getter
    /// ===================================================

    /// <summary>
    /// ビュープロジェクションを取得
    /// </summary>
    /// <returns>ViewProjection&: ビュープロジェクション参照</returns>
    Hagine::ViewProjection &GetViewProjection() { return vp_; }

    /// <summary>
    /// 演出完了フラグを取得
    /// </summary>
    /// <returns>bool: 完了していればtrue</returns>
    bool IsComplete() const { return isComplete_; }

    /// ===================================================
    /// Setter
    /// ===================================================

    /// <summary>
    /// 目標となるビュープロジェクションを設定
    /// </summary>
    /// <param name="vp">コピー元のビュープロジェクション</param>
    void SetTargetVp(Hagine::ViewProjection &vp)
    {
        targetVp_.matWorld_ = vp.matWorld_;
        targetVp_.matView_ = vp.matView_;
        targetVp_.matProjection_ = vp.matProjection_;
        targetVp_.translation_ = vp.translation_;
        targetVp_.eulerRotation_ = vp.eulerRotation_;
    }

  private:
    /// ===================================================
    /// private method
    /// ===================================================

    /// <summary>
    /// スキップ入力の確認
    /// </summary>
    /// <returns>bool: 入力があればtrue</returns>
    bool CheckSkipInput();

  private:
    /// ===================================================
    /// private variables
    /// ===================================================

    // カメラ設定
    static constexpr float kFarZ = 1100.0f;               ///< 遠面距離
    static constexpr float kInitialHeight = 42.0f;        ///< 初期高さ
    static constexpr float kInitialAngleDegrees = -90.0f; ///< 初期角度

    // 数値定数
    static constexpr float kTimerReset = 0.0f;                         ///< タイマーリセット値
    static constexpr float kMaxBlendValue = 1.0f;                      ///< 最大ブレンド値
    static constexpr float kEasingMaxValue = 1.0f;                     ///< イージング最大値
    static constexpr float kZeroRotation = 0.0f;                       ///< 回転ゼロ値
    static constexpr float kHalfPi = 0.5f * std::numbers::pi_v<float>; ///< PI/2

    // フェーズ定数
    static constexpr int kPhaseEasing1 = 1;  ///< フェーズ1: イージング1
    static constexpr int kPhaseWait1 = 2;    ///< フェーズ2: 待機1
    static constexpr int kPhaseEasing2 = 3;  ///< フェーズ3: イージング2
    static constexpr int kPhaseWait2 = 4;    ///< フェーズ4: 待機2
    static constexpr int kPhaseEasing3 = 5;  ///< フェーズ5: イージング3
    static constexpr int kPhaseWait3 = 6;    ///< フェーズ6: 待機3
    static constexpr int kPhaseComplete = 7; ///< フェーズ7: 完了

    // スキップ時のスピード倍率
    static constexpr float kSkipSpeedMultiplier = 5.0f; ///< スキップ倍率

    Hagine::ViewProjection vp_;       ///< ビュープロジェクション
    Hagine::ViewProjection targetVp_; ///< 目標ビュープロジェクション
    Hagine::WorldTransform wt_;       ///< ワールドトランスフォーム

    float speed_ = 1.5f;                               ///< 回転速度
    float angle_ = 0.0f;                               ///< 現在の角度
    float radius_ = 60.0f;                             ///< 回転半径
    Hagine::Vector3 centerPos_ = {0.0f, 0.0f, -21.0f}; ///< 中心座標

    bool isEasing_ = false;                                                                                                                  ///< イージング中フラグ
    bool isComplete_ = false;                                                                                                                ///< 完了フラグ
    float easingTimer_ = 0.0f;                                                                                                               ///< イージングタイマー
    float easingDuration_ = 2.0f;                                                                                                            ///< イージング時間
    float finalWaitDuration_ = 1.0f;                                                                                                         ///< 最終待機時間
    Hagine::Vector3 easingStartPos_;                                                                                                         ///< イージング開始位置
    Hagine::Vector3 easingStartRot_;                                                                                                         ///< イージング開始回転
    Hagine::Vector3 easingTargetPos_ = {-6.0f, 3.6f, -7.40f};                                                                                ///< 目標位置1
    Hagine::Vector3 easingTargetRot_ = {Hagine::degreesToRadians(8.6f), Hagine::degreesToRadians(40.0f), Hagine::degreesToRadians(0.0f)};    ///< 目標回転1
    int easingPhase_ = 0;                                                                                                                    ///< 現在のイージングフェーズ
    float waitDuration_ = 1.0f;                                                                                                              ///< フェーズ間の待機時間
    Hagine::Vector3 easingTargetPos2_ = {5.0f, 4.5f, -33.0f};                                                                                ///< 目標位置2
    Hagine::Vector3 easingTargetRot2_ = {Hagine::degreesToRadians(9.6f), Hagine::degreesToRadians(-149.0f), Hagine::degreesToRadians(0.0f)}; ///< 目標回転2

    Hagine::Input *pInput_ = nullptr;                     ///< 入力クラスのポインタ
    std::unique_ptr<Hagine::GamePad> pGamePad_ = nullptr; ///< ゲームパッドのインスタンス

    bool isSkipping_ = false; ///< スキップ実行中フラグ
};
