#pragma once
#include "collider/type/CylinderCollider.h"
#include "object/base/BaseObject.h"
#include <particle/gpu/ParticleCSEmitter.h>
/// <summary>
/// 周囲のフィールド（境界線）を管理するクラス
/// 円柱状の衝突判定とパーティクルによる表現を行う
/// </summary>
class AroundField : public Hagine::BaseObject
{
  public:
    /// ===================================================
    /// public method
    /// ===================================================

    /// <summary>
    /// デストラクタ（アクティブフィールドの登録を解除する）
    /// </summary>
    ~AroundField() override;

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
    /// デバッグ処理
    /// </summary>
    void Debug();

    /// <summary>
    /// 終了処理
    /// </summary>
    void Finalize();

    /// <summary>
    /// アクティブなフィールド境界の円柱コライダーを取得（存在しなければ nullptr）。
    /// カメラをフィールド内へクランプする用途などに使う
    /// </summary>
    static const Hagine::CylinderCollider *GetFieldCollider() { return activeField_; }

  private:
    /// ===================================================
    /// private variants
    /// ===================================================

    Hagine::CylinderCollider *aroundField_ = nullptr; // 円柱コライダー
    // フィールドパーティクル。実体は ParticleCSSpawner が所有する（借用ポインタ）。
    // 更新・描画はエンジンが自動で回し、シーン遷移時にまとめて破棄される。
    Hagine::ParticleCSEmitter *fieldParticle_ = nullptr;

    static inline const Hagine::CylinderCollider *activeField_ = nullptr; // アクティブシーンのフィールドコライダー
};
