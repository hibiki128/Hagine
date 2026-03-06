#define NOMINMAX
#include "ParticleCSFieldManager.h"
#include <cassert>
#include <cmath>

#ifdef USE_IMGUI
#include <imgui.h>
#endif

ParticleCSFieldManager *ParticleCSFieldManager::instance_ = nullptr;

ParticleCSFieldManager *ParticleCSFieldManager::GetInstance() {
    if (!instance_) {
        instance_ = new ParticleCSFieldManager();
    }
    return instance_;
}

void ParticleCSFieldManager::Finalize() {
    delete instance_;
    instance_ = nullptr;
}

void ParticleCSFieldManager::Initialize() {
    dxCommon_ = ParticleCommon::GetInstance()->GetDxCommon();
    srvManager_ = SrvManager::GetInstance();
    CreateGPUResources();
}

void ParticleCSFieldManager::CreateGPUResources() {
    // フィールド配列バッファ（StructuredBuffer として使う）
    size_t bufSize = sizeof(ParticleFieldData) * kMaxFields;
    fieldsResource_ = dxCommon_->CreateBufferResource(bufSize);
    fieldsResource_->Map(0, nullptr, reinterpret_cast<void **>(&fieldsMappedData_));
    ZeroMemory(fieldsMappedData_, bufSize);

    // フィールド数バッファ（ConstantBuffer）
    fieldCountResource_ = dxCommon_->CreateBufferResource(sizeof(uint32_t) * 4); // アライメント
    fieldCountResource_->Map(0, nullptr, reinterpret_cast<void **>(&fieldCountMappedData_));
    *fieldCountMappedData_ = 0;

    zeroFieldCountResource_ = dxCommon_->CreateBufferResource(sizeof(uint32_t) * 4);
    uint32_t *zeroPtr = nullptr;
    zeroFieldCountResource_->Map(0, nullptr, reinterpret_cast<void **>(&zeroPtr));
    *zeroPtr = 0;
    zeroFieldCountResource_->Unmap(0, nullptr);

    // SRV 登録
    fieldsSrvIndex_ = srvManager_->Allocate() + 1;
    fieldsSrvHandle_.first = srvManager_->GetCPUDescriptorHandle(fieldsSrvIndex_);
    fieldsSrvHandle_.second = srvManager_->GetGPUDescriptorHandle(fieldsSrvIndex_);
    srvManager_->CreateSRVforStructuredBuffer(
        fieldsSrvIndex_,
        fieldsResource_.Get(),
        kMaxFields,
        sizeof(ParticleFieldData));
}

void ParticleCSFieldManager::Update() {
    UploadToGPU();
}

void ParticleCSFieldManager::UploadToGPU() {
    uint32_t count = 0;
    for (auto &f : fields_) {
        if (!f.enabled)
            continue;
        if (count >= kMaxFields)
            break;
        fieldsMappedData_[count++] = f.data;
    }
    *fieldCountMappedData_ = count;
}

void ParticleCSFieldManager::AddField(const ParticleField &field) {
    if (static_cast<uint32_t>(fields_.size()) >= kMaxFields)
        return;
    fields_.push_back(field);
}

void ParticleCSFieldManager::RemoveField(int index) {
    if (index < 0 || index >= static_cast<int>(fields_.size()))
        return;
    fields_.erase(fields_.begin() + index);
}

ParticleField *ParticleCSFieldManager::GetField(int index) {
    if (index < 0 || index >= static_cast<int>(fields_.size()))
        return nullptr;
    return &fields_[index];
}

// =============================================
// セーブ / ロード
// =============================================

void ParticleCSFieldManager::SaveFieldData(DataHandler &data, const ParticleField &field) {
    // 基本情報
    data.Save("name", field.name);
    data.Save("enabled", field.enabled);

    // フィールドデータ
    data.Save("fieldType", field.data.fieldType);
    data.Save<Vector3>("position", field.data.position);
    data.Save("radius", field.data.radius);
    data.Save<Vector3>("direction", field.data.direction);
    data.Save("strength", field.data.strength);
    data.Save("falloff", field.data.falloff);
}

