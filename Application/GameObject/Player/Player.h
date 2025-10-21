#pragma once
#include "Bullet/PlayerBullet.h"
#include "Data/DataHandler.h"
#include "Hand/PlayerHand.h"
#include "Object/Base/BaseObject.h"
#include "PlayerData.h"
#include "State/Base/PlayerBaseState.h"
#include <Application/Utility/Shake/Shake.h>
#include <Particle/CSParticle/ParticleCSEmitter.h>
#include <application/Utility/ComboSystem/ComboSystem.h>

class ChargeShot;
class FollowCamera;
class Enemy;

/// <summary>
/// プレイヤーのゲームオブジェクトクラス
/// 状態管理、移動、攻撃、カメラ制御などを行う
/// </summary>
class Player : public BaseObject {
  public:
    /// ===================================================
    /// public method
    /// ===================================================

    /// <summary>
    /// コンストラクタ
    /// </summary>
    Player();

    /// <summary>
    /// デストラクタ
    /// </summary>
    ~Player();

    /// <summary>
    /// 初期化
    /// </summary>
    /// <param name="objectName">オブジェクト名</param>
    void Init(const std::string objectName) override;

    /// <summary>
    /// 更新処理
    /// </summary>
    void Update() override;

    /// <summary>
    /// 描画処理
    /// </summary>
    /// <param name="viewProjection">ビュープロジェクション</param>
    /// <param name="offSet">描画オフセット</param>
    void Draw(const ViewProjection &viewProjection, Vector3 offSet = {0.0f, 0.0f, 0.0f}) override;

    /// <summary>
    /// パーティクルの描画処理
    /// </summary>
    /// <param name="viewProjection">ビュープロジェクション</param>
    void DrawParticle(const ViewProjection &viewProjection);

    /// <summary>
    /// 状態を変更
    /// </summary>
    /// <param name="stateName">変更する状態名</param>
    void ChangeState(const std::string &stateName);

    /// <summary>
    /// 方向情報を更新
    /// </summary>
    void DirectionUpdate();

    /// <summary>
    /// デバッグ処理
    /// </summary>
    void Debug();

    /// <summary>
    /// 突撃状態に変更
    /// </summary>
    void ChangeRush();

    /// <summary>
    /// 最短回転角度を計算
    /// </summary>
    /// <param name="from">開始角度</param>
    /// <param name="to">目標角度</param>
    /// <returns>float: 計算された最短回転角度</returns>
    float CalculateShortestRotation(float from, float to);

    /// <summary>
    /// 移動処理
    /// </summary>
    void Move();

    /// <summary>
    /// カメラを取得
    /// </summary>
    /// <returns>FollowCamera*: フォローカメラのポインタ</returns>
    FollowCamera *GetCamera() { return FollowCamera_; }

    /// <summary>
    /// 敵を取得
    /// </summary>
    /// <returns>Enemy*: 敵のポインタ</returns>
    Enemy *GetEnemy() { return enemy_; }

    /// <summary>
    /// 加速度を取得
    /// </summary>
    /// <returns>Vector3&: 加速度ベクトルの参照</returns>
    Vector3 &GetAcceleration() { return acceleration_; }

    /// <summary>
    /// 速度を取得
    /// </summary>
    /// <returns>Vector3&: 速度ベクトルの参照</returns>
    Vector3 &GetVelocity() { return velocity_; }

    /// <summary>
    /// 移動方向を取得
    /// </summary>
    /// <returns>Vector3: 移動方向ベクトル</returns>
    Vector3 GetMovementDirection() const;

    /// <summary>
    /// 前方向を取得
    /// </summary>
    /// <returns>Vector3: 前方向ベクトル</returns>
    Vector3 GetForward() const;

    /// <summary>
    /// 後方向を取得
    /// </summary>
    /// <returns>Vector3: 後方向ベクトル</returns>
    Vector3 GetBackward() const;

    /// <summary>
    /// 右方向を取得
    /// </summary>
    /// <returns>Vector3: 右方向ベクトル</returns>
    Vector3 GetRight() const;

    /// <summary>
    /// 左方向を取得
    /// </summary>
    /// <returns>Vector3: 左方向ベクトル</returns>
    Vector3 GetLeft() const;

    /// <summary>
    /// 上方向を取得
    /// </summary>
    /// <returns>Vector3: 上方向ベクトル</returns>
    Vector3 GetUp() const;

    /// <summary>
    /// 下方向を取得
    /// </summary>
    /// <returns>Vector3: 下方向ベクトル</returns>
    Vector3 GetDown() const;

    /// <summary>
    /// 後ろの位置を取得
    /// </summary>
    /// <param name="distance">距離</param>
    /// <returns>Vector3: 後ろの位置座標</returns>
    Vector3 GetPositionBehind(float distance = 3.0f) const;

