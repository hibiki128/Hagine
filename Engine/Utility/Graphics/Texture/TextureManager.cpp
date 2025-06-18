#include "TextureManager.h"
#include "DirectXCommon.h"
#include "StringUtility.h"

TextureManager *TextureManager::instance = nullptr;

// ImGuiで0番を使用するため、1番から使用
uint32_t TextureManager::kSRVIndexTop = 1;

void TextureManager::LoadTexture(const std::string &filePath) {
    // ファイル名を取り出して、resources/images/を付ける
    std::string newFilePath = "resources/images/" + filePath;

    // 読み込み済みテクスチャを検索
    if (textureDatas.contains(newFilePath)) {
        return;
    }

    // テクスチャ枚数上限をチェック
    assert(srvManager_->CanAllocate());

    // テクスチャファイルを読んでプログラムで扱えるようにする
    DirectX::ScratchImage image{};
    std::wstring filePathW = StringUtility::ConvertString(newFilePath);
    HRESULT hr;
    if (filePathW.ends_with(L".dds")) {
        isDDS_ = true;
        hr = DirectX::LoadFromDDSFile(filePathW.c_str(), DirectX::DDS_FLAGS_NONE, nullptr, image);
    } else {
        isDDS_ = false;
        hr = DirectX::LoadFromWICFile(filePathW.c_str(), DirectX::WIC_FLAGS_FORCE_SRGB, nullptr, image);
    }
    assert(SUCCEEDED(hr));

    DirectX::ScratchImage *imageToUse = &image; // 初期値はオリジナルのイメージ

    // ミニマップの作成
    DirectX::ScratchImage mipImages{};
    if (DirectX::IsCompressed(image.GetMetadata().format)) {
        mipImages = std::move(image);
    } else {
        hr = DirectX::GenerateMipMaps(image.GetImages(), image.GetImageCount(), image.GetMetadata(), DirectX::TEX_FILTER_SRGB, 4, mipImages);
    }

    if (SUCCEEDED(hr)) {
        imageToUse = &mipImages; // ミップマップが生成された場合はこれを使用
    }

    // テクスチャデータを追加して書き込む
    TextureData &textureData = textureDatas[newFilePath];

    textureData.metadata = imageToUse->GetMetadata();
    textureData.resource = dxCommon_->CreateTextureResource(textureData.metadata);
    textureData.intermediateResource = dxCommon_->UploadTextureData(textureData.resource, *imageToUse); // ミップマップも含めてアップロード

    textureData.srvIndex = srvManager_->Allocate() + kSRVIndexTop;
    textureData.srvHandleCPU = srvManager_->GetCPUDescriptorHandle(textureData.srvIndex);
    textureData.srvHandleGPU = srvManager_->GetGPUDescriptorHandle(textureData.srvIndex);

    srvManager_->CreateSRVforTexture2D(textureData.srvIndex, textureData.resource.Get(), textureData.metadata, UINT(textureData.metadata.mipLevels));
}

void TextureManager::Initialize(SrvManager *srvManager) {
    dxCommon_ = DirectXCommon::GetInstance();
    srvManager_ = srvManager;
    // SRVの数と同数
    textureDatas.reserve(SrvManager::kMaxSRVCount);
}

TextureManager *TextureManager::GetInstance() {
    if (instance == nullptr) {
        instance = new TextureManager;
    }
    return instance;
}

void TextureManager::Finalize() {
    // 全テクスチャのSRVインデックスを解放
    for (auto &pair : textureDatas) {
        srvManager_->Free(pair.second.srvIndex - kSRVIndexTop);
    }

    // インスタンスの削除
    delete instance;
    instance = nullptr;
}

uint32_t TextureManager::GetTextureIndexByFilePath(const std::string &filePath) {
    // ファイル名を取り出して、resources/images/を付ける
    std::string newFilePath = "resources/images/" + filePath;

    // unordered_mapを使って直接インデックスを取得
    auto it = textureDatas.find(newFilePath);
    if (it != textureDatas.end()) {
        return it->second.srvIndex;
    }

    // 見つからない場合はassertでエラーにする
    assert(0);
    return 0;
}

D3D12_GPU_DESCRIPTOR_HANDLE TextureManager::GetSrvHandleGPU(const std::string &filePath) {
    // 指定されたファイルパスが存在するかチェック
    assert(textureDatas.find(filePath) != textureDatas.end());

    TextureData &textureData = textureDatas[filePath];
    return textureData.srvHandleGPU;
}

const DirectX::TexMetadata &TextureManager::GetMetaData(const std::string &filePath) {
    std::string fullPath = ("resources/images/" + filePath);
    // 指定されたファイルパスが存在するかチェック
    assert(textureDatas.find(fullPath) != textureDatas.end());

    TextureData &textureData = textureDatas[fullPath];
    return textureData.metadata;
}
