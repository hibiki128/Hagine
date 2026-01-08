#define NOMINMAX
#include "Enemy.h"
#include "BehaviorTree/BehaviorNode/BehaviorNode.h"
#include "BehaviorTree/Nodes/ActionNodes.h"
#include "BehaviorTree/Nodes/CompositeNodes.h"
#include "BehaviorTree/Nodes/ConditionNodes.h"
#include "Particle/ParticleEditor.h"
#include "application/GameObject/Player/Bullet/ChargeShot/ChargeShot.h"
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
    enemyCollider_->AddCollisionMask("PlayerBullet");
    enemyCollider_->AddCollisionMask("Player");
    enemyCollider_->AddCollisionMask("PlayerHand");
    enemyCollider_->AddCollisionMask("PlayerChargeBullet");
    enemyCollider_->AddCollisionMask("makan");

    enemyCollider_->SetOnCollisionEnter([this](ColliderBase *other) {
        this->OnCollisionEnter(other);
    });

    BaseObject::SetTexture("debug/white1x1.png", kTextureIndex);
    BaseObject::SetColor(Vector4(kColorRed, kColorZero, kColorZero, kColorOpaque));
    shadow_ = std::make_unique<BaseObject>();
    shadow_->Init("shadow");
    shadow_->CreatePrimitiveModel(PrimitiveType::Plane);
    shadow_->SetTexture("game/shadow.png");
    shadow_->GetWorldTransform()->SetRotationEuler(Vector3(degreesToRadians(kShadowRotationDegrees), kRotationZero, kRotationZero));
    shadow_->GetLocalScale() = {kShadowScale, kShadowScale, kShadowScale};
    hitEmitter_ = ParticleEditor::GetInstance()->CreateEmitterFromTemplate("smokeEmitter");
    chageShake_ = std::make_unique<Shake>();
    isGuarding_ = false;
    // ビヘイビアツリーの初期化
    InitializeBehaviorTree();
}

void Enemy::Update() {
    shadow_->GetLocalPosition() = {transform_->translation_.x, kShadowYPosition, transform_->translation_.z};
    shadow_->Update();

    if (started_) {
        if (damage_ > kNoDamage) {
            float actualDamage = static_cast<float>(damage_);

            if (isGuarding_) {
                actualDamage *= kGuardDamageMultiplier;
            }

            HP_ -= actualDamage;
            damage_ = kNoDamage;
        }

        if (HP_ <= kMinHP) {
            isAlive_ = false;
            HP_ = kMinHP;
        }

        if (isGuarding_) {
            const float blinkInterval = kBlinkInterval;
            int blinkCount = static_cast<int>(Frame::Time() / blinkInterval);
            if (blinkCount % kBlinkModulo == kEvenBlink) {
                SetColor(Vector4(kColorOpaque, kColorZero, kColorZero, kColorOpaque));
            } else {
                SetColor(Vector4(kColorOpaque, kColorOpaque, kColorOpaque, kColorOpaque));
            }
        } else {
            SetColor(Vector4(kColorOpaque, kColorZero, kColorZero, kColorOpaque));
        }

        if (isAlive_ && target_->GetAlive()) {
            ExecuteBehaviorTree(Frame::DeltaTime());
        }

        // ダメージリアクション中でない時のみ向きを更新
        if (!isDamageReact_) {
            RotateUpdate();
        }

        UpdateShadowScale();
        chageShake_->Update();

        if (isDamageReact_) {
            damageReactTimer_ += Frame::DeltaTime();

            // イージングされたX回転角(ラジアン)
            float angleX = tiltEase_.Update(Frame::DeltaTime());

            // ワールド空間のX軸で回転を作成
            tiltRotation_ = Quaternion::FromAxisAngle(Vector3(kXAxisX, kXAxisY, kXAxisZ), angleX);

            // 重要: tiltRotation_ × baseRotation_ の順序（ワールド空間での回転を先に適用）
            transform_->quateRotation_ = tiltRotation_ * baseRotation_;

            // 高速点滅
            float blinkInterval = kDamageBlinkInterval;
            int blink = static_cast<int>(damageReactTimer_ / blinkInterval);
            SetAlpha((blink % kBlinkModulo == kEvenBlink) ? kAlphaTransparent : kAlphaOpaque);

            // 終了処理
            if (damageReactTimer_ >= damageReactDuration_) {
                isDamageReact_ = false;
                transform_->quateRotation_ = baseRotation_;
                SetAlpha(kAlphaOpaque);
            }
        }
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
    auto farDistCheck = std::make_unique<DistanceCheckNode>(kFarDistanceMin, kFarDistanceMax);
    auto fastApproach = std::make_unique<ApproachNode>(MoveSpeedType::Fast);
    auto farSequence = std::make_unique<SequenceNode>();
    farSequence->AddChild(std::move(farDistCheck));
    farSequence->AddChild(std::move(fastApproach));

    //--- 中距離時の接近 ---
    auto midDistCheck = std::make_unique<DistanceCheckNode>(kMidDistanceMin, kMidDistanceMax);
    auto slowApproach = std::make_unique<ApproachNode>(MoveSpeedType::Slow);
    auto midSequence = std::make_unique<SequenceNode>();
    midSequence->AddChild(std::move(midDistCheck));
    midSequence->AddChild(std::move(slowApproach));

    //--- 近距離時の行動 ---
    auto closeDistCheck = std::make_unique<DistanceCheckNode>(kCloseDistanceMin, kCloseDistanceMax);
    auto closeApproach = std::make_unique<CloseApproachNode>();
    auto strafe = std::make_unique<StrafeNode>();
    auto retreat = std::make_unique<RetreatNode>();
    auto guard = std::make_unique<GuardNode>();

    auto weightedSelector = std::make_unique<WeightedSelectorNode>();
    weightedSelector->AddChild(std::move(closeApproach), kCloseApproachWeight);
    weightedSelector->AddChild(std::move(strafe), kStrafeWeight);
    weightedSelector->AddChild(std::move(retreat), kRetreatWeight);
    weightedSelector->AddChild(std::move(guard), kGuardWeight);

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
        velocity_.x = kVelocityZero;
        velocity_.z = kVelocityZero;
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
    if (transform_->translation_.y < kGroundLevel) {
        return;
    }
    shadow_->SetIsModelDraw(drawShadow_);
    shadow_->Draw(viewProjection, offSet);
}

void Enemy::DrawParticle(const ViewProjection &viewProjection) {
    hitEmitter_->Draw(viewProjection);
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
        hitEmitter_->SetPosition(transform_->translation_);
        hitEmitter_->UpdateOnce();
    }
    if (other->GetTag() == "PlayerChargeBullet" || other->GetTag() == "Makan") {
        chageShake_->StartShake();
    }

    // ダメージリアクション開始
    isDamageReact_ = true;
    damageReactTimer_ = kTimerReset;

    // 今の向きを保存
    baseRotation_ = transform_->quateRotation_;

    // X軸回転のみをイージング（上向きに8度）
    float startAngle = transform_->quateRotation_.x;
    float endAngle = degreesToRadians(kDamageTiltDegrees);
    tiltEase_.Reset(startAngle, endAngle, damageReactDuration_, EasingType::OutQuad);
}

