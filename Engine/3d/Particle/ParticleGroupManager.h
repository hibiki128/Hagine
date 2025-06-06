#pragma once

#include "Data/DataHandler.h"
#include "ParticleGroup.h"
#include "memory"

class ParticleGroupManager {
  private:
    /// ===================================================
    /// private method
    /// ===================================================
    static ParticleGroupManager *instance;
    ParticleGroupManager() = default;
    ~ParticleGroupManager() = default;
    ParticleGroupManager(ParticleGroupManager &) = delete;
    ParticleGroupManager &operator=(ParticleGroupManager &) = delete;

  public:
    /// ===================================================
    /// public method
    /// ===================================================
    static ParticleGroupManager *GetInstance();

    void Initialize();

    void Finalize();

    void AddParticleGroup(std::unique_ptr<ParticleGroup> particleGroup);

    void CreateParticleGroup(const std::string &groupName, const std::string &filename, const std::string &texturePath = {});
    void CreatePrimitiveParticleGroup(const std::string &groupName, PrimitiveType type, const std::string &texturePath);

    // 既存の取得メソッド（参照用）
    ParticleGroup *GetParticleGroup(const std::string &name) {
        for (const auto &group : particleGroups_) {
            if (group->GetGroupName() == name) {
                return group.get();
            }
        }
        return nullptr;
    }

    // 新しいコピー作成メソッド
    std::unique_ptr<ParticleGroup> CreateParticleGroupCopy(const std::string &name) {
        ParticleGroup *originalGroup = GetParticleGroup(name);
        if (!originalGroup) {
            return nullptr;
        }

        auto copiedGroup = std::make_unique<ParticleGroup>();

        // プリミティブタイプか通常のモデルかを判定してコピー
        if (originalGroup->GetPrimitiveType() != PrimitiveType::kCount) {
            // プリミティブパーティクルグループの場合
            std::string texturePath = originalGroup->GetParticleGroupData().materials.empty() ? "" : originalGroup->GetParticleGroupData().materials[0].textureFilePath;
            copiedGroup->CreatePrimitiveParticleGroup(name, originalGroup->GetPrimitiveType(), texturePath);
        } else {
            // 通常のモデルパーティクルグループの場合
            std::string texturePath = originalGroup->GetParticleGroupData().materials.empty() ? "" : originalGroup->GetParticleGroupData().materials[0].textureFilePath;
            copiedGroup->CreateParticleGroup(name, originalGroup->GetModelPath(), texturePath);
        }

        return copiedGroup;
    }

    // エミッター用の独立したパーティクルグループを取得
    ParticleGroup *GetIndependentParticleGroup(const std::string &name) {
        auto copiedGroup = CreateParticleGroupCopy(name);
        if (!copiedGroup) {
            return nullptr;
        }

        ParticleGroup *groupPtr = copiedGroup.get();
        independentGroups_.emplace_back(std::move(copiedGroup));
        return groupPtr;
    }

    std::vector<ParticleGroup *> GetParticleGroups() {
        std::vector<ParticleGroup *> result;
        for (const auto &group : particleGroups_) {
            result.push_back(group.get()); // unique_ptr から生ポインタを取得
        }
        return result;
    }

  private:
    /// ============================================
    /// private variaus
    /// ============================================

    std::vector<std::unique_ptr<ParticleGroup>> particleGroups_;
    // エミッター用の独立したパーティクルグループを管理
    std::vector<std::unique_ptr<ParticleGroup>> independentGroups_;
};