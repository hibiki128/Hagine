#pragma once
#include "Collider/type/CylinderCollider.h"
#include "Object/Base/BaseObject.h"
#include <Particle/CSParticle/ParticleCSEmitter.h>
/// <summary>
/// 周囲のフィールド（境界線）を管理するクラス
/// 円柱状の衝突判定とパーティクルによる表現を行う
/// </summary>
class AroundField : public Hagine::BaseObject {
  public:
    /// ===================================================
    /// public method
    /// ===================================================

    /// <summary>
    /// 初期化
    /// </summary>
    /// <param name="objectName">オブジェクト名</param>
    void Init(const std::string objectName) override;

    /// <summary>
    /// 更新処理
    /// </summary>
    void Update() override;

    /// <summary>
    /// 描画処理
    /// </summary>
    /// <param name="viewProjection">ビュープロジェクション</param>
    /// <param name="offSet">描画オフセット</param>
    void Draw(const Hagine::ViewProjection &viewProjection) override;

    /// <summary>
    /// パーティクルの描画処理
    /// </summary>
    /// <param name="viewProjection">ビュープロジェクション</param>
    void DrawParticle(const Hagine::ViewProjection &viewProjection);
    void DrawParticleCompute(const Hagine::ViewProjection &viewProjection);

    /// <summary>
    /// デバッグ処理
    /// </summary>
    void Debug();

    /// <summary>
    /// 終了処理
    /// </summary>
    void Finalize();

  private:
    /// ===================================================
    /// private variants
    /// ===================================================

    Hagine::CylinderCollider *AroundField_ = nullptr; // 円柱コライダー
    std::unique_ptr<Hagine::ParticleCSEmitter> fieldParticle_ = nullptr; // フィールドパーティクル
};
