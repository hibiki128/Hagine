#pragma once
#include "ICameraPart.h"

namespace Hagine {
class BaseObject;
}

/// <summary>
/// フォローカメラの必殺技「顔アップ演出」パーツ
/// 技の使用者の顔前へ回り込み、注視するカメラワークを担当する
/// </summary>
class CameraSkillCutscene : public ICameraPart
{
  public:
    /// ===================================================
    /// public method
    /// ===================================================

    /// <summary>
    /// 初期化（調整パラメータをゲームパラメータHubへ登録する）
    /// </summary>
    /// <param name="pOwner">所有者のフォローカメラ</param>
    void Init(FollowCamera *pOwner) override;

    /// <summary>
    /// デストラクタ（ゲームパラメータHubからの登録解除）
    /// </summary>
    ~CameraSkillCutscene() override;

    /// <summary>
    /// 顔アップ演出を開始する
    /// </summary>
    /// <param name="pPerformer">技の使用者（プレイヤー/敵どちらでも可）</param>
    void StartSkillCloseUp(Hagine::BaseObject *pPerformer) { pSkillCloseUpTarget_ = pPerformer; }

    /// <summary>
    /// 顔アップ演出を終了する（次フレームから通常カメラへ補間なしで即復帰）
    /// </summary>
    void EndSkillCloseUp() { pSkillCloseUpTarget_ = nullptr; }

    /// <summary>
    /// 顔アップ演出中かどうか
    /// </summary>
    bool IsSkillCloseUpActive() const { return pSkillCloseUpTarget_ != nullptr; }

    /// <summary>
    /// 顔アップ演出の更新（Update()冒頭から呼ぶ）
    /// </summary>
    /// <returns>bool: trueなら演出中でカメラを確定済み（以降の通常処理を行わない）</returns>
    bool UpdateSkillCloseUp();

    /// <summary>
    /// 顔アップ演出のImGui表示
    /// </summary>
    void DrawImGui();

  private:
    /// ===================================================
    /// private variables
    /// ===================================================

    // 閾値定数
    static constexpr float kEpsilon = 0.001f;           ///< 微小値
    static constexpr float kParallelThreshold = 0.999f; ///< 平行判定しきい値

    FollowCamera *pOwner_ = nullptr; ///< 所有者のフォローカメラ

    // 必殺技の顔アップ演出関連（すべてImGui/ParamHubで調整可）
    Hagine::BaseObject *pSkillCloseUpTarget_ = nullptr; ///< 顔アップ対象（nullptrなら演出なし）
    float closeUpDistance_ = 6.0f;                     ///< 顔からカメラまでの距離
    float closeUpFaceHeight_ = 4.0f;                   ///< 対象位置から顔までの高さオフセット
    float closeUpApproachSpeed_ = 8.0f;                ///< 回り込みの速さ（指数補間の係数）
};
