#pragma once

#include <variant>
// std
#include <array>
#include <memory>
#include <vector>
#include <wrl.h>

#include "type/Vector2.h"

#define DIRECTNPUT_VERSION 0x0800 // バージョン指定
#include <XInput.h>
#include <dinput.h>
// input
#include "Mouse.h"
#include <Camera/ViewProjection/ViewProjection.h>
#include <myMath.h>
#include <type/Vector3.h>
#include <type/Vector4.h>

struct Ray {
    Vector3 origin;    // レイの開始点
    Vector3 direction; // レイの方向（正規化済み）
    float length;      // レイの最大長
};

struct RayHitInfo {
    bool hit;               // ヒットしたかどうか
    Vector3 hitPoint;       // ヒット点
    Vector3 hitNormal;      // ヒット面の法線
    float distance;         // レイの開始点からヒット点までの距離
    std::string objectName; // ヒットしたオブジェクトのID
};

// ImGuiシーン描画領域情報
struct SceneViewport {
    Vector2 position; // シーンウィンドウの左上座標
    Vector2 size;     // シーンウィンドウのサイズ
};

class BaseObject;
class Input {

  private:
    enum class PadType {
        DirectInput,
        XInput,
    };
    using State = std::variant<DIJOYSTATE2, XINPUT_STATE>;

    struct Joystick {
        Microsoft::WRL::ComPtr<IDirectInputDevice8> device_;
        int32_t deadZoneL_;
        int32_t deadZoneR_;
        PadType type_;
        State state_;
        State statePre_;
    };

  private:
    Microsoft::WRL::ComPtr<IDirectInput8> directInput_ = nullptr;
    Microsoft::WRL::ComPtr<IDirectInputDevice8> keyboard_ = nullptr;
    std::array<BYTE, 256> key_;
    std::array<BYTE, 256> keyPre_;
    std::vector<Joystick> joysticks_;
    // マウス
    static std::unique_ptr<Mouse> mouse_;

    Ray currentRay_;
    SceneViewport currentViewport_;

  public:
    // シングルトンインスタンスの取得
    static Input *GetInstance();
    void Init(HINSTANCE hInstance, HWND hwnd);
    void Update();

    /// <summary>
    /// 押し込んでいるか
    /// </summary>
    /// <param name="キー番号"></param>
    /// <returns></returns>
    bool PushKey(BYTE keyNumber) const;

    /// <summary>
    /// トリガーしているか
    /// </summary>
    /// <param name="キー番号"></param>
    /// <returns></returns>
    bool TriggerKey(BYTE keyNumber) const;

    /// <summary>
    /// 　離しているか
    /// </summary>
    /// <param name="キー番号"></param>
    /// <returns></returns>
    bool ReleaseKey(BYTE keyNumber) const;

    /// <summary>
    /// 　離した瞬間か
    /// </summary>
    /// <param name="キー番号"></param>
    /// <returns></returns>
    bool ReleaseMomentKey(BYTE keyNumber) const;

    /// <summary>
    /// 現在のジョイスティック状態を取得する
    /// </summary>
    /// <param name="stickNo">ジョイスティック番号</param>
    /// <param name="out">現在のジョイスティック状態</param>
    /// <returns>正しく取得できたか</returns>
    template <typename T>
    bool GetJoystickState(int32_t stickNo, T &out) const;

    /// <summary>
    /// 前回のジョイスティック状態を取得する
    /// </summary>
    /// <param name="stickNo">ジョイスティック番号</param>
    /// <param name="out">前回のジョイスティック状態</param>
    /// <returns>正しく取得できたか</returns>
    template <typename T>
    bool GetJoystickStatePrevious(int32_t stickNo, T &out) const;

    /// <summary>
    /// デッドゾーンを設定する
    /// </summary>
    /// <param name="stickNo">ジョイスティック番号</param>
    /// <param name="deadZoneL">デッドゾーン左スティック 0~32768</param>
    /// <param name="deadZoneR">デッドゾーン右スティック 0~32768</param>
    /// <returns>正しく取得できたか</returns>
    void SetJoystickDeadZone(int32_t stickNo, int32_t deadZoneL, int32_t deadZoneR);

    /// <summary>
    /// 接続されているジョイスティック数を取得する
    /// </summary>
    /// <returns>接続されているジョイスティック数</returns>
    size_t GetNumberOfJoysticks() const;

    ///// <summary>
    ///// 全マウス情報取得
    ///// </summary>
    ///// <returns>マウス情報</returns>
    // const DIMOUSESTATE2& GetAllMouse() const;

    /// <summary>
    /// マウスの押下をチェック
    /// </summary>
    /// <param name="buttonNumber">マウスボタン番号(0:左,1:右,2:中,3~7:拡張マウスボタン)</param>
    /// <returns>押されているか</returns>
    static bool IsPressMouse(int32_t mouseNumber);

