#define NOMINMAX
#ifdef USE_IMGUI
#include "ImGuizmoManager.h"
#include "ImGuiNotification.h"
#include "Input.h"
#include "Sprite.h"
#include <line/LineRenderer.h>
#include <object/base/BaseObjectManager.h>
#include <transform/WorldTransform.h>
#include <edit/undo/UndoRedoManager.h>
#include "WinApp.h"
#include <format>
#include <imgui.h>
// DebugUIHelper.h は ImVec4 / ImGui:: を使うので imgui.h の後に include する
#include "DebugUIHelper.h"

// =======================================================================
// GizmoTarget メンバ関数実装
// =======================================================================

// 各型に対応したワールド行列を返す
// FreeTransform の場合は translate/rotate/scale ポインタから行列を構築する
namespace Hagine {
Matrix4x4 GizmoTarget::GetWorldMatrix() const
{
    switch (type)
    {
    case Type::BaseObject:
        if (baseObject && baseObject->GetWorldTransform())
        {
            return baseObject->GetWorldTransform()->matWorld_;
        }
        break;

    case Type::WorldTransform:
        if (worldTransform)
        {
            return worldTransform->matWorld_;
        }
        break;

    case Type::FreeTransform:
        if (translate)
        {
            Vector3 s = scale ? *scale : Vector3{1.0f, 1.0f, 1.0f};
            Vector3 r = rotate ? *rotate : Vector3{0.0f, 0.0f, 0.0f};
            return MakeAffineMatrix(s, r, *translate);
        }
        break;

    case Type::Sprite2D:
        if (position2D)
        {
            Matrix4x4 m = MakeIdentity4x4();
            m.m[3][0] = position2D->x;
            m.m[3][1] = position2D->y;
            m.m[3][2] = 0.0f;
            return m;
        }
        break;
    }
    return MakeIdentity4x4();
}

// ワールド座標（位置成分）を返す
Vector3 GizmoTarget::GetWorldPosition() const
{
    switch (type)
    {
    case Type::BaseObject:
        if (baseObject)
            return baseObject->GetWorldPosition();
        break;
    case Type::WorldTransform:
        if (worldTransform)
            return worldTransform->GetWorldPosition();
        break;
    case Type::FreeTransform:
        if (translate)
            return *translate;
        break;
    case Type::Sprite2D:
        if (position2D)
            return {position2D->x, position2D->y, 0.0f};
        break;
    }
    return {0.0f, 0.0f, 0.0f};
}

// ギズモ操作によって生じた平行移動デルタを各型に適用する
void GizmoTarget::ApplyTranslationDelta(const Vector3 &delta)
{
    switch (type)
    {
    case Type::BaseObject:
        if (baseObject)
        {
            baseObject->GetLocalPosition() = baseObject->GetLocalPosition() + delta;
            WorldTransform *wt = baseObject->GetWorldTransform();
            if (wt)
            {
                wt->translation_ = baseObject->GetLocalPosition();
                wt->UpdateMatrix();
                baseObject->UpdateWorldTransformHierarchy();
            }
        }
        break;

    case Type::WorldTransform:
        if (worldTransform)
        {
            worldTransform->translation_ = worldTransform->translation_ + delta;
            worldTransform->UpdateMatrix();
        }
        break;

    case Type::FreeTransform:
        if (translate)
        {
            translate->x += delta.x;
            translate->y += delta.y;
            if (!isScreenSpace)
            {
                translate->z += delta.z;
            }
        }
        break;

    case Type::Sprite2D:
        if (position2D)
        {
            position2D->x += delta.x;
            position2D->y += delta.y;
            // Z は無視（スクリーン空間 XY のみ）
        }
        break;
    }
}

namespace {
// アフィン行列をスケール・回転・平行移動へ分解する。
// この行列規約は行ベクトル（v * M）なので、各行が基底ベクトルになる。
void DecomposeAffine(const Matrix4x4 &matrix, Vector3 &scale, Quaternion &rotation, Vector3 &translation)
{
    translation = {matrix.m[3][0], matrix.m[3][1], matrix.m[3][2]};

    Vector3 axis[3] = {
        {matrix.m[0][0], matrix.m[0][1], matrix.m[0][2]},
        {matrix.m[1][0], matrix.m[1][1], matrix.m[1][2]},
        {matrix.m[2][0], matrix.m[2][1], matrix.m[2][2]}};

    float length[3] = {axis[0].Length(), axis[1].Length(), axis[2].Length()};

    // 左手系で行列式が負なら鏡映が入っている。X軸のスケールを負にして回転成分から追い出す。
    const Vector3 cross = axis[0].Cross(axis[1]);
    if (cross.Dot(axis[2]) < 0.0f)
    {
        length[0] = -length[0];
        axis[0] = axis[0] * -1.0f;
    }

    scale = {length[0], length[1], length[2]};

    // スケール0の軸は方向が求まらないので、その軸だけ単位行列の行を使う
    constexpr float kEpsilon = 1e-6f;
    Matrix4x4 rotationMatrix = MakeIdentity4x4();
    for (int row = 0; row < 3; ++row)
    {
        const float absLength = std::abs(length[row]);
        if (absLength <= kEpsilon)
        {
            continue;
        }
        rotationMatrix.m[row][0] = axis[row].x / absLength;
        rotationMatrix.m[row][1] = axis[row].y / absLength;
        rotationMatrix.m[row][2] = axis[row].z / absLength;
    }

    rotation = Quaternion::FromMatrix(rotationMatrix).Normalize();
}

// 親を持つ WorldTransform について、親側のワールド行列（継承フラグ反映済み）を返す。
// UpdateMatrix と同じ組み立てにしないと、ワールド→ローカル変換がずれる。
Matrix4x4 BuildInheritedParentMatrix(const WorldTransform &transform)
{
    const WorldTransform *pParent = transform.pParent_;
    if (!pParent)
    {
        return MakeIdentity4x4();
    }
    if (transform.inheritTranslation_ && transform.inheritRotation_ && transform.inheritScale_)
    {
        return pParent->matWorld_;
    }
    const Vector3 parentScale = transform.inheritScale_ ? pParent->GetWorldScale() : Vector3{1.0f, 1.0f, 1.0f};
    const Quaternion parentRotation = transform.inheritRotation_ ? pParent->GetWorldRotationQuaternion() : Quaternion::IdentityQuaternion();
    const Vector3 parentTranslation = transform.inheritTranslation_ ? pParent->GetWorldPosition() : Vector3{0.0f, 0.0f, 0.0f};
    return MakeAffineMatrix(parentScale, parentRotation, parentTranslation);
}

// ワールド行列を WorldTransform のローカル成分へ書き戻す
void ApplyWorldMatrixToTransform(WorldTransform &transform, const Matrix4x4 &worldMatrix)
{
    // 親がいる場合は親のぶんを打ち消してローカル成分に戻す
    Matrix4x4 localMatrix = worldMatrix;
    if (transform.pParent_)
    {
        localMatrix = worldMatrix * Inverse(BuildInheritedParentMatrix(transform));
    }

    Vector3 scale{};
    Quaternion rotation{};
    Vector3 translation{};
    DecomposeAffine(localMatrix, scale, rotation, translation);

    transform.scale_ = scale;
    transform.translation_ = translation;
    // SetRotationQuaternion はオイラー角モードのときだけ eulerRotation_ を更新するので、
    // クォータニオンモードでは UpdateQuaternion のオイラー再変換に巻き込まれない。
    transform.SetRotationQuaternion(rotation);
    transform.UpdateMatrix();
}
} // namespace

// ギズモ操作後のワールド行列を各型へ反映する
void GizmoTarget::ApplyWorldMatrix(const Matrix4x4 &worldMatrix)
{
    // スクリーン空間は XY 平行移動しか意味を持たないので、従来どおりデルタ適用に落とす
    if (isScreenSpace)
    {
        const Vector3 current = GetWorldPosition();
        ApplyTranslationDelta({worldMatrix.m[3][0] - current.x, worldMatrix.m[3][1] - current.y, 0.0f});
        return;
    }

    switch (type)
    {
    case Type::BaseObject:
        if (baseObject && baseObject->GetWorldTransform())
        {
            ApplyWorldMatrixToTransform(*baseObject->GetWorldTransform(), worldMatrix);
            // 子オブジェクトのワールド行列も追従させる
            baseObject->UpdateWorldTransformHierarchy();
        }
        break;

    case Type::WorldTransform:
        if (worldTransform)
        {
            ApplyWorldMatrixToTransform(*worldTransform, worldMatrix);
        }
        break;

    case Type::FreeTransform:
        if (translate)
        {
            Vector3 decomposedScale{};
            Quaternion decomposedRotation{};
            Vector3 decomposedTranslation{};
            DecomposeAffine(worldMatrix, decomposedScale, decomposedRotation, decomposedTranslation);

            *translate = decomposedTranslation;
            // rotate / scale は持っていない対象もあるので、あるものだけ書き込む
            if (rotate)
            {
                *rotate = decomposedRotation.ToEulerAngles();
            }
            if (scale)
            {
                *scale = decomposedScale;
            }
        }
        break;

    case Type::Sprite2D:
        if (position2D)
        {
            position2D->x = worldMatrix.m[3][0];
            position2D->y = worldMatrix.m[3][1];
        }
        break;
    }
}

// マウス選択・フォーカス用のローカル空間AABBを返す
AABB GizmoTarget::GetLocalBounds() const
{
    if (type == Type::BaseObject && baseObject)
    {
        return baseObject->GetLocalBounds();
    }
    // BaseObject 以外（エミッター・ライト等）は実体の形が無いので、掴める大きさの箱を返す
    return AABB{{-1.3f, -1.3f, -1.3f}, {1.3f, 1.3f, 1.3f}};
}

// ImGui で変換詳細を表示する
// imguiCallback が設定されている場合はそちらを優先する
void GizmoTarget::ShowImGui()
{
    switch (type)
    {
    case Type::BaseObject:
        if (baseObject)
        {
            baseObject->DrawImGui();
        }
        break;

    case Type::WorldTransform:
        if (worldTransform)
        {
            if (imguiCallback)
            {
                imguiCallback();
            }
            else
            {
                ImGui::DragFloat3("Translation", &worldTransform->translation_.x, 0.1f);
                ImGui::DragFloat3("Scale", &worldTransform->scale_.x, 0.01f);
                Vector3 euler = worldTransform->GetRotationEuler();
                if (ImGui::DragFloat3("Rotation (rad)", &euler.x, 0.01f))
                {
                    worldTransform->SetRotationEuler(euler);
                }
                if (ImGui::Button("UpdateMatrix"))
                {
                    worldTransform->UpdateMatrix();
                }
            }
        }
        break;

    case Type::FreeTransform:
        if (imguiCallback)
        {
            imguiCallback();
        }
        else
        {
            if (translate)
            {
                if (isScreenSpace)
                {
                    ImGui::DragFloat2("Position (px)", &translate->x, 1.0f);
                }
                else
                {
                    ImGui::DragFloat3("Translation", &translate->x, 0.1f);
                }
            }
            if (rotate)
                ImGui::DragFloat3("Rotation (rad)", &rotate->x, 0.01f);
            if (scale)
                ImGui::DragFloat3("Scale", &scale->x, 0.01f);
        }
        break;

    case Type::Sprite2D:
        if (imguiCallback)
        {
            imguiCallback();
        }
        else if (position2D)
        {
            ImGui::DragFloat2("Position (px)", &position2D->x, 1.0f);
        }
        break;
    }
}

// =======================================================================
// ImGuizmoManager メンバ関数実装
// =======================================================================

void ImGuizmoManager::Finalize()
{
    transformMap_.clear();
    selectedNames_.clear();
    copiedNames_.clear();
}

void ImGuizmoManager::BeginFrame()
{
    ImGuizmo::BeginFrame();
}

void ImGuizmoManager::SetViewProjection(ViewProjection *pViewProjection)
{
    pViewProjection_ = pViewProjection;
}

// ---- AddTarget オーバーロード群 ----------------------------------------

// BaseObject を登録する
void ImGuizmoManager::AddTarget(const std::string &name, BaseObject *pObject, bool selectable)
{
    GizmoTarget target;
    target.type = GizmoTarget::Type::BaseObject;
    target.category = GizmoCategory::Object;
    target.name = name;
    target.baseObject = pObject;
    target.selectable = selectable;
    transformMap_[name] = target;

    UpdateFilteredNames();
}

// WorldTransform のみを持つオブジェクトを登録する
void ImGuizmoManager::AddTarget(const std::string &name, WorldTransform *worldTransform,
                                bool selectable,
                                std::function<void()> imguiCallback)
{
    GizmoTarget target;
    target.type = GizmoTarget::Type::WorldTransform;
    // WorldTransform 単体は 3Dオブジェクト扱いを既定とする。
    // パーティクル等で分類を変えたい場合は AddTarget 後に SetCategory を呼ぶ。
    target.category = GizmoCategory::Object;
    target.name = name;
    target.worldTransform = worldTransform;
    target.selectable = selectable;
    target.imguiCallback = imguiCallback;
    transformMap_[name] = target;

    UpdateFilteredNames();
}

// Vector3 ポインタを直接指定して登録する（Sprite・ParticleEmitter など）
void ImGuizmoManager::AddTarget(const std::string &name,
                                Vector3 *translate,
                                Vector3 *rotate,
                                Vector3 *scale,
                                bool selectable,
                                std::function<void()> imguiCallback)
{
    GizmoTarget target;
    target.type = GizmoTarget::Type::FreeTransform;
    // Vector3 直接指定はパーティクルエミッターで多く使われるため既定はParticle。
    // スプライト等は AddTarget 後に SetCategory で上書きする。
    target.category = GizmoCategory::Particle;
    target.name = name;
    target.translate = translate;
    target.rotate = rotate;
    target.scale = scale;
    target.selectable = selectable;
    target.imguiCallback = imguiCallback;
    transformMap_[name] = target;

    UpdateFilteredNames();
}

// Sprite を登録する（スクリーン空間 XY のみ操作）
void ImGuizmoManager::AddTarget(const std::string &name, Sprite *pSprite, bool selectable)
{
    GizmoTarget target;
    target.type = GizmoTarget::Type::Sprite2D;
    target.category = GizmoCategory::Sprite;
    target.name = name;
    target.position2D = &pSprite->GetPositionRef();
    target.selectable = selectable;
    target.isScreenSpace = true;
    target.screenHitRadius = 50.0f;
    transformMap_[name] = target;

    UpdateFilteredNames();
}

// ---- 選択オブジェクト取得（BaseObject 互換用）---------------------------

// 選択中の最初のエントリが BaseObject である場合に返す
BaseObject *ImGuizmoManager::GetSelectedTarget()
{
    if (selectedNames_.empty())
        return nullptr;

    auto it = transformMap_.find(*selectedNames_.begin());
    if (it != transformMap_.end() && it->second.type == GizmoTarget::Type::BaseObject)
    {
        return it->second.baseObject;
    }
    return nullptr;
}

// 選択中のエントリのうち BaseObject のもののみを返す
std::vector<BaseObject *> ImGuizmoManager::GetSelectedTargets()
{
    std::vector<BaseObject *> selected;
    for (const std::string &name : selectedNames_)
    {
        auto it = transformMap_.find(name);
        if (it != transformMap_.end() && it->second.type == GizmoTarget::Type::BaseObject)
        {
            selected.push_back(it->second.baseObject);
        }
    }
    return selected;
}

// ---- imgui ------------------------------------------------------------

void ImGuizmoManager::DrawImGui()
{
    if (!pViewProjection_)
        return;

    ImGui::PushStyleColor(ImGuiCol_CheckMark, DebugTheme::kAccentGreen);
    ImGui::Checkbox("デバッグ表示する", &isDrawDebug_);
    ImGui::PopStyleColor();
    ImGui::SetItemTooltip("選択中オブジェクトの AABB / スフィア / レイを線で表示します");

    if (isDrawDebug_)
    {
        ImGui::Indent();
        ImGui::PushStyleColor(ImGuiCol_CheckMark, DebugTheme::kAccentGreen);
        ImGui::Checkbox("選択中のみ", &debugSelectedOnly_);
        ImGui::SameLine();
        ImGui::Checkbox("AABB", &showDebugAABB_);
        ImGui::SameLine();
        ImGui::Checkbox("スフィア", &showDebugSphere_);
        ImGui::SameLine();
        ImGui::Checkbox("レイ", &showDebugHitPoints_);
        ImGui::PopStyleColor();
        ImGui::Unindent();
    }

    // ---- 操作説明 ----
    ImGui::Spacing();
    if (ImGui::CollapsingHeader("[ ショートカット ]"))
    {
        ImGui::BulletText("1 / 2 / 3 : 移動 / 回転 / スケール");
        ImGui::BulletText("4 : ローカル ⇔ ワールド 切替");
        ImGui::BulletText("5 : スナップ ON/OFF （Shift 押下中は一時反転）");
        ImGui::BulletText("F : 選択オブジェクトへ視点を寄せる");
        ImGui::BulletText("Tab : 重なったオブジェクトを順に選択");
        ImGui::BulletText("Ctrl+D : 複製 / Ctrl+C・Ctrl+V : コピー・貼り付け");
        ImGui::BulletText("空ドラッグ : 矩形選択（Ctrl 併用で選択に追加）");
        ImGui::TextDisabled("※ シーンウィンドウにマウスがある時だけ効きます");
    }

    // ---- 操作対象フィルタ ----
    // 4種類（オブジェクト/スプライト/パーティクル/ライト）が同時にあると掴みたい物を選びづらいので、
    // チェックした種類だけを選択・マウスピック・ギズモ表示・デバッグ描画の対象にする。
    ImGui::Spacing();
    SectionHeader("[ 操作対象フィルタ ]", DebugTheme::kAccentGreen);
    ImGui::TextDisabled("チェックした種類だけ選択・操作できます");
    bool filterChanged = false;

    // 分類の表示名。追加時はここと GizmoCategory を対応させる
    static const char *kCategoryLabels[kGizmoCategoryCount] = {
        "オブジェクト", "スプライト", "パーティクル", "ライト"};

    ImGui::PushStyleColor(ImGuiCol_CheckMark, DebugTheme::kAccentGreen);
    for (int i = 0; i < kGizmoCategoryCount; ++i)
    {
        if (i > 0)
            ImGui::SameLine();
        filterChanged |= ImGui::Checkbox(kCategoryLabels[i], &categoryEnabled_[i]);
    }
    ImGui::PopStyleColor();

    // 「この種類だけ」を素早く選べるショートカット
    auto SoloCategory = [this](int index) {
        for (int i = 0; i < kGizmoCategoryCount; ++i)
            categoryEnabled_[i] = (i == index);
    };
    if (ImGui::SmallButton("全部##catAll"))
    {
        for (bool &e : categoryEnabled_)
            e = true;
        filterChanged = true;
    }
    for (int i = 0; i < kGizmoCategoryCount; ++i)
    {
        ImGui::SameLine();
        ImGui::PushID(i);
        if (ImGui::SmallButton(std::format("{}のみ", kCategoryLabels[i]).c_str()))
        {
            SoloCategory(i);
            filterChanged = true;
        }
        ImGui::PopID();
    }
    if (filterChanged)
    {
        // 無効化された種類の選択を解除し、一覧も更新する
        PruneSelectionByFilter();
        UpdateFilteredNames();
    }

    ImGui::Spacing();
    SectionHeader("[ 操作モード ]", DebugTheme::kAccentBlue);
    ImGui::PushStyleColor(ImGuiCol_CheckMark, DebugTheme::kAccentBlue);
    if (ImGui::RadioButton("移動", currentOperation_ == ImGuizmo::TRANSLATE))
        currentOperation_ = ImGuizmo::TRANSLATE;
    ImGui::SameLine();
    if (ImGui::RadioButton("回転", currentOperation_ == ImGuizmo::ROTATE))
        currentOperation_ = ImGuizmo::ROTATE;
    ImGui::SameLine();
    if (ImGui::RadioButton("スケール", currentOperation_ == ImGuizmo::SCALE))
        currentOperation_ = ImGuizmo::SCALE;
    ImGui::PopStyleColor();

    ImGui::Spacing();
    SectionHeader("[ 座標系 ]", DebugTheme::kAccentCyan);
    ImGui::PushStyleColor(ImGuiCol_CheckMark, DebugTheme::kAccentCyan);
    if (ImGui::RadioButton("ローカル", currentMode_ == ImGuizmo::LOCAL))
        currentMode_ = ImGuizmo::LOCAL;
    ImGui::SameLine();
    if (ImGui::RadioButton("ワールド", currentMode_ == ImGuizmo::WORLD))
        currentMode_ = ImGuizmo::WORLD;
    ImGui::PopStyleColor();

    ImGui::Spacing();
    SectionHeader("[ スナップ（グリッド吸着）]", DebugTheme::kAccentYellow);
    ImGui::PushStyleColor(ImGuiCol_CheckMark, DebugTheme::kAccentYellow);
    ImGui::Checkbox("スナップを使う", &useSnap_);
    ImGui::PopStyleColor();
    ImGui::SetItemTooltip("操作量を刻み幅に丸めます。Shift 押下中はこの設定が一時的に反転します");

    ImGui::SetNextItemWidth(120.0f);
    ImGui::DragFloat("移動の刻み", &snapTranslate_, 0.05f, 0.01f, 100.0f, "%.2f");
    ImGui::SameLine();
    // 等間隔に並べるときによく使う刻みをワンタッチで
    if (ImGui::SmallButton("0.5##snapT"))
        snapTranslate_ = 0.5f;
    ImGui::SameLine();
    if (ImGui::SmallButton("1##snapT"))
        snapTranslate_ = 1.0f;
    ImGui::SameLine();
    if (ImGui::SmallButton("5##snapT"))
        snapTranslate_ = 5.0f;

    ImGui::SetNextItemWidth(120.0f);
    ImGui::DragFloat("回転の刻み(度)", &snapRotateDegree_, 1.0f, 1.0f, 180.0f, "%.0f");
    ImGui::SameLine();
    if (ImGui::SmallButton("15##snapR"))
        snapRotateDegree_ = 15.0f;
    ImGui::SameLine();
    if (ImGui::SmallButton("45##snapR"))
        snapRotateDegree_ = 45.0f;
    ImGui::SameLine();
    if (ImGui::SmallButton("90##snapR"))
        snapRotateDegree_ = 90.0f;

    ImGui::SetNextItemWidth(120.0f);
    ImGui::DragFloat("拡縮の刻み", &snapScale_, 0.01f, 0.01f, 10.0f, "%.2f");

    ImGui::Spacing();
    SectionHeader("[ 整列・配置 ]", DebugTheme::kAccentPurple);
    ImGui::TextDisabled("選択中のオブジェクトをまとめて並べます");

    static const char *kAxisLabels[3] = {"X", "Y", "Z"};
    // 軸ごとに「最小 / 中央 / 最大 に揃える」を並べる
    for (int axis = 0; axis < 3; ++axis)
    {
        ImGui::PushID(axis);
        ImGui::TextUnformatted(kAxisLabels[axis]);
        ImGui::SameLine();
        if (ImGui::SmallButton("最小##align"))
            AlignSelected(axis, AlignMode::Min);
        ImGui::SameLine();
        if (ImGui::SmallButton("中央##align"))
            AlignSelected(axis, AlignMode::Center);
        ImGui::SameLine();
        if (ImGui::SmallButton("最大##align"))
            AlignSelected(axis, AlignMode::Max);
        ImGui::SameLine();
        if (ImGui::SmallButton("等間隔##dist"))
            DistributeSelected(axis);
        ImGui::PopID();
    }
    ImGui::SetItemTooltip("等間隔は両端をそのままに、間のオブジェクトを均等な位置へ動かします（3つ以上必要）");

    if (ImGui::Button("地面に接地##snapGround", ImVec2(-1, 0)))
    {
        SnapSelectedToGround();
    }
    ImGui::SetItemTooltip("選択中のオブジェクトを、真下にある他のオブジェクトの上面へ落とします\n"
                          "（下に何も無ければ Y=0 へ）");

    ImGui::Separator();

    SectionHeader("[ オブジェクト選択 ]", DebugTheme::kAccentPurple);
    // 検索ボックス（ヒント付き・全幅）
    ImGui::SetNextItemWidth(-1);
    bool searchChanged = ImGui::InputTextWithHint("##ObjectSearch", "名前で絞り込み...", searchBuffer_, sizeof(searchBuffer_));
    if (searchChanged)
        UpdateFilteredNames();
    if (filteredNames_.empty())
        UpdateFilteredNames();

    std::string currentDisplayName = selectedNames_.empty() ? "なし"
                                                            : (selectedNames_.size() == 1 ? *selectedNames_.begin()
                                                                                          : "複数選択 (" + std::to_string(selectedNames_.size()) + "個)");

    if (ImGui::BeginCombo("選択オブジェクト", currentDisplayName.c_str()))
    {
        bool isNoneSelected = selectedNames_.empty();
        if (ImGui::Selectable("なし", isNoneSelected))
            selectedNames_.clear();
        if (isNoneSelected)
            ImGui::SetItemDefaultFocus();

        for (const std::string &name : filteredNames_)
        {
            auto it = transformMap_.find(name);
            if (it != transformMap_.end())
            {
                bool isSelected = (selectedNames_.find(name) != selectedNames_.end());
                if (ImGui::Selectable(name.c_str(), isSelected))
                {
                    selectedNames_.clear();
                    selectedNames_.insert(name);
                }
                if (isSelected)
                    ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }

    if (strlen(searchBuffer_) > 0)
    {
        ImGui::Text("検索結果: %zu個", filteredNames_.size());
    }

    ImGui::Spacing();
    ImGui::Text("選択中のオブジェクト数: %zu", selectedNames_.size());
    if (!selectedNames_.empty())
    {
        ImGui::Text("選択中:");
        for (const std::string &name : selectedNames_)
        {
            ImGui::BulletText("%s", name.c_str());
        }
    }

    ImGui::Separator();

    if (ImGui::Button("全選択"))
    {
        selectedNames_.clear();
        for (const auto &pair : transformMap_)
            selectedNames_.insert(pair.first);
    }
    ImGui::SameLine();
    if (ImGui::Button("選択解除"))
        selectedNames_.clear();

    ImGui::Spacing();

    if (!selectedNames_.empty())
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 0.6f, 1.0f));
        ImGui::Text("オブジェクト詳細 (%s)", selectedNames_.begin()->c_str());
        ImGui::PopStyleColor();
        ImGui::Separator();

        ShowSelectedObjectImGui();

        ImGui::Spacing();
        ImGui::Spacing();

        // BaseObject のみコピー・ペーストが可能
        auto it = transformMap_.find(*selectedNames_.begin());
        if (it != transformMap_.end() && it->second.type == GizmoTarget::Type::BaseObject)
        {
            if (ImGui::Button("複製 (Ctrl+D)", ImVec2(-1, 30)))
                DuplicateSelectedObjects();
            ImGui::SetItemTooltip("選択中のオブジェクトをその場で複製し、複製したほうを選択状態にします");
            if (ImGui::Button("コピー", ImVec2(-1, 30)))
                CopySelectedObjects();
            if (!copiedNames_.empty())
            {
                if (ImGui::Button("ペースト", ImVec2(-1, 30)))
                    PasteObjects();
            }
            ImGui::Spacing();
        }

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.3f, 0.3f, 0.8f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.4f, 0.4f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.7f, 0.2f, 0.2f, 1.0f));
        if (ImGui::Button("選択オブジェクトを削除", ImVec2(-1, 0)))
            DeleteSelectedObjects();
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("選択中の全オブジェクトを削除します");
        ImGui::PopStyleColor(3);
    }

    // 重複オブジェクト候補（Tab でサイクル）
    if (overlapCandidates_.size() > 1)
    {
        ImGui::Separator();
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.9f, 0.4f, 1.0f));
        ImGui::Text("重複候補: %zu個 (Tab でサイクル選択)", overlapCandidates_.size());
        ImGui::PopStyleColor();
        for (int i = 0; i < static_cast<int>(overlapCandidates_.size()); ++i)
        {
            bool isCurrent = (i == overlapCycleIndex_);
            if (isCurrent)
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 1.0f, 0.4f, 1.0f));
            ImGui::Text("  [%d] %s", i, overlapCandidates_[i].first.c_str());
            if (isCurrent)
                ImGui::PopStyleColor();
        }
    }

    ImGui::Separator();
    if (isDrawDebug_)
        DrawDebugRaycast();
}

