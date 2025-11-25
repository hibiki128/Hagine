#pragma once
#include "Camera/ViewProjection/ViewProjection.h"
#include "Data/DataHandler.h"
#include "myMath.h"
#include "type/Quaternion.h"
#include "type/Vector3.h"
#include <functional>
#include <memory>
#include <string>
#include <unordered_set>

class ColliderTagManager {
  public:
    static ColliderTagManager *GetInstance() {
        static ColliderTagManager instance;
        return &instance;
    }

    // タグの追加
    void AddTag(const std::string &tag) {
        if (!tag.empty()) {
            availableTags_.insert(tag);
        }
    }

    // タグの削除
    void RemoveTag(const std::string &tag) {
        availableTags_.erase(tag);
    }

    // 全タグ取得
    const std::unordered_set<std::string> &GetAllTags() const {
        return availableTags_;
    }

    // タグが存在するか確認
    bool HasTag(const std::string &tag) const {
        return availableTags_.find(tag) != availableTags_.end();
    }

    // デフォルトタグの初期化
    void InitializeDefaultTags() {
        AddTag("None");
        AddTag("Environment");
        AddTag("Player");
        AddTag("Enemy");
        AddTag("Projectile");
        AddTag("Makan");
        AddTag("PlayerBullet");
        AddTag("PlayerChargeBullet");
        AddTag("PlayerHand");
    }

#ifdef _DEBUG
    // ImGuiでタグ管理UI表示
    void ImGuiTagManager();
#endif

  private:
    ColliderTagManager() {
        InitializeDefaultTags();
    }
    ~ColliderTagManager() = default;
    ColliderTagManager(const ColliderTagManager &) = delete;
    ColliderTagManager &operator=(const ColliderTagManager &) = delete;

    std::unordered_set<std::string> availableTags_;
};