#define NOMINMAX
#include "TextRenderer.h"
#include "../SpriteManager.h"
#include <Graphics/Texture/TextureManager.h>
#include <String/StringUtility.h>
#include <externals/DirectXTex/DirectXTex.h>
#include <externals/imgui/imstb_truetype.h>
#ifdef _DEBUG
#include <imgui.h>
#endif // _DEBUG
#include <String/StringUtility.h>
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <vector>
#include <wincodec.h>

const std::string TextRenderer::kSaveFolderRelative = "Text";
const std::string TextRenderer::kSaveFolder = "resources/images/Text";

void TextRenderer::CreateTextSprite(
    const std::string &spriteName,
    const std::string &text,
    const std::string &fontKey,
    Vector2 position,
    Vector4 color,
    bool outlineEnabled,
    float outlineThickness,
    Vector4 outlineColor) {
    // 同名スプライトが存在する場合は再生成のために先に削除する
    if (SpriteManager::GetInstance()->GetSprite(spriteName) != nullptr) {
        SpriteManager::GetInstance()->UnregisterSprite(spriteName);
    }

    // テキストをPNGとして保存し、TextureManagerへロードして相対パスを受け取る
    const std::string relPath = RenderTextToFile(spriteName, text, fontKey, outlineEnabled, outlineThickness, outlineColor);

    // SpriteManagerにスプライトとして登録する
    SpriteTransform transform;
    transform.position = position;
    transform.color = color;
    SpriteManager::GetInstance()->RegisterSprite(spriteName, relPath, transform);
}

