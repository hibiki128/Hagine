#pragma once
#include "Sprite.h"
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

/// <summary>
/// インスタンス単位でのSRTデータ構造体
/// </summary>
struct InstanceSRT {
    Vector3 scale = {1.0f, 1.0f, 1.0f};       // スケール
    Vector3 rotation = {0.0f, 0.0f, 0.0f};    // 回転
    Vector3 translation = {0.0f, 0.0f, 0.0f}; // 移動
    bool isActive = true;                     // 描画フラグ
};

/// <summary>
/// スプライト情報を管理する構造体
/// </summary>
struct SpriteTransform {
    Vector2 position = {0.0f, 0.0f};
    Vector4 color = {1.0f, 1.0f, 1.0f, 1.0f};
    Vector2 anchorPoint = {0.0f, 0.0f};
    bool isFlipX = false;
    bool isFlipY = false;
    uint32_t instanceCount = 1;

    /// <summary>
    /// デフォルトコンストラクタ
    /// </summary>
    SpriteTransform() = default;

    /// <summary>
    /// パラメータ付きコンストラクタ
    /// </summary>
    /// <param name="pos">位置</param>
    /// <param name="col">色</param>
    /// <param name="anchor">アンカーポイント</param>
    /// <param name="flipX">左右反転</param>
    /// <param name="flipY">上下反転</param>
    /// <param name="count">インスタンス数</param>
    SpriteTransform(Vector2 pos, Vector4 col = {1.0f, 1.0f, 1.0f, 1.0f},
                    Vector2 anchor = {0.0f, 0.0f}, bool flipX = false, bool flipY = false, uint32_t count = 1)
        : position(pos), color(col), anchorPoint(anchor), isFlipX(flipX), isFlipY(flipY), instanceCount(count) {}
};

/// <summary>
/// スプライトデータを管理する構造体
/// </summary>
struct SpriteData {
    std::unique_ptr<Sprite> sprite;
    std::string name;                                        // スプライト名
    std::string textureFilePath;                             // テクスチャファイルパス
    std::vector<InstanceSRT> instanceData;                   // インスタンスデータ
    std::function<void(SpriteData &, float)> updateFunction; // カスタム更新関数
    bool isVisible = true;                                   // 表示フラグ
    bool isBackMost = false;                                 // 背面フラグ

    /// <summary>
    /// コンストラクタ
    /// </summary>
    /// <param name="spriteName">スプライト名</param>
    /// <param name="texturePath">テクスチャファイルパス</param>
    /// <param name="instanceCount">インスタンス数</param>
    SpriteData(const std::string &spriteName, const std::string &texturePath, uint32_t instanceCount = 1)
        : name(spriteName), textureFilePath(texturePath), instanceData(instanceCount) {}
};

/// <summary>
/// スプライト管理のシングルトンクラス
/// 複数のスプライトの登録、更新、描画を一元管理
/// </summary>
class SpriteManager {
  private:
    /// ===================================================
    /// private method
    /// ===================================================

    /// <summary>
    /// プライベートコンストラクタ
    /// </summary>
    SpriteManager() = default;

    /// <summary>
    /// コピーコンストラクタ削除
    /// </summary>
    SpriteManager(const SpriteManager &) = delete;

    /// <summary>
    /// 代入演算子削除
    /// </summary>
    SpriteManager &operator=(const SpriteManager &) = delete;

    /// <summary>
    /// 描画順序を保存
    /// </summary>
    void SaveDrawOrder();

    /// <summary>
    /// 描画順序を読み込み
    /// </summary>
    void LoadDrawOrder();

    /// <summary>
    /// 名前でスプライトを検索
    /// </summary>
    /// <param name="name">検索するスプライト名</param>
    /// <returns>SpriteData*: スプライトデータのポインタ</returns>
    SpriteData *FindSpriteByName(const std::string &name);

    /// <summary>
    /// スプライトのインデックスを検索
    /// </summary>
    /// <param name="name">検索するスプライト名</param>
    /// <returns>int: スプライトのインデックス</returns>
    int FindSpriteIndex(const std::string &name);