    /// <summary>
    /// 前の位置を取得
    /// </summary>
    /// <param name="distance">距離</param>
    /// <returns>Vector3: 前の位置座標</returns>
    Vector3 GetPositionFront(float distance = 3.0f) const;

    /// <summary>
    /// 右の位置を取得
    /// </summary>
    /// <param name="distance">距離</param>
    /// <returns>Vector3: 右の位置座標</returns>
    Vector3 GetPositionRight(float distance = 3.0f) const;

    /// <summary>
    /// 左の位置を取得
    /// </summary>
    /// <param name="distance">距離</param>
    /// <returns>Vector3: 左の位置座標</returns>
    Vector3 GetPositionLeft(float distance = 3.0f) const;

    /// <summary>
    /// 上の位置を取得
    /// </summary>
    /// <param name="distance">距離</param>
    /// <returns>Vector3: 上の位置座標</returns>
    Vector3 GetPositionAbove(float distance = 3.0f) const;

    /// <summary>
    /// 下の位置を取得
    /// </summary>
    /// <param name="distance">距離</param>
    /// <returns>Vector3: 下の位置座標</returns>
    Vector3 GetPositionBelow(float distance = 3.0f) const;

    /// <summary>
    /// 速度の大きさを取得
    /// </summary>
    /// <returns>float: 速度の大きさ</returns>
    float GetVelocityMagnitude() const;

    /// <summary>
    /// 落下速度を取得
    /// </summary>
    /// <returns>float&: 落下速度の参照</returns>
    float &GetFallSpeed() { return fallSpeed_; }

    /// <summary>
    /// 移動速度を取得
    /// </summary>
    /// <returns>float&: 移動速度の参照</returns>
    float &GetMoveSpeed() { return moveSpeed_; }

    /// <summary>
    /// ジャンプ速度を取得
    /// </summary>
    /// <returns>float&: ジャンプ速度の参照</returns>
    float &GetJumpSpeed() { return jumpSpeed_; }

    /// <summary>
    /// 最大速度を取得
    /// </summary>
    /// <returns>float&: 最大速度の参照</returns>
    float &GetMaxSpeed() { return maxSpeed_; }

    /// <summary>
    /// 加速度レートを取得
    /// </summary>
    /// <returns>float&: 加速度レートの参照</returns>
    float &GetAccelRate() { return accelRate_; }

    /// <summary>
    /// デルタタイムを取得
    /// </summary>
    /// <returns>float&: デルタタイムの参照</returns>
    float &GetDt() { return dt_; }

    /// <summary>
    /// ジャンプ可能かを取得
    /// </summary>
    /// <returns>bool&: ジャンプ可能フラグの参照</returns>
    bool &GetCanJump() { return canJump_; }

    /// <summary>
    /// 生存状態を取得
    /// </summary>
    /// <returns>bool&: 生存フラグの参照</returns>
    bool &GetAlive() { return isAlive_; }

    /// <summary>
    /// 接地状態を取得
    /// </summary>
    /// <returns>bool&: 接地フラグの参照</returns>
    bool &GetIsGrounded() { return isGrounded_; }

    /// <summary>
    /// ロックオン状態を取得
    /// </summary>
    /// <returns>bool&: ロックオンフラグの参照</returns>
    bool &GetIsLockOn() { return isLockOn_; }

    /// <summary>
    /// ビュープロジェクションを取得
    /// </summary>
    /// <returns>ViewProjection&: ビュープロジェクションの参照</returns>
    ViewProjection &GetViewProjection();

    /// <summary>
    /// 右手を取得
    /// </summary>
    /// <returns>PlayerHand*: 右手のポインタ</returns>
    PlayerHand *GetRightHand() { return rightHand_ptr_; }

    /// <summary>
    /// 左手を取得
    /// </summary>
    /// <returns>PlayerHand*: 左手のポインタ</returns>
    PlayerHand *GetLeftHand() { return leftHand_ptr_; }

    /// <summary>
    /// カメラを設定
    /// </summary>
    /// <param name="camera">設定するカメラのポインタ</param>
    void SetCamera(FollowCamera *camera);

    /// <summary>
    /// ビュープロジェクションを設定
    /// </summary>
    /// <param name="vp">設定するビュープロジェクションのポインタ</param>
    void SetVp(ViewProjection *vp);

    void SetStart(bool flag) {
        started_ = flag;
    }

    /// <summary>
    /// 敵を設定
    /// </summary>
    /// <param name="enemy">設定する敵のポインタ</param>
    void SetEnemy(Enemy *enemy) {
        enemy_ = enemy;
        leftHand_ptr_->SetEnemy(enemy);
        rightHand_ptr_->SetEnemy(enemy);
    }

