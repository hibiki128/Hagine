#define NOMINMAX
#include "Enemy.h"
#include "collider/CollisionManager.h"
#include "particle/ParticleEditor.h"
#include "utility/debug/imgui/ImGuiNotification.h"
#include <Application/camera/follow/FollowCamera.h>
#include <3d/line/DrawLine3D.h>
#include <Frame.h>
#include <utility/debug/param/GameParamHub.h>
#include <algorithm>
#include <array>
#include <cmath>

using namespace Hagine;

Enemy::Enemy()
{
    movement_ = std::make_unique<EnemyMovement>();
    combat_ = std::make_unique<EnemyCombat>();
    status_ = std::make_unique<EnemyStatus>();
    visual_ = std::make_unique<EnemyVisual>();
}

Enemy::~Enemy()
{
    // ポインタ失効前にゲームパラメータHubから登録を解除する
    // （"必殺演出(Enemy)" は EnemyCombat のデストラクタで解除する）
    GameParamHub::GetInstance()->Unregister("Enemy");
}

void Enemy::Init(const std::string objectName)
{
    BaseObject::Init(objectName);

    // プレイヤーと同じスケルトン付きモデルを使い、同一クリップでアニメーションさせる。
    // モデル素体のマテリアル色は赤なので、敵はそのまま赤で表示される
    BaseObject::CreateModel("animation/Player/Idle_Ground.gltf");
    BaseObject::SetOffset({0.0f, kModelOffsetY, 0.0f}); // 描画オフセット（足が地面につくように）
    BaseObject::GetLocalScale() = {4.0f, 4.0f, 4.0f};
    BaseObject::SetAnimationSpeed(1.0f);
    BaseObject::SetAnimationBlendDuration(0.2f);

    pEnemyCollider_ = AddOBBCollider("enemy_Collider");
    pEnemyCollider_->SetTag("Enemy");
    pEnemyCollider_->AddCollisionMask("PlayerBullet");
    pEnemyCollider_->AddCollisionMask("Player");
    pEnemyCollider_->AddCollisionMask("PlayerHand"); // PlayerAttackColliderのタグ
    pEnemyCollider_->AddCollisionMask("PlayerChargeBullet");
    pEnemyCollider_->AddCollisionMask("makan");
    pEnemyCollider_->AddCollisionMask("CylinderField");

    pEnemyWallCollider_ = AddAABBCollider("enemy_WallCollider");
    pEnemyWallCollider_->SetTag("EnemyWall");
    pEnemyWallCollider_->AddCollisionMask("PlayerWall");
    pEnemyWallCollider_->SetSize({2.75f, 1000.0f, 2.5f});

    pEnemyCollider_->SetOnCollisionEnter([this](ColliderBase *other) {
        this->OnCollisionEnter(other);
    });
    pEnemyCollider_->SetOnCollision([this](ColliderBase *other) {
        this->OnCollision(other);
    });
    pEnemyWallCollider_->SetOnCollision([this](ColliderBase *other) {
        this->OnCollision(other);
    });

    BaseObject::SetColor(Vector4(kColorRed, kColorZero, kColorZero, kColorOpaque));

    hitEmitter_ = ParticleEditor::GetInstance()->CreateEmitterFromTemplate("smokeEmitter");

    deathStaging_ = std::make_unique<DeathStaging>();

    // ─── 各パーツの初期化 ───
    movement_->Init(this);
    status_->Init(this);
    combat_->Init(this); // 大技演出エミッタ・ビーム判定・攻撃コライダー・コンボを生成する
    visual_->Init(this); // アニメーションコントローラを構築する

    transform_->SetRotationEuler({0.0f, degreesToRadians(180.0f), 0.0f});

    // ─── 調整パラメータをゲームパラメータHubへ登録 ───
    movement_->RegisterParams();
    status_->RegisterParams();
    combat_->RegisterParams();
}

