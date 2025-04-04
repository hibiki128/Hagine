#pragma once
#include "Object3dCommon.h"
#include "ViewProjection.h"
#include "d3d12.h"
#include "externals/nlohmann/json.hpp"
#include "wrl.h"
#include <Vector3.h>
#include <Vector4.h>
#include <string>
enum class LightType {
    Directional,
    Point,
};

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

    /// <summary>
    /// json関連
    /// </summary>
    /// <param name="filePath"></param>
    void SaveDirectionalLight();
    void SavePointLight();
    void SaveSpotLight();
    void LoadDirectionalLight();
    void LoadPointLight();
    void LoadSpotLight();

  private:
    /// <summary>
    /// 平行光源データ作成
    /// </summary>
    void CreateDirectionLight();

    /// <summary>
    /// 点光源データ作成
    /// </summary>
    void CreatePointLight();

    /// <summary>
    /// スポットライト作成
    /// </summary>
    void CreateSpotLight();

    /// <summary>
    /// カメラ作成
    /// </summary>
    void CreateCamera();

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
        float padding[2];
    };

    struct CameraForGPU {
        Vector3 worldPosition;
    };

    // バッファリソース
    Microsoft::WRL::ComPtr<ID3D12Resource> directionalLightResource;
    // バッファリソース内のデータを指すポインタ
    DirectionLight *directionalLightData = nullptr;

    // バッファリソース
    Microsoft::WRL::ComPtr<ID3D12Resource> pointLightResource;
    // バッファリソース内のデータを指すポインタ
    PointLight *pointLightData = nullptr;

    // バッファリソース
    Microsoft::WRL::ComPtr<ID3D12Resource> spotLightResource;
    // バッファリソース内のデータを指すポインタ
    SpotLight *spotLightData = nullptr;

    // バッファリソース
    Microsoft::WRL::ComPtr<ID3D12Resource> cameraForGPUResource;
    // バッファリソース内のデータを指すポインタ
    CameraForGPU *cameraForGPUData = nullptr;

    Object3dCommon *obj3dCommon = nullptr;
    bool isDirectionalLight = true;
    bool isPointLight = false;
    bool isSpotLight = false;
};
