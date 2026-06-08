#define NOMINMAX
#include "AnimationController.h"
#include "Animator.h"
#include "ModelAnimation.h"
#include "Object/Object3d.h"
#include <Data/DataHandler.h>
#include <string>

#ifdef USE_IMGUI
#include <imgui.h>
#endif

namespace Hagine {

void AnimationController::Initialize(Object3d *object) {
    object_ = object;
    clips_.clear();
    index_.clear();
    currentClipName_.clear();
    globalSpeed_ = 1.0f;
    currentClipSpeed_ = 1.0f;
    paused_ = false;
}

void AnimationController::RegisterClip(const std::string &name, const std::string &filePath,
                                      bool loop, float speed, float blendDuration) {
    if (!object_) {
        return;
    }

    auto it = index_.find(name);
    if (it != index_.end()) {
        // 既存クリップのパラメータを更新
        AnimationClip &clip = clips_[it->second];
        clip.filePath = filePath;
        clip.loop = loop;
        clip.speed = speed;
        clip.blendDuration = blendDuration;
    } else {
        // 新規登録
        AnimationClip clip;
        clip.name = name;
        clip.filePath = filePath;
        clip.loop = loop;
        clip.speed = speed;
        clip.blendDuration = blendDuration;
        index_[name] = static_cast<int>(clips_.size());
        clips_.push_back(clip);
    }

    // モデルへアニメーションを追加し、ループフラグを登録
    object_->AddAnimation(filePath, loop);
}

void AnimationController::ApplyClipParams(const AnimationClip &clip) {
    if (!object_) {
        return;
    }
    currentClipSpeed_ = clip.speed;
    object_->SetAnimationBlendDuration(clip.blendDuration);
    object_->SetAnimationSpeed(clip.speed * globalSpeed_);
    object_->SetAnimationLoop(clip.filePath, clip.loop);
}

void AnimationController::Play(const std::string &name) {
    if (!object_) {
        return;
    }
    auto it = index_.find(name);
    if (it == index_.end()) {
        return;
    }

    const AnimationClip &clip = clips_[it->second];
    ApplyClipParams(clip);
    object_->SetAnimation(clip.filePath); // 同一クリップ・補間中でなければ内部で無視される
    currentClipName_ = name;
    paused_ = false;
}

void AnimationController::PlayImmediate(const std::string &name) {
    if (!object_) {
        return;
    }
    auto it = index_.find(name);
    if (it == index_.end()) {
        return;
    }

    const AnimationClip &clip = clips_[it->second];
    ApplyClipParams(clip);
    object_->SetAnimationImmediate(clip.filePath);
    // 即時切り替えで currentModelAnimation_ が差し替わるため速度を再適用
    object_->SetAnimationSpeed(clip.speed * globalSpeed_);
    currentClipName_ = name;
    paused_ = false;
}

void AnimationController::PlayFile(const std::string &filePath, bool loop, float speed, float blend) {
    if (!object_) {
        return;
    }

    // 同じファイルパスの登録済みクリップがあれば、そのパラメータ（速度・ループ・補間）で再生する。
    // これにより ImGui や JSON で調整したコンボ用クリップの設定が再生に反映され、
    // currentClipName_ もクリップ名になるためクリップ一覧の即時編集・ハイライトが効く
    int clipIndex = FindClipIndexByFilePath(filePath);
    if (clipIndex >= 0) {
        const AnimationClip &clip = clips_[clipIndex];
        ApplyClipParams(clip);
        object_->SetAnimation(clip.filePath);
        currentClipName_ = clip.name;
        paused_ = false;
        return;
    }

    // 未登録ファイル：呼び出し側が渡したパラメータで再生する
    object_->AddAnimation(filePath, loop);

    currentClipSpeed_ = speed;
    object_->SetAnimationBlendDuration(blend);
    object_->SetAnimationSpeed(speed * globalSpeed_);
    object_->SetAnimationLoop(filePath, loop);
    object_->SetAnimation(filePath);
    currentClipName_ = filePath;
    paused_ = false;
}

int AnimationController::FindClipIndexByFilePath(const std::string &filePath) const {
    for (int i = 0; i < static_cast<int>(clips_.size()); ++i) {
        if (clips_[i].filePath == filePath) {
            return i;
        }
    }
    return -1;
}

bool AnimationController::HasClip(const std::string &name) const {
    return index_.find(name) != index_.end();
}

Animator *AnimationController::GetAnimator() const {
    if (!object_) {
        return nullptr;
    }
    ModelAnimation *modelAnimation = object_->GetCurrentModelAnimation();
    if (!modelAnimation) {
        return nullptr;
    }
    return modelAnimation->GetAnimator();
}

bool AnimationController::IsFinished() const {
    Animator *animator = GetAnimator();
    return animator ? animator->IsFinish() : false;
}

bool AnimationController::IsBlending() const {
    return object_ ? object_->IsAnimationBlending() : false;
}

float AnimationController::GetAnimationTime() const {
    Animator *animator = GetAnimator();
    return animator ? animator->GetAnimationTime() : 0.0f;
}

float AnimationController::GetDuration() const {
    Animator *animator = GetAnimator();
    return animator ? animator->GetMutableAnimation().duration : 0.0f;
}

void AnimationController::SetPaused(bool paused) {
    paused_ = paused;
    Animator *animator = GetAnimator();
    if (animator) {
        animator->SetIsAnimation(!paused);
    }
}

void AnimationController::SetTime(float time) {
    Animator *animator = GetAnimator();
    if (animator) {
        animator->SetAnimationTime(time);
    }
}

void AnimationController::SetGlobalSpeed(float speed) {
    globalSpeed_ = speed;
    if (object_) {
        object_->SetAnimationSpeed(currentClipSpeed_ * globalSpeed_);
    }
}

void AnimationController::SaveClips(const std::string &folder, const std::string &file) {
    DataHandler data(folder, file);
    data.Save("count", static_cast<int>(clips_.size()));
    data.Save("globalSpeed", globalSpeed_);

    for (int i = 0; i < static_cast<int>(clips_.size()); ++i) {
        const AnimationClip &clip = clips_[i];
        std::string prefix = "clip" + std::to_string(i) + "_";
        data.Save(prefix + "name", clip.name);
        data.Save(prefix + "file", clip.filePath);
        data.Save(prefix + "loop", clip.loop ? 1 : 0);
        data.Save(prefix + "speed", clip.speed);
        data.Save(prefix + "blend", clip.blendDuration);
    }
    // DataHandler のデストラクタで自動的にファイルへ書き出される
}

void AnimationController::LoadClips(const std::string &folder, const std::string &file) {
    DataHandler data(folder, file);
    if (!data.Exists()) {
        return;
    }

    int count = data.Load<int>("count", 0);
    for (int i = 0; i < count; ++i) {
        std::string prefix = "clip" + std::to_string(i) + "_";
        std::string name = data.Load<std::string>(prefix + "name", "");
        auto it = index_.find(name);
        if (it == index_.end()) {
            continue; // コード側に存在しないクリップはスキップ
        }

        AnimationClip &clip = clips_[it->second];
        clip.loop = data.Load<int>(prefix + "loop", clip.loop ? 1 : 0) != 0;
        clip.speed = data.Load<float>(prefix + "speed", clip.speed);
        clip.blendDuration = data.Load<float>(prefix + "blend", clip.blendDuration);
        if (object_) {
            object_->SetAnimationLoop(clip.filePath, clip.loop);
        }
    }

    globalSpeed_ = data.Load<float>("globalSpeed", globalSpeed_);
    if (object_) {
        object_->SetAnimationSpeed(currentClipSpeed_ * globalSpeed_);
    }
}

void AnimationController::DrawImGui() {
#ifdef USE_IMGUI
    if (!object_) {
        ImGui::TextDisabled("Object3d が未設定です");
        return;
    }

    DrawTransportImGui();
    ImGui::Separator();
    DrawClipListImGui();
    ImGui::Separator();
    DrawKeyframeImGui();

    ImGui::Separator();
    if (ImGui::Button("クリップ設定を保存")) {
        SaveClips("AnimationController", "PlayerClips");
    }
    ImGui::SameLine();
    if (ImGui::Button("クリップ設定を読込")) {
        LoadClips("AnimationController", "PlayerClips");
    }
#endif // USE_IMGUI
}

void AnimationController::DrawTransportImGui() {
#ifdef USE_IMGUI
    ImGui::Text("現在クリップ: %s", currentClipName_.empty() ? "(なし)" : currentClipName_.c_str());
    if (IsBlending()) {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "[補間中]");
    }