void Enemy::Update()
{
    // 開始フラグが立っており、ポーズ中でなく、ターゲットが生きている場合に更新
    if (started_ && !isPause_ && pTarget_->GetIsAlive())
    {
        const float dt = Frame::DeltaTime();

        status_->DamageUpdate();
        status_->RecoverEnergy();

        // 死亡判定
        if (status_->GetHP() <= kMinHP)
        {
            if (dummyMode_)
            {
                // ダミーはHPが尽きたら即復活する（満タン・初期位置へ）
                Revive();
            }
            else if (isAlive_)
            {
                isAlive_ = false;
                status_->SetHP(kMinHP);

                // 必殺技の演出・ビームは死亡で打ち切り、死体に判定が残らないようにする
                combat_->CancelBeamStaging();
                combat_->DeactivateBeam();
                pEnemyCollider_->SetEnabled(false);
                pEnemyWallCollider_->SetEnabled(false);

                // 被弾点滅の途中（透明フレーム）で死ぬと見えないまま固まるため、不透明へ戻す
                SetAlpha(kAlphaOpaque);
            }
        }

        // 死亡中は行動処理を行わず、死亡アニメーションだけ再生して進める。
        // 再生し終わった後の粒子化演出は DrawParticle 側で描画する
        if (!isAlive_)
        {
            visual_->PlayDeathAnimation();
            combat_->UpdateEffects(dt); // ビーム等の残存パーティクルを自然消滅させる
            BaseObject::Update();       // アニメーション・行列の更新
            return;
        }

        // ガード中のエフェクト（点滅）
        visual_->UpdateGuardBlink();

        // ─── プレイヤーの必殺技カメラワーク中は完全停止させる ───
        // カメラワーク（顔アップ演出）中は相手も動けない、という仕様。
        // ただし自分がビーム発動者の場合(beamCutscene_ がアクティブ)は、照準追従の
        // 回転を維持したいのでここではロックしない（発動中の停止はBT側が速度0で担保）。
        FollowCamera *followCamera = pTarget_ ? pTarget_->GetCamera() : nullptr;
        const bool cameraCloseUp = followCamera && followCamera->IsSkillCloseUpActive();
        const bool frozenByOpponentSkill = cameraCloseUp && !combat_->IsBeamStaging();

        // 回転を更新（敵をプレイヤーへ向ける）
        if (!frozenByOpponentSkill)
        {
            movement_->RotateUpdate();
        }

        // ダミーモードでは自身の攻撃(コンボ)は行わない。カメラワーク中も攻撃させない
        if (!dummyMode_ && !frozenByOpponentSkill)
        {
            combat_->ComboUpdate();
        }

        // シェイク・大技演出エミッタ・発動前演出・ビームの毎フレーム更新
        combat_->UpdateEffects(dt);

        // ダメージリアクション処理（高速点滅のみ。傾き(のけぞり)演出は廃止）
        status_->UpdateDamageReact();

        // ビヘイビアツリーの更新（ダミーモードではAIを動かさない）
        if (frozenByOpponentSkill)
        {
            // プレイヤーの必殺技カメラワーク中は移動・重力ごと完全停止させ、その場に固定する
            movement_->Freeze();
        }
        else if (rootNode_ && !dummyMode_)
        {
            rootNode_->SetContext(this, pTarget_);
            rootNode_->Tick();

            // ガード中は移動させない（EnemyGuardNode が毎フレーム速度を0にしているため、
            // ここで移動イージングを適用すると追跡速度で上書きされて動いてしまう）
            if (status_->IsGuarding())
            {
                movement_->StopHorizontal();
            }
            // 速度イージングの更新（ガード中は適用しない）
            else
            {
                movement_->UpdateVelocityEase(dt);
            }

            // 重力処理
            movement_->ApplyGravity(dt);
        }
        else if (dummyMode_)
        {
            // ダミー: AIは動かさないが、被弾ノックバックは残す。
            movement_->ApplyDummyFriction(dt);
        }
        else
        {
            // ルートノードがなければ停止
            movement_->StopAll();
        }

        // 移動・コンボ・ガード状態に応じてアニメーションクリップを切り替える
        visual_->UpdateAnimation();

        // 接地判定
        movement_->CollisionGround();
        UpdateFrustumLockOn();

        // 弾の更新（ダミーモードでは弾を撃たないためスキップ）
        if (!dummyMode_)
        {
            combat_->UpdateBullets();
        }
    }

    // ワールドトランスフォームの行列更新は started_ に関わらず毎フレーム実行する。
    // StartCamera演出中は started_ が false のためゲームロジックはスキップされるが、
    // 行列が未更新のままだと Draw() に正しい行列が渡らず描画されなくなる。
    BaseObject::Update();
}

