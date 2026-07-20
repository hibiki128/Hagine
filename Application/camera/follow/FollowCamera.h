#pragma once
#include <camera/projection/ViewProjection.h>
#include <Easing.h>
#include <Application/camera/follow/parts/CameraLockOn.h>
#include <Application/camera/follow/parts/CameraRush.h>
#include <Application/camera/follow/parts/CameraSkillCutscene.h>
#include <transform/WorldTransform.h>
#include <memory>
#include <numbers>

class Player;
namespace Hagine {
class DrawLine3D;
class BaseObject;
} // namespace Hagine

/// <summary>
/// ターゲットを追従するカメラクラス
/// 基本追従・手動ヨー回転・行列反映を本体が担い、ロックオン（肩・高さ・視錐台）・
/// Rush（突進）・必殺技の顔アップ演出は各パーツ（Parts/）が担当するファサード
/// </summary>
class FollowCamera
{
  public:
    /// ===================================================
    /// public method
    /// ===================================================

    /// <summary>
    /// コンストラクタ（パーツを生成する）
    /// </summary>
    FollowCamera();

    /// <summary>
    /// デストラクタ
    /// </summary>
    ~FollowCamera();

    /// <summary>
    /// 初期化
    /// </summary>
    void Init();

    /// <summary>
    /// 更新処理
    /// </summary>
    void Update();

    /// <summary>
    /// デバッグ用のImGui表示
    /// </summary>
    void imgui();

    /// <summary>
    /// 視錐台の描画
    /// </summary>
    void DrawFrustum() { pLockOn_->DrawFrustum(); }

    /// <summary>
    /// 視錐台ロックオンの更新処理
    /// 視錐台内に敵が入った際、自動的にロックオンを試みる
    /// </summary>
    void UpdateFrustumLockOn() { pLockOn_->UpdateFrustumLockOn(); }

    /// <summary>
    /// 視錐台のデバッグ描画
    /// </summary>
    /// <param name="pDrawLine3D">ライン描画クラスのポインタ</param>
    void DrawLockOnFrustum(Hagine::DrawLine3D *pDrawLine3D) const { pLockOn_->DrawLockOnFrustum(pDrawLine3D); }

    /// <summary>
    /// 必殺技の顔アップ演出を開始する
    /// </summary>
    /// <param name="pPerformer">技の使用者（プレイヤー/敵どちらでも可）</param>
    void StartSkillCloseUp(Hagine::BaseObject *pPerformer) { pSkillCutscene_->StartSkillCloseUp(pPerformer); }

    /// <summary>
    /// 必殺技の顔アップ演出を終了する（次フレームから通常カメラへ補間なしで即復帰）
    /// </summary>
    void EndSkillCloseUp() { pSkillCutscene_->EndSkillCloseUp(); }

    /// <summary>
    /// 顔アップ演出中かどうか
    /// </summary>
    /// <returns>bool: 演出中なら true</returns>
    bool IsSkillCloseUpActive() const { return pSkillCutscene_->IsSkillCloseUpActive(); }

    /// ===================================================
    /// パーツ向けアクセサ（各パーツが owner 経由で参照する）
    /// ===================================================

    /// <summary>カメラのワールドトランスフォームを取得</summary>
    Hagine::WorldTransform &GetCameraWorldTransform() { return worldTransform_; }

    /// <summary>追従対象のプレイヤーを取得</summary>
    Player *GetTarget() { return pTarget_; }

    /// <summary>ヨー角を設定</summary>
    void SetYaw(float yaw) { yaw_ = yaw; }

    /// <summary>
    /// worldTransform_ の位置・回転を ViewProjection へ反映して行列を更新する
    /// </summary>
    void ApplyToViewProjection();

    /// <summary>
    /// カメラ位置の衝突解消。
    /// 地形メッシュに遮蔽・めり込みしないよう注視点→カメラのレイで手前へ引き寄せ、
    /// AroundField の円柱境界の内側へクランプする。
    /// worldTransform_ の位置を直接補正するため、ApplyToViewProjection の直前に呼ぶこと
    /// </summary>
    void ResolveCameraCollision();

    /// ===================================================
    /// Getter
    /// ===================================================

    /// <summary>ヨー角を取得</summary>
    float GetYaw() { return yaw_; }

    /// <summary>ビュープロジェクションを取得</summary>
    Hagine::ViewProjection &GetViewProjection() { return viewProjection_; }

    /// <summary>ロックオン有効距離を取得</summary>
    float GetLockOnRange() const { return pLockOn_->GetLockOnRange(); }

    /// <summary>ロックオン水平半角を取得</summary>
    float GetLockOnHalfFovH() const { return pLockOn_->GetLockOnHalfFovH(); }

    /// <summary>ロックオン垂直半角を取得</summary>
    float GetLockOnHalfFovV() const { return pLockOn_->GetLockOnHalfFovV(); }

