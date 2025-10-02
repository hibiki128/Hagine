#pragma once
#include "Sprite.h"
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

// インスタンス単位でのSRTデータ
struct InstanceSRT {
    Vector3 scale = {1.0f, 1.0f, 1.0f};
    Vector3 rotation = {0.0f, 0.0f, 0.0f};
    Vector3 translation = {0.0f, 0.0f, 0.0f};
    bool isActive = true; // このインスタンスが描画されるかどうか
};

// スプライト情報を管理する構造体
struct SpriteTransform {
    Vector2 position = {0.0f, 0.0f};
    Vector4 color = {1.0f, 1.0f, 1.0f, 1.0f};
    Vector2 anchorPoint = {0.0f, 0.0f};
    bool isFlipX = false;
    bool isFlipY = false;
    uint32_t instanceCount = 1;

    // コンストラクタで簡単に初期化できるように
    SpriteTransform() = default;
    SpriteTransform(Vector2 pos, Vector4 col = {1.0f, 1.0f, 1.0f, 1.0f},
                    Vector2 anchor = {0.0f, 0.0f}, bool flipX = false, bool flipY = false, uint32_t count = 1)
        : position(pos), color(col), anchorPoint(anchor), isFlipX(flipX), isFlipY(flipY), instanceCount(count) {}
};

// ManagedSpriteに名前とテクスチャパスを追加
struct SpriteData {
    std::unique_ptr<Sprite> sprite;
    std::string name;
    std::string textureFilePath; // テクスチャファイルパスを保持
    std::vector<InstanceSRT> instanceData;
    std::function<void(SpriteData &, float)> updateFunction;
    bool isVisible = true;
    bool isBackMost = false;

    SpriteData(const std::string &spriteName, const std::string &texturePath, uint32_t instanceCount = 1)
        : name(spriteName), textureFilePath(texturePath), instanceData(instanceCount) {}
};

class SpriteManager {
  public:
    static SpriteManager *GetInstance();

    // スプライトの登録
    void RegisterSprite(const std::string &name, const std::string &textureFilePath, const SpriteTransform &transform = SpriteTransform());

    // スプライトの削除
    void UnregisterSprite(const std::string &name);

    // 全スプライトの一括描画
    void DrawAll();

    // 全スプライトの一括更新
    void UpdateAll(float deltaTime);

    void UpdateImGui();

    void ShowSpriteCreationModal() { showSpriteCreationModal_ = true; }
    void DrawSpriteCreationModal();
    void DrawSpriteManager();

    // 特定のスプライトを取得
    SpriteData *GetSprite(const std::string &name);

    std::string GetTextureFilePath(const std::string &name);

    // 特定インスタンスのSRTデータを設定
    void SetInstanceSRT(const std::string &name, uint32_t index, const InstanceSRT &srt);
    void SetInstanceScale(const std::string &name, uint32_t index, const Vector3 &scale);
    void SetInstanceRotation(const std::string &name, uint32_t index, const Vector3 &rotation);
    void SetInstanceTranslation(const std::string &name, uint32_t index, const Vector3 &translation);
    void SetInstanceActive(const std::string &name, uint32_t index, bool isActive);

    // 特定インスタンスのSRTデータを取得
    InstanceSRT *GetInstanceSRT(const std::string &name, uint32_t index);

    // スプライト全体の設定
    void SetSpriteVisible(const std::string &name, bool visible);
    void SetSpriteBackMost(const std::string &name, bool isBackMost);
    void SetSpritePosition(const std::string &name, const Vector2 &position);
    void SetSpriteSize(const std::string &name, const Vector2 &size);
    void SetSpriteColor(const std::string &name, const Vector4 &color);
    void SetTextureFilePath(const std::string &name, const std::string &textureFilePath);

    // カスタム更新関数の設定
    void SetUpdateFunction(const std::string &name, std::function<void(SpriteData &, float)> updateFunc);

    void SetSaveFolder(const std::string &folderName);
    void SaveAllSprites();
    void LoadAllSprites();

    // 全スプライトをクリア
    void Clear();

  private:
    SpriteManager() = default;
    ~SpriteManager() = default;
    SpriteManager(const SpriteManager &) = delete;
    SpriteManager &operator=(const SpriteManager &) = delete;

    void SaveDrawOrder();
    void LoadDrawOrder();
    SpriteData *FindSpriteByName(const std::string &name);
    int FindSpriteIndex(const std::string &name);

    std::vector<std::unique_ptr<SpriteData>> sprites_;

    // 内部でTransformationMatrixを更新する関数
    void UpdateSpriteInstances(SpriteData *spriteData);

    bool showSpriteCreationModal_ = false;
    std::string texturePath_ = "";
    std::string saveFolder_ = "Sprite";
};