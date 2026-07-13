#pragma once
#include <Animation/AnimationController.h>
#include <functional>
#include <type/Quaternion.h>
#include <type/Vector3.h>

class Player;
namespace Hagine {
class DataHandler;
}

/// <summary>
/// プレイヤーの見た目パーツクラス
/// アニメーションの切り替えと飛行リーン（体の傾き）を担当する
/// </summary>
class PlayerVisual
{
  public:
    /// ===================================================
    /// public method
    /// ===================================================

    /// <summary>
    /// 初期化（アニメーションクリップの登録を行う）
    /// </summary>
    /// <param name="owner">所有者のプレイヤー</param>
    void Init(Player *owner);

    /// <summary>
    /// アニメーションの更新
    /// ステート・コンボに応じてクリップを切り替える
    /// </summary>
    void UpdateAnimation();

    /// <summary>
    /// 飛行移動中の体の傾き（リーン）を更新する
    /// ロックオン中は顔（向き）を敵に向けたまま、進行方向へ体を傾けて見せる。
    /// 描画専用の回転オフセットとして適用するため、射撃などの向きには影響しない
    /// </summary>
    void UpdateFlyLean();

    /// <summary>
    /// 見た目関連パラメータを保存する
    /// </summary>
    /// <param name="data">保存先のデータハンドラ</param>
    void Save(Hagine::DataHandler *data);

    /// <summary>
    /// 見た目関連パラメータを読み込む
    /// </summary>
    /// <param name="data">読み込み元のデータハンドラ</param>
    void Load(Hagine::DataHandler *data);

    /// <summary>
    /// アニメーション・飛行リーンのImGui表示
    /// </summary>
    /// <param name="onSave">保存ボタン押下時に呼ぶコールバック</param>
    void DrawImGui(const std::function<void()> &onSave);

    /// <summary>
    /// 調整パラメータをゲームパラメータHubへ登録する
    /// </summary>
    void RegisterParams();

  private:
    /// ===================================================
    /// private variants
    /// ===================================================

    static constexpr float kFlyVerticalAnimThreshold = 0.5f; ///< 飛行中、上昇・下降アニメに切り替えるY速度の閾値

    Player *owner_ = nullptr; ///< 所有者のプレイヤー

    Hagine::AnimationController animationController_; ///< アニメーション制御

    // ─── 飛行移動中の体の傾き（リーン）───
    // ロックオン飛行移動中、顔は敵向きのまま進行方向へ体を倒して見せるための描画専用の傾き。
    // すべて ImGui で調整可・セーブ対象。後ろ移動＝仰向け、左右移動＝バンク表現
    bool flyLeanEnabled_ = true;                                                    ///< リーン演出の有効/無効
    float flyLeanMaxFwdPitchDeg_ = 10.0f;                                           ///< 前進時の前傾の最大角（度）
    float flyLeanMaxBackPitchDeg_ = 50.0f;                                          ///< 後退時の仰け反り（仰向け）の最大角（度）
    float flyLeanMaxSideDeg_ = 70.0f;                                               ///< 左右移動時に体を進行方向へ向けるヨー(Y回転)の最大角（度。90で真横）
    float flyLeanRefSpeed_ = 8.0f;                                                  ///< 最大傾きに達する基準の水平速度
    float flyLeanResponse_ = 10.0f;                                                 ///< 傾きの追従速度（大きいほど即座に追従）
    Hagine::Vector3 flyLeanPivot_ = {0.0f, 1.0f, 0.0f};                             ///< 回転中心（モデル中心が原点にないため補正用）
    Hagine::Quaternion flyLeanRotation_ = Hagine::Quaternion::IdentityQuaternion(); ///< 現在の傾き（平滑化後・クォータニオン）
};
