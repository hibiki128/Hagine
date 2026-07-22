#pragma once
#include "Application/entity/player/PlayerData.h"
#include <type/Vector3.h>

class Player;
namespace Hagine {
class DataHandler;
}

/// <summary>
/// プレイヤーの移動パーツクラス
/// 移動・回転・ダッシュ・接地・方向判定と、それらのパラメータを担当する
/// </summary>
class PlayerMovement
{
  public:
    /// ===================================================
    /// public method
    /// ===================================================

    /// <summary>
    /// 初期化
    /// </summary>
    /// <param name="owner">所有者のプレイヤー</param>
    void Init(Player *owner);

    /// <summary>
    /// 移動処理
    /// カメラ方向・入力・加速度に基づいて velocity を更新する
    /// </summary>
    void Move();

    /// <summary>
    /// 方向情報を更新
    /// </summary>
    void DirectionUpdate();

    /// <summary>
    /// 回転を更新（ロックオン追従 / スティック手動回転）
    /// </summary>
    void RotateUpdate();

    /// <summary>
    /// 指定したワールド座標へ即座に（補間なしで）正面を向ける。
    /// 瞬間移動コンボで敵へ貼り付いた瞬間に正しく攻撃判定を向けるために使う
    /// </summary>
    /// <param name="targetPos">向く先のワールド座標</param>
    void FaceTargetInstant(const Hagine::Vector3 &targetPos);

    /// <summary>
    /// 地面との衝突判定処理（落下速度の制限も行う）
    /// </summary>
    void CollisionGround();

    /// <summary>
    /// ダッシュ継続時間と開始フラグを更新する
    /// </summary>
    void UpdateDashState();

    /// <summary>
    /// 近接攻撃の踏み込みを開始する（コンボの各段が発火した瞬間に呼ぶ）
    /// 敵がいれば敵方向、いなければ正面へ短く前進する。近すぎる場合は
    /// すり抜け・押し込みを避けるため踏み込まない
    /// </summary>
    void StartMeleeLunge();

    /// <summary>
    /// ダッシュ状態をリセットする
    /// </summary>
    void ClearDashState()
    {
        isDashing_ = false;
        dashDuration_ = 0.0f;
        dashGraceTimer_ = 0.0f;
    }

    /// <summary>
    /// 最短回転角度を計算
    /// </summary>
    /// <param name="from">開始角度</param>
    /// <param name="to">目標角度</param>
    /// <returns>float: 計算された最短回転角度</returns>
    float CalculateShortestRotation(float from, float to);

    /// <summary>
    /// 角度を正規化
    /// </summary>
    /// <param name="angle">正規化する角度</param>
    /// <returns>float: 正規化された角度</returns>
    float NormalizeAngle(float angle);

    /// <summary>
    /// Direction値を文字列で取得
    /// </summary>
    /// <param name="dir">方向の値</param>
    /// <returns>const char*: 方向の名前文字列</returns>
    const char *GetDirectionName(Direction dir);

    /// <summary>
    /// 正規化した移動方向を取得
    /// </summary>
    /// <returns>Vector3: 移動方向（速度ゼロならゼロベクトル）</returns>
    Hagine::Vector3 GetMovementDirection() const;

    /// <summary>
    /// 速度の大きさを取得
    /// </summary>
    /// <returns>float: 速度の大きさ</returns>
    float GetVelocityMagnitude() const;

    /// <summary>
    /// 移動関連パラメータを保存する
    /// </summary>
    /// <param name="data">保存先のデータハンドラ</param>
    void Save(Hagine::DataHandler *data);

    /// <summary>
    /// 移動関連パラメータを読み込む
    /// </summary>
    /// <param name="data">読み込み元のデータハンドラ</param>
    void Load(Hagine::DataHandler *data);

    /// <summary>
    /// 移動関連のImGui表示
    /// </summary>
    void DrawImGui();

    /// <summary>
    /// 調整パラメータをゲームパラメータHubへ登録する
    /// </summary>
    void RegisterParams();

