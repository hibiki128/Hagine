#define NOMINMAX
#include "Collider.h"
#include "CollisionManager.h"
#include <line/DrawLine3D.h>

int Collider::counter = -1; // 初期値を-1に変更

Collider::Collider() {
}

Collider::~Collider() {
    CollisionManager::RemoveCollider(this);
    counter--; // カウンターをデクリメント
}

Collider &Collider::AddCollider(const std::string &objName) {
    // CollisionManager にこのコライダーを登録
    CollisionManager::AddCollider(this);

    counter++;

    // 各コライダー形状の初期化
    SphereOffset_.center = {0.0f, 0.0f, 0.0f};
    SphereOffset_.radius = 0.0f;
    AABBOffset_.min = {0.0f, 0.0f, 0.0f};
    AABBOffset_.max = {0.0f, 0.0f, 0.0f};
    OBBOffset_.rotationCenter = {0.0f, 0.0f, 0.0f};
    OBBOffset_.scaleCenter = {0.0f, 0.0f, 0.0f};
    OBBOffset_.size = {1.0f, 1.0f, 1.0f};

    // オブジェクト名を設定
    objName_ = objName;

    // JSONから設定をロード
    LoadFromJson();

    // 自分自身を参照で返す
    return *this;
}


void Collider::UpdateWorldTransform() {

    // 球用のワールドトランスフォームを更新
    sphere_.center = GetCenterPosition() + SphereOffset_.center;
    sphere_.radius = radius_ + SphereOffset_.radius;

    // AABBの現在の最小点と最大点を取得
    aabb_.min = GetCenterPosition() - Vector3(1.0f, 1.0f, 1.0f);
    aabb_.max = GetCenterPosition() + Vector3(1.0f, 1.0f, 1.0f);
    aabb_.min = aabb_.min + AABBOffset_.min;
    aabb_.max = aabb_.max + AABBOffset_.max;

    // OBBの各プロパティを更新
    obb_.rotationCenter = GetCenterPosition() + OBBOffset_.rotationCenter; // 回転中心
    obb_.scaleCenter = GetCenterPosition() + OBBOffset_.scaleCenter;       // スケール中心

    // OBBの向きベクトルを計算
    MakeOBBOrientations(obb_, GetCenterRotation());

    // サイズを更新
    obb_.size = OBBOffset_.size;

    UpdateOBB();
}

void Collider::DebugDraw(const ViewProjection &viewProjection) {
    if (!isVisible_ || !isCollisionEnabled_) {
        return;
    }
    if (isSphere_) {
        DrawSphere(viewProjection);
    }
    if (isAABB_) {
        DrawAABB(viewProjection);
    }
    if (isOBB_) {
        DrawOBB(viewProjection);
    }
}

void Collider::DrawSphere(const ViewProjection &viewProjection) {
    const uint32_t kSubdivision = 10;                                        // 分割数
    const float kLonEvery = 2.0f * std::numbers::pi_v<float> / kSubdivision; // 経度分割1つ分の角度
    const float kLatEvery = std::numbers::pi_v<float> / kSubdivision;        // 緯度分割1つ分の角度

    // 緯度の方向に分割　-π/2 ～ π/2
    for (uint32_t latIndex = 0; latIndex < kSubdivision; ++latIndex) {
        float lat = -std::numbers::pi_v<float> / 2.0f + kLatEvery * latIndex; // 現在の緯度

        // 経度の方向に分割 0 ～ 2π
        for (uint32_t lonIndex = 0; lonIndex < kSubdivision; ++lonIndex) {
            float lon = lonIndex * kLonEvery; // 現在の経度

            // 現在の点を求める
            Vector3 start = {
                sphere_.center.x + sphere_.radius * std::cosf(lat) * std::cosf(lon),
                sphere_.center.y + sphere_.radius * std::sinf(lat),
                sphere_.center.z + sphere_.radius * std::cosf(lat) * std::sinf(lon)};

            // 次の点を求める（経度方向）
            Vector3 end1 = {
                sphere_.center.x + sphere_.radius * std::cosf(lat) * std::cosf(lon + kLonEvery),
                sphere_.center.y + sphere_.radius * std::sinf(lat),
                sphere_.center.z + sphere_.radius * std::cosf(lat) * std::sinf(lon + kLonEvery),
            };

            // 次の点を求める（緯度方向）
            Vector3 end2 = {
                sphere_.center.x + sphere_.radius * std::cosf(lat + kLatEvery) * std::cosf(lon),
                sphere_.center.y + sphere_.radius * std::sinf(lat + kLatEvery),
                sphere_.center.z + sphere_.radius * std::cosf(lat + kLatEvery) * std::sinf(lon),
            };

            // 線を描画（経度方向）
            DrawLine3D::GetInstance()->SetPoints(start, end1, color_);
            // 線を描画（緯度方向）
            DrawLine3D::GetInstance()->SetPoints(start, end2, color_);
        }
    }
}

