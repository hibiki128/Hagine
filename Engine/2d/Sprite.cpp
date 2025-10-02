#define NOMINMAX
#include "Sprite.h"
#include "SpriteCommon.h"
#include <Graphics/Texture/TextureManager.h>
#include <myMath.h>

void Sprite::Initialize(const std::string &textureFilePath, Vector2 position, Vector4 color, Vector2 anchorpoint, bool isFlipX, bool isFlipY) {
    // 引数で受け取ってメンバ変数に記録する
    spriteCommon_ = SpriteCommon::GetInstance();
    srvManager_ = TextureManager::GetInstance()->GetSrvManager();

    fullpath = textureFilePath;

    TextureManager::GetInstance()->LoadTexture(fullpath);

    CreateVartexData();

    CreateMaterial();

    CreateTransformationMatrix();
    SetInstanceCount(1);

    if (instanceCount <= 1) {
        Transform transform{
            {size.x, size.y, 1.0f},          // scale
            {0.0f, 0.0f, rotation},          // rotation
            {position_.x, position_.y, 0.0f} // translation
        };

        Matrix4x4 world = MakeAffineMatrix(transform.scale, transform.rotate, transform.translate);
        Matrix4x4 view = MakeIdentity4x4();
        Matrix4x4 proj = MakeOrthographicMatrix(0.0f, 0.0f, float(WinApp::kClientWidth), float(WinApp::kClientHeight), 0.0f, 100.0f);
        Matrix4x4 wvp = world * (view * proj);

        transformationMatrixData[0].World = world;
        transformationMatrixData[0].WVP = wvp;
    }

    position_ = position;
    materialData->color = color;
    anchorPoint_ = anchorpoint;
    isFlipX_ = isFlipX;
    isFlipY_ = isFlipY;

    AdjustTextureSize();
}

void Sprite::SetInstanceCount(uint32_t count) {
    // バッファサイズを超えないようにチェック
    const uint32_t maxInstances = 1000; // CreateTransformationMatrixと同じ値
    instanceCount = std::min(count, maxInstances);
}

void Sprite::SetInstanceTransform(uint32_t index, const TransformationMatrix &transform) {
    const uint32_t maxInstances = 1000; // 最大インスタンス数
    if (index < instanceCount && index < maxInstances && transformationMatrixData != nullptr) {
        transformationMatrixData[index] = transform;
    }
}

void Sprite::Update(bool isBackMost) {
    // 頂点座標の計算
    float left = 0.0f - anchorPoint_.x;
    float right = 1.0f - anchorPoint_.x;
    float top = 0.0f - anchorPoint_.y;
    float bottom = 1.0f - anchorPoint_.y;

    if (isFlipX_) {
        std::swap(left, right);
    }
    if (isFlipY_) {
        std::swap(top, bottom);
    }

    const DirectX::TexMetadata &metadata = TextureManager::GetInstance()->GetMetaData(fullpath);
    float tex_left = textureLeftTop.x / metadata.width;
    float tex_right = (textureLeftTop.x + textureSize.x) / metadata.width;
    float tex_top = textureLeftTop.y / metadata.height;
    float tex_bottom = (textureLeftTop.y + textureSize.y) / metadata.height;

    // 頂点データの設定
    vertexResource->Map(0, nullptr, reinterpret_cast<void **>(&vertexData));
    vertexData[0] = {{left, bottom, 0.0f, 1.0f}, {tex_left, tex_bottom}};
    vertexData[1] = {{left, top, 0.0f, 1.0f}, {tex_left, tex_top}};
    vertexData[2] = {{right, bottom, 0.0f, 1.0f}, {tex_right, tex_bottom}};
    vertexData[3] = {{right, top, 0.0f, 1.0f}, {tex_right, tex_top}};

    // インデックスの設定
    indexResource->Map(0, nullptr, reinterpret_cast<void **>(&indexData));
    indexData[0] = 0;
    indexData[1] = 1;
    indexData[2] = 2;
    indexData[3] = 1;
    indexData[4] = 3;
    indexData[5] = 2;

    // 単体描画のときだけTransform更新
    if (instanceCount <= 1) {
        Transform transform;
        transform.scale = {size.x, size.y, 1.0f};
        transform.rotate = {0.0f, 0.0f, rotation};
        transform.translate = {
            position_.x,
            position_.y,
            isBackMost ? 10000.0f : 0.0f};

        Matrix4x4 world = MakeAffineMatrix(transform.scale, transform.rotate, transform.translate);
        Matrix4x4 view = MakeIdentity4x4();
        Matrix4x4 proj = MakeOrthographicMatrix(0.0f, 0.0f, float(WinApp::kClientWidth), float(WinApp::kClientHeight), 0.0f, 100.0f);
        Matrix4x4 wvp = world * (view * proj);

        transformationMatrixData[0].WVP = wvp;
        transformationMatrixData[0].World = world;
    }

    Vector3 pos = {uvPosition_.x, uvPosition_.y, 0.0f};
    Vector3 scale = {uvSize_.x, uvSize_.y, 1.0f};
    Vector3 rotate = {0.0f, 0.0f, uvRotate_};
    materialData->uvTransform = MakeAffineMatrix(scale, rotate, pos);
}

