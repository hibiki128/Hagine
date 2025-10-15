#pragma once
#include "Data/DataHandler.h"
#include <Particle/CSParticle/ParticleCSGroup.h>
#include <memory>
class ParticleCSGroupManager {
  private:
    /// ===================================================
    /// private methods
    /// ===================================================
    static ParticleCSGroupManager *instance;
    ParticleCSGroupManager() = default;
    ~ParticleCSGroupManager() = default;
    ParticleCSGroupManager(ParticleCSGroupManager &) = delete;
    ParticleCSGroupManager &operator=(ParticleCSGroupManager &) = delete;

  public:
    /// ===================================================
    /// public methods
    /// ===================================================
    static ParticleCSGroupManager *GetInstance();

    void Initialize();

    void Finalize();

    void AddParticleCSGroup(std::unique_ptr<ParticleCSGroup> particleCSGroup);

    void CreateParticleCSGroup(const std::string &groupName, const std::string &fileName, uint32_t maxParticleCount = 10000, const std::string &texturePath = {}, BlendMode blendMode = BlendMode::kAdd);
    void CreatePrimitiveParticleCSGroup(const std::string &groupName, PrimitiveType type, uint32_t maxParticleCount = 10000, const std::string &texturePath = {}, BlendMode blendMode = BlendMode::kAdd);

    ParticleCSGroup *GetParticleCSGroup(const std::string &name) {
        for (const auto &group : particleGroups_) {
            if (group->GetGroupName() == name) {
                return group.get();
            }
        }
        return nullptr;
    }

    std::unique_ptr<ParticleCSGroup> CreateParticleCSGroupCopy(const std::string &name) {
        ParticleCSGroup *originalGroup = GetParticleCSGroup(name);
        if (!originalGroup) {
            return nullptr;
        }

        auto copiedGroup = std::make_unique<ParticleCSGroup>();

        // プリミティブタイプか通常のモデルかを判定してコピー
        if (originalGroup->GetPrimitiveType() != PrimitiveType::None) {
            // プリミティブパーティクルグループの場合
            std::string texturePath = originalGroup->GetParticleGroupData().materials.empty() ? "" : originalGroup->GetParticleGroupData().materials[0].textureFilePath;
            uint32_t maxParticleCount = originalGroup->GetSettingsData()->maxParticleCount;
            copiedGroup->CreatePrimitiveParticleGroup(name, originalGroup->GetPrimitiveType(), maxParticleCount, texturePath);
        } else {
            // 通常のモデルパーティクルグループの場合
            std::string texturePath = originalGroup->GetParticleGroupData().materials.empty() ? "" : originalGroup->GetParticleGroupData().materials[0].textureFilePath;
            uint32_t maxParticleCount = originalGroup->GetSettingsData()->maxParticleCount;
            copiedGroup->CreateParticleGroup(name, originalGroup->GetModelPath(), maxParticleCount, texturePath);
        }

        return copiedGroup;
    }

    // エミッター用の独立したパーティクルグループを取得
    ParticleCSGroup *GetIndependentParticleGroup(const std::string &name) {
        auto copiedGroup = CreateParticleCSGroupCopy(name);
        if (!copiedGroup) {
            return nullptr;
        }

        ParticleCSGroup *groupPtr = copiedGroup.get();
        independentGroups_.emplace_back(std::move(copiedGroup));
        return groupPtr;
    }

    std::vector<ParticleCSGroup *> GetParticleGroups() {
        std::vector<ParticleCSGroup *> result;
        for (const auto &group : particleGroups_) {
            result.push_back(group.get()); // unique_ptr から生ポインタを取得
        }
        return result;
    }

    void ClearIndependentGroups() {
        independentGroups_.clear();
    }

    void RemoveUnusedIndependentGroups(const std::unordered_set<std::string> &usedGroupNames) {
        independentGroups_.erase(
            std::remove_if(independentGroups_.begin(), independentGroups_.end(),
                           [&usedGroupNames](const std::unique_ptr<ParticleCSGroup> &group) {
                               return usedGroupNames.find(group->GetGroupName()) == usedGroupNames.end();
                           }),
            independentGroups_.end());
    }

  private:
    /// ===================================================
    /// private variaus
    /// ===================================================

    std::vector<std::unique_ptr<ParticleCSGroup>> particleGroups_;

    std::vector<std::unique_ptr<ParticleCSGroup>> independentGroups_;
};
