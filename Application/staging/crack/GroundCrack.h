#pragma once
#include "object/base/BaseObject.h"
#include <memory>
#include <vector>

/// <summary>
/// 叩きつけ攻撃で地面に走る「地割れ」の演出クラス
/// 地面に貼りつく板ポリ（デカール）を複数枚プールし、
/// 出現時に一瞬で広がってから徐々に透明になって消える。
///
/// 叩きつけた「瞬間」ではなく、叩きつけられた相手が地面に到達した瞬間に出したいので、
/// RequestOnLanding() で対象を監視し、接地を検知してから発生させる。
/// 通常のノックバックやひるみでの落下では呼ばれないため、地割れは叩きつけ時のみ出る。
///
/// 板ポリは BaseObjectManager へ非所有で登録する（所有はこのクラス）。
/// 登録することでオブジェクト一覧に並んで状態を確認でき、描画もマネージャの
/// 通常経路に乗るため、この演出だけ描画順が特殊になることがない。
/// 未使用のデカールはスケール0にして、マネージャから描かれても何も出ないようにする
/// </summary>
class GroundCrack
{
  public:
    /// ===================================================
    /// public method
    /// ===================================================

    /// <summary>
    /// 初期化（デカール用の板ポリをプールぶん生成し、BaseObjectManagerへ登録する）
    /// </summary>
    void Init();

    /// <summary>
    /// 更新処理（毎フレーム呼ぶ）
    /// 着地監視の判定と、発生済みデカールの拡大・フェードを進める
    /// </summary>
    /// <param name="deltaTime">フレームの経過時間</param>
    void Update(float deltaTime);

    /// <summary>
    /// デバッグ表示（発生状況の確認とテスト発生）
    /// </summary>
    /// <param name="testPosition">テスト発生ボタンで地割れを出す位置</param>
    void DrawImGui(const Hagine::Vector3 &testPosition);

    /// <summary>
    /// 叩きつけた相手が地面に到達したら地割れを出すよう予約する。
    /// 叩きつけの発生時（相手がまだ空中にいる時点）に呼ぶ
    /// </summary>
    /// <param name="pTarget">叩きつけられた対象（着地を監視する）</param>
    void RequestOnLanding(Hagine::BaseObject *pTarget);

    /// <summary>
    /// 指定位置へ即座に地割れを出す（着地を待たずに出したい場合に使う）
    /// </summary>
    /// <param name="position">発生位置（高さは地表へ合わせ直す）</param>
    void Spawn(const Hagine::Vector3 &position);

    /// <summary>
    /// 着地の監視を打ち切る（技の中断・シーン切り替え時に呼ぶ）
    /// </summary>
    void CancelWatch() { pWatchTarget_ = nullptr; }

    /// <summary>
    /// 調整パラメータをゲームパラメータHubへ登録する
    /// </summary>
    void RegisterParams();

    /// <summary>ゲームパラメータHub上の出所ラベル（登録解除に使う）</summary>
    static constexpr const char *kParamOwner = "地割れ演出";

  private:
    /// ===================================================
    /// private struct
    /// ===================================================

    /// <summary>
    /// デカール1枚分の状態
    /// </summary>
    struct Decal
    {
        std::unique_ptr<Hagine::BaseObject> pObject; // 表示に使う板ポリ
        float timer = 0.0f;                          // 発生からの経過時間
        bool isActive = false;                       // 表示中かどうか
    };

    /// ===================================================
    /// private method
    /// ===================================================

    /// <summary>
    /// 使用するデカールを選ぶ（空きが無ければ最も古いものを使い回す）
    /// </summary>
    /// <returns>Decal*: 使用するデカール</returns>
    Decal *AcquireDecal();

    /// <summary>
    /// デカールを非表示にする（スケール0にして描画されないようにする）
    /// </summary>
    /// <param name="decal">非表示にするデカール</param>
    void HideDecal(Decal &decal);

    /// <summary>
    /// 着地の監視を進め、地面に到達していたら地割れを発生させる
    /// </summary>
    /// <param name="deltaTime">フレームの経過時間</param>
    void UpdateWatch(float deltaTime);

    /// ===================================================
    /// private variables
    /// ===================================================

    // 板ポリの元サイズは 2×2 なので、スケール1あたり2ユニット四方になる
    static constexpr int kDecalPoolSize = 6;                             ///< 同時に出せる地割れの枚数
    static constexpr const char *kTexturePath = "Game/groundCrack.png";  ///< 地割れのテクスチャ
    static constexpr float kRaycastStartHeight = 4.0f;                   ///< 地表を探すレイの開始高さ
    static constexpr float kRaycastDistance = 12.0f;                     ///< 地表を探すレイの長さ
    static constexpr float kEpsilon = 0.001f;                            ///< 微小値

    std::vector<Decal> decals_; ///< デカールのプール
    int nextDecalIndex_ = 0;    ///< 空きが無いときに使い回す位置

    // ─── 着地監視 ───
    Hagine::BaseObject *pWatchTarget_ = nullptr; ///< 着地を監視する対象（nullptrなら監視なし）
    float watchTimer_ = 0.0f;                    ///< 監視を始めてからの経過時間

    // ─── 調整パラメータ（GameParamで調整可）───
    float decalScale_ = 5.0f;       ///< 地割れの大きさ（スケール1で2ユニット四方）
    float lifeTime_ = 2.6f;         ///< 消えるまでの時間（秒）
    float fadeDuration_ = 1.2f;     ///< 透明になっていく時間（秒。寿命の終わり側から数える）
    float expandDuration_ = 0.12f;  ///< 出現時に広がりきるまでの時間（秒）
    float startScaleRate_ = 0.45f;  ///< 出現時の大きさの割合（1.0で最初から原寸）
    float heightOffset_ = 0.06f;    ///< 地表から浮かせる高さ（Zファイティング防止）
    float landingThreshold_ = 0.8f; ///< 地表からこの高さまで来たら着地とみなす
    float watchTimeout_ = 2.5f;     ///< 着地しないまま監視を打ち切るまでの時間（秒）
};