void ParticleCSFieldManager::LoadFieldData(DataHandler &data, ParticleField &field) {
    // 基本情報
    field.name = data.Load("name", field.name);
    field.enabled = data.Load("enabled", field.enabled);

    // フィールドデータ
    field.data.fieldType = data.Load("fieldType", field.data.fieldType);
    field.data.position = data.Load<Vector3>("position", field.data.position);
    field.data.radius = data.Load("radius", field.data.radius);
    field.data.direction = data.Load<Vector3>("direction", field.data.direction);
    field.data.strength = data.Load("strength", field.data.strength);
    field.data.falloff = data.Load("falloff", field.data.falloff);
}

void ParticleCSFieldManager::SaveField(const ParticleField &field) {
    // フォルダ: resources/jsons/ParticleField/  ファイル名: field.name.json
    std::unique_ptr<DataHandler> data = std::make_unique<DataHandler>("ParticleField", field.name);
    SaveFieldData(*data, field);
}

ParticleField ParticleCSFieldManager::LoadField(const std::string &fileName, const ParticleField &defaultField) {
    std::unique_ptr<DataHandler> data = std::make_unique<DataHandler>("ParticleField", fileName);
    if (!data->Exists()) {
        return defaultField;
    }
    ParticleField field = defaultField;
    LoadFieldData(*data, field);
    return field;
}

// =============================================
// CreateField
// =============================================

ParticleField *ParticleCSFieldManager::CreateField(const std::string &name, const std::string &templateName) {
    // 上限チェック
    if (static_cast<uint32_t>(fields_.size()) >= kMaxFields) {
        return nullptr;
    }

    ParticleField newField;

    if (!templateName.empty()) {
        // ★ テンプレートjsonが指定されていれば、そのデータを複製して土台にする
        newField = LoadField(templateName, ParticleField{});
    }

    // 名前は引数で上書き（テンプレートの名前ではなく指定名を使う）
    newField.name = name;

    // 自身のjsonが既に存在すれば、それをロードして上書きする
    // （再起動後の復元など、name.json が保存済みの場合に対応）
    {
        std::unique_ptr<DataHandler> selfData = std::make_unique<DataHandler>("ParticleField", name);
        if (selfData->Exists()) {
            LoadFieldData(*selfData, newField);
            newField.name = name; // name だけは引数を優先
        }
    }

    fields_.push_back(newField);
    return &fields_.back();
}