void Collider::DrawAABB(const ViewProjection &viewProjection) {
    // AABBの頂点リスト
    std::array<Vector3, 8> vertices = {
        aabb_.min,
        {aabb_.max.x, aabb_.min.y, aabb_.min.z},
        {aabb_.min.x, aabb_.max.y, aabb_.min.z},
        {aabb_.max.x, aabb_.max.y, aabb_.min.z},
        {aabb_.min.x, aabb_.min.y, aabb_.max.z},
        {aabb_.max.x, aabb_.min.y, aabb_.max.z},
        {aabb_.min.x, aabb_.max.y, aabb_.max.z},
        {aabb_.max.x, aabb_.max.y, aabb_.max.z}};

    // エッジ接続リスト
    const std::array<std::pair<int, int>, 12> edges = {
        std::make_pair(0, 1), std::make_pair(1, 3), std::make_pair(3, 2), std::make_pair(2, 0), // 前面
        std::make_pair(4, 5), std::make_pair(5, 7), std::make_pair(7, 6), std::make_pair(6, 4), // 背面
        std::make_pair(0, 4), std::make_pair(1, 5), std::make_pair(2, 6), std::make_pair(3, 7)  // 側面
    };

    // 線を描画
    for (const auto &edge : edges) {
        DrawLine3D::GetInstance()->SetPoints(vertices[edge.first], vertices[edge.second], color_);
    }
}

void Collider::DrawOBB(const ViewProjection &viewProjection) {
    // OBBの8つの頂点を計算
    std::array<Vector3, 8> vertices;
    Vector3 halfSize = obb_.size; // サイズの半分を計算

    // OBBの8頂点を計算するループ
    for (int i = 0; i < 8; i++) {
        // 各頂点のローカル座標を計算
        Vector3 localPosition = Vector3(
            (i & 1) ? halfSize.x : -halfSize.x,
            (i & 2) ? halfSize.y : -halfSize.y,
            (i & 4) ? halfSize.z : -halfSize.z);

        // scaleCenter を基準にスケール変換
        Vector3 scaledPosition = localPosition + (obb_.scaleCenter - obb_.rotationCenter);

        // 回転中心を基準に回転を適用
        Vector3 rotatedPosition =
            obb_.orientations[0] * scaledPosition.x +
            obb_.orientations[1] * scaledPosition.y +
            obb_.orientations[2] * scaledPosition.z;

        // ワールド座標へ変換
        vertices[i] = obb_.rotationCenter + rotatedPosition;
    }

    // scaleCenterに球を描画
    DrawSphereAtCenter(viewProjection, obb_.scaleCenterRotated, 0.1f); // 半径0.1fで球を描画

    // エッジ接続リスト
    const std::array<std::pair<int, int>, 12> edges = {
        std::make_pair(0, 1), std::make_pair(1, 3), std::make_pair(3, 2), std::make_pair(2, 0), // 前面
        std::make_pair(4, 5), std::make_pair(5, 7), std::make_pair(7, 6), std::make_pair(6, 4), // 背面
        std::make_pair(0, 4), std::make_pair(1, 5), std::make_pair(2, 6), std::make_pair(3, 7)  // 側面
    };

    // 線を描画
    for (const auto &edge : edges) {
        DrawLine3D::GetInstance()->SetPoints(vertices[edge.first], vertices[edge.second], color_);
    }

    DrawRotationCenter(viewProjection);
}