Vector3 Enemy::GetMovementDirection() const {
    return Vector3();
}

float Enemy::GetVelocityMagnitude() const {
    return kVelocityZero;
}

void Enemy::Save() {
}

void Enemy::Load() {
}

void Enemy::UpdateShadowScale() {
    if (transform_->translation_.y < kGroundLevel) {
        return;
    }
    float height = transform_->translation_.y;
    float baseScale = kShadowBaseScale;
    float scaleFactor = std::max(kShadowMinScale, baseScale - height * kShadowScaleFactor);
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
    if (toTarget.Length() < kMinRotationDistance) {
        return;
    }

    // 正規化して方向ベクトルに
    toTarget = toTarget.Normalize();

    // プレイヤーと同様に基準ベクトルを作成
    Vector3 forward = toTarget;                             // 敵の正面方向（ターゲット方向）
    Vector3 worldUp = {kUpVectorX, kUpVectorY, kUpVectorZ}; // 上方向
    Vector3 right;                                          // 右方向

    // forwardとupがほぼ平行なら補正
    if (std::abs(forward.Dot(worldUp)) > kParallelThreshold) {
        right = {kRightVectorX, kRightVectorY, kRightVectorZ};
    } else {
        right = (worldUp.Cross(forward)).Normalize();
    }

    // upを再計算して正規直交化
    Vector3 up = (forward.Cross(right)).Normalize();

    // 回転行列からクォータニオンを生成
    Matrix4x4 rotMatrix = MakeRotateMatrix(right, up, forward);
    Quaternion targetRot = Quaternion::FromMatrix(rotMatrix);

    // 回転速度（大きいほど素早く向く）
    float rotateSpeed = kRotationSpeed;
    transform_->quateRotation_ = Quaternion::Slerp(
        transform_->quateRotation_,
        targetRot,
        rotateSpeed * Frame::DeltaTime());
}

void Enemy::CollisionGround() {
    float nextY = GetLocalPosition().y + velocity_.y * Frame::DeltaTime();

    GetLocalPosition().x += velocity_.x * Frame::DeltaTime();
    GetLocalPosition().z += velocity_.z * Frame::DeltaTime();

    if (nextY <= kGroundLevel) {
        GetLocalPosition().y = kGroundLevel;

        if (!isGrounded_) {
            velocity_.y = kVelocityZero;
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
    return TransformNormal(Vector3(kForwardVectorX, kForwardVectorY, kForwardVectorZ), QuaternionToMatrix4x4(transform_->quateRotation_));
}

Vector3 Enemy::GetBackward() const {
    return -GetForward();
}

Vector3 Enemy::GetRight() const {
    // クォータニオンから右方向ベクトルを計算（X軸の正方向が右方向）
    return TransformNormal(Vector3(kRightVectorX, kRightVectorY, kRightVectorZ), QuaternionToMatrix4x4(transform_->quateRotation_));
}

Vector3 Enemy::GetLeft() const {
    return -GetRight();
}

Vector3 Enemy::GetUp() const {
    // クォータニオンから上方向ベクトルを計算（Y軸の正方向が上方向）
    return TransformNormal(Vector3(kUpVectorX, kUpVectorY, kUpVectorZ), QuaternionToMatrix4x4(transform_->quateRotation_));
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