    /// <summary>
    /// スプライトインスタンスを更新
    /// </summary>
    /// <param name="spriteData">更新するスプライトデータ</param>
    void UpdateSpriteInstances(SpriteData *spriteData);

  private:
    /// ===================================================
    /// private varians
    /// ===================================================

    static SpriteManager *instance;
    std::vector<std::unique_ptr<SpriteData>> sprites_;

    bool showSpriteCreationModal_ = false;
    std::string texturePath_ = "";
    std::string saveFolder_ = "Sprite";

  public:
    /// ===================================================
    /// public method
    /// ===================================================

    /// <summary>
    /// シングルトンインスタンスを取得
    /// </summary>
    /// <returns>SpriteManager*: インスタンスのポインタ</returns>
    static SpriteManager *GetInstance();

    /// <summary>
    /// スプライトを登録
    /// </summary>
    /// <param name="name">スプライト名</param>
    /// <param name="textureFilePath">テクスチャファイルパス</param>
    /// <param name="transform">スプライト情報</param>
    void RegisterSprite(const std::string &name, const std::string &textureFilePath, const SpriteTransform &transform = SpriteTransform());

    /// <summary>
    /// スプライトを削除
    /// </summary>
    /// <param name="name">削除するスプライト名</param>
    void UnregisterSprite(const std::string &name);

    /// <summary>
    /// すべてのスプライトを描画
    /// </summary>
    void DrawAll();

    /// <summary>
    /// すべてのスプライトを更新
    /// </summary>
    /// <param name="deltaTime">フレームの経過時間</param>
    void UpdateAll(float deltaTime);

    /// <summary>
    /// ImGui更新処理
    /// </summary>
    void UpdateImGui();

    /// <summary>
    /// スプライト作成モーダルを表示
    /// </summary>
    void ShowSpriteCreationModal() { showSpriteCreationModal_ = true; }

    /// <summary>
    /// スプライト作成モーダルを描画
    /// </summary>
    void DrawSpriteCreationModal();

    /// <summary>
    /// スプライトマネージャーUIを描画
    /// </summary>
    void DrawSpriteManager();

    /// <summary>
    /// 終了処理
    /// </summary>
    void Finalize();

    /// <summary>
    /// Getter
    /// </summary>
    SpriteData *GetSprite(const std::string &name);
    std::string GetTextureFilePath(const std::string &name);

    /// <summary>
    /// Setter
    /// </summary>
    void SetInstanceSRT(const std::string &name, uint32_t index, const InstanceSRT &srt);
    void SetInstanceScale(const std::string &name, uint32_t index, const Vector3 &scale);
    void SetInstanceRotation(const std::string &name, uint32_t index, const Vector3 &rotation);
    void SetInstanceTranslation(const std::string &name, uint32_t index, const Vector3 &translation);
    void SetInstanceActive(const std::string &name, uint32_t index, bool isActive);
    InstanceSRT *GetInstanceSRT(const std::string &name, uint32_t index);
    void SetSpriteVisible(const std::string &name, bool visible);
    void SetSpriteBackMost(const std::string &name, bool isBackMost);
    void SetSpritePosition(const std::string &name, const Vector2 &position);
    void SetSpriteSize(const std::string &name, const Vector2 &size);
    void SetSpriteColor(const std::string &name, const Vector4 &color);
    void SetTextureFilePath(const std::string &name, const std::string &textureFilePath);
    void SetUpdateFunction(const std::string &name, std::function<void(SpriteData &, float)> updateFunc);

    /// <summary>
    /// 保存フォルダを設定
    /// </summary>
    /// <param name="folderName">設定するフォルダ名</param>
    void SetSaveFolder(const std::string &folderName);

    /// <summary>
    /// すべてのスプライトを保存
    /// </summary>
    void SaveAllSprites();

    /// <summary>
    /// すべてのスプライトを読み込み
    /// </summary>
    void LoadAllSprites();

    /// <summary>
    /// すべてのスプライトをクリア
    /// </summary>
    void Clear();
};