#pragma once
#include "Data/DataHandler.h"
#include <Particle/CSParticle/ParticleCSGroup.h>
#include <algorithm>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
class ParticleCSGroupManager {
  private:
    /// ===================================================
    /// private methods
    /// ===================================================
    ParticleCSGroupManager() = default;
    ~ParticleCSGroupManager() = default;
    ParticleCSGroupManager(ParticleCSGroupManager &) = delete;
    ParticleCSGroupManager &operator=(ParticleCSGroupManager &) = delete;

  public:
    /// ===================================================
    /// public methods
    /// ===================================================
    static ParticleCSGroupManager *GetInstance() {
        static ParticleCSGroupManager instance;
        return &instance;
    }

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

    // エミッター用の独立したパーティクルグループを取得する。
    // テンプレート名キーの再利用プールに空きがあれば GPU バッファ/SRV を
    // 再確保せず使い回し（InitParticle で状態だけリセット）、無ければ新規生成する。
    // これによりクローン毎の maxParticleCount*sizeof(CSParticle) 確保 + SRV churn を解消する。
    ParticleCSGroup *GetIndependentParticleGroup(const std::string &name) {
        auto poolIt = groupPool_.find(name);
        if (poolIt != groupPool_.end() && !poolIt->second.empty()) {
            std::unique_ptr<ParticleCSGroup> reused = std::move(poolIt->second.back());
            poolIt->second.pop_back();
            ParticleCSGroup *groupPtr = reused.get();
            groupPtr->ResetForReuse(); // GPU 上のパーティクル状態を初期化し直す
            independentGroups_.emplace_back(std::move(reused));
            return groupPtr;
        }

        // プールに無ければ新規生成（従来通り）
        auto copiedGroup = CreateParticleCSGroupCopy(name);
        if (!copiedGroup) {
            return nullptr;
        }

        ParticleCSGroup *groupPtr = copiedGroup.get();
        independentGroups_.emplace_back(std::move(copiedGroup));
        return groupPtr;
    }

    // 使用済みの独立グループを破棄せず再利用プールへ返却する。
    // エミッター破棄時に呼ぶことで、シーン内での無制限なバッファ累積を防ぐ。
    //
    // 重要: 引数 group は既に破棄済み(ダングリング)の可能性があるため、
    //       ここでは「ポインタ値の比較」しか行わずデリファレンスしない。
    //       名前は所有側 (*it) の有効なオブジェクトから取得する。
    //       Finalize 後など independentGroups_ が空なら単に何もしない（安全）。
    void ReleaseIndependentGroup(ParticleCSGroup *group) {
        if (!group) {
            return;
        }
        for (auto it = independentGroups_.begin(); it != independentGroups_.end(); ++it) {
            if (it->get() == group) {                          // ポインタ比較のみ（derefしない）
                const std::string name = (*it)->GetGroupName(); // 所有側は有効なので安全
                auto &pool = groupPool_[name];
                if (pool.size() < kMaxPooledPerTemplate) {
                    pool.emplace_back(std::move(*it)); // 上限内なら再利用のため保持
                }
                // 上限超過分は unique_ptr 破棄（GPU リソース解放）
                independentGroups_.erase(it);
                return;
            }
        }
        // 見つからない（既に返却済み / Finalize 済み）→ 何もしない
    }

    std::vector<ParticleCSGroup *> GetParticleGroups() {
        std::vector<ParticleCSGroup *> result;
        for (const auto &group : particleGroups_) {
            result.push_back(group.get()); // unique_ptr から生ポインタを取得
        }
        return result;
    }

    // シーン遷移時に呼ばれる。使用中の独立グループを破棄せず（上限内で）プールへ
    // 退避し、次シーンで再利用できるようにする。上限超過分のみ解放する。
    void ClearIndependentGroups() {
        for (auto &group : independentGroups_) {
            if (!group) {
                continue;
            }
            auto &pool = groupPool_[group->GetGroupName()];
            if (pool.size() < kMaxPooledPerTemplate) {
                pool.emplace_back(std::move(group));
            }
        }
        independentGroups_.clear(); // 退避できなかった(上限超過)分はここで解放
    }

    // 名前を指定してパーティクルグループを削除する
    // particleGroups_ と independentGroups_ の両方から探して削除する
    void RemoveParticleCSGroup(const std::string &groupName) {
        // particleGroups_ から削除
        particleGroups_.erase(
            std::remove_if(particleGroups_.begin(), particleGroups_.end(),
                           [&groupName](const std::unique_ptr<ParticleCSGroup> &group) {
                               return group->GetGroupName() == groupName;
                           }),
            particleGroups_.end());
        // independentGroups_ からも削除（エミッターにアタッチされたコピー）
        independentGroups_.erase(
            std::remove_if(independentGroups_.begin(), independentGroups_.end(),
                           [&groupName](const std::unique_ptr<ParticleCSGroup> &group) {
                               return group->GetGroupName() == groupName;
                           }),
            independentGroups_.end());
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

    // 現在エミッターに割り当て中の独立グループ
    std::vector<std::unique_ptr<ParticleCSGroup>> independentGroups_;

    // 解放済みグループの再利用プール（テンプレート名 → 空きグループ群）
    std::unordered_map<std::string, std::vector<std::unique_ptr<ParticleCSGroup>>> groupPool_;

    // テンプレートごとのプール保持上限（メモリの青天井退避を防ぐ）
    static constexpr size_t kMaxPooledPerTemplate = 32;
};