    float duration = GetDuration();
    float time = GetAnimationTime();
    float fraction = (duration > 0.0f) ? (time / duration) : 0.0f;
    ImGui::ProgressBar(fraction, ImVec2(-1.0f, 0.0f));

    // 再生 / 一時停止トグル
    if (ImGui::Button(paused_ ? "再生" : "一時停止")) {
        SetPaused(!paused_);
    }
    ImGui::SameLine();
    if (ImGui::Button("先頭へ")) {
        SetTime(0.0f);
    }

    // 時間スクラブ（一時停止中のみ手動操作を反映）
    if (duration > 0.0f) {
        if (ImGui::SliderFloat("再生時間", &time, 0.0f, duration, "%.3f s")) {
            if (!paused_) {
                SetPaused(true);
            }
            SetTime(time);
        }
    }

    // 全体速度
    if (ImGui::DragFloat("全体速度", &globalSpeed_, 0.01f, 0.0f, 8.0f)) {
        SetGlobalSpeed(globalSpeed_);
    }
#endif // USE_IMGUI
}

void AnimationController::DrawClipListImGui() {
#ifdef USE_IMGUI
    if (!ImGui::CollapsingHeader("クリップ一覧", ImGuiTreeNodeFlags_DefaultOpen)) {
        return;
    }

    for (int i = 0; i < static_cast<int>(clips_.size()); ++i) {
        AnimationClip &clip = clips_[i];
        ImGui::PushID(i);

        if (ImGui::Button("再生")) {
            Play(clip.name);
        }
        ImGui::SameLine();
        bool isCurrent = (clip.name == currentClipName_);
        if (isCurrent) {
            ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "%s", clip.name.c_str());
        } else {
            ImGui::Text("%s", clip.name.c_str());
        }

        ImGui::Indent();
        ImGui::TextDisabled("%s", clip.filePath.c_str());
        if (ImGui::Checkbox("ループ", &clip.loop)) {
            object_->SetAnimationLoop(clip.filePath, clip.loop);
        }
        ImGui::SameLine();
        ImGui::SetNextItemWidth(120.0f);
        ImGui::DragFloat("速度", &clip.speed, 0.01f, 0.0f, 8.0f);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(120.0f);
        ImGui::DragFloat("補間", &clip.blendDuration, 0.01f, 0.0f, 2.0f);
        // 現在再生中のクリップなら変更を即時反映
        if (isCurrent) {
            ApplyClipParams(clip);
        }
        ImGui::Unindent();

        ImGui::PopID();
        ImGui::Spacing();
    }