void Enemy::Draw(const ViewProjection &viewProjection)
{
    if (!isAlive_)
    {
        pEnemyCollider_->SetEnabled(false);

        // 死亡アニメーション中は本体を描画し続け、粒子化が始まったら非表示にする
        if (deathStaging_->GetIsStart())
        {
            return;
        }
    }
    BaseObject::Draw(viewProjection);
    if (transform_->translation_.y < kGroundLevel)
        return;
}

void Enemy::DrawParticleCompute(const ViewProjection &viewProjection)
{
    combat_->DrawParticleCompute(viewProjection);
}

void Enemy::DrawParticle(const ViewProjection &viewProjection)
{
    // 死亡アニメーションを再生し終わったら、死亡ポーズメッシュ(die.obj)の
    // 表面からパーティクルを発生させて粒子化して消える演出を行う
    if (!isAlive_ && visual_->IsDeathAnimationFinished())
    {
        deathStaging_->Initialize(
            GetWorldPosition() + Vector3(0.0f, kModelOffsetY, 0.0f),
            BaseObject::GetWorldRotation(), BaseObject::GetWorldScale(), GetColor());
        deathStaging_->Update();
        deathStaging_->Draw(viewProjection);
    }

    hitEmitter_->Draw(viewProjection); // CPU emitter
    combat_->DrawParticle(viewProjection);
}

