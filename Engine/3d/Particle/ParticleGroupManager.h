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

    ParticleGroup *GetParticleGroup(const std::string &name) {
        for (const auto &group : particleGroups_) {
            if (group->GetGroupName() == name) {
                return group.get();
            }
        }
        return nullptr;
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
};
