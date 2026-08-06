#pragma once
#include "ComboDefinition.h"
#include <functional>
#include <string>
#include <vector>

namespace Hagine {
class BaseObject;
}

/// <summary>
/// コンボシステムを管理するクラス
/// 連続攻撃のシーケンスと時間管理を制御する。
/// 段の並び・ダメージ・ノックバック・判定タイミング・アニメーションといった
/// データは ComboDefinition（JSON）から読み込み、このクラスは進行だけを担う
/// </summary>
class ComboSystem
{
  private:
    /// ===================================================
    /// private struct
    /// ===================================================

    /// <summary>
    /// コンボ1段分の実行時データ（定義データ＋モーション再生対象）
    /// </summary>
    struct ComboData
    {
        Hagine::BaseObject *pTarget = nullptr; // モーションを再生するオブジェクト（見た目用。不要なら nullptr）
        ComboStepData step;                    // 定義から読み込んだ段データ

        ComboData(Hagine::BaseObject *pMotionTarget, const ComboStepData &stepData)
            : pTarget(pMotionTarget), step(stepData) {}
    };

  private:
    /// ===================================================
    /// private method
    /// ===================================================

    void ExecuteComboAttack();
    void ResetCombo();
    void SaveComboStartPositions();

    /// <summary>
    /// モーション再生対象をすべてコンボ開始姿勢へ戻す
    /// </summary>
    void ReturnAllToComboStart();

    /// ===================================================
    /// private variables
    /// ===================================================

    ComboDefinition definition_; // 読み込んだコンボ定義（JSONとの受け渡し役）

    std::vector<ComboData> comboData_;                     // コンボデータ配列
    std::vector<Hagine::BaseObject *> pComboStartObjects_; // コンボ開始オブジェクト配列

    std::string lastAttackName_; // 直近に発火した攻撃の名前（入力表示UI等が参照）

    // 最終段を出し切ってコンボが終了したあとの硬直時間（秒）。
    // FINAL_RETURN_DELAY（戻りの余韻）が明けてからさらにこの時間だけ次のコンボを受け付けない
    float finishRecovery_ = 0.5f;

    int comboIndex_ = 0;            // 現在のコンボインデックス
    float comboCooldown_ = 0.0f;    // コンボクールダウン
    bool comboStarted_ = false;     // コンボ開始フラグ
    bool waitingForReturn_ = false; // 戻り待ちフラグ
    float returnDelay_ = 0.0f;      // 戻り遅延
    float comboTimeout_ = 0.0f;     // コンボタイムアウトタイマー
    bool inputBuffered_ = false;    // 入力バッファフラグ
    float inputBufferTime_ = 0.0f;  // 入力バッファタイマー

    static const float kComboInterval;        // コンボ間隔
    static const float kInputBufferDuration;  // 入力バッファ有効時間
    static const float kFinalReturnDelay;     // 最終攻撃後の戻り遅延
    static const float kComboTimeoutDuration; // コンボタイムアウト時間

    // 攻撃発火コールバック
    // 引数: damage, knockbackPower, colliderActiveDuration, colliderActivateDelay
    using AttackFiredCallback = std::function<void(float, float, float, float)>;
    AttackFiredCallback onAttackFired_; // 攻撃発火時のコールバック

  public:
    /// ===================================================
    /// public method
    /// ===================================================

    ComboSystem();
    ~ComboSystem();

    /// <summary>
    /// コンボ定義JSON（jsons/ComboDefinition/(ファイル名).json）を読み込んでコンボを構築する
    /// </summary>
    /// <param name="fileName">拡張子を除いたファイル名（"PunchCombo" 等）</param>
    /// <param name="pMotionTarget">段ごとにモーションを再生するオブジェクト（本体アニメーションで再生する場合は nullptr）</param>
    /// <returns>bool: 1段以上構築できれば true</returns>
    bool LoadDefinition(const std::string &fileName, Hagine::BaseObject *pMotionTarget = nullptr);