void Enemy::Debug()
{
#ifdef USE_IMGUI
    EnemyMovement &mv = *movement_;
    EnemyStatus &st = *status_;

    if (ImGui::BeginTabBar("EnemyTabs"))
    {

        // ──────────────────────────────────────────
        // 基本情報タブ
        // ──────────────────────────────────────────
        if (ImGui::BeginTabItem("基本情報"))
        {
            ImGui::Text("HP");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(120.0f);
            if (ImGui::DragFloat("##HP", &st.GetHPRef(), 1.0f, 0.0f, st.GetMaxHP(), "%.1f"))
                st.SetHP(std::clamp(st.GetHP(), kMinHP, st.GetMaxHP()));
            ImGui::SameLine();
            ImGui::Text("/");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(100.0f);
            if (ImGui::DragFloat("最大HP##maxHP", &st.GetMaxHPRef(), 1.0f, 1.0f, 9999.0f, "%.1f"))
                st.SetHP(std::clamp(st.GetHP(), kMinHP, st.GetMaxHP()));
            ImGui::SameLine();
            if (ImGui::SmallButton("全回復##hp"))
                st.SetHP(st.GetMaxHP());

            ImGui::Text("Energy");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(120.0f);
            if (ImGui::DragFloat("##Energy", &st.GetEnergy(), 1.0f, 0.0f, st.GetMaxEnergy(), "%.1f"))
                st.SetEnergy(st.GetEnergy());
            ImGui::SameLine();
            ImGui::Text("/");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(100.0f);
            if (ImGui::DragFloat("最大Energy##maxEnergy", &st.GetMaxEnergyRef(), 1.0f, 1.0f, 9999.0f, "%.1f"))
                st.SetEnergy(st.GetEnergy());
            ImGui::SameLine();
            if (ImGui::SmallButton("全回復##energy"))
                st.SetEnergy(st.GetMaxEnergy());

            ImGui::Spacing();
            if (ImGui::Button("HP・Energy 全回復"))
            {
                st.SetHP(st.GetMaxHP());
                st.SetEnergy(st.GetMaxEnergy());
            }

            ImGui::Separator();
            ImGui::Text("位置: (%.2f, %.2f, %.2f)",
                        transform_->translation_.x, transform_->translation_.y, transform_->translation_.z);
            const Vector3 &vel = mv.GetVelocity();
            ImGui::Text("速度: (%.2f, %.2f, %.2f)", vel.x, vel.y, vel.z);
            ImGui::Text("地上判定: %s", mv.GetIsGrounded() ? "地上" : "空中");
            ImGui::Separator();

            ImGui::Text("【視錐台ロックオン】");
            if (isLockOn_)
            {
                ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.0f, 1.0f), "ロックオン: ON");
                if (ImGui::SmallButton("解除##frustum"))
                    ReleaseLockOn();
            }
            else
            {
                ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "ロックオン: OFF");
            }
            ImGui::Checkbox("視錐台デバッグ描画", &drawFrustumDebug_);
            if (pTarget_)
            {
                float dist = (pTarget_->GetLocalPosition() - transform_->translation_).Length();
                ImGui::Text("プレイヤーまでの距離: %.2f / %.1f", dist, frustumLockOnRange_);
            }
            const float kToDeg = 180.0f / 3.14159265f;
            const float kToRad = 3.14159265f / 180.0f;
            ImGui::DragFloat("有効距離##frustum", &frustumLockOnRange_, 0.5f, 1.0f, 300.0f, "%.1f");
            float halfFovHDeg = frustumLockOnHalfFovH_ * kToDeg;
            float halfFovVDeg = frustumLockOnHalfFovV_ * kToDeg;
            if (ImGui::DragFloat("水平半角 (度)##frustum", &halfFovHDeg, 0.5f, 1.0f, 89.0f, "%.1f"))
                frustumLockOnHalfFovH_ = halfFovHDeg * kToRad;
            if (ImGui::DragFloat("垂直半角 (度)##frustum", &halfFovVDeg, 0.5f, 1.0f, 89.0f, "%.1f"))
                frustumLockOnHalfFovV_ = halfFovVDeg * kToRad;
            ImGui::Text("  水平全角: %.1f°  垂直全角: %.1f°", halfFovHDeg * 2.0f, halfFovVDeg * 2.0f);

            ImGui::Separator();
            ImGui::Checkbox("ストップ", &isStop_);
            ImGui::Separator();
            ImGui::Text("ガード状態: %s", st.IsGuarding() ? "ON" : "OFF");
            if (st.IsGuarding())
            {
                ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "ダメージ85%%軽減中");
            }
            ImGui::EndTabItem();
        }

        // ──────────────────────────────────────────
        // ジャンプ/重力タブ
        // ──────────────────────────────────────────
        if (ImGui::BeginTabItem("ジャンプ/重力"))
        {
            ImGui::Text("=== 状態 ===");
            ImGui::TextColored(
                mv.GetIsGrounded() ? ImVec4(0.2f, 1.0f, 0.2f, 1.0f) : ImVec4(1.0f, 0.5f, 0.2f, 1.0f),
                "%s", mv.GetIsGrounded() ? "■ 地上" : "■ 空中");
            ImGui::Separator();
            const Vector3 &vel = mv.GetVelocity();
            ImGui::Text("Y座標: %.2f  垂直速度: %.2f  垂直加速度: %.2f",
                        transform_->translation_.y, vel.y, mv.GetAcceleration().y);
            ImGui::Separator();
            ImGui::DragFloat("重力加速度 (fallSpeed)", &mv.GetFallSpeed(), 0.5f, 1.0f, 100.0f);
            ImGui::DragFloat("ジャンプ力 (jumpSpeed)", &mv.GetJumpSpeed(), 0.5f, 1.0f, 50.0f);
            ImGui::DragFloat("移動速度 (moveSpeed)", &mv.GetMoveSpeed(), 0.1f, 0.0f, 20.0f);
            ImGui::Separator();
            if (ImGui::Button("手動ジャンプテスト"))
            {
                if (mv.GetIsGrounded())
                {
                    mv.GetVelocity().y = mv.GetJumpSpeed();
                    mv.SetIsGrounded(false);
                    mv.GetAcceleration().y = -mv.GetFallSpeed();
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("リセット（地上に戻す）"))
            {
                transform_->translation_.y = 0.0f;
                mv.GetVelocity().y = 0.0f;
                mv.GetAcceleration().y = 0.0f;
                mv.SetIsGrounded(true);
            }
            ImGui::EndTabItem();
        }

        // ──────────────────────────────────────────
        // コンボ攻撃パラメータタブ
        // ──────────────────────────────────────────
        if (ImGui::BeginTabItem("コンボパラメータ"))
        {
            combat_->DrawComboImGui();
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }
#endif
}

