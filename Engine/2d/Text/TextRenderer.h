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
    /// </summary>
    /// <param name="spriteName">SpriteManagerへの登録名（ファイル名にも使用される）</param>
    /// <param name="text">描画するテキスト（ASCII 32〜127）</param>
    /// <param name="fontKey">TextureManager::MakeFontKey() で生成したフォントキー</param>
    /// <param name="position">スプライトの表示座標</param>
    /// <param name="color">文字色（スプライトのマテリアルカラー）</param>
    void CreateTextSprite(
        const std::string &spriteName,
        const std::string &text,
        const std::string &fontKey,
        Vector2 position = {0.0f, 0.0f},
        Vector4 color = {1.0f, 1.0f, 1.0f, 1.0f});

    /// <summary>
    /// テキストスプライト作成UIを描画する（ImGui）
    /// </summary>
    void UpdateImGui();

  private:
    /// <summary>
    /// フォントアトラスから各グリフをサンプリングしてRGBAテクスチャを生成し、
    /// PNGファイルとして保存してTextureManagerにロードする。
    /// 戻り値はTextureManager::LoadTexture用の相対パス（"Text/xxx.png"）。
    /// </summary>
    std::string RenderTextToFile(
        const std::string &spriteName,
        const std::string &text,
        const std::string &fontKey);

    /// <summary>
    /// kSaveFolderが存在しない場合にディレクトリを再帰的に作成する
    /// </summary>
    void EnsureOutputDirectory();

    // ImGui入力バッファ
    char imguiSpriteName_[128] = {};
    char imguiText_[256] = {};
    int imguiFontIndex_ = 0;
    float imguiPosition_[2] = {0.0f, 0.0f};
    float imguiColor_[4] = {1.0f, 1.0f, 1.0f, 1.0f};
};