    /// <summary>
    /// 現在の攻撃パラメータをコンボ定義JSONへ書き戻す
    /// </summary>
    void SaveDefinition();

    /// <summary>
    /// コンボをすべてクリア
    /// </summary>
    void Clear();

    /// <summary>
    /// 攻撃入力時に呼ぶ。実行可能ならコンボを進める
    /// </summary>
    /// <returns>true: 攻撃を実行した / false: クールダウン中などで実行できなかった</returns>
    bool TryExecuteCombo();

    /// <summary>
    /// 毎フレーム呼ぶ更新処理
    /// </summary>
    void Update(float deltaTime);

    /// <summary>
    /// コンボを途中で打ち切る（派生技へ移行したとき等に呼ぶ）。
    /// 開始姿勢へ戻したうえで、出し切ったときと同じ硬直を入れて即再開できないようにする
    /// </summary>
    void Interrupt();

    /// <summary>
    /// 攻撃発火時のコールバックを設定する
    /// PlayerAttackCollider::Activate(...) を呼び出すのに使う
    /// callback(damage, knockbackPower, colliderActiveDuration, colliderActivateDelay)
    /// </summary>
    void SetOnAttackFired(AttackFiredCallback callback) { onAttackFired_ = callback; }

    /// ===================================================
    /// Getter
    /// ===================================================

    bool IsComboActive() const { return comboStarted_; }

    /// <summary>
    /// 現在のコンボで実際に繰り出した段数を取得する（1段目を出したら1）。
    /// GetCurrentComboIndex() は最終段で0へ折り返すため、段数の判定にはこちらを使う
    /// </summary>
    /// <returns>int: 繰り出した段数（コンボ中でなければ0）</returns>
    int GetExecutedStepCount() const;

    bool IsObjectAttackCompleted(Hagine::BaseObject *pTarget) const;
    bool IsCurrentAttackCompleted() const;
    int GetCurrentComboIndex() const { return comboIndex_; }

    /// <summary>
    /// コンボを出し切ったあとの硬直時間（秒）への参照。ImGui・パラメータ調整用
    /// </summary>
    float &GetFinishRecovery() { return finishRecovery_; }
    int GetComboLength() const { return static_cast<int>(comboData_.size()); }

    /// <summary>
    /// 直近に発火した攻撃段の名前を取得する（"Jab" 等）。入力表示UIが参照する
    /// </summary>
    const std::string &GetCurrentAttackName() const { return lastAttackName_; }

    /// <summary>
    /// 指定した段の識別名を取得する（"Jab" 等。ImGuiのラベル等に使う）
    /// </summary>
    /// <param name="index">段のインデックス（0始まり）</param>
    /// <returns>const std::string&amp;: 段の識別名（範囲外なら空文字）</returns>
    const std::string &GetAttackName(int index) const;

    /// <summary>
    /// 指定した段で再生する本体アニメーションのパスを取得する
    /// </summary>
    /// <param name="index">段のインデックス（0始まり）</param>
    /// <returns>const std::string&amp;: アニメーションのパス（範囲外・未設定なら空文字）</returns>
    const std::string &GetAnimationPath(int index) const;

    /// ===================================================
    /// Setter
    /// ===================================================

    /// <summary>
    /// 指定した段で再生する本体アニメーションのパスを設定する（ImGuiでの編集用）
    /// </summary>
    /// <param name="index">段のインデックス（0始まり）</param>
    /// <param name="animationPath">アニメーションのパス（空文字なら再生しない）</param>
    void SetAnimationPath(int index, const std::string &animationPath);

#ifdef _DEBUG
    /// <summary>
    /// ImGuiによる攻撃パラメータ調整UI
    /// 各攻撃のダメージ・ノックバック・コライダータイミングを調整してセーブできる
    /// </summary>
    void DrawImGui();
#endif
};