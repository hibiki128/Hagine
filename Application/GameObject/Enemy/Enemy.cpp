#define NOMINMAX
#include "Enemy.h"
#include "BehaviorTree/BehaviorNode/BehaviorNode.h"
#include "BehaviorTree/Nodes/ActionNodes.h"
#include "BehaviorTree/Nodes/CompositeNodes.h"
#include "BehaviorTree/Nodes/ConditionNodes.h"
#include "Particle/ParticleEditor.h"
#include "application/GameObject/Player/Bullet/ChageShot/ChageShot.h"
#include "application/GameObject/Player/Bullet/PlayerBullet.h"
#include <Frame.h>
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
    enemyCollider_ = AddOBBCollider("enemy_Collider");
    enemyCollider_->SetTag("Enemy");
    enemyCollider_->AddCollisionMask("player_bullet");
    enemyCollider_->AddCollisionMask("Player");
    enemyCollider_->AddCollisionMask("PlayerHand");
    enemyCollider_->AddCollisionMask("PlayerChargeBullet");
    enemyCollider_->AddCollisionMask("makan");

    enemyCollider_->SetOnCollisionEnter([this](ColliderBase *other) {
        this->OnCollisionEnter(other);
    });

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
    isGuarding_ = false;
    // ビヘイビアツリーの初期化
    InitializeBehaviorTree();
}

void Enemy::Update() {
    shadow_->GetLocalPosition() = {transform_->translation_.x, -0.95f, transform_->translation_.z};
    shadow_->Update();

    if (started_) {
        if (damage_ > 0) {
            float actualDamage = static_cast<float>(damage_);

            // ガード中はダメージを85%軽減
            if (isGuarding_) {
                actualDamage *= 0.15f;
            }

            HP_ -= actualDamage;
            damage_ = 0;
        }

        if (HP_ <= 0.0f) {
            isAlive_ = false;
            HP_ = 0.0f;
        }

        // ガード中点滅処理
        if (isGuarding_) {
            const float blinkInterval = 0.1f;
            int blinkCount = static_cast<int>(Frame::Time() / blinkInterval);
            if (blinkCount % 2 == 0) {
                SetColor(Vector4(1.0f, 0.0f, 0.0f, 1.0f));
            } else {
                SetColor(Vector4(1.0f, 1.0f, 1.0f, 1.0f));
            }
        } else {
            SetColor(Vector4(1.0f, 0.0f, 0.0f, 1.0f));
        }

        // 生存中のみ行動
        if (isAlive_ && target_->GetAlive()) {
            ExecuteBehaviorTree(Frame::DeltaTime());
        }

        // 向きを更新
        RotateUpdate();

        UpdateShadowScale();
        chageShake_->Update();
    }

    CollisionGround();
}

void Enemy::InitializeBehaviorTree() {
#ifdef _DEBUG
    behaviorTreeEditor_ = std::make_unique<BehaviorTreeEditor>();
    BehaviorNode::SetEditor(behaviorTreeEditor_.get());
#endif

    //==================== 通常行動ツリー構築 ====================

    //--- 遠距離時の接近 ---
    auto farDistCheck = std::make_unique<DistanceCheckNode>(10.0f, 100.0f);
    auto fastApproach = std::make_unique<ApproachNode>(MoveSpeedType::Fast);
    auto farSequence = std::make_unique<SequenceNode>();
    farSequence->AddChild(std::move(farDistCheck));
    farSequence->AddChild(std::move(fastApproach));

    //--- 中距離時の接近 ---
    auto midDistCheck = std::make_unique<DistanceCheckNode>(5.0f, 10.0f);
    auto slowApproach = std::make_unique<ApproachNode>(MoveSpeedType::Slow);
    auto midSequence = std::make_unique<SequenceNode>();
    midSequence->AddChild(std::move(midDistCheck));
    midSequence->AddChild(std::move(slowApproach));

    //--- 近距離時の行動 ---
    auto closeDistCheck = std::make_unique<DistanceCheckNode>(0.0f, 5.0f);
    auto closeApproach = std::make_unique<CloseApproachNode>();
    auto strafe = std::make_unique<StrafeNode>();
    auto retreat = std::make_unique<RetreatNode>();
    auto guard = std::make_unique<GuardNode>();

    auto weightedSelector = std::make_unique<WeightedSelectorNode>();
    weightedSelector->AddChild(std::move(closeApproach), 1.0f);
    weightedSelector->AddChild(std::move(strafe), 1.5f);
    weightedSelector->AddChild(std::move(retreat), 1.0f);
    weightedSelector->AddChild(std::move(guard), 1.2f);

    auto closeSequence = std::make_unique<SequenceNode>();
    closeSequence->AddChild(std::move(closeDistCheck));
    closeSequence->AddChild(std::move(weightedSelector));

    //--- 全距離まとめ ---
    auto mainSelector = std::make_unique<SelectorNode>();
    mainSelector->AddChild(std::move(closeSequence));
    mainSelector->AddChild(std::move(midSequence));
    mainSelector->AddChild(std::move(farSequence));

    //--- 停止ノードを最後に ---
    mainSelector->AddChild(std::make_unique<StopNode>());

    //==================== 割り込み対応ルート構築 ====================

    // InterruptSelectorNode が各子ノードの割り込み条件を監視
    auto interruptRoot = std::make_unique<InterruptSelectorNode>();

    // 通常行動全体をまとめて追加
    interruptRoot->AddChild(std::move(mainSelector));

    // ルートノード設定
    behaviorTreeRoot_ = std::move(interruptRoot);

#ifdef _DEBUG
    behaviorTreeEditor_->LoadSettings("default", behaviorTreeRoot_.get());
#endif // _DEBUG
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

    if (isStop_) {
        velocity_.x = 0.0f;
        velocity_.z = 0.0f;
        return;
    }

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
        enemyCollider_->SetEnabled(false);
        return;
    }
    BaseObject::Draw(viewProjection, offSet);
    if (transform_->translation_.y < 0) {
        return;
    }
    shadow_->SetIsModelDraw(drawShadow_);
    shadow_->Draw(viewProjection, offSet);
}