// 球を描画する関数
void Collider::DrawSphereAtCenter(const ViewProjection &viewProjection, const Vector3 &center, float radius) {
    const int slices = 16; // 球の横方向の分割数
    const int stacks = 8;  // 球の縦方向の分割数

    for (int i = 0; i < slices; i++) {
        float theta1 = (i * 2.0f * std::numbers::pi_v<float>) / slices;
        float theta2 = ((i + 1) * 2.0f * std::numbers::pi_v<float>) / slices;

        for (int j = 0; j < stacks; j++) {
            float phi1 = (j * std::numbers::pi_v<float>) / stacks;
            float phi2 = ((j + 1) * std::numbers::pi_v<float>) / stacks;

            // 球の4つの頂点を計算
            Vector3 p1 = Vector3(
                center.x + radius * sin(phi1) * cos(theta1),
                center.y + radius * cos(phi1),
                center.z + radius * sin(phi1) * sin(theta1));

            Vector3 p2 = Vector3(
                center.x + radius * sin(phi1) * cos(theta2),
                center.y + radius * cos(phi1),
                center.z + radius * sin(phi1) * sin(theta2));

            Vector3 p3 = Vector3(
                center.x + radius * sin(phi2) * cos(theta1),
                center.y + radius * cos(phi2),
                center.z + radius * sin(phi2) * sin(theta1));

            Vector3 p4 = Vector3(
                center.x + radius * sin(phi2) * cos(theta2),
                center.y + radius * cos(phi2),
                center.z + radius * sin(phi2) * sin(theta2));

            // 4つの三角形で球の断片を描画
            DrawLine3D::GetInstance()->SetPoints(p1, p2);
            DrawLine3D::GetInstance()->SetPoints(p2, p4);
            DrawLine3D::GetInstance()->SetPoints(p4, p3);
            DrawLine3D::GetInstance()->SetPoints(p3, p1);
        }
    }
}

