#include "Enemy.h"

#include "application/GameObject/Player/Bullet/PlayerBullet.h"
#include "application/GameObject/Player/Bullet/ChageShot/ChageShot.h"
#include "Particle/ParticleEditor.h"

Enemy::Enemy() {
}

Enemy::~Enemy() {
}

void Enemy::Init(const std::string objectName) {

    BaseObject::Init(objectName);
    BaseObject::CreatePrimitiveModel(PrimitiveType::Cube);
    BaseObject::AddCollider();
    BaseObject::SetCollisionType(CollisionType::Sphere);
    BaseObject::objColor_.GetColor() = Vector4(1, 0, 0, 1);
    shadow_ = std::make_unique<BaseObject>();
    shadow_->Init("shadow");
    shadow_->CreatePrimitiveModel(PrimitiveType::Plane);
    shadow_->SetTexture("game/shadow.png");
    shadow_->GetWorldRotation().x = degreesToRadians(90.0f);
    shadow_->GetWorldScale() = {1.5f, 1.5f, 1.5f};
    emitter_ = ParticleEditor::GetInstance()->CreateEmitterFromTemplate("hitEmitter");
}

void Enemy::Update() {
   shadow_->GetWorldPosition() = {transform_.translation_.x, -0.95f, transform_.translation_.z};
    shadow_->Update();
}

void Enemy::Draw(const ViewProjection &viewProjection, Vector3 offSet) {
    shadow_->Draw(viewProjection, offSet);
    BaseObject::Draw(viewProjection, offSet);
}

void Enemy::DrawParticle(const ViewProjection &viewProjection) {
    emitter_->Draw(viewProjection);
}

void Enemy::Debug() {
}

void Enemy::OnCollisionEnter(Collider *other) {
    if (dynamic_cast<PlayerBullet *>(other)||dynamic_cast<ChageShot*>(other)) {
        emitter_->UpdateOnce();
    }
}

Vector3 Enemy::GetMovementDirection() const {
    return Vector3();
}

float Enemy::GetVelocityMagnitude() const {
    return 0.0f;
}

void Enemy::Save() {
}

void Enemy::Load() {
}

void Enemy::RotateUpdate() {
}

void Enemy::CollisionGround() {
}

Direction Enemy::CalculateDirectionFromRotation() {
    return Direction();
}

const char *Enemy::GetDirectionName(Direction dir) {
    return nullptr;
}
