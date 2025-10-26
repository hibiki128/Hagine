#pragma once
#include <string>
#include <vector>
class BaseObject;
class MotionEditor;

/// <summary>
/// コンボシステムを管理するクラス
/// 連続攻撃のシーケンスと時間管理を制御
/// </summary>
class ComboSystem {
  private:
    /// ===================================================
    /// private struct
    /// ===================================================

    /// <summary>
    /// コンボデータ構造体
    /// </summary>
    struct ComboData {
        BaseObject *target;     // 対象オブジェクト
        std::string attackData; // 攻撃データ

        /// <summary>
        /// コンストラクタ
        /// </summary>
        /// <param name="obj">対象オブジェクトのポインタ</param>
        /// <param name="attack">攻撃データ</param>
        ComboData(BaseObject *obj, const std::string &attack)
            : target(obj), attackData(attack) {}
    };

  private:
    /// ===================================================
    /// private method
    /// ===================================================

    /// <summary>
    /// コンボ攻撃を実行
    /// </summary>
    void ExecuteComboAttack();

    /// <summary>
    /// コンボをリセット
    /// </summary>
    void ResetCombo();

    /// <summary>
    /// コンボ開始時のオブジェクト位置を保存
    /// </summary>
    void SaveComboStartPositions();

  private:
    /// ===================================================
    /// private varians
    /// ===================================================

    std::vector<ComboData> comboData_;

    int comboIndex_;        // 現在のコンボインデックス
    float comboCooldown_;   // コンボクールダウン
    bool comboStarted_;     // コンボ開始フラグ
    bool waitingForReturn_; // 復帰待機フラグ
    float returnDelay_;     // 復帰遅延
    float comboTimeout_;    // コンボタイムアウト
    bool inputBuffered_;    // 入力バッファフラグ
    float inputBufferTime_; // 入力バッファ時間

    static const float COMBO_INTERVAL;         // コンボ間隔
    static const float INPUT_BUFFER_DURATION;  // 入力バッファ時間
    static const float FINAL_RETURN_DELAY;     // 最終復帰遅延
    static const float COMBO_TIMEOUT_DURATION; // コンボタイムアウト時間

    std::vector<BaseObject *> comboStartObjects_; // コンボ開始オブジェクト

  public:
    /// ===================================================
    /// public method
    /// ===================================================

    /// <summary>
    /// コンストラクタ
    /// </summary>
    ComboSystem();

    /// <summary>
    /// デストラクタ
    /// </summary>
    ~ComboSystem();

    /// <summary>
    /// チェーン形式でコンボを追加
    /// </summary>
    /// <param name="target">対象オブジェクトのポインタ</param>
    /// <param name="attackData">攻撃データ</param>
    /// <returns>ComboSystem&: チェーン用の参照</returns>
    ComboSystem &Add(BaseObject *target, const std::string &attackData);

    /// <summary>
    /// コンボをクリア
    /// </summary>
    void Clear();

    /// <summary>
    /// コンボを実行
    /// 入力時に呼び出される
    /// </summary>
    /// <returns>bool: 実行可能かどうか</returns>
    bool TryExecuteCombo();

    /// <summary>
    /// 時間管理用の更新
    /// 毎フレーム呼び出し
    /// </summary>
    /// <param name="deltaTime">フレームの経過時間</param>
    void Update(float deltaTime);

    /// <summary>
    /// Getter
    /// </summary>
    bool IsComboActive() const { return comboStarted_; }
    bool IsObjectAttackCompleted(BaseObject *target) const;
    bool IsCurrentAttackCompleted() const;
    int GetCurrentComboIndex() const { return comboIndex_; }
    int GetComboLength() const { return static_cast<int>(comboData_.size()); }
};