void Collider::OffsetImgui() {
    // スタイル設定とカラー定義
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 4.0f));

    // ヘッダーカラー
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.2f, 0.4f, 0.6f, 0.55f));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.3f, 0.5f, 0.7f, 0.65f));
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.4f, 0.6f, 0.8f, 0.8f));

    if (ImGui::CollapsingHeader("コライダー")) {
        ImGui::PopStyleColor(3); // ヘッダーカラー終了

        // 基本設定部分 - フレーム追加
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.18f, 0.18f, 0.2f, 0.6f));
        ImGui::PushStyleColor(ImGuiCol_CheckMark, ImVec4(0.5f, 0.8f, 0.5f, 1.0f));

        ImGui::BeginGroup();
        ImGui::BeginChild("BasicSettings", ImVec2(0, 40), true);
        ImGui::Checkbox("可視化", &isVisible_);
        ImGui::SameLine(0, 30.0f);
        ImGui::Checkbox("コライダーの有無", &isCollisionEnabled_);
        ImGui::EndChild();
        ImGui::EndGroup();

        ImGui::PopStyleColor(2); // 基本設定カラー終了

        if (isCollisionEnabled_) {
            // タブバーのスタイル
            ImGui::PushStyleColor(ImGuiCol_Tab, ImVec4(0.2f, 0.2f, 0.3f, 0.8f));
            ImGui::PushStyleColor(ImGuiCol_TabHovered, ImVec4(0.4f, 0.4f, 0.5f, 0.8f));
            ImGui::PushStyleColor(ImGuiCol_TabActive, ImVec4(0.3f, 0.3f, 0.6f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_TabUnfocused, ImVec4(0.15f, 0.15f, 0.2f, 0.8f));
            ImGui::PushStyleColor(ImGuiCol_TabUnfocusedActive, ImVec4(0.25f, 0.25f, 0.4f, 0.9f));

            // タブバーを使って各コライダータイプを整理
            ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_FittingPolicyScroll | ImGuiTabBarFlags_Reorderable;
            if (ImGui::BeginTabBar("ColliderTypesTabBar", tab_bar_flags)) {
                // 球体コライダータブ
                if (ImGui::BeginTabItem("球体コライダー")) {
                    ImGui::PopStyleColor(5); // タブバーカラー終了
                    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.3f, 0.3f, 0.5f, 0.5f));
                    ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImVec4(0.5f, 0.5f, 0.9f, 0.8f));
                    ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, ImVec4(0.6f, 0.6f, 1.0f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_CheckMark, ImVec4(0.5f, 0.5f, 1.0f, 1.0f));

                    ImGui::Checkbox("球判定を使用する", &isSphere_);
                    if (isSphere_) {
                        ImGui::Separator();
                        ImGui::BeginChild("SphereSettings", ImVec2(0, 70), true);
                        ImGui::Indent(10.0f);
                        ImGui::PushItemWidth(150.0f);

                        // 横並びにレイアウト
                        ImGui::AlignTextToFramePadding();
                        ImGui::Text("中心点:");
                        ImGui::SameLine();
                        ImGui::PushID("SphereCenter");
                        ImGui::DragFloat3("##center", &SphereOffset_.center.x, 0.1f);
                        ImGui::PopID();

                        ImGui::AlignTextToFramePadding();
                        ImGui::Text("半径:  ");
                        ImGui::SameLine();
                        ImGui::DragFloat("##radius", &SphereOffset_.radius, 0.1f);

                        ImGui::PopItemWidth();
                        ImGui::Unindent(10.0f);
                        ImGui::EndChild();
                    }

                    ImGui::PopStyleColor(4); // 球体コライダーカラー終了
                    ImGui::EndTabItem();
                } else {
                    // タブ外ですので、色だけポップする必要がある
                    ImGui::PopStyleColor(5); // タブバーカラー終了
                }

                // AABBコライダータブ
                ImGui::PushStyleColor(ImGuiCol_Tab, ImVec4(0.2f, 0.3f, 0.2f, 0.8f));
                ImGui::PushStyleColor(ImGuiCol_TabHovered, ImVec4(0.4f, 0.5f, 0.4f, 0.8f));
                ImGui::PushStyleColor(ImGuiCol_TabActive, ImVec4(0.3f, 0.6f, 0.3f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_TabUnfocused, ImVec4(0.15f, 0.2f, 0.15f, 0.8f));
                ImGui::PushStyleColor(ImGuiCol_TabUnfocusedActive, ImVec4(0.25f, 0.4f, 0.25f, 0.9f));

                if (ImGui::BeginTabItem("AABBコライダー")) {
                    ImGui::PopStyleColor(5); // タブバーカラー終了
                    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.2f, 0.4f, 0.2f, 0.5f));
                    ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImVec4(0.4f, 0.7f, 0.4f, 0.8f));
                    ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, ImVec4(0.5f, 0.9f, 0.5f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_CheckMark, ImVec4(0.4f, 0.9f, 0.4f, 1.0f));

                    ImGui::Checkbox("AABB判定を使用する", &isAABB_);
                    if (isAABB_) {
                        ImGui::Separator();
                        ImGui::BeginChild("AABBSettings", ImVec2(0, 70), true);
                        ImGui::Indent(10.0f);
                        ImGui::PushItemWidth(150.0f);

                        ImGui::AlignTextToFramePadding();
                        ImGui::Text("最大値:");
                        ImGui::SameLine();
                        ImGui::PushID("AABBMax");
                        ImGui::DragFloat3("##max", &AABBOffset_.max.x, 0.1f);
                        ImGui::PopID();

                        ImGui::AlignTextToFramePadding();
                        ImGui::Text("最小値:");
                        ImGui::SameLine();
                        ImGui::PushID("AABBMin");
                        ImGui::DragFloat3("##min", &AABBOffset_.min.x, 0.1f);
                        ImGui::PopID();

                        ImGui::PopItemWidth();
                        ImGui::Unindent(10.0f);
                        ImGui::EndChild();
                    }

                    ImGui::PopStyleColor(4); // AABBコライダーカラー終了
                    ImGui::EndTabItem();
                } else {
                    ImGui::PopStyleColor(5); // タブバーカラー終了
                }

                // OBBコライダータブ
                ImGui::PushStyleColor(ImGuiCol_Tab, ImVec4(0.3f, 0.2f, 0.2f, 0.8f));
                ImGui::PushStyleColor(ImGuiCol_TabHovered, ImVec4(0.5f, 0.4f, 0.4f, 0.8f));
                ImGui::PushStyleColor(ImGuiCol_TabActive, ImVec4(0.6f, 0.3f, 0.3f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_TabUnfocused, ImVec4(0.2f, 0.15f, 0.15f, 0.8f));
                ImGui::PushStyleColor(ImGuiCol_TabUnfocusedActive, ImVec4(0.4f, 0.25f, 0.25f, 0.9f));

                if (ImGui::BeginTabItem("OBBコライダー")) {
                    ImGui::PopStyleColor(5); // タブバーカラー終了
                    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.4f, 0.2f, 0.2f, 0.5f));
                    ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImVec4(0.7f, 0.4f, 0.4f, 0.8f));
                    ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, ImVec4(0.9f, 0.5f, 0.5f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_CheckMark, ImVec4(0.9f, 0.4f, 0.4f, 1.0f));

                    ImGui::Checkbox("OBB判定を使用する", &isOBB_);
                    if (isOBB_) {
                        ImGui::Separator();
                        ImGui::BeginChild("OBBSettings", ImVec2(0, 70), true);
                        ImGui::Indent(10.0f);
                        ImGui::PushItemWidth(150.0f);

                        ImGui::AlignTextToFramePadding();
                        ImGui::Text("中心:  ");
                        ImGui::SameLine();
                        ImGui::PushID("OBBCenter");
                        ImGui::DragFloat3("##center", &OBBOffset_.scaleCenter.x, 0.1f);
                        ImGui::PopID();

                        ImGui::AlignTextToFramePadding();
                        ImGui::Text("大きさ:");
                        ImGui::SameLine();
                        ImGui::PushID("OBBSize");
                        ImGui::DragFloat3("##size", &OBBOffset_.size.x, 0.1f);
                        ImGui::PopID();

                        ImGui::PopItemWidth();
                        ImGui::Unindent(10.0f);
                        ImGui::EndChild();
                    }

                    ImGui::PopStyleColor(4); // OBBコライダーカラー終了
                    ImGui::EndTabItem();
                } else {
                    ImGui::PopStyleColor(5); // タブバーカラー終了
                }

                ImGui::EndTabBar();
            }
        } else {
            ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "コライダーが無効になっています");
        }
    } else {
        ImGui::PopStyleColor(3); // 使用されなかったヘッダーカラー終了
    }

    ImGui::PopStyleVar(2); // スタイル変数終了
}

