#pragma once
#include <functional>
#include <string>

class FollowCamera;
namespace Hagine {
class BaseObject;
}

/// <summary>
/// 必殺技発動前の演出（カメラ顔アップ→通常カメラ復帰→発動遅延）を管理するクラス
/// 予備動作のない必殺技にテレグラフを与え、相手に回避の猶予を作る。
/// 演出中も技の照準追従は呼び出し側で維持し、onActivate の中で固定する想定
/// </summary>
class SkillCutscene
{
  public:
    /// ===================================================
    /// public method
    /// ===================================================

    /// <summary>
    /// 演出を開始する
    /// カメラ顔アップ（closeUpDuration_）→通常カメラへ即復帰→
    /// activationDelay_ 経過後に onActivate を1回呼ぶ
    /// </summary>
    /// <param name="performer">技の使用者（このモデルの顔にカメラが寄る）</param>
    /// <param name="camera">演出に使うフォローカメラ（nullptr なら遅延のみ）</param>
    /// <param name="onActivate">発動処理コールバック</param>
    void Start(Hagine::BaseObject *performer, FollowCamera *camera, std::function<void()> onActivate);

    /// <summary>
    /// 更新処理（所有者の Update から毎フレーム呼ぶ）
    /// </summary>
    /// <param name="dt">デルタタイム</param>
    void Update(float dt);

    /// <summary>
    /// 演出を中断する（カメラを通常へ戻し、発動コールバックは破棄する）
    /// </summary>
    void Cancel();

    /// <summary>
    /// 演出中（顔アップ or 発動待ち）かどうか
    /// </summary>
    /// <returns>bool: 演出中なら true</returns>
    bool IsActive() const { return phase_ != Phase::Idle; }

    /// <summary>
    /// 調整パラメータをゲームパラメータHubへ登録する
    /// </summary>
    /// <param name="owner">Hub上の出所ラベル（例:"必殺演出(Player)"）</param>
    void RegisterParams(const std::string &owner);

    /// ===================================================
    /// Getter
    /// ===================================================
    float &GetCloseUpDuration() { return closeUpDuration_; }
    float &GetActivationDelay() { return activationDelay_; }

  private:
    /// ===================================================
    /// private variants
    /// ===================================================

    /// <summary>演出の進行フェーズ</summary>
    enum class Phase
    {
        Idle,    ///< 何もしていない
        CloseUp, ///< カメラが顔アップ中
        Delay,   ///< 通常カメラ復帰後の発動待ち
    };

    Phase phase_ = Phase::Idle;    ///< 現在のフェーズ
    float timer_ = 0.0f;           ///< フェーズ内経過時間
    float closeUpDuration_ = 1.2f; ///< 顔アップの長さ(秒)。ImGui/Hubで調整可
    float activationDelay_ = 0.5f; ///< カメラ復帰から発動までの遅延(秒)。ImGui/Hubで調整可

    FollowCamera *pCamera_ = nullptr;   ///< 演出に使うカメラ
    std::function<void()> onActivate_; ///< 発動処理コールバック
};
