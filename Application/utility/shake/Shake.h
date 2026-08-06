#pragma once
#include <camera/Camera.h>
#ifdef _DEBUG
#include "imgui.h"
#endif // _DEBUG
#include "utility/data/DataHandler.h"
#include "json.hpp"
#include <fstream>
#include <type/Vector2.h>

/// <summary>
/// カメラ揺れ効果を管理するクラス
/// 攻撃ヒット時などにカメラを振動させる演出を制御
/// </summary>
class Shake
{
  public:
    /// ===================================================
    /// public method
    /// ===================================================

    /// <summary>
    /// 初期化
    /// </summary>
    /// <remarks>
    /// 揺らす対象は「今描画に使われているカメラ」なので指定しない。
    /// カメラが切り替わっても常に画面が揺れる。
    /// </remarks>
    /// <param name="jsonName">設定ファイル名</param>
    void Initialize(std::string jsonName = {});

    /// <summary>
    /// 更新処理
    /// </summary>
    void Update();

    /// <summary>
    /// ImGui表示
    /// </summary>
    void DrawImGui();

    /// <summary>
    /// 揺れを開始
    /// </summary>
    void StartShake();

  private:
    /// ===================================================
    /// private method
    /// ===================================================

    /// <summary>
    /// 設定を読み込み
    /// </summary>
    /// <param name="jsonName">設定ファイル名</param>
    void LoadSettings(std::string jsonName);

    /// <summary>
    /// 設定を保存
    /// </summary>
    /// <param name="jsonName">設定ファイル名</param>
    void SaveSettings(std::string jsonName);

  private:
    /// ===================================================
    /// private variables
    /// ===================================================

    Hagine::Vector3 currentOffset_{};    // 今フレームの位置のずれ
    float currentRotationOffset_ = 0.0f; // 今フレームの傾き

    Hagine::Vector2 shakeMin_ = {-0.5f, -0.5f}; // 揺れ最小値
    Hagine::Vector2 shakeMax_ = {0.5f, 0.5f};   // 揺れ最大値
    float rotationShakeMin_ = -0.1f;            // 回転揺れ最小値
    float rotationShakeMax_ = 0.1f;             // 回転揺れ最大値

    int shakeInterval_ = 2;  // 揺らす間隔（フレーム）
    int shakeDuration_ = 30; // 揺れの持続時間（フレーム）
    int currentFrame_ = 0;   // 現在の経過フレーム数
    bool isShaking_ = false; // 揺れ中フラグ

    std::unique_ptr<Hagine::DataHandler> dataHandler_; // データハンドラ
};