// ---- Update -----------------------------------------------------------

void ImGuizmoManager::Update(const ImVec2 &scenePosition, const ImVec2 &sceneSize, bool sceneHovered)
{
    if (!pViewProjection_)
        return;

    ImGuizmo::SetRect(scenePosition.x, scenePosition.y, sceneSize.x, sceneSize.y);
    ImGuizmo::SetDrawlist();

    if (!ImGuizmo::IsUsing())
    {
        // 矩形選択のドラッグ判定を先に回す。
        // しきい値未満のドラッグはクリック扱いになり、下の単体選択がそのまま働く。
        HandleBoxSelection(scenePosition, sceneSize, sceneHovered);
        HandleMouseSelection(scenePosition, sceneSize, sceneHovered);
        HandleHotkeys(sceneHovered);
    }
    else
    {
        // ギズモ操作に入ったら矩形選択は取り消す（枠が出しっぱなしにならないように）
        isBoxSelecting_ = false;
    }

    DrawSelectedObjectHighlight();

    if (!selectedNames_.empty())
    {
        // スプライト用正射影 VP を使うためシーン情報を渡す
        DisplayGizmo(scenePosition, sceneSize);
    }
}

// ---- ShowSelectedObjectImGui ------------------------------------------

// 選択中エントリの ShowImGui を呼び出す
void ImGuizmoManager::ShowSelectedObjectImGui()
{
    if (selectedNames_.empty())
        return;

    std::string firstName = *selectedNames_.begin();
    auto it = transformMap_.find(firstName);
    if (it != transformMap_.end())
    {
        it->second.ShowImGui();
    }

    if (selectedNames_.size() > 1)
    {
        ImGui::Separator();
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.7f, 1.0f, 1.0f));
        ImGui::Text("※ %zu個のオブジェクトが選択されています", selectedNames_.size());
        ImGui::Text("表示しているのは '%s' の設定です", firstName.c_str());
        ImGui::PopStyleColor();
    }
}

