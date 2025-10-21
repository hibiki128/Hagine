#pragma once
#include "Application/Utility/Shake/Shake.h"

#include "BehaviorTree/BehaviorNode/BehaviorNode.h"
#include "Object/Base/BaseObject.h"
#include "Particle/ParticleEmitter.h"
#include <Application/GameObject/Player/Player.h>
#include <application/GameObject/Player/PlayerData.h>

/// <summary>
/// 敵のゲームオブジェクトクラス
/// ビヘイビアツリーに基づいて行動し、プレイヤーとの相互作用を管理する
/// </summary>
class Enemy : public BaseObject {
  public:
    /// ===================================================
    /// public method
    /// ===================================================

    /// <summary>
    /// コンストラクタ
    /// </summary>
    Enemy();

    /// <summary>
    /// デストラクタ
    /// </summary>
    ~Enemy();

    /// <summary>
    /// 初期化
    /// </summary>
    /// <param name="objectName">オブジェクト名</param>
    void Init(const std::string objectName) override;

    /// <summary>
    /// ビヘイビアツリーの初期化
    /// </summary>
    void InitializeBehaviorTree();

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
    /// デバッグ処理
    /// </summary>
    void Debug();

    /// <summary>
    /// 衝突判定時の処理
    /// </summary>
    /// <param name="other">衝突したコライダー</param>
    void OnCollisionEnter([[maybe_unused]] Collider *other) override;

    /// <summary>
    /// 加速度を取得
    /// </summary>
    /// <returns>加速度のベクトル参照</returns>
    Vector3 &GetAcceleration() { return acceleration_; }

    /// <summary>
    /// 速度を取得
    /// </summary>
    /// <returns>速度のベクトル参照</returns>
    Vector3 &GetVelocity() { return velocity_; }

    /// <summary>
    /// 移動方向を取得
    /// </summary>
    /// <returns>移動方向のベクトル</returns>
    Vector3 GetMovementDirection() const;

    /// <summary>
    /// 前方向を取得
    /// </summary>
    /// <returns>前方向のベクトル</returns>
    Vector3 GetForward() const;

    /// <summary>
    /// 後方向を取得
    /// </summary>
    /// <returns>後方向のベクトル</returns>
    Vector3 GetBackward() const;

    /// <summary>
    /// 右方向を取得
    /// </summary>
    /// <returns>右方向のベクトル</returns>
    Vector3 GetRight() const;

    /// <summary>
    /// 左方向を取得
    /// </summary>
    /// <returns>左方向のベクトル</returns>
    Vector3 GetLeft() const;

    /// <summary>
    /// 上方向を取得
    /// </summary>
    /// <returns>上方向のベクトル</returns>
    Vector3 GetUp() const;

    /// <summary>
    /// 下方向を取得
    /// </summary>
    /// <returns>下方向のベクトル</returns>
    Vector3 GetDown() const;

    /// <summary>
    /// 後ろの位置を取得
    /// </summary>
    /// <param name="distance">距離</param>
    /// <returns>後ろの位置座標</returns>
    Vector3 GetPositionBehind(float distance = 3.0f) const;

    /// <summary>
    /// 前の位置を取得
    /// </summary>
    /// <param name="distance">距離</param>
    /// <returns>前の位置座標</returns>
    Vector3 GetPositionFront(float distance = 3.0f) const;

    /// <summary>
    /// 右の位置を取得
    /// </summary>
    /// <param name="distance">距離</param>
    /// <returns>右の位置座標</returns>
    Vector3 GetPositionRight(float distance = 3.0f) const;

    /// <summary>
    /// 左の位置を取得
    /// </summary>
    /// <param name="distance">距離</param>
    /// <returns>左の位置座標</returns>
    Vector3 GetPositionLeft(float distance = 3.0f) const;

    /// <summary>
    /// 上の位置を取得
    /// </summary>
    /// <param name="distance">距離</param>
    /// <returns>上の位置座標</returns>
    Vector3 GetPositionAbove(float distance = 3.0f) const;

    /// <summary>
    /// 下の位置を取得
    /// </summary>
    /// <param name="distance">距離</param>
    /// <returns>下の位置座標</returns>
    Vector3 GetPositionBelow(float distance = 3.0f) const;

    /// <summary>
    /// 速度の大きさを取得
    /// </summary>
    /// <returns>速度の大きさ</returns>
    float GetVelocityMagnitude() const;

    /// <summary>
    /// 落下速度を取得
    /// </summary>
    /// <returns>落下速度の参照</returns>
    float &GetFallSpeed() { return fallSpeed_; }

    /// <summary>
    /// 移動速度を取得
    /// </summary>
    /// <returns>移動速度の参照</returns>
    float &GetMoveSpeed() { return moveSpeed_; }