void Collider::DrawRotationCenter(const ViewProjection &viewProjection) {
    // 回転中心を表す球の半径
    float rotationCenterRadius = 0.1f;

    // 球を構成する点を描画
    const uint32_t kSubdivision = 10; // 分割数
    const float kLonEvery = 2.0f * std::numbers::pi_v<float> / kSubdivision;
    const float kLatEvery = std::numbers::pi_v<float> / kSubdivision;

    for (uint32_t latIndex = 0; latIndex < kSubdivision; ++latIndex) {
        float lat = -std::numbers::pi_v<float> / 2.0f + kLatEvery * latIndex;

        for (uint32_t lonIndex = 0; lonIndex < kSubdivision; ++lonIndex) {
            float lon = lonIndex * kLonEvery;

            Vector3 start = {
                obb_.rotationCenter.x + rotationCenterRadius * std::cosf(lat) * std::cosf(lon),
                obb_.rotationCenter.y + rotationCenterRadius * std::sinf(lat),
                obb_.rotationCenter.z + rotationCenterRadius * std::cosf(lat) * std::sinf(lon)};

            Vector3 end1 = {
                obb_.rotationCenter.x + rotationCenterRadius * std::cosf(lat) * std::cosf(lon + kLonEvery),
                obb_.rotationCenter.y + rotationCenterRadius * std::sinf(lat),
                obb_.rotationCenter.z + rotationCenterRadius * std::cosf(lat) * std::sinf(lon + kLonEvery),
            };

            Vector3 end2 = {
                obb_.rotationCenter.x + rotationCenterRadius * std::cosf(lat + kLatEvery) * std::cosf(lon),
                obb_.rotationCenter.y + rotationCenterRadius * std::sinf(lat + kLatEvery),
                obb_.rotationCenter.z + rotationCenterRadius * std::cosf(lat + kLatEvery) * std::sinf(lon),
            };

            DrawLine3D::GetInstance()->SetPoints(start, end1);
            DrawLine3D::GetInstance()->SetPoints(start, end2);
        }
    }
}

