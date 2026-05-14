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
  private:
    TextRenderer() = default;
    ~TextRenderer() = default;
    TextRenderer(const TextRenderer &) = delete;
    TextRenderer &operator=(const TextRenderer &) = delete;

  public:
    /// <summary>
    /// シングルトンインスタンスの取得
    /// </summary>
    static TextRenderer *GetInstance() {
        static TextRenderer instance;
        return &instance;
    }

    // テキストテクスチャの保存先（resources/images/ 以下の相対パスと実パス）
    static const std::string kSaveFolderRelative; // "Text"
    static const std::string kSaveFolder;         // "resources/images/Text"

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

  private:
    /// <summary>
    /// フォントアトラスから各グリフをサンプリングしてRGBAテクスチャを生成し、
    /// PNGファイルとして保存してTextureManagerにロードする。
    /// アウトライン設定が有効な場合はグリフの外周にアウトラインを描画する。
    /// 戻り値はTextureManager::LoadTexture用の相対パス（"Text/xxx.png"）。
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

    // ImGui入力バッファ（スプライト名・テキスト・フォント・座標・色）
    char imguiSpriteName_[128] = {};
    char imguiText_[256] = {};
    int imguiFontIndex_ = 0;
    float imguiPosition_[2] = {0.0f, 0.0f};
    float imguiColor_[4] = {1.0f, 1.0f, 1.0f, 1.0f};

    // ImGui入力バッファ（アウトライン設定）
    bool imguiOutlineEnabled_ = false;
    float imguiOutlineThickness_ = 2.0f;
    float imguiOutlineColor_[4] = {0.0f, 0.0f, 0.0f, 1.0f};
};