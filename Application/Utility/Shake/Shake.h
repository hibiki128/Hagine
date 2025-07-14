#pragma once
#include <Camera/ViewProjection/ViewProjection.h>
#ifdef _DEBUG
#include "imgui.h"
#endif // _DEBUG
#include "json.hpp"
#include <type/Vector2.h>
#include <fstream>

class Shake {
  public:
    /// ====================================
    /// public methods
    /// ====================================
    void Initialize(ViewProjection *viewProjection);
    void Update();
    void imgui();
    void StartShake();

  private:
    /// ====================================
    /// private methods
    /// ====================================
    void LoadSettings();
    void SaveSettings();

  private:
    /// ====================================
    /// private variaus
    /// ====================================
    ViewProjection *viewProjection_ = nullptr;
    Vector2 shakeMin_ = {-0.5f, -0.5f};
    Vector2 shakeMax_ = {0.5f, 0.5f};
    float rotationShakeMin_ = -0.1f;
    float rotationShakeMax_ = 0.1f;
    int shakeInterval_ = 2;  // 何フレームごとに揺らすか
    int shakeDuration_ = 30; // 揺れの持続時間
    int currentFrame_ = 0;
    bool isShaking_ = false;
};
