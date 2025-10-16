#pragma once
#include "SpriteStruct.h"
#include "d3d12.h"
#include "string"
#include "wrl.h"
#include <Graphics/Srv/SrvManager.h>

class SpriteCommon;

/// <summary>
/// スプライト描画を管理するクラス
/// 2D画像の描画、トランスフォーム、UV変換などを制御
/// </summary>
class Sprite {
  public:
    /// ===================================================
    /// public method
    /// ===================================================

    /// <summary>
    /// 初期化
    /// </summary>
    /// <param name="textureFilePath">テクスチャファイルパス</param>
    /// <param name="position">座標</param>
    /// <param name="color">色</param>
    /// <param name="anchorpoint">アンカーポイント</param>
    /// <param name="isFlipX">左右反転フラグ</param>
    /// <param name="isFlipY">上下反転フラグ</param>
    void Initialize(const std::string &textureFilePath, Vector2 position, Vector4 color = {1, 1, 1, 1}, Vector2 anchorpoint = {0.0f, 0.0f}, bool isFlipX = false, bool isFlipY = false);

    /// <summary>
    /// 描画処理
    /// </summary>
    /// <param name="isBackMost">背面フラグ</param>
    void Draw(bool isBackMost = false);

    /// <summary>
    /// 座標を取得
    /// </summary>
    /// <returns>const Vector2&: 座標</returns>
    const Vector2 &GetPosition() const { return position_; }

    /// <summary>
    /// 回転を取得
    /// </summary>
    /// <returns>float: 回転角度</returns>
    float GetRotation() const { return rotation; }

    /// <summary>
    /// サイズを取得
    /// </summary>
    /// <returns>const Vector2&: サイズ</returns>
    const Vector2 &GetSize() const { return size; }

    /// <summary>
    /// 色を取得
    /// </summary>
    /// <returns>const Vector4&: 色</returns>
    const Vector4 &GetColor() const { return materialData->color; }

    /// <summary>
    /// アンカーポイントを取得
    /// </summary>
    /// <returns>const Vector2&: アンカーポイント</returns>
    const Vector2 &GetAnchorPoint() const { return anchorPoint_; }

    /// <summary>
    /// 左右反転状態を取得
    /// </summary>
    /// <returns>const bool: 左右反転フラグ</returns>
    const bool GetFlipX() const { return isFlipX_; }

    /// <summary>
    /// 上下反転状態を取得
    /// </summary>
    /// <returns>const bool: 上下反転フラグ</returns>
    const bool GetFilpY() const { return isFlipY_; }

    /// <summary>
    /// テクスチャ左上座標を取得
    /// </summary>
    /// <returns>const Vector2&: テクスチャ左上座標</returns>
    const Vector2 &GetTexLeftTop() const { return textureLeftTop; }

    /// <summary>
    /// テクスチャサイズを取得
    /// </summary>
    /// <returns>const Vector2&: テクスチャサイズ</returns>
    const Vector2 &GetTexSize() const { return textureSize; }

    /// <summary>
    /// インスタンス数を取得
    /// </summary>
    /// <returns>uint32_t&: インスタンス数</returns>
    uint32_t &GetInstanceCount() { return instanceCount; }

    /// <summary>
    /// UV変換マトリックスを取得
    /// </summary>
    /// <returns>Matrix4x4: UV変換マトリックス</returns>
    Matrix4x4 GetUVTransform() { return materialData->uvTransform; }

    /// <summary>
    /// UV座標を取得
    /// </summary>
    /// <returns>Vector2: UV座標</returns>
    Vector2 GetUVPosition() { return uvPosition_; }

    /// <summary>
    /// UVサイズを取得
    /// </summary>
    /// <returns>Vector2: UVサイズ</returns>
    Vector2 GetUVSize() { return uvSize_; }

    /// <summary>
    /// UV回転を取得
    /// </summary>
    /// <returns>float: UV回転角度</returns>
    float GetUVRotate() { return uvRotate_; }

    /// <summary>
    /// 座標を設定
    /// </summary>
    /// <param name="position">設定する座標</param>
    void SetPosition(const Vector2 &position) { this->position_ = position; }

    /// <summary>
    /// 回転を設定
    /// </summary>
    /// <param name="rotation">設定する回転角度</param>
    void SetRotation(float rotation) { this->rotation = rotation; }

    /// <summary>
    /// サイズを設定
    /// </summary>
    /// <param name="size">設定するサイズ</param>
    void SetSize(const Vector2 &size) { this->size = size; }

    /// <summary>
    /// 色を設定
    /// </summary>
    /// <param name="color">設定する色</param>
    void SetColor(const Vector3 &color) { materialData->color.x = color.x, materialData->color.y = color.y, materialData->color.z = color.z; }

    /// <summary>
    /// アルファ値を設定
    /// </summary>
    /// <param name="alpha">設定するアルファ値</param>
    void SetAlpha(const float &alpha) { materialData->color.w = alpha; }

    /// <summary>
    /// テクスチャパスを設定
    /// </summary>
    /// <param name="textureFilePath">設定するテクスチャファイルパス</param>
    void SetTexturePath(std::string textureFilePath);

    /// <summary>
    /// アンカーポイントを設定
    /// </summary>
    /// <param name="anchorPoint">設定するアンカーポイント</param>
    void SetAnchorPoint(const Vector2 &anchorPoint) { this->anchorPoint_ = anchorPoint; }