    /// <summary>
    /// ジャンプ速度を取得
    /// </summary>
    /// <returns>ジャンプ速度の参照</returns>
    float &GetJumpSpeed() { return jumpSpeed_; }

    /// <summary>
    /// 最大速度を取得
    /// </summary>
    /// <returns>最大速度の参照</returns>
    float &GetMaxSpeed() { return maxSpeed_; }

    /// <summary>
    /// 加速度レートを取得
    /// </summary>
    /// <returns>加速度レートの参照</returns>
    float &GetAccelRate() { return accelRate_; }

    /// <summary>
    /// 現在のHPを取得
    /// </summary>
    /// <returns>現在のHP</returns>
    int GetHP() const { return HP_; }

    /// <summary>
    /// 最大HPを取得
    /// </summary>
    /// <returns>最大HP</returns>
    int GetMaxHP() const { return maxHP_; }

    /// <summary>
    /// ジャンプ可能かを取得
    /// </summary>
    /// <returns>ジャンプ可能フラグの参照</returns>
    bool &GetCanJump() { return canJump_; }

    /// <summary>
    /// 生存しているかを取得
    /// </summary>
    /// <returns>生存フラグの参照</returns>
    bool &GetAlive() { return isAlive_; }

    /// <summary>
    /// 接地しているかを取得
    /// </summary>
    /// <returns>接地フラグの参照</returns>
    bool &GetIsGrounded() { return isGrounded_; }

    /// <summary>
    /// ターゲットを取得
    /// </summary>
    /// <returns>ターゲットのPlayerポインタ</returns>
    Player *GetTarget() { return target_; }

    /// <summary>
    /// 向きを取得
    /// </summary>
    /// <returns>向きの参照</returns>
    Direction &GetDirection() { return dir_; }

    /// <summary>
    /// 移動方向を取得
    /// </summary>
    /// <returns>移動方向の参照</returns>
    MoveDirection &GetMoveDirection() { return moveDir_; }

    /// <summary>
    /// ダメージ量を設定
    /// </summary>
    /// <param name="damage">設定するダメージ量</param>
    void SetDamage(int damage) { damage_ = damage; }

    /// <summary>
    /// ビュープロジェクションを設定
    /// </summary>
    /// <param name="vp">設定するビュープロジェクション</param>
    void SetVp(ViewProjection *vp);

    /// <summary>
    /// ターゲットを設定
    /// </summary>
    /// <param name="target">設定するターゲットのPlayerポインタ</param>
    void SetTarget(Player *target) { target_ = target; }

    /// <summary>
    /// ビヘイビアツリーを設定
    /// </summary>
    /// <param name="root">設定するルートノード</param>
    void SetBehaviorTree(std::unique_ptr<BehaviorNode> root) {
        behaviorRoot_ = std::move(root);
    }

    /// <summary>
    /// ビヘイビアツリーのルートノードを取得
    /// </summary>
    /// <returns>ルートノードのポインタ</returns>
    BehaviorNode *GetBehaviorRoot() { return behaviorRoot_.get(); }

    #ifdef _DEBUG
    /// <summary>
    /// ビヘイビアツリーエディターを設定
    /// </summary>
    /// <param name="editor">設定するBehaviorTreeEditorへのポインタ</param>
    void SetBehaviorTreeEditor(BehaviorTreeEditor *editor) {
        BehaviorNode::SetEditor(editor);
    }
#endif

  private:
    /// ===================================================
    /// private method
    /// ===================================================

    /// <summary>
    /// 設定を保存
    /// </summary>
    void Save();

    /// <summary>
    /// 設定を読み込み
    /// </summary>
    void Load();

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
    /// <returns>計算されたDirection</returns>
    Direction CalculateDirectionFromRotation();

    /// <summary>
    /// Direction値を文字列で取得
    /// </summary>
    /// <param name="dir">方向の値</param>
    /// <returns>方向の名前文字列</returns>
    const char *GetDirectionName(Direction dir);

  private:
    /// ===================================================
    /// private varians
    /// ===================================================

    Direction dir_;
    MoveDirection moveDir_;

    Vector3 velocity_{};
    Vector3 acceleration_{};
    Player *target_ = nullptr;
    std::unique_ptr<BehaviorNode> behaviorRoot_;

    int HP_ = 40;
    int maxHP_ = 40;
    int damage_ = 0;

    float moveSpeed_ = 0.0f;
    float fallSpeed_ = 0.0f;
    float jumpSpeed_ = 0.0f;
    float maxSpeed_ = 0.0f;
    float accelRate_ = 0.0f;

    bool canJump_ = false;
    bool isAlive_ = true;
    bool isLockOn_ = false;
    bool isGrounded_ = true;

    std::unique_ptr<DataHandler> data_;
    std::unique_ptr<BaseObject> shadow_;
    std::unique_ptr<ParticleEmitter> emitter_;
    std::unique_ptr<Shake> chageShake_;
};