// ---- PruneSelectionByFilter -------------------------------------------

// 操作対象フィルタで無効化された分類の名前を選択セットから取り除く。
// これによりフィルタOFFにした種類のギズモが表示され続けるのを防ぐ。
void ImGuizmoManager::PruneSelectionByFilter()
{
    for (auto it = selectedNames_.begin(); it != selectedNames_.end();)
    {
        auto found = transformMap_.find(*it);
        if (found != transformMap_.end() && !IsCategoryEnabled(found->second.category))
        {
            it = selectedNames_.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

// ---- HandleMouseSelection ---------------------------------------------

// マウスクリック時のレイキャストによる選択判定
// BaseObject/WorldTransform/FreeTransform すべての型に対応するため
// 行列版の RayIntersectOBBByMatrix を使用する（回転した対象でも形どおりに当たる）
void ImGuizmoManager::HandleMouseSelection(const ImVec2 &scenePosition, const ImVec2 &sceneSize, bool sceneHovered)
{
    ImVec2 mousePos = ImGui::GetMousePos();
    bool isInScene = (mousePos.x >= scenePosition.x && mousePos.x <= scenePosition.x + sceneSize.x &&
                      mousePos.y >= scenePosition.y && mousePos.y <= scenePosition.y + sceneSize.y);

    // 他の ImGui ウィンドウがシーンに重なっている上でのクリックは無視（誤選択防止）。
    // 選択の確定は「押した瞬間」ではなく「ドラッグせずに離した瞬間」（HandleBoxSelection が判定）。
    if (!sceneHovered || ImGuizmo::IsUsing() || !isInScene || !clickSelectRequested_ || !pViewProjection_)
        return;
    clickSelectRequested_ = false;

    bool isCtrlPressed = Input::GetInstance()->PushKey(DIK_LCONTROL);
    std::string pickedName;
    bool foundHit = false;

    // マウス位置をシーンウィンドウ相対座標に変換し、スプライト座標系にスケール。
    // scenePosition は ImGui 座標系なので、ImGui のマウス座標(mousePos)を使って整合させる
    // （Input::GetMousePos() はクライアント座標系なのでマルチビューポート時にずれる）。
    float relX = mousePos.x - scenePosition.x;
    float relY = mousePos.y - scenePosition.y;
    float spriteSpaceX = (relX / sceneSize.x) * static_cast<float>(WinApp::GetVirtualWidth());
    float spriteSpaceY = (relY / sceneSize.y) * static_cast<float>(WinApp::GetVirtualHeight());

    // ---- パス1: スクリーン空間ターゲット優先 2D ヒットテスト ----
    float minDist2D = std::numeric_limits<float>::max();
    for (const auto &pair : transformMap_)
    {
        const GizmoTarget &target = pair.second;
        if (!target.selectable || !target.isScreenSpace)
            continue;
        if (!IsCategoryEnabled(target.category))
            continue;
        if (isMultiSelecting_ && selectedNames_.find(pair.first) != selectedNames_.end())
            continue;

        float posX = 0.0f, posY = 0.0f;
        if (target.type == GizmoTarget::Type::Sprite2D)
        {
            if (!target.position2D)
                continue;
            posX = target.position2D->x;
            posY = target.position2D->y;
        }
        else
        {
            if (!target.translate)
                continue;
            posX = target.translate->x;
            posY = target.translate->y;
        }

        float dx = spriteSpaceX - posX;
        float dy = spriteSpaceY - posY;
        float dist = std::sqrt(dx * dx + dy * dy);

        // カスタム判定があれば実際の形状で、無ければ従来どおり原点まわりの円で判定する
        const bool isHit = target.screenHitTest
                               ? target.screenHitTest(Vector2{spriteSpaceX, spriteSpaceY})
                               : (dist <= target.screenHitRadius);

        // 距離は候補が重なった場合の優先度にのみ使う（近い原点のものを優先）
        if (isHit && dist < minDist2D)
        {
            minDist2D = dist;
            pickedName = pair.first;
            foundHit = true;
        }
    }

    if (foundHit)
    {
        // 2D ヒット時は重複候補をリセット
        overlapCandidates_.clear();
        overlapCycleIndex_ = 0;
    }
    else
    {
        // ---- パス2: 3D レイキャスト（全ヒット候補収集）----
        // スクリーン上でクリック位置に中心が近いオブジェクトを優先するため
        // スクリーン距離でソートし、大きなオブジェクト内の小さいオブジェクトを選択しやすくする
        Ray currentRay = Input::GetInstance()->GetCurrentRay();

        struct HitCandidate
        {
            std::string name;
            float rayDist;    // レイ上の距離（カメラからの奥行き）
            float screenDist; // マウスクリックから中心のスクリーン距離
        };
        std::vector<HitCandidate> candidates;

        for (const auto &pair : transformMap_)
        {
            const GizmoTarget &target = pair.second;
            if (!target.selectable || target.isScreenSpace)
                continue;
            if (!IsCategoryEnabled(target.category))
                continue;
            if (target.type == GizmoTarget::Type::BaseObject)
            {
                if (!target.baseObject || !target.baseObject->IsGizmoSelectable())
                    continue;
            }
            if (isMultiSelecting_ && selectedNames_.find(pair.first) != selectedNames_.end())
                continue;

            // モデルの実形状（ローカルAABB）で判定する。
            // 固定サイズの箱で判定していた頃は、地面のような平たいモデルや
            // 細長いモデルで「見た目と掴める場所がずれる」ことになっていた。
            const AABB localBounds = target.GetLocalBounds();
            Matrix4x4 worldMatrix = target.GetWorldMatrix();
            RayHitInfo currentHit;
            bool hit = Input::RayIntersectOBBByMatrix(currentRay, worldMatrix, currentHit, localBounds);

            if (hit)
            {
                // オブジェクト中心のスクリーン投影位置を求め、クリック位置との距離を計算
                float screenDist = std::numeric_limits<float>::max();
                Vector3 screenCenter;
                if (WorldToScreen(target.GetWorldPosition(), screenCenter, scenePosition, sceneSize))
                {
                    float sdx = mousePos.x - screenCenter.x;
                    float sdy = mousePos.y - screenCenter.y;
                    screenDist = std::sqrt(sdx * sdx + sdy * sdy);
                }
                candidates.push_back({pair.first, currentHit.distance, screenDist});
            }
        }

        // スクリーン距離を主キー、レイ距離を副キーでソート
        // → 大スケール emitter に囲まれていても、画面上でクリックに近い小オブジェクトが優先される
        std::sort(candidates.begin(), candidates.end(), [](const HitCandidate &a, const HitCandidate &b) {
            constexpr float kScreenDistThreshold = 20.0f;
            if (std::abs(a.screenDist - b.screenDist) > kScreenDistThreshold)
                return a.screenDist < b.screenDist;
            return a.rayDist < b.rayDist;
        });

        // 重複候補を保存（Tab キーでサイクル可能）
        overlapCandidates_.clear();
        for (const auto &c : candidates)
        {
            overlapCandidates_.push_back({c.name, c.rayDist});
        }
        overlapCycleIndex_ = 0;

        if (!candidates.empty())
        {
            pickedName = candidates[0].name;
            foundHit = true;
        }
    }

    // 選択状態を更新
    if (foundHit && !pickedName.empty())
    {
        if (isCtrlPressed)
        {
            if (selectedNames_.find(pickedName) != selectedNames_.end())
            {
                selectedNames_.erase(pickedName);
            }
            else
            {
                selectedNames_.insert(pickedName);
            }
            isMultiSelecting_ = true;
        }
        else
        {
            selectedNames_.clear();
            selectedNames_.insert(pickedName);
            isMultiSelecting_ = false;
        }
    }
    else
    {
        if (!isCtrlPressed)
        {
            selectedNames_.clear();
            overlapCandidates_.clear();
            overlapCycleIndex_ = 0;
            isMultiSelecting_ = false;
        }
    }

    if (!isCtrlPressed && isMultiSelecting_)
    {
        isMultiSelecting_ = false;
    }
}

// シーンウィンドウ上でだけ効くギズモ操作のホットキー。
// デバッグカメラが WASD を使うため、移動キーと衝突しないキーだけを割り当てている。
void ImGuizmoManager::HandleHotkeys(bool sceneHovered)
{
    // 文字入力中やシーン以外のウィンドウを触っている間は誤爆させない
    if (!sceneHovered || ImGui::GetIO().WantTextInput)
    {
        return;
    }

    Input *input = Input::GetInstance();

    // Ctrl 併用のショートカット（コピー等）と食い合わないよう、単独押しのときだけ反応させる
    const bool ctrlHeld = input->PushKey(DIK_LCONTROL) || input->PushKey(DIK_RCONTROL);

    if (input->TriggerKey(DIK_TAB) && overlapCandidates_.size() > 1)
    {
        CycleOverlapSelection();
    }

    if (ctrlHeld)
    {
        return;
    }

    if (input->TriggerKey(DIK_1))
    {
        currentOperation_ = ImGuizmo::TRANSLATE;
    }
    if (input->TriggerKey(DIK_2))
    {
        currentOperation_ = ImGuizmo::ROTATE;
    }
    if (input->TriggerKey(DIK_3))
    {
        currentOperation_ = ImGuizmo::SCALE;
    }
    if (input->TriggerKey(DIK_4))
    {
        currentMode_ = (currentMode_ == ImGuizmo::LOCAL) ? ImGuizmo::WORLD : ImGuizmo::LOCAL;
        ImGuiNotification::Post(currentMode_ == ImGuizmo::LOCAL ? "座標系: ローカル" : "座標系: ワールド",
                                {0.42f, 0.66f, 0.68f, 1.0f});
    }
    if (input->TriggerKey(DIK_5))
    {
        useSnap_ = !useSnap_;
        ImGuiNotification::Post(useSnap_ ? "スナップ: ON" : "スナップ: OFF", {0.45f, 0.68f, 0.52f, 1.0f});
    }
    if (input->TriggerKey(DIK_F))
    {
        RequestFocusOnSelection();
    }
}

// 選択中ターゲットの重心と大きさからフォーカス要求を立てる
void ImGuizmoManager::RequestFocusOnSelection()
{
    Vector3 center = {0.0f, 0.0f, 0.0f};
    float radius = 0.0f;
    int count = 0;

    for (const std::string &name : selectedNames_)
    {
        auto it = transformMap_.find(name);
        if (it == transformMap_.end() || it->second.isScreenSpace)
        {
            continue;
        }
        const GizmoTarget &target = it->second;
        center = center + target.GetWorldPosition();
        ++count;

        // ローカルAABBにワールドスケールを掛けて、画面に収まる距離の目安にする
        const AABB bounds = target.GetLocalBounds();
        const Matrix4x4 world = target.GetWorldMatrix();
        const Vector3 worldScale = {
            Vector3{world.m[0][0], world.m[0][1], world.m[0][2]}.Length(),
            Vector3{world.m[1][0], world.m[1][1], world.m[1][2]}.Length(),
            Vector3{world.m[2][0], world.m[2][1], world.m[2][2]}.Length()};
        const Vector3 halfExtent = {
            (bounds.max.x - bounds.min.x) * 0.5f * worldScale.x,
            (bounds.max.y - bounds.min.y) * 0.5f * worldScale.y,
            (bounds.max.z - bounds.min.z) * 0.5f * worldScale.z};
        radius = (std::max)(radius, halfExtent.Length());
    }

    if (count == 0)
    {
        ImGuiNotification::Post("フォーカスする対象が選択されていません", {0.82f, 0.58f, 0.36f, 1.0f});
        return;
    }

    focusTarget_ = center / static_cast<float>(count);
    focusRadius_ = (std::max)(radius, 0.5f);
    focusRequested_ = true;
}

// フォーカス要求を取り出す（DebugCamera から毎フレーム呼ばれる）
bool ImGuizmoManager::ConsumeFocusRequest(Vector3 &outTarget, float &outRadius)
{
    if (!focusRequested_)
    {
        return false;
    }
    outTarget = focusTarget_;
    outRadius = focusRadius_;
    focusRequested_ = false;
    return true;
}

// 新規オブジェクトの既定配置位置（カメラ前方）を返す
Vector3 ImGuizmoManager::GetSpawnPosition(float distance) const
{
    if (!pViewProjection_)
    {
        return {0.0f, 0.0f, 0.0f};
    }

    // カメラのワールド行列は行ベクトル規約なので、3行目が位置・2行目がZ軸（視線方向）
    const Matrix4x4 &cameraWorld = pViewProjection_->matWorld_;
    const Vector3 cameraPosition = {cameraWorld.m[3][0], cameraWorld.m[3][1], cameraWorld.m[3][2]};
    const Vector3 forward = Vector3{cameraWorld.m[2][0], cameraWorld.m[2][1], cameraWorld.m[2][2]}.Normalize();

    Vector3 spawn = cameraPosition + forward * distance;

    // スナップ中はグリッドに乗せて出す（並べる作業の初手がずれない）
    if (useSnap_ && snapTranslate_ > 0.0f)
    {
        spawn.x = std::round(spawn.x / snapTranslate_) * snapTranslate_;
        spawn.y = std::round(spawn.y / snapTranslate_) * snapTranslate_;
        spawn.z = std::round(spawn.z / snapTranslate_) * snapTranslate_;
    }
    return spawn;
}

// ---- 矩形（ラバーバンド）選択 ------------------------------------------

void ImGuizmoManager::HandleBoxSelection(const ImVec2 &scenePosition, const ImVec2 &sceneSize, bool sceneHovered)
{
    ImGuiIO &io = ImGui::GetIO();
    const ImVec2 mousePos = io.MousePos;
    const bool isInScene = (mousePos.x >= scenePosition.x && mousePos.x <= scenePosition.x + sceneSize.x &&
                            mousePos.y >= scenePosition.y && mousePos.y <= scenePosition.y + sceneSize.y);

    clickSelectRequested_ = false;

    // ギズモの上で押した場合はギズモ操作を優先する
    if (!isBoxSelecting_)
    {
        if (sceneHovered && isInScene && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGuizmo::IsOver())
        {
            isBoxSelecting_ = true;
            boxSelectStart_ = mousePos;
        }
        return;
    }

    // ドラッグ中は枠を描く。シーンウィンドウの前面に出したいので前景の描画リストを使う。
    const ImVec2 rectMin = {(std::min)(boxSelectStart_.x, mousePos.x), (std::min)(boxSelectStart_.y, mousePos.y)};
    const ImVec2 rectMax = {(std::max)(boxSelectStart_.x, mousePos.x), (std::max)(boxSelectStart_.y, mousePos.y)};
    const float dragWidth = rectMax.x - rectMin.x;
    const float dragHeight = rectMax.y - rectMin.y;
    const bool isDragEnough = (dragWidth >= kBoxSelectThreshold || dragHeight >= kBoxSelectThreshold);

    if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
    {
        if (isDragEnough)
        {
            ImDrawList *drawList = ImGui::GetForegroundDrawList();
            drawList->AddRectFilled(rectMin, rectMax, IM_COL32(90, 150, 220, 40));
            drawList->AddRect(rectMin, rectMax, IM_COL32(120, 190, 255, 220));
        }
        return;
    }

    // ボタンを離した：しきい値を超えていれば矩形選択として確定する。
    // 超えていない場合は「クリックだった」ことにして単体選択へ渡す。
    if (isDragEnough)
    {
        SelectInsideScreenRect(rectMin, rectMax, scenePosition, sceneSize, io.KeyCtrl);
    }
    else
    {
        clickSelectRequested_ = true;
    }
    isBoxSelecting_ = false;
}

void ImGuizmoManager::SelectInsideScreenRect(const ImVec2 &rectMin, const ImVec2 &rectMax,
                                             const ImVec2 &scenePosition, const ImVec2 &sceneSize, bool additive)
{
    if (!additive)
    {
        selectedNames_.clear();
    }

    size_t hitCount = 0;
    for (const auto &[name, target] : transformMap_)
    {
        if (!target.selectable || !IsCategoryEnabled(target.category))
        {
            continue;
        }
        if (target.type == GizmoTarget::Type::BaseObject &&
            (!target.baseObject || !target.baseObject->IsGizmoSelectable()))
        {
            continue;
        }

        // スクリーン空間（スプライト）は仮想解像度座標なので、シーン矩形へ写してから判定する
        ImVec2 screenPoint;
        if (target.isScreenSpace)
        {
            const Vector3 position = target.GetWorldPosition();
            screenPoint.x = scenePosition.x + (position.x / static_cast<float>(WinApp::GetVirtualWidth())) * sceneSize.x;
            screenPoint.y = scenePosition.y + (position.y / static_cast<float>(WinApp::GetVirtualHeight())) * sceneSize.y;
        }
        else
        {
            Vector3 projected;
            if (!WorldToScreen(target.GetWorldPosition(), projected, scenePosition, sceneSize))
            {
                continue; // カメラの後ろにある
            }
            screenPoint = {projected.x, projected.y};
        }

        if (screenPoint.x >= rectMin.x && screenPoint.x <= rectMax.x &&
            screenPoint.y >= rectMin.y && screenPoint.y <= rectMax.y)
        {
            selectedNames_.insert(name);
            ++hitCount;
        }
    }

    // 重なり候補は矩形選択では意味を持たないので畳んでおく
    overlapCandidates_.clear();
    overlapCycleIndex_ = 0;
    isMultiSelecting_ = selectedNames_.size() > 1;

    ImGuiNotification::Post("矩形選択: " + std::to_string(hitCount) + "個", {0.4f, 0.8f, 1.0f, 1.0f});
}

// ---- 整列・等間隔配置・地面スナップ ------------------------------------

namespace {
// ボタン1発で完了する一括操作を Undo 履歴へ積むためのヘルパー。
// ImGuiUndoTracker は「ウィジェット編集ジェスチャ」を追う仕組みなので、
// こうした即時実行のコマンドは Copy/Paste と同様に明示的に Push する。
template <typename Operation>
void RunAsUndoableCommand(const std::string &label, Operation &&operation)
{
    nlohmann::json before = BaseObjectManager::GetInstance()->CaptureUndoState();
    operation();
    nlohmann::json after = BaseObjectManager::GetInstance()->CaptureUndoState();
    if (before == after)
    {
        return; // 何も変わらなかったら履歴を汚さない
    }
    auto [diffBefore, diffAfter] = MakeTopLevelJsonDiff(before, after);
    UndoRedoManager::GetInstance()->Push(std::make_unique<JsonStateCommand>(
        label, std::move(diffBefore), std::move(diffAfter),
        [](const nlohmann::json &s) { BaseObjectManager::GetInstance()->RestoreUndoState(s); }));
}
} // namespace

std::vector<GizmoTarget *> ImGuizmoManager::CollectMovableSelection()
{
    std::vector<GizmoTarget *> targets;
    for (const std::string &name : selectedNames_)
    {
        auto it = transformMap_.find(name);
        if (it == transformMap_.end() || it->second.isScreenSpace)
        {
            continue; // スプライトは3D整列の対象外
        }
        targets.push_back(&it->second);
    }
    // 並びが実行のたびに変わらないよう名前順に固定する（等間隔配置の結果を安定させるため）
    std::sort(targets.begin(), targets.end(),
              [](const GizmoTarget *lhs, const GizmoTarget *rhs) { return lhs->name < rhs->name; });
    return targets;
}

void ImGuizmoManager::SetTargetWorldPosition(GizmoTarget &target, const Vector3 &worldPosition)
{
    // ワールド行列の平行移動成分だけ差し替えて適用する。
    // ApplyWorldMatrix が親のぶんを打ち消してくれるので、親子付けされていても正しく動く。
    Matrix4x4 worldMatrix = target.GetWorldMatrix();
    worldMatrix.m[3][0] = worldPosition.x;
    worldMatrix.m[3][1] = worldPosition.y;
    worldMatrix.m[3][2] = worldPosition.z;
    target.ApplyWorldMatrix(worldMatrix);
}

void ImGuizmoManager::AlignSelected(int axis, AlignMode mode)
{
    if (axis < 0 || axis > 2)
    {
        return;
    }
    std::vector<GizmoTarget *> targets = CollectMovableSelection();
    if (targets.size() < 2)
    {
        ImGuiNotification::Post("整列するには2つ以上選択してください", {0.82f, 0.58f, 0.36f, 1.0f});
        return;
    }

    auto axisOf = [axis](const Vector3 &v) { return (&v.x)[axis]; };

    float minValue = axisOf(targets.front()->GetWorldPosition());
    float maxValue = minValue;
    float sum = 0.0f;
    for (const GizmoTarget *target : targets)
    {
        const float value = axisOf(target->GetWorldPosition());
        minValue = (std::min)(minValue, value);
        maxValue = (std::max)(maxValue, value);
        sum += value;
    }

    float alignedValue = minValue;
    switch (mode)
    {
    case AlignMode::Min:
        alignedValue = minValue;
        break;
    case AlignMode::Center:
        alignedValue = sum / static_cast<float>(targets.size());
        break;
    case AlignMode::Max:
        alignedValue = maxValue;
        break;
    }

    RunAsUndoableCommand("整列", [&] {
        for (GizmoTarget *target : targets)
        {
            Vector3 position = target->GetWorldPosition();
            (&position.x)[axis] = alignedValue;
            SetTargetWorldPosition(*target, position);
        }
    });

    static const char *kAxisNames[3] = {"X", "Y", "Z"};
    ImGuiNotification::Post(std::string("整列しました (") + kAxisNames[axis] + ")", {0.45f, 0.68f, 0.52f, 1.0f});
}

void ImGuizmoManager::DistributeSelected(int axis)
{
    if (axis < 0 || axis > 2)
    {
        return;
    }
    std::vector<GizmoTarget *> targets = CollectMovableSelection();
    if (targets.size() < 3)
    {
        ImGuiNotification::Post("等間隔にするには3つ以上選択してください", {0.82f, 0.58f, 0.36f, 1.0f});
        return;
    }

    // 対象軸の座標順に並べ替えてから、両端の間を均等割りする
    std::sort(targets.begin(), targets.end(), [axis](const GizmoTarget *lhs, const GizmoTarget *rhs) {
        const Vector3 lhsPos = lhs->GetWorldPosition();
        const Vector3 rhsPos = rhs->GetWorldPosition();
        return (&lhsPos.x)[axis] < (&rhsPos.x)[axis];
    });

    const Vector3 firstPos = targets.front()->GetWorldPosition();
    const Vector3 lastPos = targets.back()->GetWorldPosition();
    const float begin = (&firstPos.x)[axis];
    const float end = (&lastPos.x)[axis];
    const float step = (end - begin) / static_cast<float>(targets.size() - 1);

    RunAsUndoableCommand("等間隔配置", [&] {
        // 両端は動かさない
        for (size_t i = 1; i + 1 < targets.size(); ++i)
        {
            Vector3 position = targets[i]->GetWorldPosition();
            (&position.x)[axis] = begin + step * static_cast<float>(i);
            SetTargetWorldPosition(*targets[i], position);
        }
    });

    static const char *kAxisNames[3] = {"X", "Y", "Z"};
    ImGuiNotification::Post(std::string("等間隔に並べました (") + kAxisNames[axis] + ")", {0.45f, 0.68f, 0.52f, 1.0f});
}

void ImGuizmoManager::SnapSelectedToGround()
{
    std::vector<GizmoTarget *> targets = CollectMovableSelection();
    if (targets.empty())
    {
        ImGuiNotification::Post("接地させる対象が選択されていません", {0.82f, 0.58f, 0.36f, 1.0f});
        return;
    }

    // 選択されているものどうしでぶつからないよう、選択外のオブジェクトだけを床候補にする
    std::vector<const GizmoTarget *> groundCandidates;
    for (const auto &[name, target] : transformMap_)
    {
        if (target.isScreenSpace || target.type != GizmoTarget::Type::BaseObject || !target.baseObject)
        {
            continue;
        }
        if (selectedNames_.find(name) != selectedNames_.end())
        {
            continue;
        }
        groundCandidates.push_back(&target);
    }

    // 対象1つを真下の地面へ落とす
    auto snapOne = [&](GizmoTarget &target) {
        const Vector3 position = target.GetWorldPosition();

        // オブジェクトの底面がどれだけ下にあるかを求める（原点が足元でないモデルへの対応）
        const AABB localBounds = target.GetLocalBounds();
        const Matrix4x4 worldMatrix = target.GetWorldMatrix();
        const float scaleY = Vector3{worldMatrix.m[1][0], worldMatrix.m[1][1], worldMatrix.m[1][2]}.Length();
        const float bottomOffset = localBounds.min.y * scaleY;

        // 十分上から真下へレイを飛ばす（すでにめり込んでいる場合も拾えるように）
        Ray downRay;
        downRay.origin = {position.x, position.y + 1000.0f, position.z};
        downRay.direction = {0.0f, -1.0f, 0.0f};
        downRay.length = 100000.0f;

        bool found = false;
        float highestHitY = 0.0f;
        for (const GizmoTarget *candidate : groundCandidates)
        {
            RayHitInfo hit;
            if (!Input::RayIntersectOBBByMatrix(downRay, candidate->GetWorldMatrix(), hit, candidate->GetLocalBounds()))
            {
                continue;
            }
            // 上から撃っているので、一番高い交点がその地点の地面になる
            if (!found || hit.hitPoint.y > highestHitY)
            {
                highestHitY = hit.hitPoint.y;
                found = true;
            }
        }

        // 何も無ければ Y=0 の地面へ落とす
        const float groundY = found ? highestHitY : 0.0f;
        SetTargetWorldPosition(target, {position.x, groundY - bottomOffset, position.z});
    };

    RunAsUndoableCommand("地面へ接地", [&] {
        for (GizmoTarget *target : targets)
        {
            snapOne(*target);
        }
    });

    ImGuiNotification::Post("地面に接地させました: " + std::to_string(targets.size()) + "個", {0.45f, 0.68f, 0.52f, 1.0f});
}

// マウスカーソルの指す先（地面との交点）を返す
Vector3 ImGuizmoManager::GetSpawnPositionUnderCursor(float fallbackDistance) const
{
    const Ray ray = Input::GetInstance()->GetCurrentRay();

    // ray.length == 0 はシーン外を指している（Input::CreateRayFromMouse の無効レイ）
    if (ray.length > 0.0f)
    {
        // 地面（Y=0平面）との交点を求める。真横を向いている場合は交わらない扱いにする。
        constexpr float kEpsilon = 1e-4f;
        if (std::abs(ray.direction.y) > kEpsilon)
        {
            const float distance = -ray.origin.y / ray.direction.y;
            // カメラの後ろ側や遠すぎる交点は使わない（地平線の彼方に飛ばさないため）
            if (distance > 0.0f && distance <= ray.length)
            {
                Vector3 hit = ray.origin + ray.direction * distance;
                if (useSnap_ && snapTranslate_ > 0.0f)
                {
                    hit.x = std::round(hit.x / snapTranslate_) * snapTranslate_;
                    hit.y = std::round(hit.y / snapTranslate_) * snapTranslate_;
                    hit.z = std::round(hit.z / snapTranslate_) * snapTranslate_;
                }
                return hit;
            }
        }
    }

    // 地面が拾えないときはカメラ前方へ置く
    return GetSpawnPosition(fallbackDistance);
}

// Tab キーで重複候補をサイクルして次のオブジェクトを選択する
void ImGuizmoManager::CycleOverlapSelection()
{
    if (overlapCandidates_.size() <= 1)
        return;
    overlapCycleIndex_ = (overlapCycleIndex_ + 1) % static_cast<int>(overlapCandidates_.size());
    selectedNames_.clear();
    selectedNames_.insert(overlapCandidates_[overlapCycleIndex_].first);
}

// ---- DisplayGizmo -----------------------------------------------------

// 選択中の全エントリの重心位置にギズモを表示し、操作量を各エントリに反映する
void ImGuizmoManager::DisplayGizmo(const ImVec2 &scenePosition, const ImVec2 &sceneSize)
{
    if (!pViewProjection_ || selectedNames_.empty())
        return;

    std::vector<GizmoTarget *> selectedTargets;
    for (const std::string &name : selectedNames_)
    {
        auto it = transformMap_.find(name);
        if (it != transformMap_.end())
        {
            selectedTargets.push_back(&it->second);
        }
    }
    if (selectedTargets.empty())
        return;

    // 親と子を同時に選んでいる場合に、子を先に動かすと親の移動で二重にずれる。
    // 必ず親から順に適用できるよう、階層の浅い順に並べておく。
    std::stable_sort(selectedTargets.begin(), selectedTargets.end(),
                     [](const GizmoTarget *lhs, const GizmoTarget *rhs) {
                         auto depthOf = [](const GizmoTarget *target) {
                             const WorldTransform *transform = nullptr;
                             if (target->type == GizmoTarget::Type::BaseObject && target->baseObject)
                             {
                                 transform = target->baseObject->GetWorldTransform();
                             }
                             else if (target->type == GizmoTarget::Type::WorldTransform)
                             {
                                 transform = target->worldTransform;
                             }
                             int depth = 0;
                             for (const WorldTransform *node = transform; node && node->pParent_; node = node->pParent_)
                             {
                                 ++depth;
                             }
                             return depth;
                         };
                         return depthOf(lhs) < depthOf(rhs);
                     });

    // 選択中にスクリーン空間（Sprite）が含まれるか確認
    // ※ 3Dオブジェクトとスプライトを同時選択した場合は動作が未定義
    bool anyScreenSpace = std::any_of(selectedTargets.begin(), selectedTargets.end(),
                                      [](const GizmoTarget *t) { return t->isScreenSpace; });

    // ギズモを置く基準行列（ピボット）を決める。
    // ・単一選択の3D対象 … その対象のワールド行列そのもの
    //   （回転・拡縮がその場で効き、「ローカル」座標系もオブジェクトの向きを向く）
    // ・複数選択／スクリーン空間 … 重心位置の無回転行列を共通ピボットにする
    // スケール0の軸があるオブジェクトの行列をそのまま渡すと、ImGuizmo 内部の逆行列計算が
    // 破綻して座標が NaN になる。その場合は重心ピボットへ逃がす。
    auto isDegenerate = [](const Matrix4x4 &matrix) {
        constexpr float kEpsilon = 1e-5f;
        for (int row = 0; row < 3; ++row)
        {
            if (Vector3{matrix.m[row][0], matrix.m[row][1], matrix.m[row][2]}.Length() <= kEpsilon)
            {
                return true;
            }
        }
        return false;
    };

    const bool useTargetMatrix = (selectedTargets.size() == 1 && !anyScreenSpace &&
                                  !isDegenerate(selectedTargets[0]->GetWorldMatrix()));

    Matrix4x4 pivotMatrix;
    if (useTargetMatrix)
    {
        pivotMatrix = selectedTargets[0]->GetWorldMatrix();
    }
    else
    {
        Vector3 centerPos = {0.0f, 0.0f, 0.0f};
        for (GizmoTarget *target : selectedTargets)
        {
            centerPos = centerPos + target->GetWorldPosition();
        }
        centerPos = centerPos / static_cast<float>(selectedTargets.size());

        pivotMatrix = MakeIdentity4x4();
        pivotMatrix.m[3][0] = centerPos.x;
        pivotMatrix.m[3][1] = centerPos.y;
        pivotMatrix.m[3][2] = centerPos.z;
    }
    const Matrix4x4 centerMatrix = pivotMatrix;

    float matrixArray[16];
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            matrixArray[i * 4 + j] = centerMatrix.m[i][j];

    float viewArray[16], projArray[16];

    if (anyScreenSpace)
    {
        // スプライト用：単位ビュー行列 + スプライトと同じ正射影行列
        // これにより ImGuizmo がピクセル座標系でギズモを正しい位置に描画する
        Matrix4x4 identView = MakeIdentity4x4();
        Matrix4x4 orthoProj = MakeOrthographicMatrix(
            0.0f, 0.0f,
            static_cast<float>(WinApp::GetVirtualWidth()),
            static_cast<float>(WinApp::GetVirtualHeight()),
            0.0f, 100.0f);

        for (int i = 0; i < 4; ++i)
        {
            for (int j = 0; j < 4; ++j)
            {
                viewArray[i * 4 + j] = identView.m[i][j];
                projArray[i * 4 + j] = orthoProj.m[i][j];
            }
        }
    }
    else
    {
        // 3Dオブジェクト用：カメラの View/Projection をそのまま使用
        for (int i = 0; i < 4; ++i)
        {
            for (int j = 0; j < 4; ++j)
            {
                viewArray[i * 4 + j] = pViewProjection_->matView_.m[i][j];
                projArray[i * 4 + j] = pViewProjection_->matProjection_.m[i][j];
            }
        }
    }

    // スクリーン空間（Sprite 等）は XY 移動のみ許可
    ImGuizmo::OPERATION effectiveOp = currentOperation_;
    if (anyScreenSpace)
    {
        effectiveOp = ImGuizmo::TRANSLATE_X | ImGuizmo::TRANSLATE_Y;
    }

    // 正射影かどうかを ImGuizmo に伝える（グローバル状態なので毎回明示的に設定する）。
    // これを怠ると、スプライト用の正射影（nearClip=0）では原点の clip z が 0 になり、
    // ImGuizmo の「カメラ後方」判定 (mIsOrthographic==false && z < 0.001) に引っかかって
    // Manipulate が描画前に return し、ギズモが一切表示されない。
    ImGuizmo::SetOrthographic(anyScreenSpace);

    // スナップ値は操作モードごとに意味が変わる（移動=距離 / 回転=度 / 拡縮=倍率）。
    // Shift 押下中は設定を一時的に反転させ、ON時の微調整・OFF時の一時吸着を両立させる。
    const bool shiftHeld = ImGui::GetIO().KeyShift;
    const bool snapActive = (useSnap_ != shiftHeld) && !anyScreenSpace;
    float snapValues[3] = {snapTranslate_, snapTranslate_, snapTranslate_};
    if (currentOperation_ == ImGuizmo::ROTATE)
    {
        snapValues[0] = snapValues[1] = snapValues[2] = snapRotateDegree_;
    }
    else if (currentOperation_ == ImGuizmo::SCALE)
    {
        snapValues[0] = snapValues[1] = snapValues[2] = snapScale_;
    }

    if (ImGuizmo::Manipulate(viewArray, projArray, effectiveOp, currentMode_, matrixArray,
                             nullptr, snapActive ? snapValues : nullptr))
    {
        Matrix4x4 newMatrix;
        for (int i = 0; i < 4; ++i)
            for (int j = 0; j < 4; ++j)
                newMatrix.m[i][j] = matrixArray[i * 4 + j];

        if (useTargetMatrix)
        {
            // 単一選択はギズモの結果をそのまま反映する（余計な行列演算を挟まず誤差を出さない）
            selectedTargets[0]->ApplyWorldMatrix(newMatrix);
        }
        else
        {
            // 複数選択はピボット基準の差分を全員に掛ける。
            // 行ベクトル規約なので「ピボット空間へ戻す → 新しいピボットへ乗せ直す」の順に掛ける。
            const Matrix4x4 pivotDelta = Inverse(centerMatrix) * newMatrix;

            // 親と子を同時に選んでいる場合、親を先に動かすと子のワールド行列が更新されてしまい、
            // その後に子へ差分を掛けると二重に効いてしまう。適用前に全員の行列を控えておく。
            std::vector<Matrix4x4> beforeMatrices;
            beforeMatrices.reserve(selectedTargets.size());
            for (const GizmoTarget *target : selectedTargets)
            {
                beforeMatrices.push_back(target->GetWorldMatrix());
            }

            for (size_t i = 0; i < selectedTargets.size(); ++i)
            {
                GizmoTarget *target = selectedTargets[i];
                if (target->isScreenSpace)
                {
                    // スクリーン空間はピクセル単位の XY 平行移動のみ
                    target->ApplyTranslationDelta({newMatrix.m[3][0] - centerMatrix.m[3][0],
                                                   newMatrix.m[3][1] - centerMatrix.m[3][1],
                                                   0.0f});
                    continue;
                }
                target->ApplyWorldMatrix(beforeMatrices[i] * pivotDelta);
            }
        }
    }
}

// ---- DecomposeMatrix --------------------------------------------------

// 行列からスケール・回転（クォータニオン）・位置を分解して返す
void ImGuizmoManager::DecomposeMatrix(const Matrix4x4 &matrix, Vector3 &position, Quaternion &rotation, Vector3 &scale)
{
    position = {matrix.m[3][0], matrix.m[3][1], matrix.m[3][2]};

    Vector3 col0 = {matrix.m[0][0], matrix.m[0][1], matrix.m[0][2]};
    Vector3 col1 = {matrix.m[1][0], matrix.m[1][1], matrix.m[1][2]};
    Vector3 col2 = {matrix.m[2][0], matrix.m[2][1], matrix.m[2][2]};

    scale.x = col0.Length();
    scale.y = col1.Length();
    scale.z = col2.Length();

    Matrix4x4 rotMatrix = matrix;
    if (scale.x != 0.0f)
    {
        rotMatrix.m[0][0] /= scale.x;
        rotMatrix.m[0][1] /= scale.x;
        rotMatrix.m[0][2] /= scale.x;
    }
    if (scale.y != 0.0f)
    {
        rotMatrix.m[1][0] /= scale.y;
        rotMatrix.m[1][1] /= scale.y;
        rotMatrix.m[1][2] /= scale.y;
    }
    if (scale.z != 0.0f)
    {
        rotMatrix.m[2][0] /= scale.z;
        rotMatrix.m[2][1] /= scale.z;
        rotMatrix.m[2][2] /= scale.z;
    }

    rotation = Quaternion::FromMatrix(rotMatrix);
}

// ---- WorldToScreen ----------------------------------------------------

// ワールド座標をシーンウィンドウのスクリーン座標に変換する
bool ImGuizmoManager::WorldToScreen(const Vector3 &worldPos, Vector3 &screenPos, const ImVec2 &scenePosition, const ImVec2 &sceneSize)
{
    Vector4 clipPos;
    {
        Vector3 v = worldPos;
        float x = v.x * pViewProjection_->matView_.m[0][0] + v.y * pViewProjection_->matView_.m[1][0] + v.z * pViewProjection_->matView_.m[2][0] + pViewProjection_->matView_.m[3][0];
        float y = v.x * pViewProjection_->matView_.m[0][1] + v.y * pViewProjection_->matView_.m[1][1] + v.z * pViewProjection_->matView_.m[2][1] + pViewProjection_->matView_.m[3][1];
        float z = v.x * pViewProjection_->matView_.m[0][2] + v.y * pViewProjection_->matView_.m[1][2] + v.z * pViewProjection_->matView_.m[2][2] + pViewProjection_->matView_.m[3][2];
        float w = v.x * pViewProjection_->matView_.m[0][3] + v.y * pViewProjection_->matView_.m[1][3] + v.z * pViewProjection_->matView_.m[2][3] + pViewProjection_->matView_.m[3][3];

        clipPos.x = x * pViewProjection_->matProjection_.m[0][0] + y * pViewProjection_->matProjection_.m[1][0] + z * pViewProjection_->matProjection_.m[2][0] + w * pViewProjection_->matProjection_.m[3][0];
        clipPos.y = x * pViewProjection_->matProjection_.m[0][1] + y * pViewProjection_->matProjection_.m[1][1] + z * pViewProjection_->matProjection_.m[2][1] + w * pViewProjection_->matProjection_.m[3][1];
        clipPos.z = x * pViewProjection_->matProjection_.m[0][2] + y * pViewProjection_->matProjection_.m[1][2] + z * pViewProjection_->matProjection_.m[2][2] + w * pViewProjection_->matProjection_.m[3][2];
        clipPos.w = x * pViewProjection_->matProjection_.m[0][3] + y * pViewProjection_->matProjection_.m[1][3] + z * pViewProjection_->matProjection_.m[2][3] + w * pViewProjection_->matProjection_.m[3][3];
    }

    if (clipPos.w <= 0.0f)
        return false;

    float ndcX = clipPos.x / clipPos.w;
    float ndcY = clipPos.y / clipPos.w;

    screenPos.x = scenePosition.x + (ndcX * 0.5f + 0.5f) * sceneSize.x;
    screenPos.y = scenePosition.y + (0.5f - ndcY * 0.5f) * sceneSize.y;
    screenPos.z = clipPos.z / clipPos.w;

    return true;
}

// ---- GenerateUniqueName -----------------------------------------------

// 同名エントリが存在しないユニークな名前を生成する
std::string ImGuizmoManager::GenerateUniqueName(const std::string &baseName)
{
    std::string newName;
    int counter = 1;

    std::string cleanBaseName = baseName;
    size_t underscorePos = baseName.find_last_of('_');
    if (underscorePos != std::string::npos)
    {
        std::string suffix = baseName.substr(underscorePos + 1);
        bool isNumber = true;
        for (char c : suffix)
        {
            if (!std::isdigit(c))
            {
                isNumber = false;
                break;
            }
        }
        if (isNumber)
            cleanBaseName = baseName.substr(0, underscorePos);
    }

    do
    {
        newName = cleanBaseName + "_" + std::to_string(counter++);
    } while (transformMap_.find(newName) != transformMap_.end());

    return newName;
}

// ---- CopySelectedObjects / PasteObjects / DeleteSelectedObjects --------

// 選択中の BaseObject をコピーバッファに保存する（非 BaseObject はスキップ）
void ImGuizmoManager::CopySelectedObjects()
{
    copiedNames_.clear();
    for (const std::string &name : selectedNames_)
    {
        auto it = transformMap_.find(name);
        if (it != transformMap_.end() && it->second.type == GizmoTarget::Type::BaseObject)
        {
            copiedNames_.push_back(name);
        }
    }
    if (!copiedNames_.empty())
    {
        ImGuiNotification::Post("コピーしました: " + std::to_string(copiedNames_.size()) + "個",
                                {0.4f, 0.8f, 1.0f, 1.0f});
    }
}

// BaseObject を複製して BaseObjectManager へ追加する（貼り付け・複製の共通処理）
std::string ImGuizmoManager::CloneObject(BaseObject *pSource, const Vector3 &offset)
{
    if (!pSource)
        return {};

    // 名前は先に決める。Init 前に確定させないと DataHandler が元の名前で作られてしまう
    const std::string uniqueName = GenerateUniqueName(pSource->GetName());

    std::unique_ptr<BaseObject> newObject = std::make_unique<BaseObject>();
    newObject->SetPrimitive(pSource->IsPrimitive());
    newObject->Init(uniqueName);

    if (!pSource->GetModelPath().empty())
    {
        newObject->CreateModel(pSource->GetModelPath());
    }
    else if (pSource->GetPrimitiveType() != PrimitiveType::Count)
    {
        newObject->CreatePrimitiveModel(pSource->GetPrimitiveType());
    }
    else
    {
        return {}; // モデルもプリミティブも無いものは複製できない
    }

    // マテリアルごとのテクスチャ・色を全て引き継ぐ
    // （0番だけコピーしていた頃は、複数マテリアルのモデルで見た目が変わってしまっていた）
    const int materialCount = newObject->GetObject3d() ? static_cast<int>(newObject->GetObject3d()->GetMaterialCount()) : 0;
    const int textureCount = pSource->IsPrimitive() ? (materialCount > 0 ? 1 : 0) : materialCount;
    for (int i = 0; i < textureCount; ++i)
    {
        newObject->SetTexture(pSource->GetTexturePath(i), i);
    }
    for (int i = 0; i < materialCount; ++i)
    {
        newObject->SetColor(pSource->GetColor(i), i);
    }

    newObject->GetLocalPosition() = pSource->GetLocalPosition() + offset;
    newObject->GetLocalRotation() = pSource->GetLocalRotation();
    newObject->GetLocalScale() = pSource->GetLocalScale();
    newObject->GetLighting() = pSource->GetLighting();
    newObject->SetShouldSave(pSource->GetShouldSave());

    // AddObject → RegisterExternal 内でギズモへの登録も行われる
    BaseObjectManager::GetInstance()->AddObject(std::move(newObject));
    return uniqueName;
}

// コピー済み BaseObject を複製して BaseObjectManager に追加する
void ImGuizmoManager::PasteObjects()
{
    if (copiedNames_.empty())
        return;

    // ショートカット起点の操作はImGuiの編集ジェスチャに乗らないため、明示的にUndo履歴へ積む
    nlohmann::json undoBefore = BaseObjectManager::GetInstance()->CaptureUndoState();

    selectedNames_.clear();

    for (const std::string &copiedName : copiedNames_)
    {
        // コピー後に元が消えている可能性があるので、貼り付け時に引き直す
        BaseObject *copiedObj = BaseObjectManager::GetInstance()->GetObjectByName(copiedName);
        const std::string uniqueName = CloneObject(copiedObj, {1.0f, 0.0f, 0.0f});
        if (!uniqueName.empty())
        {
            selectedNames_.insert(uniqueName);
        }
    }

    if (selectedNames_.empty())
    {
        ImGuiNotification::Post("貼り付け元のオブジェクトが見つかりませんでした", {0.82f, 0.58f, 0.36f, 1.0f});
        return;
    }

    // 連続で貼り付けられるようコピーバッファは保持する（従来はここで消えていた）

    // 貼り付け操作をUndo履歴へ積む
    {
        nlohmann::json undoAfter = BaseObjectManager::GetInstance()->CaptureUndoState();
        auto [diffBefore, diffAfter] = MakeTopLevelJsonDiff(undoBefore, undoAfter);
        UndoRedoManager::GetInstance()->Push(std::make_unique<JsonStateCommand>(
            "オブジェクト貼り付け", std::move(diffBefore), std::move(diffAfter),
            [](const nlohmann::json &s) { BaseObjectManager::GetInstance()->RestoreUndoState(s); }));
    }

    ImGuiNotification::Post("オブジェクトを貼り付けました", {0.4f, 0.8f, 1.0f, 1.0f});
}

// 選択中の BaseObject をその場で複製し、複製後を選択状態にする
void ImGuizmoManager::DuplicateSelectedObjects()
{
    // 複製元を先に確定させる。複製すると transformMap_ が増えるため、走査中に追加すると壊れる
    std::vector<BaseObject *> sources = GetSelectedTargets();
    if (sources.empty())
        return;

    nlohmann::json undoBefore = BaseObjectManager::GetInstance()->CaptureUndoState();

    selectedNames_.clear();
    for (BaseObject *source : sources)
    {
        const std::string uniqueName = CloneObject(source, {0.0f, 0.0f, 0.0f});
        if (!uniqueName.empty())
        {
            selectedNames_.insert(uniqueName);
        }
    }

    {
        nlohmann::json undoAfter = BaseObjectManager::GetInstance()->CaptureUndoState();
        auto [diffBefore, diffAfter] = MakeTopLevelJsonDiff(undoBefore, undoAfter);
        UndoRedoManager::GetInstance()->Push(std::make_unique<JsonStateCommand>(
            "オブジェクト複製", std::move(diffBefore), std::move(diffAfter),
            [](const nlohmann::json &s) { BaseObjectManager::GetInstance()->RestoreUndoState(s); }));
    }

    ImGuiNotification::Post("オブジェクトを複製しました: " + std::to_string(sources.size()) + "個",
                            {0.4f, 0.8f, 1.0f, 1.0f});
}

// 選択中の全エントリを削除する
// BaseObject の場合は BaseObjectManager からも削除する
void ImGuizmoManager::DeleteSelectedObjects()
{
    if (selectedNames_.empty())
        return;

    // ショートカット起点の操作はImGuiの編集ジェスチャに乗らないため、明示的にUndo履歴へ積む
    nlohmann::json undoBefore = BaseObjectManager::GetInstance()->CaptureUndoState();

    // selectedNames_ を走査しながら消すと、削除経路が選択集合に触った時点で壊れる。
    // 先に対象名を確定させてから消す。
    const std::vector<std::string> targets(selectedNames_.begin(), selectedNames_.end());
    const size_t count = targets.size();
    for (const std::string &name : targets)
    {
        auto it = transformMap_.find(name);
        if (it != transformMap_.end() && it->second.type == GizmoTarget::Type::BaseObject)
        {
            // RemoveObject 側でギズモ・モーションエディタからの登録解除も行われる
            BaseObjectManager::GetInstance()->RemoveObject(name);
        }
        transformMap_.erase(name);

        // 消えたオブジェクトをコピーバッファに残さない
        copiedNames_.erase(std::remove(copiedNames_.begin(), copiedNames_.end(), name), copiedNames_.end());
    }

    // 削除操作をUndo履歴へ積む
    {
        nlohmann::json undoAfter = BaseObjectManager::GetInstance()->CaptureUndoState();
        auto [diffBefore, diffAfter] = MakeTopLevelJsonDiff(undoBefore, undoAfter);
        UndoRedoManager::GetInstance()->Push(std::make_unique<JsonStateCommand>(
            "オブジェクト削除", std::move(diffBefore), std::move(diffAfter),
            [](const nlohmann::json &s) { BaseObjectManager::GetInstance()->RestoreUndoState(s); }));
    }

    UpdateFilteredNames();
    // 削除後に別のオブジェクトを勝手に選ぶと、続けて Delete を押したときに
    // 意図しないものを消してしまうので、選択は空のままにする
    selectedNames_.clear();
    overlapCandidates_.clear();
    overlapCycleIndex_ = 0;

    ImGuiNotification::Post("選択オブジェクトを削除しました: " + std::to_string(count) + "個", {0.9f, 0.7f, 0.2f, 1.0f});
}

// ---- DrawSelectedObjectHighlight / DrawSelectionMarker ----------------

// 選択中の全エントリにハイライトマーカーを描画する
void ImGuizmoManager::DrawSelectedObjectHighlight()
{
    if (selectedNames_.empty() || !pViewProjection_)
        return;

    for (const std::string &selectedName : selectedNames_)
    {
        auto it = transformMap_.find(selectedName);
        if (it == transformMap_.end())
            continue;

        // スクリーン空間ターゲットはピクセル座標を3D世界座標として扱えないためスキップ
        if (it->second.isScreenSpace)
            continue;

        DrawSelectionMarker(it->second.GetWorldPosition());
    }
}

// オブジェクトの上方に逆ピラミッド型の選択マーカーを描画する
void ImGuizmoManager::DrawSelectionMarker(const Vector3 &worldPosition)
{
    Vector3 markerPos = worldPosition + Vector3(0.0f, 2.0f, 0.0f);
    Vector4 markerColor = {1.0f, 1.0f, 0.0f, 1.0f};
    float markerSize = 0.5f;

    Vector3 apex = markerPos - Vector3(0.0f, markerSize, 0.0f);
    Vector3 topLeft = markerPos + Vector3(-markerSize, markerSize, -markerSize);
    Vector3 topRight = markerPos + Vector3(markerSize, markerSize, -markerSize);
    Vector3 topFront = markerPos + Vector3(-markerSize, markerSize, markerSize);
    Vector3 topBack = markerPos + Vector3(markerSize, markerSize, markerSize);

    LineRenderer *pLine = LineRenderer::GetInstance();
    pLine->AddLine(apex, topLeft, markerColor);
    pLine->AddLine(apex, topRight, markerColor);
    pLine->AddLine(apex, topFront, markerColor);
    pLine->AddLine(apex, topBack, markerColor);
    pLine->AddLine(topLeft, topRight, markerColor);
    pLine->AddLine(topRight, topBack, markerColor);
    pLine->AddLine(topBack, topFront, markerColor);
    pLine->AddLine(topFront, topLeft, markerColor);
}

// ---- UpdateFilteredNames ----------------------------------------------

// 検索バッファに基づいてフィルタ済みの名前リストを更新する
void ImGuizmoManager::UpdateFilteredNames()
{
    filteredNames_.clear();

    std::vector<std::string> allNames;
    for (const auto &pair : transformMap_)
    {
        // 操作対象フィルタで無効化された種類は一覧に出さない
        if (!IsCategoryEnabled(pair.second.category))
            continue;
        allNames.push_back(pair.first);
    }
    std::sort(allNames.begin(), allNames.end());

    std::string searchStr = searchBuffer_;
    std::transform(searchStr.begin(), searchStr.end(), searchStr.begin(), ::tolower);

    for (const std::string &name : allNames)
    {
        if (strlen(searchBuffer_) == 0)
        {
            filteredNames_.push_back(name);
        }
        else
        {
            std::string lowerName = name;
            std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
            if (lowerName.find(searchStr) != std::string::npos)
            {
                filteredNames_.push_back(name);
            }
        }
    }
}

// ---- DrawDebugRaycast / DrawAABBWireframe / DrawSphereWireframe -------

// 全エントリのAABB・スフィアワイヤーフレームとレイを描画する
void ImGuizmoManager::DrawDebugRaycast()
{
    if (!showDebugRaycast_)
        return;

    Ray currentRay = Input::GetInstance()->GetCurrentRay();
    if (showDebugHitPoints_)
    {
        Vector3 rayEnd = currentRay.origin + (currentRay.direction * currentRay.length);
        LineRenderer::GetInstance()->AddLine(currentRay.origin, rayEnd, {1.0f, 0.0f, 0.0f, 1.0f});
    }

    for (const auto &pair : transformMap_)
    {
        const GizmoTarget &target = pair.second;

        // スクリーン空間ターゲットは3Dデバッグ描画対象外
        if (target.isScreenSpace)
            continue;
        // 操作対象フィルタで無効化された種類は描画しない（画面の見やすさのため）
        if (!IsCategoryEnabled(target.category))
            continue;

        bool isSelected = selectedNames_.find(pair.first) != selectedNames_.end();
        // 全オブジェクトぶんの枠を出すと配置作業中の画面が線だらけになるので、
        // 既定では選択中のものだけ描く
        if (debugSelectedOnly_ && !isSelected)
            continue;

        Matrix4x4 worldMatrix = target.GetWorldMatrix();
        const AABB localBounds = target.GetLocalBounds();
        Vector4 aabbColor = isSelected ? Vector4{1.0f, 1.0f, 0.0f, 1.0f} : Vector4{0.0f, 0.0f, 1.0f, 1.0f};
        Vector4 sphereColor = isSelected ? Vector4{1.0f, 0.5f, 0.0f, 1.0f} : Vector4{1.0f, 0.0f, 1.0f, 1.0f};

        if (showDebugAABB_)
            DrawAABBWireframe(worldMatrix, localBounds, aabbColor);
        if (showDebugSphere_)
            DrawSphereWireframe(worldMatrix, localBounds, sphereColor);
        if (showDebugHitPoints_)
            TestAndDrawRayHit(currentRay, target);
    }
}

// ローカル空間のAABBをワールド変換してワイヤーフレームを描画する
void ImGuizmoManager::DrawAABBWireframe(const Matrix4x4 &worldMatrix, const AABB &aabb, const Vector4 &color)
{
    Vector3 vertices[8] = {
        {aabb.min.x, aabb.min.y, aabb.min.z},
        {aabb.max.x, aabb.min.y, aabb.min.z},
        {aabb.max.x, aabb.min.y, aabb.max.z},
        {aabb.min.x, aabb.min.y, aabb.max.z},
        {aabb.min.x, aabb.max.y, aabb.min.z},
        {aabb.max.x, aabb.max.y, aabb.min.z},
        {aabb.max.x, aabb.max.y, aabb.max.z},
        {aabb.min.x, aabb.max.y, aabb.max.z},
    };

    for (int i = 0; i < 8; i++)
    {
        vertices[i] = Transformation(vertices[i], worldMatrix);
    }

    // vertices は 0-3 が下面、4-7 が対応する上面。AddBoxCorners の並びと一致する
    LineRenderer::GetInstance()->AddBoxCorners(vertices, color);
}

// ローカルAABBに外接するスフィアのワイヤーフレームを描画する
void ImGuizmoManager::DrawSphereWireframe(const Matrix4x4 &worldMatrix, const AABB &localBounds, const Vector4 &color)
{
    const Vector3 localCenter = {
        (localBounds.max.x + localBounds.min.x) * 0.5f,
        (localBounds.max.y + localBounds.min.y) * 0.5f,
        (localBounds.max.z + localBounds.min.z) * 0.5f};
    const Vector3 localHalfExtent = {
        (localBounds.max.x - localBounds.min.x) * 0.5f,
        (localBounds.max.y - localBounds.min.y) * 0.5f,
        (localBounds.max.z - localBounds.min.z) * 0.5f};

    Vector3 worldCenter = Transformation(localCenter, worldMatrix);

    // 行ベクトル規約なので各行が基底ベクトル。その長さがワールドスケールになる
    Vector3 scale = {
        Vector3{worldMatrix.m[0][0], worldMatrix.m[0][1], worldMatrix.m[0][2]}.Length(),
        Vector3{worldMatrix.m[1][0], worldMatrix.m[1][1], worldMatrix.m[1][2]}.Length(),
        Vector3{worldMatrix.m[2][0], worldMatrix.m[2][1], worldMatrix.m[2][2]}.Length()};
    float worldRadius = Vector3{localHalfExtent.x * scale.x,
                                localHalfExtent.y * scale.y,
                                localHalfExtent.z * scale.z}
                            .Length();

    LineRenderer::GetInstance()->AddSphere(worldCenter, worldRadius, color, 16);
}

// GizmoTarget のワールド行列を使ってAABBのレイヒット点を描画する
void ImGuizmoManager::TestAndDrawRayHit(const Ray &ray, const GizmoTarget &target)
{
    Matrix4x4 worldMatrix = target.GetWorldMatrix();
    const AABB aabb = target.GetLocalBounds();

    RayHitInfo aabbHit;
    if (!Input::RayIntersectOBBByMatrix(ray, worldMatrix, aabbHit, aabb))
    {
        return;
    }

    LineRenderer *pLine = LineRenderer::GetInstance();
    pLine->AddSphere(aabbHit.hitPoint, 0.05f, {0.0f, 1.0f, 0.0f, 1.0f}, 8);
    Vector3 normalEnd = aabbHit.hitPoint + (aabbHit.hitNormal * 0.3f);
    pLine->AddLine(aabbHit.hitPoint, normalEnd, {0.0f, 1.0f, 0.0f, 1.0f});
}

} // namespace Hagine
#endif // USE_IMGUI
