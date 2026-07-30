#pragma once

class FollowCamera;

/// <summary>
/// フォローカメラの各パーツが実装する共通インターフェース
/// ファサード（FollowCamera）はこのインターフェース越しにパーツの初期化を行い、
/// パーツの生成は外部（FollowCameraFactory）へ委ねる。
/// これによりファサードは「どのパーツをどう作るか」を知らずに済み、依存が緩くなる
/// </summary>
class ICameraPart
{
  public:
    /// ===================================================
    /// public method
    /// ===================================================

    /// <summary>
    /// デストラクタ（派生クラスをインターフェース経由で破棄できるようにする）
    /// </summary>
    virtual ~ICameraPart() = default;

    /// <summary>
    /// 所有者（ファサード）を受け取って初期化する
    /// </summary>
    /// <param name="pOwner">所有者のフォローカメラ</param>
    virtual void Init(FollowCamera *pOwner) = 0;
};
