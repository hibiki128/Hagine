#pragma once
#include "ParticleManager.h"
#include "ViewProjection/ViewProjection.h"
#include "WorldTransform.h"
#include <string>
#ifdef _DEBUG
#include "imgui.h"
#endif

#include "externals/nlohmann/json.hpp"

#include "Data/DataHandler.h"
#include <filesystem>
#include <fstream>

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

    void AddParticleGroup(ParticleGroup *particleGroup);
    void RemoveParticleGroup(const std::string &name) {
        Manager_->RemoveParticleGroup(name);
    }

    void SetPosition(const Vector3 &position) { particleSetting_.translate = position; }
    void SetPositionY(const float &positionY) { particleSetting_.translate.y = positionY; }
    void SetRotate(const Vector3 &rotate) { particleSetting_.rotation = rotate; }
    void SetRotateY(const float &rotateY) { particleSetting_.rotation.y = rotateY; }
    void SetScale(const Vector3 &scale) { particleSetting_.scale = scale; }
    void SetCount(const int &count) { particleSetting_.count = count; }
    void SetActive(bool isActive) { isActive_ = isActive; }
    void SetStartRotate(const Vector3 &startRotate) { particleSetting_.startRote = startRotate; }
    void SetEndRotate(const Vector3 &endRotate) { particleSetting_.endRote = endRotate; }
    void SetFrequency(const float &frequency) { emitFrequency_ = frequency; }
    void SetName(const std::string &name) {
        name_ = name;
    }

  private:
    // パーティクルを発生させるEmit関数
    void Emit();
    void SaveToJson();
    void LoadFromJson();
    void LoadParticleGroup();

    void DebugParticleData();

  private:
    using json = nlohmann::json;
    float elapsedTime_;   // 経過時間
    float emitFrequency_; // パーティクルの発生頻度

    bool isVisible_ = false;
    bool isBillBoard_ = false;
    bool isActive_ = false;
    bool isAuto_ = false;

    std::string name_;         // パーティクルの名前
    WorldTransform transform_; // 位置や回転などのトランスフォーム

    ParticleSetting particleSetting_;

    std::unique_ptr<ParticleManager> Manager_;
    std::unique_ptr<DataHandler> datas_;
    std::vector<std::string> particleGroupNames_;
};