void TextRenderer::UpdateImGui() {
#ifdef _DEBUG
    if (!ImGui::Begin("テキストレンダラー (TextRenderer)")) {
        ImGui::End();
        return;
    }

    ImGui::SeparatorText("テキストスプライト作成");

    ImGui::InputText("スプライト名", imguiSpriteName_, sizeof(imguiSpriteName_));
    ImGui::InputText("テキスト", imguiText_, sizeof(imguiText_));

    std::vector<std::string> fontKeys = TextureManager::GetInstance()->GetAllFontKeys();

    if (!fontKeys.empty()) {
        imguiFontIndex_ = std::clamp(imguiFontIndex_, 0, static_cast<int>(fontKeys.size()) - 1);
        const char *current = fontKeys[imguiFontIndex_].c_str();
        if (ImGui::BeginCombo("フォント", current)) {
            for (int i = 0; i < static_cast<int>(fontKeys.size()); ++i) {
                const bool selected = (i == imguiFontIndex_);
                if (ImGui::Selectable(fontKeys[i].c_str(), selected)) {
                    imguiFontIndex_ = i;
                }
                if (selected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
    } else {
        ImGui::TextDisabled("フォントがありません。TextureManager::LoadFontTexture()を呼んでください。");
    }

    ImGui::DragFloat2("座標", imguiPosition_, 1.0f);
    ImGui::ColorEdit4("文字色", imguiColor_);

    // ---- アウトライン設定 ----
    ImGui::SeparatorText("アウトライン設定");

    ImGui::Checkbox("アウトラインを有効にする", &imguiOutlineEnabled_);

    if (imguiOutlineEnabled_) {
        // 有効時のみ太さと色を編集可能にする
        ImGui::DragFloat("太さ (px)", &imguiOutlineThickness_, 0.5f, 0.5f, 20.0f, "%.1f");
        ImGui::ColorEdit4("アウトライン色", imguiOutlineColor_);
    } else {
        // 無効時はグレーアウト表示
        ImGui::BeginDisabled();
        ImGui::DragFloat("太さ (px)", &imguiOutlineThickness_, 0.5f, 0.5f, 20.0f, "%.1f");
        ImGui::ColorEdit4("アウトライン色", imguiOutlineColor_);
        ImGui::EndDisabled();
    }

    const bool canCreate = (imguiSpriteName_[0] != '\0') &&
                           (imguiText_[0] != '\0') &&
                           !fontKeys.empty();

    if (!canCreate) {
        ImGui::BeginDisabled();
    }
    if (ImGui::Button("テキストスプライトを作成")) {
        CreateTextSprite(
            std::string(imguiSpriteName_),
            std::string(imguiText_),
            fontKeys[imguiFontIndex_],
            {imguiPosition_[0], imguiPosition_[1]},
            {imguiColor_[0], imguiColor_[1], imguiColor_[2], imguiColor_[3]},
            imguiOutlineEnabled_,
            imguiOutlineThickness_,
            {imguiOutlineColor_[0], imguiOutlineColor_[1], imguiOutlineColor_[2], imguiOutlineColor_[3]});
    }
    if (!canCreate) {
        ImGui::EndDisabled();
    }

    ImGui::End();
#endif // _DEBUG
}

std::string TextRenderer::RenderTextToFile(
    const std::string &spriteName,
    const std::string &text,
    const std::string &fontKey,
    bool outlineEnabled,
    float outlineThickness,
    Vector4 outlineColor) {
    // フォントデータとバッファの存在を確認
    const TextureManager::FontData *fontData = TextureManager::GetInstance()->GetFontData(fontKey);
    assert(fontData != nullptr);
    assert(fontData->ttfBuffer != nullptr);

    // stb_truetypeの初期化
    stbtt_fontinfo fontInfo;
    if (!stbtt_InitFont(&fontInfo, fontData->ttfBuffer->data(), 0)) {
        assert(0 && "Failed to init font");
    }

    float scale = stbtt_ScaleForPixelHeight(&fontInfo, fontData->fontSize);
    int ascent, descent, lineGap;
    stbtt_GetFontVMetrics(&fontInfo, &ascent, &descent, &lineGap);
    int maxAscent = static_cast<int>(std::round(ascent * scale));
    int maxDescent = static_cast<int>(std::round(-descent * scale));

    // UTF-8からUTF-16へ変換し、コードポイント配列に展開
    std::wstring wText = StringUtility::ConvertString(text);
    std::vector<uint32_t> codepoints;
    for (size_t i = 0; i < wText.length(); ++i) {
        uint32_t cp = static_cast<uint32_t>(wText[i]);
        if (cp >= 0xD800 && cp <= 0xDBFF && i + 1 < wText.length()) {
            uint32_t low = static_cast<uint32_t>(wText[i + 1]);
            if (low >= 0xDC00 && low <= 0xDFFF) {
                cp = 0x10000 + ((cp - 0xD800) << 10) + (low - 0xDC00);
                ++i;
            }
        }
        codepoints.push_back(cp);
    }

    // 全体の横幅を計算
    float totalWidth = 0.0f;
    for (size_t i = 0; i < codepoints.size(); ++i) {
        int advanceWidth, leftSideBearing;
        stbtt_GetCodepointHMetrics(&fontInfo, codepoints[i], &advanceWidth, &leftSideBearing);
        totalWidth += advanceWidth * scale;

        if (i + 1 < codepoints.size()) {
            totalWidth += stbtt_GetCodepointKernAdvance(&fontInfo, codepoints[i], codepoints[i + 1]) * scale;
        }
    }

    // パディングを考慮してテクスチャサイズを決定
    const int outlinePad = outlineEnabled ? static_cast<int>(std::ceil(outlineThickness)) : 0;
    const int texWidth = std::max(1, static_cast<int>(std::ceil(totalWidth)) + outlinePad * 2);
    const int texHeight = std::max(1, maxAscent + maxDescent + 2 + outlinePad * 2);

    std::vector<uint8_t> pixels(static_cast<size_t>(texWidth * texHeight * 4), 0);

    // グリフをビットマップ化し、ピクセルバッファに書き込む
    float cursorX = static_cast<float>(outlinePad);
    for (size_t i = 0; i < codepoints.size(); ++i) {
        uint32_t cp = codepoints[i];

        int advanceWidth, leftSideBearing;
        stbtt_GetCodepointHMetrics(&fontInfo, cp, &advanceWidth, &leftSideBearing);

        int x0, y0, x1, y1;
        stbtt_GetCodepointBitmapBox(&fontInfo, cp, scale, scale, &x0, &y0, &x1, &y1);

        int glyphW = x1 - x0;
        int glyphH = y1 - y0;

        int dstX = static_cast<int>(std::round(cursorX)) + x0;
        int dstY = maxAscent + y0 + outlinePad;

        if (glyphW > 0 && glyphH > 0) {
            std::vector<uint8_t> glyphBitmap(glyphW * glyphH);
            stbtt_MakeCodepointBitmap(&fontInfo, glyphBitmap.data(), glyphW, glyphH, glyphW, scale, scale, cp);

            for (int gy = 0; gy < glyphH; ++gy) {
                for (int gx = 0; gx < glyphW; ++gx) {
                    int px = dstX + gx;
                    int py = dstY + gy;
                    if (px < 0 || px >= texWidth || py < 0 || py >= texHeight)
                        continue;

                    uint8_t alpha = glyphBitmap[gy * glyphW + gx];
                    if (alpha > 0) {
                        int dstIdx = (py * texWidth + px) * 4;
                        pixels[dstIdx + 0] = 255;
                        pixels[dstIdx + 1] = 255;
                        pixels[dstIdx + 2] = 255;
                        pixels[dstIdx + 3] = std::max(pixels[dstIdx + 3], alpha);
                    }
                }
            }
        }

        cursorX += advanceWidth * scale;
        if (i + 1 < codepoints.size()) {
            cursorX += stbtt_GetCodepointKernAdvance(&fontInfo, cp, codepoints[i + 1]) * scale;
        }
    }

    // アウトラインが必要な場合は膨張処理（ダイレーション）を行い合成する
    if (outlineEnabled && outlineThickness > 0.0f) {
        const int radius = static_cast<int>(std::ceil(outlineThickness));
        std::vector<uint8_t> outlineMask(static_cast<size_t>(texWidth * texHeight), 0);

        for (int y = 0; y < texHeight; ++y) {
            for (int x = 0; x < texWidth; ++x) {
                const int selfIdx = (y * texWidth + x) * 4;
                if (pixels[selfIdx + 3] > 0)
                    continue;

                bool hasNeighbor = false;
                for (int dy = -radius; dy <= radius && !hasNeighbor; ++dy) {
                    for (int dx = -radius; dx <= radius && !hasNeighbor; ++dx) {
                        const float dist = std::sqrt(static_cast<float>(dx * dx + dy * dy));
                        if (dist > outlineThickness)
                            continue;

                        const int nx = x + dx;
                        const int ny = y + dy;
                        if (nx < 0 || nx >= texWidth || ny < 0 || ny >= texHeight)
                            continue;

                        const int nIdx = (ny * texWidth + nx) * 4;
                        if (pixels[nIdx + 3] > 0)
                            hasNeighbor = true;
                    }
                }

                if (hasNeighbor) {
                    outlineMask[y * texWidth + x] = 255;
                }
            }
        }

        const uint8_t outR = static_cast<uint8_t>(std::clamp(outlineColor.x, 0.0f, 1.0f) * 255.0f);
        const uint8_t outG = static_cast<uint8_t>(std::clamp(outlineColor.y, 0.0f, 1.0f) * 255.0f);
        const uint8_t outB = static_cast<uint8_t>(std::clamp(outlineColor.z, 0.0f, 1.0f) * 255.0f);
        const uint8_t outA = static_cast<uint8_t>(std::clamp(outlineColor.w, 0.0f, 1.0f) * 255.0f);

        for (int y = 0; y < texHeight; ++y) {
            for (int x = 0; x < texWidth; ++x) {
                if (outlineMask[y * texWidth + x] == 0)
                    continue;

                const int idx = (y * texWidth + x) * 4;
                if (pixels[idx + 3] > 0)
                    continue;

                pixels[idx + 0] = outR;
                pixels[idx + 1] = outG;
                pixels[idx + 2] = outB;
                pixels[idx + 3] = outA;
            }
        }
    }

    // DirectXTexを使用してPNGファイルを生成し保存する
    EnsureOutputDirectory();

    DirectX::ScratchImage scratchImage;
    HRESULT hr = scratchImage.Initialize2D(
        DXGI_FORMAT_R8G8B8A8_UNORM,
        static_cast<size_t>(texWidth),
        static_cast<size_t>(texHeight),
        1, 1);
    assert(SUCCEEDED(hr));

    const DirectX::Image *img = scratchImage.GetImages();
    for (int y = 0; y < texHeight; ++y) {
        std::memcpy(
            img->pixels + y * img->rowPitch,
            pixels.data() + y * texWidth * 4,
            static_cast<size_t>(texWidth * 4));
    }

    const std::string fileName = spriteName + ".png";
    const std::string filePath = kSaveFolder + "/" + fileName;
    const std::string loadPath = kSaveFolderRelative + "/" + fileName;

    const std::wstring wFilePath = StringUtility::ConvertString(filePath);
    hr = DirectX::SaveToWICFile(*img, DirectX::WIC_FLAGS_NONE,
                                GUID_ContainerFormatPng, wFilePath.c_str());
    assert(SUCCEEDED(hr));

    TextureManager::GetInstance()->LoadTexture(loadPath);

    return loadPath;
}

void TextRenderer::EnsureOutputDirectory() {
    // 保存先のディレクトリが存在しない場合に作成する
    const std::filesystem::path dir(kSaveFolder);
    if (!std::filesystem::exists(dir)) {
        std::filesystem::create_directories(dir);
    }
}