#endif // USE_IMGUI
}

void AnimationController::DrawKeyframeImGui() {
#ifdef USE_IMGUI
    if (!ImGui::CollapsingHeader("キーフレーム編集")) {
        return;
    }

    Animator *animator = GetAnimator();
    if (!animator) {
        ImGui::TextDisabled("アニメーターがありません");
        return;
    }

    ImGui::TextDisabled("編集は現在再生中クリップに即時反映されます（クリップ切替で元に戻ります）");

    Animation &animation = animator->GetMutableAnimation();
    if (animation.nodeAnimations.empty()) {
        ImGui::TextDisabled("ノードアニメーションがありません");
        return;
    }

    // ノード名一覧を構築
    std::vector<const std::string *> nodeNames;
    nodeNames.reserve(animation.nodeAnimations.size());
    for (auto &pair : animation.nodeAnimations) {
        nodeNames.push_back(&pair.first);
    }
    if (selectedNodeIndex_ >= static_cast<int>(nodeNames.size())) {
        selectedNodeIndex_ = 0;
    }

    // ノード選択コンボ
    if (ImGui::BeginCombo("ノード", nodeNames[selectedNodeIndex_]->c_str())) {
        for (int i = 0; i < static_cast<int>(nodeNames.size()); ++i) {
            bool selected = (i == selectedNodeIndex_);
            if (ImGui::Selectable(nodeNames[i]->c_str(), selected)) {
                selectedNodeIndex_ = i;
            }
        }
        ImGui::EndCombo();
    }

    // チャンネル選択
    const char *channelLabels[] = {"平行移動", "回転", "スケール"};
    ImGui::Combo("チャンネル", &selectedChannel_, channelLabels, 3);

    NodeAnimation &node = animation.nodeAnimations[*nodeNames[selectedNodeIndex_]];

    if (selectedChannel_ == 1) {
        // 回転（Quaternion）
        std::vector<KeyframeQuaternion> &keys = node.rotate;
        for (int i = 0; i < static_cast<int>(keys.size()); ++i) {
            ImGui::PushID(i);
            ImGui::SetNextItemWidth(90.0f);
            ImGui::DragFloat("時間", &keys[i].time, 0.01f, 0.0f, animation.duration);
            ImGui::SameLine();
            ImGui::SetNextItemWidth(260.0f);
            ImGui::DragFloat4("XYZW", &keys[i].value.x, 0.01f);
            ImGui::SameLine();
            if (ImGui::SmallButton("削除")) {
                keys.erase(keys.begin() + i);
                ImGui::PopID();
                break;
            }
            ImGui::PopID();
        }
        if (ImGui::Button("キーフレーム追加")) {
            KeyframeQuaternion kf = keys.empty() ? KeyframeQuaternion{{0.0f, 0.0f, 0.0f, 1.0f}, 0.0f} : keys.back();
            kf.time += 0.1f;
            keys.push_back(kf);
        }
    } else {
        // 平行移動 / スケール（Vector3）
        std::vector<KeyframeVector3> &keys = (selectedChannel_ == 0) ? node.translate : node.scale;
        for (int i = 0; i < static_cast<int>(keys.size()); ++i) {
            ImGui::PushID(i);
            ImGui::SetNextItemWidth(90.0f);
            ImGui::DragFloat("時間", &keys[i].time, 0.01f, 0.0f, animation.duration);
            ImGui::SameLine();
            ImGui::SetNextItemWidth(200.0f);
            ImGui::DragFloat3("XYZ", &keys[i].value.x, 0.01f);
            ImGui::SameLine();
            if (ImGui::SmallButton("削除")) {
                keys.erase(keys.begin() + i);
                ImGui::PopID();
                break;
            }
            ImGui::PopID();
        }
        if (ImGui::Button("キーフレーム追加")) {
            Vector3 defaultValue = (selectedChannel_ == 2) ? Vector3{1.0f, 1.0f, 1.0f} : Vector3{0.0f, 0.0f, 0.0f};
            KeyframeVector3 kf = keys.empty() ? KeyframeVector3{defaultValue, 0.0f} : keys.back();
            kf.time += 0.1f;
            keys.push_back(kf);
        }
    }
#endif // USE_IMGUI
}

} // namespace Hagine
