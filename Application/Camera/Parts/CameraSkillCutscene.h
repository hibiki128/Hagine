#pragma once

class FollowCamera;
namespace Hagine {
class BaseObject;
}

/// <summary>
/// フォローカメラの必殺技「顔アップ演出」パーツ
/// 技の使用者の顔前へ回り込み、注視するカメラワークを担当する
/// </summary>
class CameraSkillCutscene {
  public:
    /// ===================================================
    /// public method
    /// ===================================================

    /// <summary>
    /// 初期化（調整パラメータをゲームパラメータHubへ登録する）
    /// </summary>
    /// <param name="owner">所有者のフォローカメラ</param>
    void Init(FollowCamera *owner);

    /// <summary>
    /// デストラクタ（ゲームパラメータHubからの登録解除）
    /// </summary>
    ~CameraSkillCutscene();

    /// <summary>
    /// 顔アップ演出を開始する
    /// </summary>
    /// <param name="performer">技の使用者（プレイヤー/敵どちらでも可）</param>
    void StartSkillCloseUp(Hagine::BaseObject *performer) { skillCloseUpTarget_ = performer; }

    /// <summary>
    /// 顔アップ演出を終了する（次フレームから通常カメラへ補間なしで即復帰）
    /// </summary>
    void EndSkillCloseUp() { skillCloseUpTarget_ = nullptr; }

    /// <summary>
    /// 顔アップ演出中かどうか
    /// </summary>
    bool IsSkillCloseUpActive() const { return skillCloseUpTarget_ != nullptr; }

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
    /// private variants
    /// ===================================================

    // 閾値・ベクトル定数
    static constexpr float kEpsilon = 0.001f;           ///< 微小値
    static constexpr float kParallelThreshold = 0.999f; ///< 平行判定しきい値
    static constexpr float kMaxBlendValue = 1.0f;       ///< 最大ブレンド値
    static constexpr float kVectorZero = 0.0f;          ///< ゼロ値
    static constexpr float kUpVectorY = 1.0f;           ///< Y軸上方向
    static constexpr float kRightVectorX = 1.0f;        ///< X軸右方向

    FollowCamera *owner_ = nullptr; ///< 所有者のフォローカメラ

    // 必殺技の顔アップ演出関連（すべてImGui/ParamHubで調整可）
    Hagine::BaseObject *skillCloseUpTarget_ = nullptr; ///< 顔アップ対象（nullptrなら演出なし）
    float closeUpDistance_ = 6.0f;                     ///< 顔からカメラまでの距離
    float closeUpFaceHeight_ = 4.0f;                   ///< 対象位置から顔までの高さオフセット
    float closeUpApproachSpeed_ = 8.0f;                ///< 回り込みの速さ（指数補間の係数）
};