    /// <summary>視錐台デバッグ描画フラグを取得</summary>
    bool GetDrawLockOnFrustumDebug() const { return pLockOn_->GetDrawLockOnFrustumDebug(); }

    /// ===================================================
    /// Setter
    /// ===================================================

    /// <summary>追従対象を設定</summary>
    /// <param name="pTarget">ターゲットプレイヤーのポインタ</param>
    void SetPlayer(Player *pTarget) { pTarget_ = pTarget; }

    /// <summary>カメラの視野角を設定</summary>
    /// <param name="fov">視野角（度数）</param>
    void SetCameraFov(float fov)
    {
        viewProjection_.fovAngleY_ = fov * std::numbers::pi_v<float> / 180.0f;
    }

    /// <summary>ロックオン有効距離を設定</summary>
    void SetLockOnRange(float range) { pLockOn_->SetLockOnRange(range); }

    /// <summary>ロックオン水平半角を設定</summary>
    void SetLockOnHalfFovH(float rad) { pLockOn_->SetLockOnHalfFovH(rad); }

    /// <summary>ロックオン垂直半角を設定</summary>
    void SetLockOnHalfFovV(float rad) { pLockOn_->SetLockOnHalfFovV(rad); }

    /// <summary>視錐台デバッグ描画フラグを設定</summary>
    void SetDrawLockOnFrustumDebug(bool flag) { pLockOn_->SetDrawLockOnFrustumDebug(flag); }

  private:
    /// ===================================================
    /// private method
    /// ===================================================

    /// <summary>
    /// カメラの移動計算（手動ヨー回転）
    /// </summary>
    void Move();

    /// <summary>
    /// ロックオン有無に応じて最終的なカメラ位置を算出し、回転を worldTransform_ に設定する
    /// </summary>
    /// <param name="isCurrentlyLockedOn">今フレームのロックオン状態</param>
    /// <param name="pPlayer">追従対象プレイヤー</param>
    /// <param name="targetPos">追従対象の位置</param>
    /// <returns>Vector3: 算出したカメラ位置</returns>
    Hagine::Vector3 ComputeCameraTransform(bool isCurrentlyLockedOn, Player *pPlayer, const Hagine::Vector3 &targetPos);

    /// <summary>
    /// AroundField の円柱境界の内側へカメラ位置をクランプする
    /// </summary>
    /// <param name="cameraPos">補正対象のカメラ位置（in/out）</param>
    void ResolveFieldCollision(Hagine::Vector3 &cameraPos);

    /// <summary>
    /// 地形メッシュとの遮蔽・めり込みを解消し、最低高度を確保する
    /// </summary>
    /// <param name="cameraPos">補正対象のカメラ位置（in/out）</param>
    void ResolveTerrainCollision(Hagine::Vector3 &cameraPos);

  private:
    /// ===================================================
    /// private variables
    /// ===================================================

    // カメラ設定
    static constexpr float kFarZ = 1100.0f; ///< 描画距離の遠面

    // カメラ衝突関連
    static constexpr float kCameraPivotHeight = 2.0f;     ///< 遮蔽判定の注視点（プレイヤー位置からの上方オフセット）
    static constexpr float kCameraCollisionMargin = 0.5f; ///< 地形ヒット位置からの手前マージン
    static constexpr float kCameraMinDistance = 2.0f;     ///< 注視点へ引き寄せる際の最小距離
    static constexpr float kCameraFloorClearance = 0.5f;  ///< 地表からの最低クリアランス
    static constexpr float kFieldClampMargin = 2.0f;      ///< フィールド円柱境界からの内側マージン

    // 閾値定数
    static constexpr float kEpsilon = 0.001f; ///< 微小値

    // ロックオンカメラのピッチ上限（敵の真上/真下付近で forward が垂直になり
    // up ベクトルが退化してカメラが暴れるのを防ぐ）
    static constexpr float kMaxLockOnPitch = 60.0f * std::numbers::pi_v<float> / 180.0f;

    // ベクトル定数
    static constexpr float kVectorZero = 0.0f; ///< ゼロ値

    // 初期値
    static constexpr float kInitialYaw = 0.0f; ///< 初期ヨー角

    // ─── パーツ ───
    std::unique_ptr<CameraLockOn> pLockOn_;               ///< 肩・高さ・視錐台ロックオン
    std::unique_ptr<CameraRush> pRush_;                   ///< Rush（突進）専用カメラ
    std::unique_ptr<CameraSkillCutscene> pSkillCutscene_; ///< 必殺技の顔アップ演出

    Hagine::ViewProjection viewProjection_; ///< ビュープロジェクション
    Hagine::WorldTransform worldTransform_; ///< ワールドトランスフォーム

    Player *pTarget_ = nullptr; ///< 追従対象のプレイヤー

    Hagine::Vector3 cameraOffset_ = {0.0f, 5.0f, -25.0f}; ///< ベース of カメラオフセット

    float yaw_ = 0.0f;             ///< 現在のヨー角
    float manualYawSpeed_ = 0.04f; ///< 手動回転速度
};