    /// <summary>
    /// コントロール入力カウントをリセット
    /// </summary>
    void ResetControlCount() {
        lControlInputTime_ = 0.0f;
        lControlInputCount_ = 0;
    }

    /// <summary>
    /// 向きを取得
    /// </summary>
    /// <returns>Direction&: 向きの参照</returns>
    Direction &GetDirection() { return dir_; }

    /// <summary>
    /// 移動方向を取得
    /// </summary>
    /// <returns>MoveDirection&: 移動方向の参照</returns>
    MoveDirection &GetMoveDirection() { return moveDir_; }

    /// <summary>
    /// 現在の状態名を取得
    /// </summary>
    /// <returns>std::string: 現在の状態名</returns>
    std::string GetCurrentStateName() const;

    /// <summary>
    /// 発射した弾の一覧を取得
    /// </summary>
    /// <returns>std::vector<std::unique_ptr<PlayerBullet>>&: 弾のvector参照</returns>
    std::vector<std::unique_ptr<PlayerBullet>> &GetBullets() { return bullets_; }

  private:
    /// ===================================================
    /// private method
    /// ===================================================

    /// <summary>
    /// プレイヤー設定を保存
    /// </summary>
    void Save();

    /// <summary>
    /// プレイヤー設定を読み込み
    /// </summary>
    void Load();

    /// <summary>
    /// コンボ更新処理
    /// </summary>
    void ComboUpdate();

    /// <summary>
    /// 射撃処理
    /// </summary>
    void Shot();

    /// <summary>
    /// 影のスケールを更新
    /// </summary>
    void UpdateShadowScale();

    /// <summary>
    /// 回転を更新
    /// </summary>
    void RotateUpdate();

    /// <summary>
    /// 地面との衝突判定処理
    /// </summary>
    void CollisionGround();

    /// <summary>
    /// 回転からDirection値を計算
    /// </summary>
    /// <returns>Direction: 計算されたDirection</returns>
    Direction CalculateDirectionFromRotation();

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

  private:
    /// ===================================================
    /// private varians
    /// ===================================================

    FollowCamera *FollowCamera_;
    Enemy *enemy_ = nullptr;

    Direction dir_;
    MoveDirection moveDir_;

    Vector3 velocity_{};
    Vector3 acceleration_{};

    float moveSpeed_ = 0.0f; // 移動速度
    float fallSpeed_ = 0.0f; // 落下速度
    float jumpSpeed_ = 0.0f; // ジャンプ速度
    float maxSpeed_ = 0.0f;  // 最大速度
    float accelRate_ = 0.0f; // 加速度レート
    float dt_;               // デルタタイム

    float lControlInputTime_ = 0.0f;     // L操作入力の保持時間
    int lControlInputCount_ = 0;         // L操作入力の回数
    const float INPUT_RESET_TIME = 0.3f; // 入力リセット時間

    float currentFov_ = 45.0f;  // 現在のFOV
    float targetFov_ = 45.0f;   // 目標FOV
    float fovLerpSpeed_ = 5.0f; // FOV補間速度

    float B_acce_ = 0.0f;  // ブーストの加速度
    float B_speed_ = 0.0f; // ブーストの速度

    bool canJump_ = false;   // ジャンプ可能フラグ
    bool isAlive_ = true;    // 生存フラグ
    bool isLockOn_ = false;  // ロックオンフラグ
    bool isGrounded_ = true; // 接地フラグ
    bool isDashing_ = false; // ダッシュ中フラグ

    bool started_ = false; // ゲーム開始フラグ

    ComboSystem punchCombo_;
    bool comboInitialized_ = false; // コンボ初期化済みフラグ

    std::unordered_map<std::string, std::unique_ptr<PlayerBaseState>> states_; // 状態マップ
    PlayerBaseState *currentState_ = nullptr;                                  // 現在の状態

    std::unique_ptr<DataHandler> data_;
    std::unique_ptr<BaseObject> shadow_;

    std::vector<std::unique_ptr<PlayerBullet>> bullets_; // 発射した弾
    std::unique_ptr<ChargeShot> chargeShot_;             // チャージショット

    std::unique_ptr<PlayerHand> leftHand_;  // 左手
    std::unique_ptr<PlayerHand> rightHand_; // 右手
    PlayerHand *leftHand_ptr_;
    PlayerHand *rightHand_ptr_;

    ViewProjection *vp_;
    std::unique_ptr<Shake> shake_;

    std::unique_ptr<ParticleCSEmitter> auraEmitter_; // オーラパーティクル
    std::unique_ptr<ParticleEmitter> rushEmitter_;   // 突撃パーティクル
};