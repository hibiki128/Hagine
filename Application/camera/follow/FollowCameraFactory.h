#pragma once
#include <memory>

class FollowCamera;

/// <summary>
/// フォローカメラの生成を担当するファクトリ
/// 「どのパーツを組み合わせてフォローカメラを作るか」という知識をここへ集約し、
/// FollowCamera 本体からパーツの生成依存を取り除く
/// </summary>
class FollowCameraFactory
{
  public:
    /// ===================================================
    /// public method
    /// ===================================================

    /// <summary>
    /// 標準構成（ロックオン＋Rush＋必殺技演出）のフォローカメラを生成する
    /// </summary>
    /// <returns>std::unique_ptr&lt;FollowCamera&gt;: 生成したフォローカメラ</returns>
    static std::unique_ptr<FollowCamera> Create();
};
