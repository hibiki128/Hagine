#pragma once
#include "object/base/BaseObject.h"
#include "particle/gpu/ParticleCSEmitter.h"

class Player;
class MakanAttackSkill : public Hagine::BaseObject
{
  public:
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
    void Draw(const Hagine::ViewProjection &viewProjection) override;

    /// <summary>
    /// パーティクルの描画処理
    /// </summary>
    /// <param name="viewProjection">ビュープロジェクション</param>
    void DrawParticle(const Hagine::ViewProjection &viewProjection);

    void SetPlayer(Player *player) { pPlayer_ = player; }

    bool IsActive() const { return isActive_; }
    void Activate(Hagine::WorldTransform *playerTransform);

    /// <summary>
    /// ImGuiでのデバッグ表示
    /// </summary>
    void DebugImGui();

  private:
    /// <summary>
    /// 当たってる間
    /// </summary>
    /// <param name="other"></param>
    void OnCollisionEnter(Hagine::ColliderBase *other);

    void Deactivate();

  private:
    static constexpr float kLockOffsetY = 2.5f;

    std::unique_ptr<Hagine::ParticleCSEmitter> makanMainEffect_{};
    std::unique_ptr<Hagine::ParticleCSEmitter> makanAroundEffect_{};

    Hagine::OBBCollider *pMakanCollider_ = nullptr;

    bool isActive_ = false;
    Hagine::WorldTransform *pPlayerTransform_ = nullptr;
    // 発動時に固定した向き。発動後にプレイヤーが回転してもビームは追従しない
    // （回避可能にするため。発動までの照準追従はプレイヤー本体の回転が担う）
    Hagine::Quaternion lockedRotation_ = Hagine::Quaternion::IdentityQuaternion();
    float currentLength_ = 0.0f;
    float maxLength_ = 50.0f;
    float extendSpeed_ = 200.0f;
    float beamWidth_ = 2.0f;
    float beamHeight_ = 2.0f;
    float activeTime_ = 0.0f;
    float duration_ = 2.0f;
    float spiralTime_ = 0.0f;
    // らせんビーム制御用パラメータ
    float spiralRadius_ = 2.0f;        // らせんの半径
    float spiralRevolution_ = 3.0f;    // 最大長に達した時の巻き数
    float spiralForwardSpeed_ = 30.0f; // らせんパーティクルの前進速度
    Player *pPlayer_ = nullptr;
};