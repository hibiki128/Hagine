#include "ColliderTagManager.h"

#ifdef _DEBUG
#include <imgui.h>

void ColliderTagManager::ImGuiTagManager() {
    ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("タグマネージャー")) {
        ImGui::Text("登録済みタグ一覧");
        ImGui::Separator();

        // タグリスト表示
        std::vector<std::string> tagList(availableTags_.begin(), availableTags_.end());
        std::sort(tagList.begin(), tagList.end());

        for (const auto &tag : tagList) {
            ImGui::BulletText("%s", tag.c_str());
            ImGui::SameLine(250);

            // デフォルトタグは削除不可
            if (tag != "None" && tag != "Environment" && tag != "Player") {
                ImGui::PushID(tag.c_str());
                if (ImGui::SmallButton("削除")) {
                    RemoveTag(tag);
                }
                ImGui::PopID();
            } else {
                ImGui::TextDisabled("(デフォルト)");
            }
        }

        ImGui::Separator();
        ImGui::Spacing();

        // 新規タグ追加
        static char newTagBuffer[64] = "";
        ImGui::Text("新規タグ追加:");
        ImGui::SetNextItemWidth(200);
        ImGui::InputText("##NewTag", newTagBuffer, sizeof(newTagBuffer));
        ImGui::SameLine();

        if (ImGui::Button("追加")) {
            std::string newTag = newTagBuffer;
            if (!newTag.empty() && !HasTag(newTag)) {
                AddTag(newTag);
                newTagBuffer[0] = '\0';
            }
        }

        ImGui::End();
    }
}

#endif