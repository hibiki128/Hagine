#define NOMINMAX
#include "Enemy.h"
#include "Particle/ParticleEditor.h"
#include "application/GameObject/Player/Bullet/ChageShot/ChageShot.h"
#include "application/GameObject/Player/Bullet/PlayerBullet.h"

Enemy::Enemy() {
}

Enemy::~Enemy() {
}

void Enemy::Init(const std::string objectName) {

    BaseObject::Init(objectName);
    BaseObject::CreatePrimitiveModel(PrimitiveType::Cube);
    BaseObject::AddCollider();
    BaseObject::SetCollisionType(CollisionType::OBB);
    BaseObject::SetTexture("debug/white1x1.png", 0);
    BaseObject::objColor_.GetColor() = Vector4(1, 0, 0, 1);
    shadow_ = std::make_unique<BaseObject>();
    shadow_->Init("shadow");
    shadow_->CreatePrimitiveModel(PrimitiveType::Plane);
    shadow_->SetTexture("game/shadow.png");
    shadow_->GetWorldTransform()->SetRotationEuler(Vector3(degreesToRadians(-90.0f), 0.0f, 0.0f));
    shadow_->GetLocalScale() = {1.5f, 1.5f, 1.5f};
    emitter_ = ParticleEditor::GetInstance()->CreateEmitterFromTemplate("hitEmitter");
}

void Enemy::Update() {
    shadow_->GetLocalPosition() = {transform_->translation_.x, -0.95f, transform_->translation_.z};
    shadow_->Update();

    if (damage_ > 0) {
        HP_ -= damage_;
        damage_ = 0;
    }
    if (HP_ <= 0) {
        isAlive_ = false;
        HP_ = 0;
    }
    UpdateShadowScale();
}

void Enemy::Draw(const ViewProjection &viewProjection, Vector3 offSet) {
    if (!isAlive_) {
        return;
    }
    BaseObject::Draw(viewProjection, offSet);
    if (transform_->translation_.y < 0) {
        return;
    }
    shadow_->Draw(viewProjection, offSet);
}

void Enemy::DrawParticle(const ViewProjection &viewProjection) {
    emitter_->Draw(viewProjection);
}

void Enemy::Debug() {
    if (ImGui::BeginTabBar("敵情報")) {
        if (ImGui::BeginTabItem("敵情報")) {

            ImGui::Text("敵のHP %d", HP_);
            if (ImGui::Button("HP回復")) {
                HP_ = maxHP_;
            }

            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
}

void Enemy::OnCollisionEnter(Collider *other) {
    if (dynamic_cast<PlayerBullet *>(other) || dynamic_cast<ChageShot *>(other)) {
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

void Enemy::UpdateShadowScale() {
    if (transform_->translation_.y < 0) {
        return;
    }
    float height = transform_->translation_.y;
    float baseScale = 1.5f;
    float scaleFactor = std::max(0.3f, baseScale - height * 0.1f);
    shadow_->GetLocalScale() = {scaleFactor, scaleFactor, scaleFactor};
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

Vector3 Enemy::GetForward() const {
    // クォータニオンから前方向ベクトルを計算（Z軸の負方向が前方向）
    return TransformNormal(Vector3(0.0f, 0.0f, -1.0f), QuaternionToMatrix4x4(transform_->quateRotation_));
}

Vector3 Enemy::GetBackward() const {
    return -GetForward();
}

Vector3 Enemy::GetRight() const {
    // クォータニオンから右方向ベクトルを計算（X軸の正方向が右方向）
    return TransformNormal(Vector3(1.0f, 0.0f, 0.0f), QuaternionToMatrix4x4(transform_->quateRotation_));
}

Vector3 Enemy::GetLeft() const {
    return -GetRight();
}

Vector3 Enemy::GetUp() const {
    // クォータニオンから上方向ベクトルを計算（Y軸の正方向が上方向）
    return TransformNormal(Vector3(0.0f, 1.0f, 0.0f), QuaternionToMatrix4x4(transform_->quateRotation_));
}

Vector3 Enemy::GetDown() const {
    return -GetUp();
}

Vector3 Enemy::GetPositionBehind(float distance) const {
    return transform_->translation_ + GetBackward() * distance;
}

Vector3 Enemy::GetPositionFront(float distance) const {
    return transform_->translation_ + GetForward() * distance;
}

Vector3 Enemy::GetPositionRight(float distance) const {
    return transform_->translation_ + GetRight() * distance;
}

Vector3 Enemy::GetPositionLeft(float distance) const {
    return transform_->translation_ + GetLeft() * distance;
}

Vector3 Enemy::GetPositionAbove(float distance) const {
    return transform_->translation_ + GetUp() * distance;
}

Vector3 Enemy::GetPositionBelow(float distance) const {
    return transform_->translation_ + GetDown() * distance;
}