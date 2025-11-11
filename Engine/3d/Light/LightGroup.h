#pragma once
#include "Data/DataHandler.h"
#include "d3d12.h"
#include "externals/nlohmann/json.hpp"
#include "wrl.h"
#include <Camera/ViewProjection/ViewProjection.h>
#include <string>
#include <type/Vector3.h>
#include <type/Vector4.h>

#define MAX_POINT_LIGHTS 5
#define MAX_SPOT_LIGHTS 5

/// <summary>
/// ライトタイプ列挙型
/// </summary>
enum class LightType {
    Directional, // 平行光源
    Point,       // ポイントライト
    Spot         // スポットライト
};

class DirectXCommon;

/// <summary>
/// ライトグループクラス
/// 平行光源、ポイントライト、スポットライトを統合管理する
/// </summary>
class LightGroup {
  public:
    /// ===================================================
    /// public method
    /// ===================================================

    /// <summary>
    /// シングルトンインスタンスの取得
    /// </summary>
    /// <returns>LightGroup*: インスタンスのポインタ</returns>
    static LightGroup *GetInstance();

    /// <summary>
    /// 終了処理
    /// </summary>
    void Finalize();

    /// <summary>
    /// 初期化
    /// </summary>
    void Initialize();

    /// <summary>
    /// 更新処理
    /// </summary>
    /// <param name="viewProjection">ビュープロジェクション</param>
    void Update(const ViewProjection &viewProjection);

    /// <summary>
    /// 描画処理
    /// </summary>
    void Draw();

    /// <summary>
    /// ImGui表示
    /// </summary>
    void imgui();

    /// <summary>
    /// ライトデータを保存
    /// </summary>
    /// <param name="fileName">保存先ファイル名</param>
    void SaveLightData(const std::string &fileName);

    /// <summary>
    /// ライトデータを読み込み
    /// </summary>
    /// <param name="fileName">読み込み元ファイル名</param>
    void LoadLightData(const std::string &fileName);

    /// <summary>
    /// Setter
    /// </summary>
    void SetShowLightVisualization(bool show) { showLightVisualization_ = show; }

  private:
    /// ===================================================
    /// private method
    /// ===================================================

    LightGroup() = default;
    ~LightGroup() = default;
    LightGroup(LightGroup &) = delete;
    LightGroup &operator=(LightGroup &) = delete;

    /// <summary>
    /// 平行光源データ作成
    /// </summary>
    void CreateDirectionLight();

    /// <summary>
    /// ポイントライト配列データ作成
    /// </summary>
    void CreatePointLights();

    /// <summary>
    /// スポットライト配列データ作成
    /// </summary>
    void CreateSpotLights();

    /// <summary>
    /// カメラデータ作成
    /// </summary>
    void CreateCamera();

    /// <summary>
    /// ポイントライトを追加
    /// </summary>
    void AddPointLight();

    /// <summary>
    /// ポイントライトを削除
    /// </summary>
    /// <param name="index">削除するインデックス</param>
    void RemovePointLight(int index);

    /// <summary>
    /// スポットライトを追加
    /// </summary>
    void AddSpotLight();

    /// <summary>
    /// スポットライトを削除
    /// </summary>
    /// <param name="index">削除するインデックス</param>
    void RemoveSpotLight(int index);

    /// <summary>
    /// ポイントライトバッファを更新
    /// </summary>
    void UpdatePointLightBuffer();

    /// <summary>
    /// スポットライトバッファを更新
    /// </summary>
    void UpdateSpotLightBuffer();

    /// <summary>
    /// 光源の可視化描画
    /// </summary>
    void DrawLightVisualization();

  private:
    /// ===================================================
    /// private struct
    /// ===================================================

    /// <summary>
    /// 平行光源データ
    /// </summary>
    struct DirectionLight {
        Vector4 color;       // ライトの色
        Vector3 direction;   // ライトの向き
        float intensity;     // 輝度
        int32_t active;      // 有効フラグ
        int32_t HalfLambert; // ハーフランバート使用フラグ
        int32_t BlinnPhong;  // Blinn-Phong使用フラグ
    };

    /// <summary>
    /// ポイントライトデータ
    /// </summary>
    struct PointLight {
        Vector4 color;       // ライトの色
        Vector3 position;    // ライトの位置
        float intensity;     // 輝度
        int32_t active;      // 有効フラグ
        float radius;        // 影響半径
        float decay;         // 減衰率
        int32_t HalfLambert; // ハーフランバート使用フラグ
        int32_t BlinnPhong;  // Blinn-Phong使用フラグ
        float padding[3];
    };

    /// <summary>
    /// ポイントライト配列
    /// </summary>
    struct PointLights {
        alignas(16) PointLight lights[MAX_POINT_LIGHTS]; // ポイントライト配列
        int32_t count;                                   // 有効なライト数
        float padding[3];
    };

    /// <summary>
    /// スポットライトデータ
    /// </summary>
    struct SpotLight {
        Vector4 color;       // ライトの色
        Vector3 position;    // ライトの位置
        float intensity;     // 輝度
        Vector3 direction;   // ライトの向き
        float distance;      // 照射距離
        float decay;         // 減衰率
        float cosAngle;      // コーン角度のコサイン値
        int32_t active;      // 有効フラグ
        int32_t HalfLambert; // ハーフランバート使用フラグ
        int32_t BlinnPhong;  // Blinn-Phong使用フラグ
        float padding[3];
    };

    /// <summary>
    /// スポットライト配列
    /// </summary>
    struct SpotLights {
        SpotLight lights[MAX_SPOT_LIGHTS]; // スポットライト配列
        int32_t count;                     // 有効なライト数
        float padding[3];
    };

    /// <summary>
    /// GPU用カメラデータ
    /// </summary>
    struct CameraForGPU {
        Vector3 worldPosition; // カメラのワールド座標
    };

  private:
    /// ===================================================
    /// private varians
    /// ===================================================

    static LightGroup *instance; // シングルトンインスタンス

    DirectXCommon *dxCommon_; // DirectX共通クラス

    // 平行光源リソース
    Microsoft::WRL::ComPtr<ID3D12Resource> directionalLightResource; // バッファリソース
    DirectionLight *directionalLightData = nullptr;                  // データポインタ

    // ポイントライトリソース
    Microsoft::WRL::ComPtr<ID3D12Resource> pointLightsResource; // バッファリソース
    PointLights *pointLightsData = nullptr;                     // データポインタ

    // スポットライトリソース
    Microsoft::WRL::ComPtr<ID3D12Resource> spotLightsResource; // バッファリソース
    SpotLights *spotLightsData = nullptr;                      // データポインタ

    // カメラリソース
    Microsoft::WRL::ComPtr<ID3D12Resource> cameraForGPUResource; // バッファリソース
    CameraForGPU *cameraForGPUData = nullptr;                    // データポインタ

    // CPU側のライトデータ管理
    std::vector<PointLight> pointLights_; // ポイントライト配列
    std::vector<SpotLight> spotLights_;   // スポットライト配列

    // UI表示用
    std::string saveMessage_;  // 保存メッセージ
    int saveMessageTimer_ = 0; // メッセージ表示タイマー

    // フラグ
    bool isDirectionalLight = true;       // 平行光源有効フラグ
    bool showLightVisualization_ = false; // 光源可視化フラグ

    std::unique_ptr<DataHandler> DLightData_ = nullptr; // データハンドラー
};