    /// <summary>
    /// マウスのトリガーをチェック。押した瞬間だけtrueになる
    /// </summary>
    /// <param name="buttonNumber">マウスボタン番号(0:左,1:右,2:中,3~7:拡張マウスボタン)</param>
    /// <returns>トリガーか</returns>
    static bool IsTriggerMouse(int32_t buttonNumber);

    /// <summary>
    /// マウス移動量を取得
    /// </summary>
    /// <returns>マウス移動量</returns>
    static MouseMove GetMouseMove();

    /// <summary>
    /// ホイールスクロール量を取得する
    /// </summary>
    /// <returns>ホイールスクロール量。奥側に回したら+。Windowsの設定で逆にしてたら逆</returns>
    static int32_t GetWheel();

    /// <summary>
    /// マウスの位置を取得する（ウィンドウ座標系）
    /// </summary>
    /// <returns>マウスの位置</returns>
    static Vector2 GetMousePos();

    /// <summary>
    /// 3Dのマウス座標
    /// </summary>
    /// <param name="viewprojection"></param>
    /// <param name="depthFactor"></param>
    /// <returns></returns>
    static Vector3 GetMousePos3D(const ViewProjection &viewprojection, float depthFactor, float blockSpacing = 1.0f);

    /// <summary>
    /// レイを更新する（毎フレーム呼び出す）
    /// </summary>
    /// <param name="viewprojection">カメラのビュープロジェクション</param>
    /// <param name="viewport">ImGuiシーンの描画領域情報</param>
    /// <param name="rayLength">レイの最大長</param>
    void UpdateRay(const ViewProjection &viewprojection, const SceneViewport &viewport, float rayLength = 1000.0f);

    /// <summary>
    /// 現在のレイを取得する
    /// </summary>
    /// <returns>現在のレイ</returns>
    const Ray &GetCurrentRay() const { return currentRay_; }

    /// <summary>
    /// レイとAABBの衝突判定
    /// </summary>
    /// <param name="ray">レイ</param>
    /// <param name="aabb">バウンディングボックス</param>
    /// <param name="worldMatrix">オブジェクトのワールドマトリックス</param>
    /// <param name="hitInfo">ヒット情報（出力）</param>
    /// <returns>衝突したかどうか</returns>
    static bool RayIntersectAABB(const Ray &ray, BaseObject *targetObject, RayHitInfo &hitInfo,
                                 const AABB &aabb = {Vector3(-1.0f, -1.0f, -1.0f), Vector3(1.0f, 1.0f, 1.0f)});

    /// <summary>
    /// レイとスフィアの衝突判定
    /// </summary>
    /// <param name="ray">レイ</param>
    /// <param name="sphere">球</param>
    /// <param name="worldMatrix">オブジェクトのワールドマトリックス</param>
    /// <param name="hitInfo">ヒット情報（出力）</param>
    /// <returns>衝突したかどうか</returns>
    static bool RayIntersectSphere(const Ray &ray, BaseObject *targetObject, RayHitInfo &hitInfo,
                                   const Sphere &sphere = {Vector3(0.0f, 0.0f, 0.0f), 1.0f});

    /// <summary>
    /// マウス位置からシーン内でのレイを生成する
    /// </summary>
    /// <param name="mousePos">マウス位置（ウィンドウ座標系）</param>
    /// <param name="viewprojection">カメラのビュープロジェクション</param>
    /// <param name="viewport">シーンの描画領域</param>
    /// <param name="rayLength">レイの長さ</param>
    /// <returns>生成されたレイ</returns>
    static Ray CreateRayFromMouse(const Vector2 &mousePos, const ViewProjection &viewprojection,
                                  const SceneViewport &viewport, float rayLength = 1000.0f);

    /// <summary>
    /// 複数のマトリックスに対してAABBレイキャストを行う
    /// </summary>
    /// <param name="ray">レイ</param>
    /// <param name="worldMatrices">ワールドマトリックスのリスト</param>
    /// <returns>最も近いヒット情報</returns>
    static RayHitInfo RaycastMultipleAABB(const Ray &ray, const std::vector<BaseObject *> baseObjects);

    /// <summary>
    /// 複数のマトリックスに対してスフィアレイキャストを行う
    /// </summary>
    /// <param name="ray">レイ</param>
    /// <param name="worldMatrices">ワールドマトリックスのリスト</param>
    /// <returns>最も近いヒット情報</returns>
    static RayHitInfo RaycastMultipleSphere(const Ray &ray, const std::vector<BaseObject *> baseObjects);

    const BYTE *GetKeyState() const { return key_.data(); }
    const BYTE *GetPreviousKeyState() const { return keyPre_.data(); }
};