// =============================================
// ImGui
// =============================================
void ParticleCSFieldManager::DrawImGui() {
#ifdef USE_IMGUI
    ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImVec4(0.15f, 0.3f, 0.5f, 1.0f));
    ImGui::SetNextWindowSize(ImVec2(420, 600), ImGuiCond_FirstUseEver);

    if (!ImGui::Begin("パーティクルフィールド管理")) {
        ImGui::PopStyleColor();
        ImGui::End();
        return;
    }
    ImGui::PopStyleColor();

    // ヘッダー情報
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.9f, 1.0f, 1.0f));
    ImGui::Text("フィールド数: %d / %d", static_cast<int>(fields_.size()), kMaxFields);
    ImGui::PopStyleColor();
    ImGui::Separator();
    ImGui::Spacing();

    // フィールド追加ボタン
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.55f, 0.2f, 0.8f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.7f, 0.3f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.4f, 0.85f, 0.4f, 1.0f));
    if (ImGui::Button("フィールドを追加", ImVec2(-1, 30))) {
        ParticleField newField;
        newField.name = "Field_" + std::to_string(fields_.size());
        newField.enabled = true;
        AddField(newField);
    }
    ImGui::PopStyleColor(3);
    ImGui::Spacing();

    // フィールドリスト
    int removeIndex = -1;
    for (int i = 0; i < static_cast<int>(fields_.size()); ++i) {
        auto &f = fields_[i];

        // フィールドタイプ別の色
        ImVec4 headerColor;
        const char *typeLabel;
        switch (static_cast<ParticleFieldType>(f.data.fieldType)) {
        case ParticleFieldType::Wind:
            headerColor = ImVec4(0.2f, 0.5f, 0.8f, 0.9f);
            typeLabel = "[風]";
            break;
        case ParticleFieldType::Attract:
            headerColor = ImVec4(0.7f, 0.3f, 0.7f, 0.9f);
            typeLabel = "[引力]";
            break;
        case ParticleFieldType::Repel:
            headerColor = ImVec4(0.8f, 0.4f, 0.2f, 0.9f);
            typeLabel = "[斥力]";
            break;
        case ParticleFieldType::Vortex:
            headerColor = ImVec4(0.2f, 0.7f, 0.6f, 0.9f);
            typeLabel = "[渦巻き]";
            break;
        default:
            headerColor = ImVec4(0.4f, 0.4f, 0.4f, 0.9f);
            typeLabel = "[不明]";
            break;
        }

        // 無効時はグレーアウト
        if (!f.enabled) {
            headerColor = ImVec4(0.35f, 0.35f, 0.35f, 0.8f);
        }

        ImGui::PushStyleColor(ImGuiCol_Header, headerColor);
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(headerColor.x + 0.1f, headerColor.y + 0.1f, headerColor.z + 0.1f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(headerColor.x + 0.2f, headerColor.y + 0.2f, headerColor.z + 0.2f, 1.0f));

        std::string label = std::string(typeLabel) + " " + f.name + "##field" + std::to_string(i);
        bool open = ImGui::CollapsingHeader(label.c_str());
        ImGui::PopStyleColor(3);

        if (open) {
            ImGui::Indent();
            ImGui::PushItemWidth(200.0f);

            // 有効/無効チェック
            ImGui::Checkbox(("有効##en" + std::to_string(i)).c_str(), &f.enabled);
            ImGui::SameLine();

            // 保存ボタン
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.4f, 0.7f, 0.8f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.55f, 0.9f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.4f, 0.7f, 1.0f, 1.0f));
            if (ImGui::Button(("保存##save" + std::to_string(i)).c_str(), ImVec2(50, 0))) {
                SaveField(f);
            }
            ImGui::PopStyleColor(3);
            ImGui::SameLine();

            // 削除ボタン
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.2f, 0.2f, 0.8f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.3f, 0.3f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1.0f, 0.4f, 0.4f, 1.0f));
            if (ImGui::Button(("削除##del" + std::to_string(i)).c_str(), ImVec2(60, 0))) {
                removeIndex = i;
            }
            ImGui::PopStyleColor(3);

            ImGui::Spacing();

            // 名前
            char nameBuf[128];
            strncpy_s(nameBuf, f.name.c_str(), sizeof(nameBuf) - 1);
            if (ImGui::InputText(("名前##nm" + std::to_string(i)).c_str(), nameBuf, sizeof(nameBuf))) {
                f.name = nameBuf;
            }

            ImGui::Spacing();
            ImGui::Separator();

            // フィールドタイプ選択
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.9f, 1.0f, 1.0f));
            ImGui::TextUnformatted("フィールド種類");
            ImGui::PopStyleColor();

            const char *typeItems[] = {"風 (Wind)", "引力 (Attract)", "斥力 (Repel)", "渦巻き (Vortex)"};
            int typeIdx = static_cast<int>(f.data.fieldType);
            if (ImGui::Combo(("##type" + std::to_string(i)).c_str(), &typeIdx, typeItems, 4)) {
                f.data.fieldType = static_cast<uint32_t>(typeIdx);
            }

            ImGui::Spacing();
            ImGui::Separator();

            // 位置・範囲
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.9f, 1.0f, 1.0f));
            ImGui::TextUnformatted("位置・影響範囲");
            ImGui::PopStyleColor();

            ImGui::DragFloat3(("位置##pos" + std::to_string(i)).c_str(), &f.data.position.x, 0.1f, -9999.0f, 9999.0f, "%.2f");
            ImGui::DragFloat(("影響半径##rad" + std::to_string(i)).c_str(), &f.data.radius, 0.1f, 0.01f, 9999.0f, "%.2f");
            ImGui::DragFloat(("減衰指数##fal" + std::to_string(i)).c_str(), &f.data.falloff, 0.05f, 0.1f, 4.0f, "%.2f");
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("1.0=線形減衰  2.0=二乗減衰（端に近いほど弱くなる）");

            ImGui::Spacing();
            ImGui::Separator();

            // タイプ別パラメータ
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.9f, 1.0f, 1.0f));
            ImGui::TextUnformatted("フィールドパラメータ");
            ImGui::PopStyleColor();

            ImGui::DragFloat(("強さ##str" + std::to_string(i)).c_str(), &f.data.strength, 0.05f, -999.0f, 999.0f, "%.3f");

            auto ft = static_cast<ParticleFieldType>(f.data.fieldType);
            if (ft == ParticleFieldType::Wind) {
                ImGui::DragFloat3(("方向##dir" + std::to_string(i)).c_str(), &f.data.direction.x, 0.01f, -1.0f, 1.0f, "%.3f");
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("正規化しなくてもシェーダー側で正規化されます");
            } else if (ft == ParticleFieldType::Vortex) {
                ImGui::DragFloat3(("回転軸##dir" + std::to_string(i)).c_str(), &f.data.direction.x, 0.01f, -1.0f, 1.0f, "%.3f");
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("渦の回転軸（例: 0,1,0 = Y軸回り）");
            }

            ImGui::PopItemWidth();
            ImGui::Unindent();
        }

        ImGui::Spacing();
    }

    if (removeIndex >= 0) {
        RemoveField(removeIndex);
    }

    ImGui::End();
