#pragma once
#include "Application/GameObject/Enemy/Enemy.h"
#include "Sprite.h"

/// <summary>
/// 敵のUI表示を管理するクラス
/// HPバー、アイコン、エネルギーバーなどを描画
/// </summary>
class EnemyUI {
  public:
    /// ===================================================
    /// public method
    /// ===================================================

    /// <summary>
    /// 初期化
    /// </summary>
    /// <param name="enemy">参照する敵のポインタ</param>
    void Init(Enemy *enemy);

    /// <summary>
    /// 更新処理
    /// </summary>
    void Update();

    /// <summary>
    /// 描画処理
    /// </summary>
    void Draw();

    /// <summary>
    /// デバッグ処理
    /// </summary>
    void Debug();

  private:
    /// ===================================================
    /// private variants
    /// ===================================================

    // 定数定義
    static constexpr float kHPBarPositionX = 1560.0f;          // HPバー位置X
    static constexpr float kHPBarPositionY = 45.0f;            // HPバー位置Y
    static constexpr float kHPBarSizeX = 400.0f;               // HPバー幅
    static constexpr float kHPBarSizeY = 40.0f;                // HPバー高さ
    static constexpr float kEnemyIconPositionX = 1456.0f;      // アイコン位置X
    static constexpr float kEnemyIconPositionY = -30.0f;       // アイコン位置Y
    static constexpr float kIconSizeX = 384.0f;                // アイコン幅
    static constexpr float kIconSizeY = 216.0f;                // アイコン高さ
    static constexpr float kEnergyBarPositionX = 1560.0f;     // エネルギーバー位置X
    static constexpr float kEnergyBarPositionY = 93.0f;        // エネルギーバー位置Y
    static constexpr float kEnergyBarSizeX = 300.0f;           // エネルギーバー幅
    static constexpr float kEnergyBarSizeY = 20.0f;            // エネルギーバー高さ
    static constexpr float kBarFramePositionX = 1152.0f;       // バーフレーム位置X
    static constexpr float kBarFramePositionY = 43.0f;         // バーフレーム位置Y
    static constexpr float kBarSizeX = 410.0f;                 // バーフレーム幅
    static constexpr float kBarSizeY = 50.0f;                  // バーフレーム高さ
    static constexpr float kEnergyBarFramePositionX = 1252.0f; // エネルギーバーフレーム位置X
    static constexpr float kEnergyBarFramePositionY = 91.0f;   // エネルギーバーフレーム位置Y
    static constexpr float kEnergyBarFrameSizeX = 310.0f;      // エネルギーバーフレーム幅
    static constexpr float kEnergyBarFrameSizeY = 30.0f;       // エネルギーバーフレーム高さ

    // 色定数
    static constexpr float kHPBarColorR = 1.0f;     // HPバー色(R)
    static constexpr float kHPBarColorG = 0.2f;     // HPバー色(G)
    static constexpr float kHPBarColorB = 0.1f;     // HPバー色(B)
    static constexpr float kHPBarColorA = 1.0f;     // HPバー色(A)
    static constexpr float kFrameColorR = 0.3f;     // フレーム色(R)
    static constexpr float kFrameColorG = 0.3f;     // フレーム色(G)
    static constexpr float kFrameColorB = 0.3f;     // フレーム色(B)
    static constexpr float kFrameColorA = 0.6f;     // フレーム色(A)
    static constexpr float kEnergyBarColorR = 1.0f; // エネルギーバー色(R)
    static constexpr float kEnergyBarColorG = 0.5f; // エネルギーバー色(G)
    static constexpr float kEnergyBarColorB = 0.0f; // エネルギーバー色(B)
    static constexpr float kEnergyBarColorA = 1.0f; // エネルギーバー色(A)
    static constexpr float kAnchorX = 1.0f;         // アンカーX
    static constexpr float kAnchorY = 0.0f;         // アンカーY

    Enemy *enemy_ = nullptr; // 敵へのポインタ

    std::unique_ptr<Sprite> hpBar_;           // HPバー
    std::unique_ptr<Sprite> enemyIcon_;       // アイコン
    std::unique_ptr<Sprite> energyBar_;       // エネルギーバー
    std::unique_ptr<Sprite> barFrame_;        // HPバーフレーム
    std::unique_ptr<Sprite> energyBarFrame_;  // エネルギーバーフレーム

    Vector2 hpBarPosition_ = {kHPBarPositionX, kHPBarPositionY};                            // HPバーの位置
    Vector2 hpBarSize_ = {kHPBarSizeX, kHPBarSizeY};                                        // HPバーのサイズ
    Vector2 enemyIconPosition_ = {kEnemyIconPositionX, kEnemyIconPositionY};                // アイコンの位置
    Vector2 iconSize_ = {kIconSizeX, kIconSizeY};                                           // アイコンのサイズ
    Vector2 energyBarPosition_ = {kEnergyBarPositionX, kEnergyBarPositionY};                // エネルギーバーの位置
    Vector2 energyBarSize_ = {kEnergyBarSizeX, kEnergyBarSizeY};                            // エネルギーバーのサイズ
    Vector2 barFramePosition_ = {kBarFramePositionX, kBarFramePositionY};                   // バーフレームの位置
    Vector2 barSize_ = {kBarSizeX, kBarSizeY};                                              // バーのサイズ
    Vector2 energyBarFramePosition_ = {kEnergyBarFramePositionX, kEnergyBarFramePositionY}; // エネルギーバーフレームの位置
    Vector2 energyBarFrameSize_ = {kEnergyBarFrameSizeX, kEnergyBarFrameSizeY};             // エネルギーバーフレームのサイズ
};