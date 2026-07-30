#pragma once
#include "Application/entity/player/Player.h"
#include "Sprite.h"

/// <summary>
/// プレイヤーのUI表示を管理するクラス
/// HPバー、アイコン、エネルギーバーなどを描画
/// </summary>
class PlayerUI
{
  public:
    /// ===================================================
    /// public method
    /// ===================================================

    /// <summary>
    /// 初期化
    /// </summary>
    /// <param name="player">参照するプレイヤーのポインタ</param>
    void Init(Player *player);

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

    /// <summary>
    /// フェードイン用アルファ値を一括適用（0.0=透明, 1.0=通常）
    /// </summary>
    void SetFadeAlpha(float alpha);

  private:
    /// ===================================================
    /// private variables
    /// ===================================================

    // 定数定義
    static constexpr float kHPBarPositionX = 200.0f;          // HPバー位置X
    static constexpr float kHPBarPositionY = 45.0f;           // HPバー位置Y
    static constexpr float kHPBarSizeX = 400.0f;              // HPバー幅
    static constexpr float kHPBarSizeY = 40.0f;               // HPバー高さ
    static constexpr float kPlayerIconPositionX = -80.0f;     // プレイヤーアイコン位置X
    static constexpr float kPlayerIconPositionY = -30.0f;     // プレイヤーアイコン位置Y
    static constexpr float kIconSizeX = 384.0f;               // アイコン幅
    static constexpr float kIconSizeY = 216.0f;               // アイコン高さ
    static constexpr float kEnergyBarPositionX = 200.0f;      // エネルギーバー位置X
    static constexpr float kEnergyBarPositionY = 93.0f;       // エネルギーバー位置Y
    static constexpr float kEnergyBarSizeX = 300.0f;          // エネルギーバー幅
    static constexpr float kEnergyBarSizeY = 20.0f;           // エネルギーバー高さ
    static constexpr float kBarFramePositionX = 198.0f;       // バーフレーム位置X
    static constexpr float kBarFramePositionY = 43.0f;        // バーフレーム位置Y
    static constexpr float kBarSizeX = 410.0f;                // バーフレーム幅
    static constexpr float kBarSizeY = 50.0f;                 // バーフレーム高さ
    static constexpr float kEnergyBarFramePositionX = 198.0f; // エネルギーバーフレーム位置X
    static constexpr float kEnergyBarFramePositionY = 91.0f;  // エネルギーバーフレーム位置Y
    static constexpr float kEnergyBarFrameSizeX = 310.0f;     // エネルギーバーフレーム幅
    static constexpr float kEnergyBarFrameSizeY = 30.0f;      // エネルギーバーフレーム高さ

    // 色定数
    static constexpr float kHPBarColorR = 0.1f;     // HPバー色(R)
    static constexpr float kHPBarColorG = 0.2f;     // HPバー色(G)
    static constexpr float kHPBarColorB = 1.0f;     // HPバー色(B)
    static constexpr float kHPBarColorA = 1.0f;     // HPバー色(A)
    static constexpr float kFrameColorR = 0.3f;     // フレーム色(R)
    static constexpr float kFrameColorG = 0.3f;     // フレーム色(G)
    static constexpr float kFrameColorB = 0.3f;     // フレーム色(B)
    static constexpr float kFrameColorA = 0.6f;     // フレーム色(A)
    static constexpr float kEnergyBarColorR = 1.0f; // エネルギーバー色(R)
    static constexpr float kEnergyBarColorG = 1.0f; // エネルギーバー色(G)
    static constexpr float kEnergyBarColorB = 0.0f; // エネルギーバー色(B)
    static constexpr float kEnergyBarColorA = 1.0f; // エネルギーバー色(A)

    Player *pPlayer_ = nullptr;

    std::unique_ptr<Hagine::Sprite> hpBar_;
    std::unique_ptr<Hagine::Sprite> playerIcon_;
    std::unique_ptr<Hagine::Sprite> energyBar_;
    std::unique_ptr<Hagine::Sprite> barFrame_;
    std::unique_ptr<Hagine::Sprite> energyBarFrame_;

    Hagine::Vector2 hpBarPosition_ = {kHPBarPositionX, kHPBarPositionY};                            // HPバーの位置
    Hagine::Vector2 hpBarSize_ = {kHPBarSizeX, kHPBarSizeY};                                        // HPバーのサイズ
    Hagine::Vector2 playerIconPosition_ = {kPlayerIconPositionX, kPlayerIconPositionY};             // プレイヤーアイコンの位置
    Hagine::Vector2 iconSize_ = {kIconSizeX, kIconSizeY};                                           // プレイヤーアイコンのサイズ
    Hagine::Vector2 energyBarPosition_ = {kEnergyBarPositionX, kEnergyBarPositionY};                // エネルギーバーの位置
    Hagine::Vector2 energyBarSize_ = {kEnergyBarSizeX, kEnergyBarSizeY};                            // エネルギーバーのサイズ
    Hagine::Vector2 barFramePosition_ = {kBarFramePositionX, kBarFramePositionY};                   // バーフレームの位置
    Hagine::Vector2 barSize_ = {kBarSizeX, kBarSizeY};                                              // バーのサイズ
    Hagine::Vector2 energyBarFramePosition_ = {kEnergyBarFramePositionX, kEnergyBarFramePositionY}; // エネルギーバーフレームの位置
    Hagine::Vector2 energyBarFrameSize_ = {kEnergyBarFrameSizeX, kEnergyBarFrameSizeY};             // エネルギーバーフレームのサイズ
};