#endif
}

// =============================================
// ギズモ描画
// =============================================

void ParticleCSFieldManager::DrawFieldGizmos() {
    for (const auto &f : fields_) {
        if (!f.enabled)
            continue;

        // フィールドタイプ別に色を決定
        // strength の絶対値を alpha に反映して強さを視覚化（0.4〜1.0 にクランプ）
        float alpha = std::min(1.0f, 0.4f + std::abs(f.data.strength) * 0.06f);

        Vector4 color;
        auto ft = static_cast<ParticleFieldType>(f.data.fieldType);
        switch (ft) {
        case ParticleFieldType::Wind:
            // 風 → 水色
            color = {0.3f, 0.7f, 1.0f, alpha};
            break;
        case ParticleFieldType::Attract:
            // 引力 → 紫
            color = {0.8f, 0.3f, 1.0f, alpha};
            break;
        case ParticleFieldType::Repel:
            // 斥力 → オレンジ
            color = {1.0f, 0.5f, 0.1f, alpha};
            break;
        case ParticleFieldType::Vortex:
            // 渦巻き → 緑
            color = {0.2f, 1.0f, 0.6f, alpha};
            break;
        default:
            color = {0.6f, 0.6f, 0.6f, alpha};
            break;
        }

        // 影響範囲球（全タイプ共通）
        DrawFieldSphere(f, color);

        // タイプ別の方向・強さ表示
        switch (ft) {
        case ParticleFieldType::Wind:
            DrawWindArrows(f, color);
            break;
        case ParticleFieldType::Attract:
            // inward = true（外→中心向き）
            DrawRadialLines(f, color, true);
            break;
        case ParticleFieldType::Repel:
            // inward = false（中心→外向き）
            DrawRadialLines(f, color, false);
            break;
        case ParticleFieldType::Vortex:
            DrawVortexArcs(f, color);
            break;
        default:
            break;
        }
    }
}

// --- 影響範囲球 ---
void ParticleCSFieldManager::DrawFieldSphere(const ParticleField &field, const Vector4 &color) {
    DrawLine3D::GetInstance()->DrawSphere(field.data.position, color, field.data.radius, 16);
}