void Enemy::OnCollisionEnter(ColliderBase *other)
{
    if (other->GetTag() == "PlayerBullet" ||
        other->GetTag() == "PlayerChargeBullet" ||
        other->GetTag() == "Makan")
    {
        hitEmitter_->SetPosition(transform_->translation_);
        hitEmitter_->UpdateOnce();
    }
    if (other->GetTag() == "PlayerChargeBullet" ||
        other->GetTag() == "Makan")
    {
        combat_->StartHitShake();
    }

    // 前方攻撃判定（PlayerHand）ヒット時のパーティクル（ダメージ計算はコライダー側）
    if (other->GetTag() == "PlayerHand")
    {
        hitEmitter_->SetPosition(transform_->translation_);
        hitEmitter_->UpdateOnce();
    }
}

void Enemy::OnCollision(ColliderBase *other)
{
    Vector3 &velocity = movement_->GetVelocity();

    if (other->GetTag() == "CylinderField")
    {
        if (other->GetType() != ColliderType::Cylinder)
            return;
        auto *cyl = static_cast<CylinderCollider *>(other);
        Vector3 mtv;
        if (CollisionManager::GetInstance()->CalculateDepenetrationOBBCylinder(pEnemyCollider_, cyl, mtv))
        {
            transform_->translation_ += mtv;
            Vector3 mtvDir = mtv.Normalize();
            float dot = velocity.Dot(mtvDir);
            if (dot < 0.0f)
                velocity -= mtvDir * dot;
        }
        return;
    }

    if (other->GetType() != ColliderType::AABB)
        return;
    auto *otherAABB = static_cast<AABBCollider *>(other);
    Vector3 mtv;
    if (CollisionManager::GetInstance()->CalculateDepenetration(pEnemyWallCollider_, otherAABB, mtv))
    {
        if (mtv.Length() < 0.0001f)
            return;
        transform_->translation_ += mtv;
        Vector3 mtvDir = mtv.Normalize();
        float dot = velocity.Dot(mtvDir);
        if (dot < 0.0f)
            velocity -= mtvDir * dot;
    }
}

void Enemy::Save()
{
    ImGuiNotification::Post("エネミー設定を保存しました", {0.2f, 0.8f, 0.2f, 1.0f});
}
void Enemy::Load()
{
    ImGuiNotification::Post("エネミー設定を読み込みました", {0.2f, 0.8f, 0.8f, 1.0f});
}

void Enemy::UpdateFrustumLockOn()
{
    if (!pTarget_ || isLockOn_)
        return;

    Matrix4x4 rotMat = QuaternionToMatrix4x4(transform_->quateRotation_);
    const Vector3 origin = transform_->translation_;
    const Vector3 forward = {rotMat.m[2][0], rotMat.m[2][1], rotMat.m[2][2]};
    const Vector3 right = {rotMat.m[0][0], rotMat.m[0][1], rotMat.m[0][2]};
    const Vector3 up = {rotMat.m[1][0], rotMat.m[1][1], rotMat.m[1][2]};

    Vector3 toTarget = pTarget_->GetLocalPosition() - origin;
    float distance = toTarget.Length();
    if (distance < kMinRotationDistance || distance > frustumLockOnRange_)
        return;

    float dotF = toTarget.Dot(forward);
    if (dotF <= kVelocityZero)
        return;

    float tanH = toTarget.Dot(right) / dotF;
    if (std::abs(tanH) > std::tan(frustumLockOnHalfFovH_))
        return;

    float tanV = toTarget.Dot(up) / dotF;
    if (std::abs(tanV) > std::tan(frustumLockOnHalfFovV_))
        return;

    isLockOn_ = true;
}

