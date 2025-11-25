#pragma once
#include "Object/Base/BaseObject.h"
#include "Particle/CSParticle/ParticleCSEmitter.h"

class Player;
class MakanAttackSkill : public BaseObject {
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
    void Draw(const ViewProjection &viewProjection, Vector3 offSet = {0.0f, 0.0f, 0.0f}) override;

    /// <summary>
    /// パーティクルの描画処理
    /// </summary>
    /// <param name="viewProjection">ビュープロジェクション</param>
    void DrawParticle(const ViewProjection &viewProjection);

    void SetPlayer(Player *player) { player_ = player; }

    bool IsActive() const { return isActive_; }
    void Activate(WorldTransform *playerTransform);

        /// <summary>
    /// ImGuiでのデバッグ表示
    /// </summary>
    void DebugImGui();
  private:
    /// <summary>
    /// 当たってる間
    /// </summary>
    /// <param name="other"></param>
    void OnCollisionEnter(ColliderBase *other);

    void Deactivate();

  private:
    std::unique_ptr<ParticleCSEmitter> makanMainEffect_{};
    std::unique_ptr<ParticleCSEmitter> makanAroundEffect_{};

    OBBCollider *makanCollider_ = nullptr;

    bool isActive_ = false;
    WorldTransform *playerTransform_ = nullptr;
    float currentLength_ = 0.0f;
    float maxLength_ = 50.0f;
    float extendSpeed_ = 200.0f;
    float beamWidth_ = 2.0f;
    float beamHeight_ = 2.0f;
    float activeTime_ = 0.0f;
    float duration_ = 2.0f;
    float spiralTime_ = 0.0f;
    // らせんビーム制御用パラメータ
    float spiralRadius_ = 2.0f;     // らせんの半径
    float spiralRevolution_ = 3.0f; // 最大長に達した時の巻き数
    float spiralForwardSpeed_ = 30.0f; // らせんパーティクルの前進速度
    Player *player_ = nullptr;
};