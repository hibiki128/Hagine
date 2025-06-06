#pragma once
#include "Engine/Input/Input.h"
#include "application/Base/BaseObject.h"
#include "application/GameObject/Player/Player.h"
#include"Particle/ParticleEmitter.h"

class ChageShot : public BaseObject {
  public:
    void Init(const std::string objectName) override;
    void Update() override;
    void Draw(const ViewProjection &viewProjection, Vector3 offSet = {0.0f, 0.0f, 0.0f}) override;
    void DrawParticle(const ViewProjection &viewProjection);
    void imgui();

    void OnCollisionEnter([[maybe_unused]] Collider *other) override;

    // ChageShotが生きているか
    bool IsAlive() const { return isAlive_; }
    // 最大スケールに到達したか
    bool IsMaxScale() const { return isMaxScale_; }
    // ChageShotのスケール取得
    float GetScale() const { return scale_; }

    // 発射済みか
    bool IsFired() const { return isFired_; }

    // ChageShotの発射処理
    void Fire(const Vector3 &pos, const Vector3 &dir);

    // ChageShotのリセット
    void Reset();

    // プレイヤー参照セット
    void SetPlayer(Player *player) { player_ = player; }

    // オフセット設定用のセッター（必要に応じて使用）
    void SetPlayerRadius(float radius) { playerRadius_ = radius; }
    void SetOffsetMargin(float margin) { offsetMargin_ = margin; }
    void SetVerticalOffset(float yOffset) { verticalOffset_ = yOffset; }

  private:
    Vector3 offset_{};
    bool isAlive_ = false;
    bool isMaxScale_ = false;
    bool isFired_ = false;
    float scale_ = 1.0f;
    float scaleSpeed_ = 1.25f; // 1秒で2倍ずつ大きくなる
    float maxScale_ = 4.0f;
    Vector3 velocity_{};
    float speed_ = 60.0f;
    Player *player_ = nullptr;

    // オフセット調整用のパラメータ
    float playerRadius_ = 1.0f;   // プレイヤーの半径
    float offsetMargin_ = 0.5f;   // 余裕距離
    float verticalOffset_ = 1.0f; // 垂直方向のオフセット

    std::unique_ptr<ParticleEmitter> chageEmitter_;
    std::unique_ptr<ParticleEmitter> bulletEmitter_;
};