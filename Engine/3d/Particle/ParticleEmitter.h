#pragma once
#include "ParticleManager.h"
#include"ViewProjection/ViewProjection.h"
#include "WorldTransform.h"
#include <string>
#ifdef _DEBUG
#include "imgui.h"
#endif

#include "externals/nlohmann/json.hpp"

#include <filesystem>
#include <fstream>
#include"Data/DataHandler.h"

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

    void AddParticleGroup(ParticleGroup* particleGroup);
    void RemoveParticleGroup(const std::string &name) {
        Manager_->RemoveParticleGroup(name);
    }

    void SetPosition(const Vector3 &position) { transform_.translation_ = position; }
    void SetPositionY(const float &positionY) { transform_.translation_.y = positionY; }
    void SetRotate(const Vector3 &rotate) { transform_.rotation_ = rotate; }
    void SetRotateY(const float &rotateY) { transform_.rotation_.y = rotateY; }
    void SetScale(const Vector3 &scale) { transform_.scale_ = scale; }
    void SetCount(const int &count) { count_ = count; }
    void SetActive(bool isActive) { isActive_ = isActive; }
    void SetStartRotate(const Vector3 &startRotate) { startRote_ = startRotate; }
    void SetEndRotate(const Vector3 &endRotate) { endRote_ = endRotate; }
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
    float elapsedTime_; // 経過時間

    std::string name_;         // パーティクルの名前
    WorldTransform transform_; // 位置や回転などのトランスフォーム
    int count_;                // 一度に発生させるパーティクルの数

    float emitFrequency_; // パーティクルの発生頻度
    float lifeTimeMin_;   // ライフタイムの最小値
    float lifeTimeMax_;   // ライフタイムの最大値
    float alphaMin_;
    float alphaMax_;
    float scaleMin_;
    float scaleMax_;
    float gatherStartRatio_ = 0.5f;
    float gatherStrength_ = 2.0f;

    Vector4 color_;
    Vector3 velocityMin_; // 速度の最小値
    Vector3 velocityMax_; // 速度の最大値
    Vector3 startScale_;
    Vector3 endScale_;
    Vector3 startAcce_;
    Vector3 endAcce_;
    Vector3 startRote_ = {};
    Vector3 endRote_ = {};
    Vector3 rotateVelocityMin_;
    Vector3 rotateVelocityMax_;
    Vector3 rotateStartMax_;
    Vector3 rotateStartMin_;
    Vector3 allScaleMin_;
    Vector3 allScaleMax_;

    bool isRandomScale_ = false;
    bool isAllRamdomScale_ = false;
    bool isRandomColor_ = true;
    bool isRandomRotate_ = false;
    bool isRotateVelocity_ = false;
    bool isVisible_;
    bool isBillBoard_ = true;
    bool isActive_ = false;
    bool isAcceMultiply_ = false;
    bool isSinMove_ = false;
    bool isFaceDirection_ = false;
    bool isAuto_ = false;
    bool isEndScale_ = false;
    bool isEmitOnEdge_ = false;
    bool isGatherMode_ = false; // 中心に集まるフラグ

    

    std::unique_ptr<ParticleManager> Manager_;
    std::unique_ptr<DataHandler> datas_;
    std::vector<std::string> particleGroupNames_;
};