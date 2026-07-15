#pragma once

/// <summary>
/// プレイヤーと敵の向きを表す列挙型
/// 8方向の向きを定義
/// </summary>
enum class Direction
{
    Forward,       // 前
    ForwardRight,  // 前右
    Right,         // 右
    BackwardRight, // 後右
    Behind,        // 後ろ
    BackwardLeft,  // 後左
    Left,          // 左
    ForwardLeft    // 前左
};

/// <summary>
/// 移動方向を表す列挙型
/// 4方向の移動を定義
/// </summary>
enum class MoveDirection
{
    Right,   // 右移動
    Left,    // 左移動
    Forward, // 前移動
    Behind   // 後ろ移動
};