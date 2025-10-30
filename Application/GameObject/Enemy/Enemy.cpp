#define NOMINMAX
#include "Enemy.h"
#include "Particle/ParticleEditor.h"
#include "application/GameObject/Player/Bullet/ChageShot/ChageShot.h"
#include "application/GameObject/Player/Bullet/PlayerBullet.h"
#include <Frame.h>
#include "BehaviorTree/BehaviorNode/BehaviorNode.h"
#include "BehaviorTree/Nodes/ActionNodes.h"
#include "BehaviorTree/Nodes/ConditionNodes.h"
#include "BehaviorTree/Nodes/CompositeNodes.h"
#ifdef _DEBUG
#include "BehaviorTree/Editor/BehaviorTreeEditor.h"
#endif

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

    // ビヘイビアツリーの初期化
    InitializeBehaviorTree();
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

    // ビヘイビアツリーの実行
    if (isAlive_) {
        ExecuteBehaviorTree(Frame::DeltaTime());
    }

    // 位置更新
    CollisionGround();

    UpdateShadowScale();
    chageShake_->Update();
}

void Enemy::InitializeBehaviorTree() {
#ifdef _DEBUG
    behaviorTreeEditor_ = std::make_unique<BehaviorTreeEditor>();
    BehaviorNode::SetEditor(behaviorTreeEditor_.get());
#endif

    // ルートノード(シーケンス)を作成
    auto rootSequence = std::make_unique<SequenceNode>();

    // 1. 遠距離時の接近行動
    auto farDistCheck = std::make_unique<DistanceCheckNode>(10.0f, 100.0f);
    auto fastApproach = std::make_unique<ApproachNode>(MoveSpeedType::Fast);

    auto farSequence = std::make_unique<SequenceNode>();
    farSequence->AddChild(std::move(farDistCheck));
    farSequence->AddChild(std::move(fastApproach));

    // 2. 中距離時の接近行動
    auto midDistCheck = std::make_unique<DistanceCheckNode>(5.0f, 10.0f);
    auto slowApproach = std::make_unique<ApproachNode>(MoveSpeedType::Slow);

    auto midSequence = std::make_unique<SequenceNode>();
    midSequence->AddChild(std::move(midDistCheck));
    midSequence->AddChild(std::move(slowApproach));

    // 3. 近距離時の重み付き行動選択
    auto closeDistCheck = std::make_unique<DistanceCheckNode>(0.0f, 5.0f);

    // 近距離での3つの行動オプション
    auto closeApproach = std::make_unique<CloseApproachNode>();
    auto strafe = std::make_unique<StrafeNode>();
    auto retreat = std::make_unique<RetreatNode>();

    // 重み付きセレクターを作成
    auto weightedSelector = std::make_unique<WeightedSelectorNode>();
    weightedSelector->AddChild(std::move(closeApproach), 1.0f); // さらに近づく: 重み1.0
    weightedSelector->AddChild(std::move(strafe), 1.5f);        // 横移動: 重み1.5
    weightedSelector->AddChild(std::move(retreat), 1.0f);       // 後退: 重み1.0

    auto closeSequence = std::make_unique<SequenceNode>();
    closeSequence->AddChild(std::move(closeDistCheck));
    closeSequence->AddChild(std::move(weightedSelector));

    // 4. 全体をセレクターでまとめる
    auto mainSelector = std::make_unique<SelectorNode>();
    mainSelector->AddChild(std::move(closeSequence)); // 近距離を最優先
    mainSelector->AddChild(std::move(midSequence));   // 中距離
    mainSelector->AddChild(std::move(farSequence));   // 遠距離

    // 停止ノードをフォールバック用に追加
    auto stopNode = std::make_unique<StopNode>();
    mainSelector->AddChild(std::move(stopNode));

    rootSequence->AddChild(std::move(mainSelector));

    behaviorTreeRoot_ = std::move(rootSequence);

#ifdef _DEBUG
    // エディターに設定をロード
    behaviorTreeEditor_->LoadSettings("EnemyTree", behaviorTreeRoot_.get());
#endif
}

void Enemy::ExecuteBehaviorTree(float deltaTime) {
    if (!behaviorTreeRoot_) {
        return;
    }

#ifdef _DEBUG
    if (behaviorTreeEditor_) {
        behaviorTreeEditor_->ClearExecutingNode();
    }
#endif

    // ツリーを実行(各ノード内でエディター通知される)
    NodeStatus status = behaviorTreeRoot_->Execute(*this, deltaTime);

    (void)status;
}

void Enemy::DrawBehaviorTreeEditor() {
#ifdef _DEBUG
    if (behaviorTreeEditor_) {
        behaviorTreeEditor_->DrawEditor(behaviorTreeRoot_.get());
    }
#endif
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