void Collider::SetCollisionType(CollisionType collisionType) {
    switch (collisionType) {
    case Collider::CollisionType::Sphere:
        isSphere_ = true;
        isAABB_ = false;
        isOBB_ = false;
        break;
    case Collider::CollisionType::AABB:
        isSphere_ = false;
        isAABB_ = true;
        isOBB_ = false;
        break;
    case Collider::CollisionType::OBB:
        isSphere_ = false;
        isAABB_ = false;
        isOBB_ = true;
        break;
    default:
        isSphere_ = false;
        isAABB_ = false;
        isOBB_ = false;
        break;
    }
}

void Collider::MakeOBBOrientations(OBB &obb, const Vector3 &rotate) {
    // 回転行列を作成
    Matrix4x4 rotateMatrix = MakeRotateXMatrix(rotate.x) * MakeRotateYMatrix(rotate.y) * MakeRotateZMatrix(rotate.z);

    // 各方向ベクトルを計算
    obb.orientations[0].x = rotateMatrix.m[0][0];
    obb.orientations[0].y = rotateMatrix.m[0][1];
    obb.orientations[0].z = rotateMatrix.m[0][2];

    obb.orientations[1].x = rotateMatrix.m[1][0];
    obb.orientations[1].y = rotateMatrix.m[1][1];
    obb.orientations[1].z = rotateMatrix.m[1][2];

    obb.orientations[2].x = rotateMatrix.m[2][0];
    obb.orientations[2].y = rotateMatrix.m[2][1];
    obb.orientations[2].z = rotateMatrix.m[2][2];
}

void Collider::UpdateOBB() {
    // 回転後にscaleCenterの位置を計算
    obb_.scaleCenterRotated = obb_.orientations[0] * (obb_.scaleCenter.x - obb_.rotationCenter.x) +
                              obb_.orientations[1] * (obb_.scaleCenter.y - obb_.rotationCenter.y) +
                              obb_.orientations[2] * (obb_.scaleCenter.z - obb_.rotationCenter.z) + obb_.rotationCenter;
}

void Collider::SaveToJson() {
    // 各種フラグをJSONでセーブ
    ColliderDatas_->Save("isVisible", isVisible_);
    ColliderDatas_->Save("isCollisionEnabled", isCollisionEnabled_);
    ColliderDatas_->Save("isSphere", isSphere_);
    ColliderDatas_->Save("isAABB", isAABB_);
    ColliderDatas_->Save("isOBB", isOBB_);

    // 各オフセット値をJSONでセーブ
    ColliderDatas_->Save("center", SphereOffset_.center);
    ColliderDatas_->Save("radius", SphereOffset_.radius);
    ColliderDatas_->Save("min", AABBOffset_.min);
    ColliderDatas_->Save("max", AABBOffset_.max);
    ColliderDatas_->Save("scaleCenter", OBBOffset_.scaleCenter);
    ColliderDatas_->Save("size", OBBOffset_.size);
}

void Collider::LoadFromJson() {
    // 各種フラグをJSONから読み込み
    ColliderDatas_ = std::make_unique<DataHandler>("Collider", objName_);
    isVisible_ = ColliderDatas_->Load<bool>("isVisible", true);
    isCollisionEnabled_ = ColliderDatas_->Load<bool>("isCollisionEnabled", true);
    isSphere_ = ColliderDatas_->Load<bool>("isSphere", true);
    isAABB_ = ColliderDatas_->Load<bool>("isAABB", true);
    isOBB_ = ColliderDatas_->Load<bool>("isOBB", true);

    // 各オフセット値をJSONから読み込み
    SphereOffset_.center = ColliderDatas_->Load<Vector3>("center", {0.0f, 0.0f, 0.0f});
    SphereOffset_.radius = ColliderDatas_->Load<float>("radius", 0.0f);
    AABBOffset_.min = ColliderDatas_->Load<Vector3>("min", {0.0f, 0.0f, 0.0f});
    AABBOffset_.max = ColliderDatas_->Load<Vector3>("max", {0.0f, 0.0f, 0.0f});
    OBBOffset_.scaleCenter = ColliderDatas_->Load<Vector3>("scaleCenter", {0.0f, 0.0f, 0.0f});
    OBBOffset_.size = ColliderDatas_->Load<Vector3>("size", {1.0f, 1.0f, 1.0f});
}
