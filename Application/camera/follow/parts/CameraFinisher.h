#pragma once
#include "ICameraPart.h"
#include <type/Quaternion.h>
#include <type/Vector3.h>

namespace Hagine {
class BaseObject;
}

/// <summary>
/// コンボ派生技（フィニッシャー）中のカメラアングル種別
/// フェーズごとに切り替えて、1つの技の中でカメラワークを変化させる
/// </summary>
enum class FinisherCameraStyle
{
    SideProfile,  ///< 2人を結ぶ線の真横から。打ち合いを横位置で見せる（基本の絵）
    LowAngleUp,   ///< 低い位置から見上げる。打ち上げ・上昇を迫力よく見せる
    OverShoulder, ///< プレイヤーの肩越しに相手を捉える。連射などの主観的な場面用
    Orbit,        ///< 2人の中点まわりをゆっくり回り込む。長めのフェーズに動きを足す
};

/// <summary>
/// フォローカメラのコンボ派生技（フィニッシャー）専用カメラパーツ
/// 技の使用者と相手の2点を基準にアングルを構築し、フェーズごとの
/// スタイル切り替え・カット（瞬時切り替え）・傾き（ダッチアングル）を担当する。
///
/// 見やすさのために次の3点を守る:
///  ・2人の離れ具合に応じてカメラを引き、必ず両者が画面に収まるようにする
///  ・横位置をとる側は技の開始時に1回だけ決め、以降は反転しない
///    （カットのたびに反対側へ回り込むと前後関係が入れ替わって何も読めなくなる）
///  ・旋回と画面の傾きは控えめにし、切り替えは必要な場所だけカットする
/// </summary>
class CameraFinisher : public ICameraPart
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
    ~CameraFinisher() override;

    /// <summary>
    /// 演出カメラを開始する
    /// </summary>
    /// <param name="pPerformer">技の使用者</param>
    /// <param name="pTarget">技を受ける相手</param>
    /// <param name="style">開始時のアングル種別</param>
    void Start(Hagine::BaseObject *pPerformer, Hagine::BaseObject *pTarget, FinisherCameraStyle style);

    /// <summary>
    /// アングル種別を切り替える（フェーズの切り替わりで呼ぶ）
    /// </summary>
    /// <param name="style">切り替え先のアングル種別</param>
    /// <param name="isCut">true なら補間せず瞬時に切り替える（瞬間移動などのカット割り用）</param>
    void SetStyle(FinisherCameraStyle style, bool isCut = false);

    /// <summary>
    /// 画面の傾き（ダッチアングル）を設定する
    /// </summary>
    /// <param name="degrees">傾き角（度）</param>
    void SetRoll(float degrees) { targetRollDeg_ = degrees; }

    /// <summary>
    /// 演出カメラを終了する（次フレームから通常追従へ戻る）
    /// </summary>
    void Stop() { isActive_ = false; }

    /// <summary>演出カメラが有効かどうか</summary>
    bool IsActive() const { return isActive_; }

    /// <summary>
    /// 演出カメラの更新（FollowCamera::Update() 冒頭から呼ぶ）
    /// </summary>
    /// <returns>bool: true なら演出中でカメラを確定済み（以降の通常処理を行わない）</returns>
    bool UpdateFinisherCamera();

    /// <summary>
    /// 演出カメラのImGui表示
    /// </summary>
    void DrawImGui();

  private:
    /// ===================================================
    /// private method
    /// ===================================================

    /// <summary>
    /// 現在のスタイルに応じたカメラ位置と注視点を算出する
    /// </summary>
    /// <param name="outPosition">算出したカメラ位置</param>
    /// <param name="outLookAt">算出した注視点</param>
    void ComputeGoal(Hagine::Vector3 &outPosition, Hagine::Vector3 &outLookAt) const;

    /// <summary>
    /// 技の使用者から相手へ向かう水平方向を取得する（ゼロ長なら直前の値を返す）
    /// </summary>
    /// <returns>Vector3: 正規化済みの水平方向</returns>
    Hagine::Vector3 GetHorizontalAxis() const;

    /// <summary>
    /// 2人が離れているほどカメラを引いて、両者が画面に収まる距離を求める
    /// </summary>
    /// <param name="baseDistance">密着しているときの距離</param>
    /// <param name="framingRate">離れ具合を距離へ反映する割合</param>
    /// <returns>float: 実際に使うカメラ距離</returns>
    float GetFramingDistance(float baseDistance, float framingRate) const;

    /// <summary>
    /// 2人を結ぶ線に直交する「カメラを置く側」の水平方向を求める。
    /// 蹴り返しなどで使用者と相手の前後が入れ替わっても、カメラが反対側へ回り込まないよう
    /// 常に cameraSideDir_ と同じ側を返す（イマジナリーラインを跨がせない）
    /// </summary>
    /// <param name="axis">使用者→相手の水平方向</param>
    /// <returns>Vector3: カメラを置く側の水平方向（正規化済み）</returns>
    Hagine::Vector3 GetCameraSide(const Hagine::Vector3 &axis) const;

    /// <summary>
    /// カメラ位置が地面へめり込まないよう最低高度を確保する
    /// </summary>
    /// <param name="position">補正対象のカメラ位置（in/out）</param>
    void ClampToGround(Hagine::Vector3 &position) const;

    /// <summary>
    /// カメラを現在のアングルの目標位置・注視点へ補間なしで合わせる（カット割り用）
    /// </summary>
    void SnapToGoal();

    /// ===================================================
    /// private variables
    /// ===================================================

    // 閾値定数
    static constexpr float kEpsilon = 0.001f;           ///< ゼロ除算を避けるための微小値
    static constexpr float kParallelThreshold = 0.999f; ///< 平行判定しきい値
    static constexpr float kGroundClearance = 1.2f;     ///< 地表からの最低クリアランス

    FollowCamera *pOwner_ = nullptr; ///< 所有者のフォローカメラ

    Hagine::BaseObject *pPerformer_ = nullptr; ///< 技の使用者
    Hagine::BaseObject *pTarget_ = nullptr;    ///< 技を受ける相手

    bool isActive_ = false;                                        ///< 演出カメラ有効フラグ
    FinisherCameraStyle style_ = FinisherCameraStyle::SideProfile; ///< 現在のアングル種別
    float orbitAngle_ = 0.0f;                                      ///< 旋回アングルの現在角（ラジアン）

    // カメラを置く側のワールド方向。技の開始時に通常カメラのある側へ合わせて決め、
    // 以降は被写体の向きが変わってもこの側を保つ（反対側へ回り込むと画面が読めなくなる）。
    // 戦っている向き自体がゆっくり変わる場合に追従できるよう、毎フレーム少しずつ更新する
    Hagine::Vector3 cameraSideDir_ = {1.0f, 0.0f, 0.0f};

    Hagine::Vector3 lastAxis_ = {0.0f, 0.0f, 1.0f}; ///< 直前の水平方向（2人が重なったときの保険）
    Hagine::Vector3 currentLookAt_{};               ///< 補間後の注視点

    float currentRollDeg_ = 0.0f; ///< 現在の傾き（度）
    float targetRollDeg_ = 0.0f;  ///< 目標の傾き（度）

    // ─── 調整パラメータ（GameParamで調整可）───
    float followSpeed_ = 6.0f;          ///< 目標位置への追従速度（指数補間の係数）
    float lookSpeed_ = 12.0f;           ///< 注視点への追従速度（位置より速くして被写体を外さない）
    float rollSpeed_ = 4.0f;            ///< 傾きの追従速度
    float sideTrackSpeed_ = 1.5f;       ///< カメラを置く側が被写体の向きへ追従する速さ（速いと線を跨ぎやすい）
    float framingRate_ = 0.65f;         ///< 2人の離れ具合をカメラ距離へ反映する割合
    float framingMaxDistance_ = 55.0f;  ///< 引きすぎて豆粒にならないための距離上限
    float sideDistance_ = 19.0f;        ///< 横位置アングルの基準距離（密着時）
    float sideHeight_ = 5.0f;           ///< 横位置アングルの高さ
    float lowAngleDistance_ = 15.0f;    ///< 見上げアングルの基準水平距離
    float lowAngleDrop_ = 3.0f;         ///< 見上げアングルの下げ幅（低いほど煽る）
    float shoulderDistance_ = 11.0f;    ///< 肩越しアングルの後方距離
    float shoulderSide_ = 2.6f;         ///< 肩越しアングルの横ずらし量
    float shoulderHeight_ = 3.5f;       ///< 肩越しアングルの高さ
    float orbitRadius_ = 19.0f;         ///< 旋回アングルの基準半径
    float orbitHeight_ = 6.0f;          ///< 旋回アングルの高さ
    float orbitSpeed_ = 0.35f;          ///< 旋回速度（ラジアン/秒。速いと画面が回って見づらい）
    float lookHeightOffset_ = 1.5f;     ///< 注視点の高さオフセット（胸のあたりを見る）
};
