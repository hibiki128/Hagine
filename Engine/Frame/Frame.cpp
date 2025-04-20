#include "Frame.h"
#include <chrono>

/// <summary>
/// 静的メンバ変数の定義
/// </summary>
std::chrono::high_resolution_clock::time_point Frame::lastTime_ = std::chrono::high_resolution_clock::now();
float Frame::deltaTime_ = 0.0f;
float Frame::fps_ = 0.0f;
int Frame::frameCount_ = 0;
float Frame::accumulatedTime_ = 0.0f;

/// <summary>
/// フレームの初期化処理
/// </summary>
void Frame::Init() {
    lastTime_ = std::chrono::high_resolution_clock::now();
    deltaTime_ = 0.0f;
    fps_ = 0.0f;
    frameCount_ = 0;
    accumulatedTime_ = 0.0f;
}

/// <summary>
/// フレームの更新処理
/// </summary>
void Frame::Update() {
    // 現在の時刻を取得
    auto currentTime = std::chrono::high_resolution_clock::now();

    // 経過時間を計算
    std::chrono::duration<float> elapsed = currentTime - lastTime_;
    deltaTime_ = elapsed.count(); // 秒単位の経過時間

    // 経過時間を蓄積
    accumulatedTime_ += deltaTime_;
    frameCount_++; // フレームカウントを増加

    // 10フレームごとにFPSを計算
    if (frameCount_ >= 5) {
        fps_ = static_cast<float>(frameCount_) / accumulatedTime_; // 平均FPSを計算
        frameCount_ = 0; // フレームカウントをリセット
        accumulatedTime_ = 0.0f; // 蓄積時間をリセット
    }

    // 次回の更新のために現在の時刻を記録
    lastTime_ = currentTime;
}

/// <summary>
/// 前回の更新からの経過時間を取得
/// </summary>
/// <returns>前回の更新からの経過時間</returns>
float Frame::DeltaTime() {
    return deltaTime_; // 経過時間を返す
}

/// <summary>
/// 現在のFPSを取得
/// </summary>
/// <returns>現在のFPS</returns>
float Frame::GetFPS() {
    return fps_; // 現在のFPSを返す
}