    /// <summary>
    /// 左右反転を設定
    /// </summary>
    /// <param name="isFlipX">左右反転フラグ</param>
    void SetFlipX(bool isFlipX) { isFlipX_ = isFlipX; }

    /// <summary>
    /// 上下反転を設定
    /// </summary>
    /// <param name="isFlipY">上下反転フラグ</param>
    void SetFlipY(bool isFlipY) { isFlipY_ = isFlipY; }

    /// <summary>
    /// テクスチャ左上座標を設定
    /// </summary>
    /// <param name="textureLeftTop">設定するテクスチャ左上座標</param>
    void SetTexLeftTop(const Vector2 &textureLeftTop) { this->textureLeftTop = textureLeftTop; }

    /// <summary>
    /// テクスチャサイズを設定
    /// </summary>
    /// <param name="textureSize">設定するテクスチャサイズ</param>
    void SetTexSize(const Vector2 &textureSize) { this->textureSize = textureSize; }

    /// <summary>
    /// UV変換マトリックスを設定
    /// </summary>
    /// <param name="uvTransform">設定するUV変換マトリックス</param>
    void SetUVTransform(const Matrix4x4 &uvTransform) {
        materialData->uvTransform = uvTransform;
        uvSize_.x = sqrt(uvTransform.m[0][0] * uvTransform.m[0][0] + uvTransform.m[1][0] * uvTransform.m[1][0]);
        uvSize_.y = sqrt(uvTransform.m[0][1] * uvTransform.m[0][1] + uvTransform.m[1][1] * uvTransform.m[1][1]);
        uvRotate_ = atan2(uvTransform.m[1][0], uvTransform.m[0][0]);
        uvPosition_.x = uvTransform.m[3][0];
        uvPosition_.y = uvTransform.m[3][1];
    }

    /// <summary>
    /// UV座標を設定
    /// </summary>
    /// <param name="position">設定するUV座標</param>
    void SetUVPosition(const Vector2 &position) { uvPosition_ = position; }

    /// <summary>
    /// UVサイズを設定
    /// </summary>
    /// <param name="size">設定するUVサイズ</param>
    void SetUVSize(const Vector2 &size) { uvSize_ = size; }

    /// <summary>
    /// UV回転を設定
    /// </summary>
    /// <param name="rotate">設定するUV回転角度</param>
    void SetUVRotate(const float &rotate) { uvRotate_ = rotate; }

    /// <summary>
    /// インスタンス数を設定
    /// </summary>
    /// <param name="count">設定するインスタンス数</param>
    void SetInstanceCount(uint32_t count);

    /// <summary>
    /// インスタンスの変換マトリックスを設定
    /// </summary>
    /// <param name="index">インスタンスインデックス</param>
    /// <param name="transform">設定する変換マトリックス</param>
    void SetInstanceTransform(uint32_t index, const TransformationMatrix &transform);

  private:
    /// ===================================================
    /// private method
    /// ===================================================

    /// <summary>
    /// 更新処理
    /// </summary>
    /// <param name="isbackmost_">背面フラグ</param>
    void Update(bool isbackmost_);

    /// <summary>
    /// 頂点データを作成
    /// </summary>
    void CreateVartexData();

    /// <summary>
    /// マテリアルデータを作成
    /// </summary>
    void CreateMaterial();

    /// <summary>
    /// 座標変換行列データを作成
    /// </summary>
    void CreateTransformationMatrix();

    /// <summary>
    /// テクスチャサイズを画像に合わせる
    /// </summary>
    void AdjustTextureSize();

  private:
    /// ===================================================
    /// private varians
    /// ===================================================

    SpriteCommon *spriteCommon_ = nullptr;
    SrvManager *srvManager_ = nullptr;

    Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource = nullptr;
    Microsoft::WRL::ComPtr<ID3D12Resource> indexResource = nullptr;
    SpriteVertexData *vertexData = nullptr;
    uint32_t *indexData = nullptr;
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView{};
    D3D12_INDEX_BUFFER_VIEW indexBufferView{};

    Microsoft::WRL::ComPtr<ID3D12Resource> materialResource = nullptr;
    SpriteMaterial *materialData = nullptr;

    Microsoft::WRL::ComPtr<ID3D12Resource> transformationMatrixResource;
    TransformationMatrix *transformationMatrixData = nullptr;

    Vector2 position_ = {0.0f, 0.0f}; // 座標
    float rotation = 0.0f;            // 回転角度
    Vector2 size = {640.0f, 360.0f};  // サイズ

    std::string fullpath;
    Vector2 anchorPoint_ = {0.0f, 0.0f}; // アンカーポイント

    bool isFlipX_ = false;    // 左右反転フラグ
    bool isFlipY_ = false;    // 上下反転フラグ
    bool isbackmost_ = false; // 背面フラグ

    Vector2 textureLeftTop = {0.0f, 0.0f};  // テクスチャ左上座標
    Vector2 textureSize = {512.0f, 512.0f}; // テクスチャサイズ

    uint32_t instanceCount = 1; // インスタンス数
    uint32_t transformationMatrixSrvIndex = 0;

    float uvRotate_ = 0.0f;             // UV回転角度
    Vector2 uvSize_ = {1.0f, 1.0f};     // UVサイズ
    Vector2 uvPosition_ = {0.0f, 0.0f}; // UV座標
};