void Enemy::DrawFrustum()
{
#ifdef USE_IMGUI
    if (!drawFrustumDebug_)
        return;

    DrawLine3D *drawLine3D = DrawLine3D::GetInstance();
    if (!drawLine3D)
        return;

    Matrix4x4 rotMat = QuaternionToMatrix4x4(transform_->quateRotation_);
    const Vector3 origin = transform_->translation_;
    const Vector3 forward = {rotMat.m[2][0], rotMat.m[2][1], rotMat.m[2][2]};
    const Vector3 right = {rotMat.m[0][0], rotMat.m[0][1], rotMat.m[0][2]};
    const Vector3 up = {rotMat.m[1][0], rotMat.m[1][1], rotMat.m[1][2]};

    const Vector4 color = isLockOn_ ? Vector4{1.0f, 0.5f, 0.0f, 1.0f}
                                    : Vector4{1.0f, 1.0f, 1.0f, 0.8f};
    const Vector4 axisColor = isLockOn_ ? Vector4{1.0f, 0.5f, 0.0f, 0.5f}
                                        : Vector4{1.0f, 1.0f, 1.0f, 0.4f};

    static constexpr float kFrustumDebugNear = 1.0f;
    auto CalcCorners = [&](float depth, std::array<Vector3, 4> &corners) {
        float halfH = depth * std::tan(frustumLockOnHalfFovH_);
        float halfV = depth * std::tan(frustumLockOnHalfFovV_);
        Vector3 center = origin + forward * depth;
        corners[0] = center - right * halfH + up * halfV;
        corners[1] = center + right * halfH + up * halfV;
        corners[2] = center + right * halfH - up * halfV;
        corners[3] = center - right * halfH - up * halfV;
    };

    std::array<Vector3, 4> nearCorners, farCorners;
    CalcCorners(kFrustumDebugNear, nearCorners);
    CalcCorners(frustumLockOnRange_, farCorners);

    for (int i = 0; i < 4; ++i)
    {
        drawLine3D->SetPoints(nearCorners[i], nearCorners[(i + 1) % 4], color);
        drawLine3D->SetPoints(farCorners[i], farCorners[(i + 1) % 4], color);
    }
    for (int i = 0; i < 4; ++i)
    {
        drawLine3D->SetPoints(nearCorners[i], farCorners[i], color);
    }
    drawLine3D->SetPoints(origin, origin + forward * frustumLockOnRange_, axisColor);
#endif
}

void Enemy::SetVp(ViewProjection *vp)
{
    combat_->SetVp(vp);
}

void Enemy::SetDummy(bool enable)
{
    dummyMode_ = enable;
    if (enable && transform_)
    {
        // 呼び出し時点の位置を復活位置として記録する
        spawnPosition_ = transform_->translation_;
    }
}

void Enemy::Revive()
{
    status_->ResetForRevive();
    isAlive_ = true;

    // 速度・ノックバック・被弾リアクションをクリア
    movement_->ResetMotion();
    SetAlpha(kAlphaOpaque);

    // 初期位置へ戻す
    if (transform_)
    {
        transform_->translation_ = spawnPosition_;
    }
}

Vector3 Enemy::GetForward() const
{
    return TransformNormal(
        Vector3(kForwardVectorX, kForwardVectorY, kForwardVectorZ),
        QuaternionToMatrix4x4(transform_->quateRotation_));
}
Vector3 Enemy::GetBackward() const { return -GetForward(); }
Vector3 Enemy::GetRight() const
{
    return TransformNormal(
        Vector3(kRightVectorX, kRightVectorY, kRightVectorZ),
        QuaternionToMatrix4x4(transform_->quateRotation_));
}
Vector3 Enemy::GetLeft() const { return -GetRight(); }
Vector3 Enemy::GetUp() const
{
    return TransformNormal(
        Vector3(kUpVectorX, kUpVectorY, kUpVectorZ),
        QuaternionToMatrix4x4(transform_->quateRotation_));
}
Vector3 Enemy::GetDown() const { return -GetUp(); }

Vector3 Enemy::GetPositionBehind(float distance) const { return transform_->translation_ + GetBackward() * distance; }
Vector3 Enemy::GetPositionFront(float distance) const { return transform_->translation_ + GetForward() * distance; }
Vector3 Enemy::GetPositionRight(float distance) const { return transform_->translation_ + GetRight() * distance; }
Vector3 Enemy::GetPositionLeft(float distance) const { return transform_->translation_ + GetLeft() * distance; }
Vector3 Enemy::GetPositionAbove(float distance) const { return transform_->translation_ + GetUp() * distance; }
Vector3 Enemy::GetPositionBelow(float distance) const { return transform_->translation_ + GetDown() * distance; }