void Sprite::Draw(bool isBackMost) {
    Update(isBackMost);
    SpriteCommon::GetInstance()->DrawCommonSetting();

    spriteCommon_->GetDxCommon()->GetCommandList()->IASetVertexBuffers(0, 1, &vertexBufferView);
    spriteCommon_->GetDxCommon()->GetCommandList()->IASetIndexBuffer(&indexBufferView);
    spriteCommon_->GetDxCommon()->GetCommandList()->SetGraphicsRootConstantBufferView(0, materialResource->GetGPUVirtualAddress());

    srvManager_->SetGraphicsRootDescriptorTable(1, transformationMatrixSrvIndex);
    srvManager_->SetGraphicsRootDescriptorTable(2, TextureManager::GetInstance()->GetTextureIndexByFilePath(fullpath));

    spriteCommon_->GetDxCommon()->GetCommandList()->DrawIndexedInstanced(6, instanceCount, 0, 0, 0);
}

void Sprite::SetTexturePath(std::string textureFilePath) {
    fullpath = textureFilePath;
    TextureManager::GetInstance()->LoadTexture(textureFilePath);
    TextureManager::GetInstance()->GetTextureIndexByFilePath(fullpath);
}

void Sprite::CreateVartexData() {
    // Sprite用の頂点リソースを作る
    vertexResource = spriteCommon_->GetDxCommon()->CreateBufferResource(sizeof(SpriteVertexData) * 6);
    // リソースの先頭のアドレスから使う
    vertexBufferView.BufferLocation = vertexResource->GetGPUVirtualAddress();
    // 使用するリソースのサイズは頂点6つ分のサイズ
    vertexBufferView.SizeInBytes = sizeof(SpriteVertexData) * 6;
    // 1頂点あたりのサイズ
    vertexBufferView.StrideInBytes = sizeof(SpriteVertexData);

    // 頂点データの設定
    vertexResource->Map(0, nullptr, reinterpret_cast<void **>(&vertexData));

    // Index用のリソースを作る（sprite用）
    indexResource = spriteCommon_->GetDxCommon()->CreateBufferResource(sizeof(uint32_t) * 6);
    // リソースの先頭アドレスから使う
    indexBufferView.BufferLocation = indexResource->GetGPUVirtualAddress();
    // 使用するリソースのサイズはインデックス6つ分のサイズ
    indexBufferView.SizeInBytes = sizeof(uint32_t) * 6;
    // インデックスはuint32_tとする
    indexBufferView.Format = DXGI_FORMAT_R32_UINT;

    // インデックスリソースにデータを書き込む（sprite用）
    indexResource->Map(0, nullptr, reinterpret_cast<void **>(&indexData));
}

void Sprite::CreateMaterial() {
    // Sprite用のマテリアルリソースをつくる
    materialResource = spriteCommon_->GetDxCommon()->CreateBufferResource(sizeof(SpriteMaterial));
    // 書き込むためのアドレスを取得
    materialResource->Map(0, nullptr, reinterpret_cast<void **>(&materialData));
    // 色の設定
    materialData->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
    // Lightingの設定
    materialData->uvTransform = MakeIdentity4x4();
}

void Sprite::CreateTransformationMatrix() {
    // 最大インスタンス数を想定してバッファを作成
    uint32_t maxInstances = 1000; // または必要な最大数
    transformationMatrixResource = spriteCommon_->GetDxCommon()->CreateBufferResource(sizeof(TransformationMatrix) * maxInstances);
    transformationMatrixResource->Map(0, nullptr, reinterpret_cast<void **>(&transformationMatrixData));

    // 初期化
    for (uint32_t i = 0; i < maxInstances; ++i) {
        transformationMatrixData[i].WVP = MakeIdentity4x4();
        transformationMatrixData[i].World = MakeIdentity4x4();
    }

    srvManager_ = TextureManager::GetInstance()->GetSrvManager();
    transformationMatrixSrvIndex = srvManager_->Allocate() + 1;
    srvManager_->CreateSRVforStructuredBuffer(transformationMatrixSrvIndex, transformationMatrixResource.Get(), maxInstances, sizeof(TransformationMatrix));
}

void Sprite::AdjustTextureSize() {
    // テクスチャメタデータを取得
    const DirectX::TexMetadata &metadata = TextureManager::GetInstance()->GetMetaData(fullpath);

    textureSize.x = static_cast<float>(metadata.width);
    textureSize.y = static_cast<float>(metadata.height);
    // 画像サイズをテクスチャサイズに合わせる
    size = textureSize;
}