    /// ===================================================
    /// Getter
    /// ===================================================
    Hagine::Vector3 &GetVelocity() { return velocity_; }
    Hagine::Vector3 &GetAcceleration() { return acceleration_; }
    float &GetMoveSpeed() { return moveSpeed_; }
    float &GetFallSpeed() { return fallSpeed_; }
    float &GetJumpSpeed() { return jumpSpeed_; }
    float &GetMaxSpeed() { return maxSpeed_; }
    float &GetAccelRate() { return accelRate_; }
    bool &GetCanJump() { return canJump_; }
    bool &GetIsGrounded() { return isGrounded_; }
    bool GetIsDashing() const { return isDashing_; }
    float GetDashDuration() const { return dashDuration_; }
    bool GetDashStartedThisFrame() const { return dashStartedThisFrame_; }
    Direction &GetDirection() { return dir_; }
    MoveDirection &GetMoveDirection() { return moveDir_; }

    /// ===================================================
    /// Setter
    /// ===================================================
    void SetDashing(bool flag) { isDashing_ = flag; }
    void SetDashInput(float x, float z)
    {
        dashInputX_ = x;
        dashInputZ_ = z;
    }

    /// <summary>
    /// 回転からDirection値を計算
    /// </summary>
    /// <returns>Direction: 計算されたDirection</returns>
    Direction CalculateDirectionFromRotation();

  private:
    /// ===================================================
    /// private variants
    /// ===================================================

    // 回転・ベクトル定数
    static constexpr float kRotationZero = 0.0f;
    static constexpr float kRightVectorX = 1.0f;
    static constexpr float kRightVectorY = 0.0f;
    static constexpr float kRightVectorZ = 0.0f;
    static constexpr float kUpVectorX = 0.0f;
    static constexpr float kUpVectorY = 1.0f;
    static constexpr float kUpVectorZ = 0.0f;

    // 速度・移動関連定数
    static constexpr float kMaxFallVelocity = -40.0f;
    static constexpr float kGroundSnapDistance = 1.0f; // 下り坂で接地を維持する吸着距離
    static constexpr float kVelocityZero = 0.0f;
    static constexpr float kRushGroundOffset = 0.1f;
    static constexpr float kLandingSpeedThreshold = 0.5f;
    static constexpr float kMinRotationDistance = 0.001f;
    static constexpr float kParallelThreshold = 0.999f;
    static constexpr float kPlayerRotationSpeed = 10.0f;
    static constexpr float kManualRotationSpeed = 0.04f;

    // 入力・移動制御定数
    static constexpr float kInputZero = 0.0f;
    static constexpr float kInputValue = 1.0f;
    static constexpr float kDecelerationFactor = 0.65f;
    static constexpr float kVelocityStopThreshold = 0.01f;
    static constexpr float kYComponentZero = 0.0f;
    static constexpr float kDashSpeedMultiplier = 1.5f;
    static constexpr float kDashGraceTime = 0.3f; // A押下後、移動入力を待つ猶予時間（秒）

    Player *pOwner_ = nullptr; ///< 所有者のプレイヤー

    Direction dir_ = Direction::Forward;             ///< 向いている方向
    MoveDirection moveDir_ = MoveDirection::Forward; ///< 移動方向

    Hagine::Vector3 velocity_{};     ///< 速度
    Hagine::Vector3 acceleration_{}; ///< 加速度

    float moveSpeed_ = 0.0f; ///< 移動速度
    float fallSpeed_ = 0.0f; ///< 落下速度
    float jumpSpeed_ = 0.0f; ///< ジャンプ速度
    float maxSpeed_ = 0.0f;  ///< 最大速度
    float accelRate_ = 0.0f; ///< 加速度レート

    bool canJump_ = false;   ///< ジャンプ可能フラグ
    bool isGrounded_ = true; ///< 接地フラグ

    // ─── 近接攻撃の踏み込み ───
    // 完全に止まったまま殴ると当てづらいため、各段の発火で少し前へ踏み込む。
    // 速度は Move() のコンボ中の減衰で数フレームかけて消えるので、短い前進になる
    float meleeLungeSpeed_ = 18.0f;      ///< 踏み込みの初速
    float meleeLungeMinDistance_ = 2.5f; ///< この距離より近い敵へは踏み込まない

    bool isDashing_ = false;            ///< ダッシュ中フラグ
    float dashInputX_ = 0.0f;           ///< ダッシュ開始時のスティックX入力
    float dashInputZ_ = 0.0f;           ///< ダッシュ開始時のスティックZ入力
    bool dashStartedThisFrame_ = false; ///< ダッシュ開始フラグ
    float dashDuration_ = 0.0f;         ///< ダッシュ継続時間
    bool wasDashing_ = false;           ///< 前フレームのダッシュ状態
    float dashGraceTimer_ = 0.0f;       ///< A押下後、移動入力を待つ猶予時間の残り
};
