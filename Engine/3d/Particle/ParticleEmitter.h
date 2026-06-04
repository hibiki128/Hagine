#pragma once
#include "Camera/ViewProjection/ViewProjection.h"
#include "ParticleManager.h"
#include "Transform/WorldTransform.h"
#include <string>
#ifdef _DEBUG
#include "imgui.h"
#endif

#include "externals/nlohmann/json.hpp"

#include "Data/DataHandler.h"
#include <filesystem>
#include <fstream>

namespace Hagine {
class ParticleEmitter {
  public:
    // コンストラクタでメンバ変数を初期化
    ParticleEmitter();

    void Initialize(std::string name = {});

    // 更新処理を行うUpdate関数
    void Update();

    void UpdateOnce();

    void Draw(const ViewProjection &vp_);

    void DrawEmitter();

    void Debug(); // ImGui用の関数を追加

    bool IsAllParticlesComplete();

    void AddParticleGroup(ParticleGroup *particleGroup);
    void RemoveParticleGroup(const std::string &name) {
        Manager_->RemoveParticleGroup(name);
    }

    int selectedGroupIndex_ = 0;

    std::unique_ptr<ParticleEmitter> Clone() const;

    bool GetIsAuto() { return isAuto_; }
    Matrix4x4 GetWorldMatrix() { return transform_.matWorld_; }
    Vector3 GetPosition() { return transform_.translation_; }
    void SetPosition(const Vector3 &position) { transform_.translation_ = position; }

    bool IsGizmoSelectable() const { return isGizmoSelectable_; }
    void SetGizmoSelectable(bool selectable) { isGizmoSelectable_ = selectable; }

  public:
    void SetPositionY(const std::string &groupName, float positionY) {
        particleSettings_[groupName].translate.y = positionY;
        FlushSetting(groupName); // Manager に即時反映
    }
    void SetRotate(const std::string &groupName, const Vector3 &rotate) {
        particleSettings_[groupName].rotation = rotate;
        FlushSetting(groupName);
    }
    void SetRotateY(const std::string &groupName, float rotateY) {
        particleSettings_[groupName].rotation.y = rotateY;
        FlushSetting(groupName);
    }
    void SetScale(const std::string &groupName, const Vector3 &scale) {
        particleSettings_[groupName].scale = scale;
        FlushSetting(groupName);
    }

    // -------------------------------------------------------
    // 【修正】SetStartScale / SetEndScale
    //   particleSettings_ への書き込みと同時に Manager_ にも
    //   即時反映する。
    //   transform の dirty 判定とは独立して動作する。
    // -------------------------------------------------------
    void SetStartScale(const std::string &groupName, const Vector3 &scale) {
        particleSettings_[groupName].particleStartScale = scale;
        FlushSetting(groupName);
    }
    void SetEndScale(const std::string &groupName, const Vector3 &scale) {
        particleSettings_[groupName].particleEndScale = scale;
        FlushSetting(groupName);
    }

    void SetWorldMatrix(const Matrix4x4 &worldMatrix) {
        transform_.matWorld_ = worldMatrix;
    }
    void SetIsAuto(bool isAuto) { isAuto_ = isAuto; }
    void SetCount(const std::string &groupName, int count) {
        particleSettings_[groupName].count = count;
        FlushSetting(groupName);
    }
    void SetStartRotate(const std::string &groupName, const Vector3 &startRotate) {
        particleSettings_[groupName].startRote = startRotate;
        FlushSetting(groupName);
    }
    void SetEndRotate(const std::string &groupName, const Vector3 &endRotate) {
        particleSettings_[groupName].endRote = endRotate;
        FlushSetting(groupName);
    }
    void SetActive(bool isActive) { isActive_ = isActive; }
    void SetFrequency(float frequency) { emitFrequency_ = frequency; }
    void SetName(const std::string &name) { name_ = name; }
    void SetTrailEnabled(const std::string &groupName, bool enabled);
    void SetTrailInterval(const std::string &groupName, float interval);
    void SetMaxTrailParticles(const std::string &groupName, int maxTrails);
    void SetTrailLifeScale(const std::string &groupName, float scale);
    void SetTrailScaleMultiplier(const std::string &groupName, const Vector3 &multiplier);
    void SetTrailColorMultiplier(const std::string &groupName, const Vector4 &multiplier);
    void SetTrailVelocityInheritance(const std::string &groupName, bool inherit, float scale = 0.3f);
    void SetStartColor(const std::string &groupName, const Vector4 &color) {
        particleSettings_[groupName].startColor = color;
        FlushSetting(groupName);
    }
    void SetEndColor(const std::string &groupName, const Vector4 &color) {
        particleSettings_[groupName].endColor = color;
        FlushSetting(groupName);
    }

