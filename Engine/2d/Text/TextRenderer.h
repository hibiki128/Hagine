#pragma once
#include <string>
#include <type/Vector2.h>
#include <type/Vector4.h>
#include <vector>

/// <summary>
/// ロード済みフォントアトラスからテキストをRGBAテクスチャとして生成・保存し、
/// SpriteManagerにスプライトとして登録するシングルトンクラス。
/// 生成したPNGはresources/images/Text/配下に保存される。
/// </summary>
class TextRenderer {
  public:
    /// ===================================================
    /// public method
    /// ===================================================

    /// <summary>
    /// シングルトンインスタンスの取得
    /// </summary>
    static TextRenderer *GetInstance() {
        static TextRenderer instance;
        return &instance;
    }

    /// <summary>
    /// テキストをテクスチャとして生成し、SpriteManagerにスプライトとして登録する。
    /// 同名スプライトが既に存在する場合は削除してから再登録する。
    /// アウトライン設定を省略した場合はアウトラインなしで生成する。
    /// </summary>
    /// <param name="spriteName">SpriteManagerへの登録名（ファイル名にも使用される）</param>
    /// <param name="text">描画するテキスト</param>
    /// <param name="fontKey">TextureManager::MakeFontKey() で生成したフォントキー</param>
    /// <param name="position">スプライトの表示座標</param>
    /// <param name="color">文字色</param>
    /// <param name="outlineEnabled">アウトラインを有効にするか</param>
    /// <param name="outlineThickness">アウトラインの太さ（ピクセル単位）</param>
    /// <param name="outlineColor">アウトラインの色（RGBA）</param>
    void CreateTextSprite(
        const std::string &spriteName,
        const std::string &text,
        const std::string &fontKey,
        Vector2 position = {0.0f, 0.0f},
        Vector4 color = {1.0f, 1.0f, 1.0f, 1.0f},
        bool outlineEnabled = false,
        float outlineThickness = 2.0f,
        Vector4 outlineColor = {0.0f, 0.0f, 0.0f, 1.0f});

    /// <summary>
    /// テキストスプライト作成UIを描画する（ImGui）
    /// </summary>
    void UpdateImGui();

    /// ===================================================
    /// constants
    /// ===================================================
    static const std::string kSaveFolderRelative; // "Text"
    static const std::string kSaveFolder;         // "resources/images/Text"

  private:
    /// ===================================================
    /// private method
    /// ===================================================
    TextRenderer() = default;
    ~TextRenderer() = default;
    TextRenderer(const TextRenderer &) = delete;
    TextRenderer &operator=(const TextRenderer &) = delete;

    /// <summary>
    /// フォントアトラスから各グリフをサンプリングしてRGBAテクスチャを生成し、
    /// PNGファイルとして保存してTextureManagerにロードする。
    /// </summary>
    std::string RenderTextToFile(
        const std::string &spriteName,
        const std::string &text,
        const std::string &fontKey,
        bool outlineEnabled,
        float outlineThickness,
        Vector4 outlineColor);

    /// <summary>
    /// kSaveFolderが存在しない場合にディレクトリを再帰的に作成する
    /// </summary>
    void EnsureOutputDirectory();

  private:
    /// ===================================================
    /// private variables
    /// ===================================================

    // ImGui入力バッファ
    char imguiSpriteName_[128] = {};       // スプライト名
    char imguiText_[256] = {};             // テキスト
    int imguiFontIndex_ = 0;               // フォントインデックス
    float imguiPosition_[2] = {0.0f, 0.0f}; // 位置
    float imguiColor_[4] = {1.0f, 1.0f, 1.0f, 1.0f}; // 色

    // ImGuiアウトライン入力バッファ
    bool imguiOutlineEnabled_ = false;          // アウトライン有効フラグ
    float imguiOutlineThickness_ = 2.0f;        // アウトラインの太さ
    float imguiOutlineColor_[4] = {0.0f, 0.0f, 0.0f, 1.0f}; // アウトライン色
};