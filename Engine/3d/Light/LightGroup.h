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

enum class LightType {
    Directional,
    Point,
    Spot
};

class DirectXCommon;
class LightGroup {
  private:
    static LightGroup *instance;

    LightGroup() = default;
    ~LightGroup() = default;
    LightGroup(LightGroup &) = delete;
    LightGroup &operator=(LightGroup &) = delete;

  public:
    /// <summary>
    /// シングルトンインスタンスの取得
    /// </summary>
    /// <returns></returns>
    static LightGroup *GetInstance();

    /// <summary>
    /// 終了
    /// </summary>
    void Finalize();

    /// <summary>
    /// 初期化
    /// </summary>
    void Initialize();

    /// <summary>
    /// 更新
    /// </summary>
    void Update(const ViewProjection &viewProjection);

    /// <summary>
    /// 描画
    /// </summary>
    void Draw();

    /// <summary>
    /// デバッグ操作
    /// </summary>
    void imgui();

    void SaveLightData(const std::string &fileName);

    void LoadLightData(const std::string &fileName);

  private:
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
    /// カメラ作成
    /// </summary>
    void CreateCamera();

    /// <summary>
    /// ポイントライト追加
    /// </summary>
    void AddPointLight();

    /// <summary>
    /// ポイントライト削除
    /// </summary>
    void RemovePointLight(int index);

    /// <summary>
    /// スポットライト追加
    /// </summary>
    void AddSpotLight();

    /// <summary>
    /// スポットライト削除
    /// </summary>
    void RemoveSpotLight(int index);

    /// <summary>
    /// ポイントライトデータ更新
    /// </summary>
    void UpdatePointLightBuffer();

    /// <summary>
    /// スポットライトデータ更新
    /// </summary>
    void UpdateSpotLightBuffer();

    void DrawLightVisualization(); // 光源可視化描画
    void SetShowLightVisualization(bool show) { showLightVisualization_ = show; }

  private:
    struct PointLight {
        Vector4 color;
        Vector3 position;
        float intensity;
        int32_t active;
        float radius;
        float decay;
        int32_t HalfLambert;
        int32_t BlinnPhong;
        float padding[3];
    };

    // ポイントライト配列用構造体
    struct PointLights {
        alignas(16) PointLight lights[MAX_POINT_LIGHTS];
        int32_t count;
        float padding[3];
    };

    // 平行光源データ
    struct DirectionLight {
        Vector4 color;     //!< ライトの色
        Vector3 direction; //!< ライトの向き
        float intensity;   //!< 輝度
        int32_t active;
        int32_t HalfLambert;
        int32_t BlinnPhong;
    };

    struct SpotLight {
        Vector4 color;
        Vector3 position;
        float intensity;
        Vector3 direction;
        float distance;
        float decay;
        float cosAngle;
        int32_t active;
        int32_t HalfLambert;
        int32_t BlinnPhong;
        float padding[3];
    };

    // スポットライト配列用構造体
    struct SpotLights {
        SpotLight lights[MAX_SPOT_LIGHTS];
        int32_t count;
        float padding[3];
    };

    struct CameraForGPU {
        Vector3 worldPosition;
    };

    // バッファリソース
    Microsoft::WRL::ComPtr<ID3D12Resource> directionalLightResource;
    // バッファリソース内のデータを指すポインタ
    DirectionLight *directionalLightData = nullptr;

    // ポイントライト配列バッファリソース
    Microsoft::WRL::ComPtr<ID3D12Resource> pointLightsResource;
    // バッファリソース内のデータを指すポインタ
    PointLights *pointLightsData = nullptr;

    // スポットライト配列バッファリソース
    Microsoft::WRL::ComPtr<ID3D12Resource> spotLightsResource;
    // バッファリソース内のデータを指すポインタ
    SpotLights *spotLightsData = nullptr;

    // バッファリソース
    Microsoft::WRL::ComPtr<ID3D12Resource> cameraForGPUResource;
    // バッファリソース内のデータを指すポインタ
    CameraForGPU *cameraForGPUData = nullptr;

    DirectXCommon *dxCommon_;

    // CPU側のライトデータ管理
    std::vector<PointLight> pointLights_;
    std::vector<SpotLight> spotLights_;

    std::string saveMessage_;
    int saveMessageTimer_ = 0;

    bool isDirectionalLight = true;
    bool showLightVisualization_ = false;
    std::unique_ptr<DataHandler> DLightData_ = nullptr;
};
