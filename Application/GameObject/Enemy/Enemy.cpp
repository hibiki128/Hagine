#define NOMINMAX
#include "Enemy.h"
#include "Particle/ParticleEditor.h"
#include "application/GameObject/Player/Bullet/ChageShot/ChageShot.h"
#include "application/GameObject/Player/Bullet/PlayerBullet.h"
#include <Frame.h>
#include "BehaviorTree/Nodes/SequenceNode.h"
#include "BehaviorTree/Nodes/ConditionNodes.h"
#include "BehaviorTree/Nodes/ActionNodes.h"

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
    BaseObject::SetColor(Vector4(1, 0, 0, 1));
    shadow_ = std::make_unique<BaseObject>();
    shadow_->Init("shadow");
    shadow_->CreatePrimitiveModel(PrimitiveType::Plane);
    shadow_->SetTexture("game/shadow.png");
    shadow_->GetWorldTransform()->SetRotationEuler(Vector3(degreesToRadians(-90.0f), 0.0f, 0.0f));
    shadow_->GetLocalScale() = {1.5f, 1.5f, 1.5f};
    emitter_ = ParticleEditor::GetInstance()->CreateEmitterFromTemplate("hitEmitter");
    chageShake_ = std::make_unique<Shake>();
}

void Enemy::InitializeBehaviorTree() {
    // ルートは Selector（最初に成功したノードを実行）
    auto root = std::make_unique<SelectorNode>();

    // =========== Branch 1: プレイヤーが空中 ===========
    auto playerAirborneSequence = std::make_unique<SequenceNode>();
    playerAirborneSequence->AddChild(std::make_unique<IsPlayerAirborneNode>());

    // 距離に応じた優先度制御用 Selector
    auto airborneDistanceSelector = std::make_unique<SelectorNode>();

    // 距離が遠い場合（>15.0f）：Rush を優先
    auto farRushSequence = std::make_unique<SequenceNode>();
    farRushSequence->AddChild(std::make_unique<DistanceThresholdNode>(15.0f));
    farRushSequence->AddChild(std::make_unique<RushAttackNode>(250.0f, 3.0f));
    airborneDistanceSelector->AddChild(std::move(farRushSequence));

    // 中距離（7.0f～15.0f）：通常の飛行移動
    auto midRangeSequence = std::make_unique<SequenceNode>();
    midRangeSequence->AddChild(std::make_unique<DistanceToTargetNode>(7.0f, 15.0f));
    midRangeSequence->AddChild(std::make_unique<FlyToTargetNode>(100.0f));
    airborneDistanceSelector->AddChild(std::move(midRangeSequence));

    // 近距離（<7.0f）：通常の飛行移動
    auto closeRangeSequence = std::make_unique<SequenceNode>();
    closeRangeSequence->AddChild(std::make_unique<DistanceToTargetNode>(0.0f, 7.0f));
    closeRangeSequence->AddChild(std::make_unique<FlyToTargetNode>(80.0f));
    airborneDistanceSelector->AddChild(std::move(closeRangeSequence));

    playerAirborneSequence->AddChild(std::move(airborneDistanceSelector));
    root->AddChild(std::move(playerAirborneSequence));

    // =========== Branch 2: プレイヤーが地上 ===========
    auto playerGroundedSequence = std::make_unique<SequenceNode>();
    playerGroundedSequence->AddChild(std::make_unique<HasTargetNode>());

    auto groundedDistanceSelector = std::make_unique<SelectorNode>();

    // 距離が遠い場合（>15.0f）：Rush を優先
    auto groundFarRushSequence = std::make_unique<SequenceNode>();
    groundFarRushSequence->AddChild(std::make_unique<DistanceThresholdNode>(15.0f));
    groundFarRushSequence->AddChild(std::make_unique<RushAttackNode>(200.0f, 3.0f));
    groundedDistanceSelector->AddChild(std::move(groundFarRushSequence));

    // 中距離：通常の移動
    auto groundMidSequence = std::make_unique<SequenceNode>();
    groundMidSequence->AddChild(std::make_unique<DistanceToTargetNode>(3.0f, 15.0f));
    groundMidSequence->AddChild(std::make_unique<MoveToTargetNode>(80.0f));
    groundedDistanceSelector->AddChild(std::move(groundMidSequence));

    // 近距離：ゆっくり移動
    auto groundCloseSequence = std::make_unique<SequenceNode>();
    groundCloseSequence->AddChild(std::make_unique<DistanceToTargetNode>(0.0f, 3.0f));
    groundCloseSequence->AddChild(std::make_unique<MoveToTargetNode>(50.0f));
    groundedDistanceSelector->AddChild(std::move(groundCloseSequence));

    playerGroundedSequence->AddChild(std::move(groundedDistanceSelector));
    root->AddChild(std::move(playerGroundedSequence));

    // =========== Fallback: 何もできない場合 ===========
    auto idleSequence = std::make_unique<SequenceNode>();
    idleSequence->AddChild(std::make_unique<ApplyGravityNode>());
    root->AddChild(std::move(idleSequence));

    behaviorRoot_ = std::move(root);
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

    // ビヘイビアツリー実行
    if (behaviorRoot_) {
        behaviorRoot_->Execute(*this, Frame::DeltaTime());
    }

    // 位置更新
    CollisionGround();

    UpdateShadowScale();
    chageShake_->Update();
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
    if (dynamic_cast<PlayerBullet *>(other) || dynamic_cast<ChargeShot *>(other)) {
        emitter_->UpdateOnce();
    }

    if (dynamic_cast<ChargeShot *>(other)) {
        chageShake_->StartShake();
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
    float nextY = GetLocalPosition().y + velocity_.y * Frame::DeltaTime();

    GetLocalPosition().x += velocity_.x * Frame::DeltaTime();
    GetLocalPosition().z += velocity_.z * Frame::DeltaTime();

    if (nextY <= 0.0f) {
        GetLocalPosition().y = 0.0f;

        if (!isGrounded_) {
            velocity_.y = 0.0f;
            isGrounded_ = true;
        }
    } else {
        GetLocalPosition().y = nextY;
        isGrounded_ = false;
    }
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

void Enemy::SetVp(ViewProjection *vp) {
    chageShake_->Initialize(vp, "chagehit");
}