void Enemy::DrawParticle(const ViewProjection &viewProjection) {
    emitter_->Draw(viewProjection);
}

void Enemy::Debug() {
#ifdef USE_IMGUI
    if (ImGui::BeginTabBar("敵情報")) {
        if (ImGui::BeginTabItem("敵情報")) {

            ImGui::Text("敵のHP %.1f / %.1f", HP_, maxHP_);
            if (ImGui::Button("HP回復")) {
                HP_ = maxHP_;
            }

            ImGui::Checkbox("ストップ", &isStop_);

            // ガード状態の表示
            ImGui::Separator();
            ImGui::Text("ガード状態: %s", isGuarding_ ? "ON" : "OFF");
            if (isGuarding_) {
                ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f),
                                   "ダメージ85%%軽減中");
            }

            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
#endif // USE_IMGUI
}

void Enemy::OnCollisionEnter(ColliderBase *other) {
    if (other->GetTag() == "PlayerBullet" || other->GetTag() == "PlayerChargeBullet" || other->GetTag() == "Makan") {
        emitter_->SetPosition(transform_->translation_);
        emitter_->UpdateOnce();
    }
    if (other->GetTag() == "PlayerChargeBullet" || other->GetTag() == "Makan") {
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
    // ターゲットがいなければ何もしない
    if (!target_) {
        return;
    }

    // 自分とターゲットのワールド座標を取得
    Vector3 toTarget = target_->GetWorldPosition() - GetWorldPosition();

    // ほぼ同じ位置なら回転しない
    if (toTarget.Length() < 0.001f) {
        return;
    }

    // 正規化して方向ベクトルに
    toTarget = toTarget.Normalize();

    // プレイヤーと同様に基準ベクトルを作成
    Vector3 forward = toTarget;           // 敵の正面方向（ターゲット方向）
    Vector3 worldUp = {0.0f, 1.0f, 0.0f}; // 上方向
    Vector3 right;                        // 右方向

    // forwardとupがほぼ平行なら補正
    if (std::abs(forward.Dot(worldUp)) > 0.999f) {
        right = {1.0f, 0.0f, 0.0f};
    } else {
        right = (worldUp.Cross(forward)).Normalize();
    }

    // upを再計算して正規直交化
    Vector3 up = (forward.Cross(right)).Normalize();

    // 回転行列からクォータニオンを生成
    Matrix4x4 rotMatrix = MakeRotateMatrix(right, up, forward);
    Quaternion targetRot = Quaternion::FromMatrix(rotMatrix);

    // 回転速度（大きいほど素早く向く）
    float rotateSpeed = 8.0f;
    transform_->quateRotation_ = Quaternion::Slerp(
        transform_->quateRotation_,
        targetRot,
        rotateSpeed * Frame::DeltaTime());
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