// --- Wind：球内に等間隔で方向矢印を描く ---
void ParticleCSFieldManager::DrawWindArrows(const ParticleField &field, const Vector4 &color) {
    const Vector3 &center = field.data.position;
    const float r = field.data.radius;

    // 方向を正規化
    Vector3 dir = field.data.direction;
    float len = std::sqrt(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
    if (len < 1e-5f)
        return;
    dir.x /= len;
    dir.y /= len;
    dir.z /= len;

    // strength の絶対値で矢印の長さを決定（最大 radius の 0.6 倍）
    float arrowLen = std::min(r * 0.6f, std::abs(field.data.strength) * 0.5f + r * 0.15f);
    // 矢頭サイズ
    float headLen = arrowLen * 0.25f;

    // 球内に 3×3×3 グリッドで矢印を配置
    const int grid = 3;
    float step = r * 1.6f / (grid - 1);
    for (int ix = 0; ix < grid; ++ix) {
        for (int iy = 0; iy < grid; ++iy) {
            for (int iz = 0; iz < grid; ++iz) {
                Vector3 offset = {
                    -r * 0.8f + ix * step,
                    -r * 0.8f + iy * step,
                    -r * 0.8f + iz * step,
                };
                // 球の外側は除外
                float d2 = offset.x * offset.x + offset.y * offset.y + offset.z * offset.z;
                if (d2 > r * r)
                    continue;

                Vector3 from = {center.x + offset.x, center.y + offset.y, center.z + offset.z};
                Vector3 to = {from.x + dir.x * arrowLen, from.y + dir.y * arrowLen, from.z + dir.z * arrowLen};
                DrawLine3D::GetInstance()->SetPoints(from, to, color);

                // 矢頭：dirに垂直な軸で小さな V 字を描く
                // dir に直交するベクトルを求める
                Vector3 up = {0.0f, 1.0f, 0.0f};
                if (std::abs(dir.y) > 0.9f)
                    up = {1.0f, 0.0f, 0.0f};
                // cross(dir, up)
                Vector3 side = {
                    dir.y * up.z - dir.z * up.y,
                    dir.z * up.x - dir.x * up.z,
                    dir.x * up.y - dir.y * up.x,
                };
                float sLen = std::sqrt(side.x * side.x + side.y * side.y + side.z * side.z);
                if (sLen > 1e-5f) {
                    side.x /= sLen;
                    side.y /= sLen;
                    side.z /= sLen;
                }
                Vector3 headBase = {to.x - dir.x * headLen, to.y - dir.y * headLen, to.z - dir.z * headLen};
                Vector3 h1 = {headBase.x + side.x * headLen * 0.5f, headBase.y + side.y * headLen * 0.5f, headBase.z + side.z * headLen * 0.5f};
                Vector3 h2 = {headBase.x - side.x * headLen * 0.5f, headBase.y - side.y * headLen * 0.5f, headBase.z - side.z * headLen * 0.5f};
                DrawLine3D::GetInstance()->SetPoints(to, h1, color);
                DrawLine3D::GetInstance()->SetPoints(to, h2, color);
            }
        }
    }
}

// --- Attract / Repel：球面から中心、または中心から球面へ向かう放射線 ---
void ParticleCSFieldManager::DrawRadialLines(const ParticleField &field, const Vector4 &color, bool inward) {
    const Vector3 &center = field.data.position;
    const float r = field.data.radius;

    // strength の絶対値で線の長さ割合を決定（0.3〜1.0）
    float ratio = std::min(1.0f, 0.3f + std::abs(field.data.strength) * 0.07f);

    // 正二十面体の頂点方向（12方向）を均一配置の代わりに球面上を均等サンプル
    const int stacks = 4;
    const int slices = 8;
    const float PI = 3.1415926535f;
    for (int si = 0; si < stacks; ++si) {
        float theta = PI * (si + 0.5f) / stacks; // 0 〜 π
        for (int sj = 0; sj < slices; ++sj) {
            float phi = 2.0f * PI * sj / slices;
            Vector3 dir = {
                std::sin(theta) * std::cos(phi),
                std::cos(theta),
                std::sin(theta) * std::sin(phi),
            };
            Vector3 surface = {center.x + dir.x * r, center.y + dir.y * r, center.z + dir.z * r};
            // 線の長さを ratio で縮める（途中まで）
            Vector3 inner = {
                center.x + dir.x * r * (1.0f - ratio),
                center.y + dir.y * r * (1.0f - ratio),
                center.z + dir.z * r * (1.0f - ratio),
            };
            if (inward) {
                // 球面 → 中心方向へ（Attract）
                DrawLine3D::GetInstance()->SetPoints(surface, inner, color);
            } else {
                // 中心 → 球面方向へ（Repel）
                DrawLine3D::GetInstance()->SetPoints(inner, surface, color);
            }
        }
    }
}

// --- Vortex：回転軸周りに螺旋状の円弧を描く ---
void ParticleCSFieldManager::DrawVortexArcs(const ParticleField &field, const Vector4 &color) {
    const Vector3 &center = field.data.position;
    const float r = field.data.radius;

    // 回転軸を正規化
    Vector3 axis = field.data.direction;
    float axLen = std::sqrt(axis.x * axis.x + axis.y * axis.y + axis.z * axis.z);
    if (axLen < 1e-5f)
        return;
    axis.x /= axLen;
    axis.y /= axLen;
    axis.z /= axLen;

    // strength の符号で回転方向を決定、絶対値で螺旋の巻き数を決定
    float sign = (field.data.strength >= 0.0f) ? 1.0f : -1.0f;
    float turns = std::min(1.5f, 0.5f + std::abs(field.data.strength) * 0.1f);

    // 軸に直交するベクトルを生成
    Vector3 up = {0.0f, 1.0f, 0.0f};
    if (std::abs(axis.y) > 0.9f)
        up = {1.0f, 0.0f, 0.0f};
    // right = cross(axis, up)
    Vector3 right = {
        axis.y * up.z - axis.z * up.y,
        axis.z * up.x - axis.x * up.z,
        axis.x * up.y - axis.y * up.x,
    };
    float rLen = std::sqrt(right.x * right.x + right.y * right.y + right.z * right.z);
    right.x /= rLen;
    right.y /= rLen;
    right.z /= rLen;
    // forward = cross(right, axis)
    Vector3 forward = {
        right.y * axis.z - right.z * axis.y,
        right.z * axis.x - right.x * axis.z,
        right.x * axis.y - right.y * axis.x,
    };

    // 高さ方向の異なる3段に円弧を描く
    const int arcLayers = 3;
    const int arcSegments = 24;
    const float PI = 3.1415926535f;
    for (int layer = 0; layer < arcLayers; ++layer) {
        // 各段を軸方向にオフセット（-r*0.5 〜 r*0.5）
        float heightOffset = -r * 0.5f + r * layer / (arcLayers - 1);
        Vector3 layerCenter = {
            center.x + axis.x * heightOffset,
            center.y + axis.y * heightOffset,
            center.z + axis.z * heightOffset,
        };
        // 段ごとに半径を変えて円錐状に見せる
        float layerRadius = r * (0.5f + 0.5f * std::sin(PI * layer / (arcLayers - 1)));

        for (int seg = 0; seg < arcSegments; ++seg) {
            float t1 = sign * 2.0f * PI * turns * seg / arcSegments;
            float t2 = sign * 2.0f * PI * turns * (seg + 1) / arcSegments;

            Vector3 p1 = {
                layerCenter.x + layerRadius * (right.x * std::cos(t1) + forward.x * std::sin(t1)),
                layerCenter.y + layerRadius * (right.y * std::cos(t1) + forward.y * std::sin(t1)),
                layerCenter.z + layerRadius * (right.z * std::cos(t1) + forward.z * std::sin(t1)),
            };
            Vector3 p2 = {
                layerCenter.x + layerRadius * (right.x * std::cos(t2) + forward.x * std::sin(t2)),
                layerCenter.y + layerRadius * (right.y * std::cos(t2) + forward.y * std::sin(t2)),
                layerCenter.z + layerRadius * (right.z * std::cos(t2) + forward.z * std::sin(t2)),
            };
            DrawLine3D::GetInstance()->SetPoints(p1, p2, color);
        }
    }

    // 回転軸そのものを細い線で表示（軸の方向が分かるように）
    Vector3 axisTop = {center.x + axis.x * r * 0.6f, center.y + axis.y * r * 0.6f, center.z + axis.z * r * 0.6f};
    Vector3 axisBot = {center.x - axis.x * r * 0.6f, center.y - axis.y * r * 0.6f, center.z - axis.z * r * 0.6f};
    DrawLine3D::GetInstance()->SetPoints(axisBot, axisTop, color);
}