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
    /// private varians
    /// ===================================================

    Enemy *enemy_ = nullptr;

    std::unique_ptr<Sprite> hpBar_;
    std::unique_ptr<Sprite> enemyIcon_;
    std::unique_ptr<Sprite> energyBar_;
    std::unique_ptr<Sprite> barFrame_;
    std::unique_ptr<Sprite> energyBarFrame_;

    Vector2 hpBarPosition_ = {1560.0f, 45.0f};          // HPバーの位置
    Vector2 hpBarSize_ = {400.0f, 40.0f};               // HPバーのサイズ
    Vector2 enemyIconPosition_ = {1456.0f, -30.0f};     // アイコンの位置
    Vector2 iconSize_ = {384.0f, 216.0f};               // アイコンのサイズ
    Vector2 energyBarPosition_ = {1260.0f, 93.0f};      // エネルギーバーの位置
    Vector2 energyBarSize_ = {300.0f, 20.0f};           // エネルギーバーのサイズ
    Vector2 barFramePosition_ = {1152.0f, 43.0f};       // バーフレームの位置
    Vector2 barSize_ = {410.0f, 50.0f};                 // バーのサイズ
    Vector2 energyBarFramePosition_ = {1252.0f, 91.0f}; // エネルギーバーフレームの位置
    Vector2 energyBarFrameSize_ = {310.0f, 30.0f};      // エネルギーバーフレームのサイズ
};