    // -------------------------------------------------------
    // 【修正】SetScaleAll / SetStartAcce 系
    //   全グループへの一括書き込みも Manager_ に即時反映する
    // -------------------------------------------------------
    void SetScaleAll(const Vector3 &scale) {
        for (auto &[groupName, setting] : particleSettings_) {
            if (setting.isSinMove) {
                setting.particleStartScale = scale;
            } else {
                setting.scale = scale;
            }
            FlushSetting(groupName);
        }
    }
    void SetStartAcce(const Vector3 &acce) {
        for (auto &[groupName, setting] : particleSettings_) {
            setting.startAcce = acce;
            FlushSetting(groupName);
        }
    }
    void SetStartAcceX(const float &acce) {
        for (auto &[groupName, setting] : particleSettings_) {
            setting.startAcce.x = acce;
            FlushSetting(groupName);
        }
    }
    void SetStartAcceZ(const float &acce) {
        for (auto &[groupName, setting] : particleSettings_) {
            setting.startAcce.z = acce;
            FlushSetting(groupName);
        }
    }
    void SetEndAcce(const Vector3 &acce) {
        for (auto &[groupName, setting] : particleSettings_) {
            setting.endAcce = acce;
            FlushSetting(groupName);
        }
    }

    size_t GetActiveParticleCount() const {
        return Manager_ ? Manager_->GetActiveParticleCount() : 0;
    }

    // パーティクルマネージャーへのアクセス（デバッグ用）
    ParticleManager *GetParticleManager() const {
        return Manager_.get();
    }

  private:
    // パーティクルを発生させるEmit関数（外部用・設定同期込み）
    void Emit();
    // transform を全グループの ParticleSetting に反映する（dirty判定用）
    void SyncSettingsToTransform();
    // 設定同期なしで発射するだけの内部用関数
    void EmitInternal();

    void FlushSetting(const std::string &groupName) {
        if (!Manager_)
            return;
        auto it = particleSettings_.find(groupName);
        if (it == particleSettings_.end())
            return;
        // transform 系は常に現在の transform_ を優先して上書き
        it->second.translate = transform_.translation_;
        it->second.rotation = transform_.quateRotation_.ToEulerAngles();
        it->second.scale = transform_.scale_;
        Manager_->SetParticleSetting(groupName, it->second);
    }

    void SaveToJson();
    void LoadFromJson();
    void LoadParticleGroup();
    ParticleSetting DefaultSetting();
    void ShowBlendModeCombo(BlendMode &currentMode);
    void DebugParticleData();

  private:
    using json = nlohmann::json;
    float elapsedTime_ = 0.0f;   // 経過時間
    float emitFrequency_ = 0.1f; // パーティクルの発生頻度

    bool isVisible_ = false;
    bool isActive_ = false;
    bool isAuto_ = false;
    bool isGizmoSelectable_ = true;

    std::string name_;         // パーティクルの名前
    WorldTransform transform_; // 位置や回転などのトランスフォーム

    std::unordered_map<std::string, ParticleSetting> particleSettings_;

    std::unique_ptr<ParticleManager> Manager_;
    std::unique_ptr<DataHandler> datas_;
    std::vector<std::string> particleGroupNames_;

    // ★ dirty判定用：前フレームの transform を保持する
    Vector3 lastTranslation_ = {};
    Quaternion lastRotation_ = Quaternion::IdentityQuaternion();
    Vector3 lastScale_ = {1.0f, 1.0f, 1.0f};
};
} // namespace Hagine
