#pragma once
#include <string>

class Player;
class Input;

// ============================================================
//  チュートリアルのステップ定義
// ============================================================
enum class TutorialStep : int {
    Move = 0,      ///< 01. 地上移動
    Jump,          ///< 02. ジャンプ
    FlyTransition, ///< 03. 空中 → ホバリング移行
    Ascend,        ///< 04. 上昇（飛行中）
    Descend,       ///< 05. 下降（飛行中）※着地補正あり
    AirMove,       ///< 06. 空中移動
    Dash,          ///< 07. ダッシュ
    Rush,          ///< 08. 急接近（エネミー必要）
    Landing,       ///< 09. 空中状態解除・着地
    MeleeAttack,   ///< 10. 近接攻撃（エネミー必要）
    RangedAttack,  ///< 11. 遠距離攻撃（エネミー必要）
    ChargeAttack,  ///< 12. チャージ攻撃（エネミー必要）
    EnergyCharge,  ///< 13. エネルギーチャージ
    SpecialAttack, ///< 14. 必殺技
    Complete,      ///< 完了
    StepCount      ///< 配列サイズ用（値として使用しない）
};

// ============================================================
//  ステップごとの設定データ
// ============================================================
struct TutorialStepConfig {
    const char *instructionText; ///< 操作指示テキスト（UI表示用 / Dash は GetInstructionText() で上書き）
    const char *subText;         ///< 補足テキスト（nullptr = 非表示）
    float requiredTime;          ///< 操作の必要保持秒数（0 なら回数方式）
    int requiredCount;           ///< 操作の必要回数（0 なら時間方式）
    bool needsEnemy;             ///< このステップでエネミーが必要か
};

// ============================================================
//  TutorialSystem
//  チュートリアルの進行ロジック・入力判定を担当する
//  UI 表示は TutorialUI クラスに委譲する
// ============================================================
class TutorialSystem {
  public:
    /// ===================================================
    /// public method
    /// ===================================================

    /// <summary>
    /// 初期化（TutorialScene::Initialize 内で呼ぶ）
    /// </summary>
    /// <param name="player">プレイヤーへのポインタ</param>
    void Initialize(Player *player);

    /// <summary>
    /// 毎フレーム更新（TutorialScene::Update 内で呼ぶ）
    /// </summary>
    /// <param name="dt">デルタタイム</param>
    void Update(float dt);

    /// <summary>
    /// 終了処理
    /// </summary>
    void Finalize();

    /// ===================================================
    /// Getter
    /// ===================================================

    /// <summary>
    /// 現在のステップを取得
    /// </summary>
    /// <returns>TutorialStep: 現在のステップ</returns>
    TutorialStep GetCurrentStep() const { return currentStep_; }

    /// <summary>
    /// 現ステップの進捗 0.0 〜 1.0（進行度バーに使う）
    /// </summary>
    /// <returns>float: 進捗</returns>
    float GetProgress() const { return progress_; }

    /// <summary>
    /// チュートリアル全体が完了したか
    /// </summary>
    /// <returns>bool: 完了フラグ</returns>
    bool IsComplete() const { return currentStep_ == TutorialStep::Complete; }

    /// <summary>
    /// 操作指示テキストを返す
    /// </summary>
    /// <returns>const char*: 操作指示テキスト</returns>
    const char *GetInstructionText() const;

    /// <summary>
    /// 補足テキストを返す
    /// </summary>
    /// <returns>const char*: 補足テキスト</returns>
    const char *GetSubText() const;

    /// <summary>
    /// エネミー出現リクエストがあるか
    /// </summary>
    /// <returns>bool: リクエストフラグ</returns>
    bool ShouldSpawnEnemy() const { return spawnEnemyRequested_; }

    /// <summary>
    /// エネミー消滅リクエストがあるか
    /// </summary>
    /// <returns>bool: リクエストフラグ</returns>
    bool ShouldDespawnEnemy() const { return despawnEnemyRequested_; }

    /// <summary>
    /// 出現リクエストを消費する
    /// </summary>
    void ConsumeSpawnRequest() { spawnEnemyRequested_ = false; }

    /// <summary>
    /// 消滅リクエストを消費する
    /// </summary>
    void ConsumeDespawnRequest() { despawnEnemyRequested_ = false; }

    /// <summary>
    /// ステップが切り替わった直後か
    /// </summary>
    /// <returns>bool: 切り替わりフラグ</returns>
    bool IsStepJustChanged() const { return stepJustChanged_; }

    /// <summary>
    /// 「空中に戻れ」メッセージを表示すべきか
    /// </summary>
    /// <returns>bool: 表示フラグ</returns>
    bool IsShowingReturnToAirMessage() const { return showReturnToAirMessage_; }

    /// <summary>
    /// デバッグ描画
    /// </summary>
    void DrawImGui();

  private:
    /// ===================================================
    /// private method
    /// ===================================================

    /// <summary>
    /// 次のステップへ進む
    /// </summary>
    void AdvanceStep();

    /// <summary>
    /// ステップ切替時の内部変数初期化
    /// </summary>
    void ResetStepState();

    /// <summary>
    /// 現ステップの進行チェック＆進捗更新
    /// </summary>
    /// <param name="dt">デルタタイム</param>
    void UpdateCurrentStep(float dt);

    /// <summary>
    /// エネミー出現リクエストを発行
    /// </summary>
    void RequestSpawnEnemy();

    /// <summary>
    /// エネミー消滅リクエストを発行
    /// </summary>
    void RequestDespawnEnemy();

    // ステップ個別チェック
    bool CheckMove();
    bool CheckJump();
    bool CheckFlyTransition();
    bool CheckAscend();
    bool CheckDescend();
    bool CheckAirMove();
    bool CheckDash();
    bool CheckRush();
    bool CheckLanding();
    bool CheckMeleeAttack();
    bool CheckRangedAttack();
    bool CheckChargeAttack(float dt);
    bool CheckEnergyCharge();
    bool CheckSpecialAttack();

    // 入力判定ヘルパー
    bool IsMoveInput() const;
    bool IsJumpTrigger() const;
    bool IsAirTransTrigger() const;
    bool IsAscendInput() const;
    bool IsDescendInput() const;
    bool IsEnergyChargeInput() const;
    bool IsMeleeInput() const;
    bool IsRangedTrigger() const;

  private:
    /// ===================================================
    /// private variants
    /// ===================================================

    Player *player_ = nullptr; // プレイヤー
    Input *input_ = nullptr;   // 入力マネージャ

    TutorialStep currentStep_ = TutorialStep::Move; // 現在のステップ
    float progress_ = 0.0f; // 現ステップの進捗 0.0〜1.0
    float timer_ = 0.0f;    // 時間方式の蓄積秒数
    int count_ = 0;         // 回数方式の蓄積回数

    bool stepJustChanged_ = false;       // ステップ切り替わりフラグ
    bool spawnEnemyRequested_ = false;   // エネミー出現リクエスト
    bool despawnEnemyRequested_ = false; // エネミー消滅リクエスト

    // ── Descend ステップ専用 ──
    bool showReturnToAirMessage_ = false; // 「空中に戻れ」補正メッセージ表示中
    bool wasGroundedLastFrame_ = false;   // 前フレームの接地フラグ

    // ── ChargeAttack ステップ専用 ──
    float chargeHoldTimer_ = 0.0f;   // J/Y を押し続けている時間
    bool chargeInputActive_ = false; // 現在チャージ入力中か

    // ── フレームをまたぐ状態保持 ──
    std::string prevStateName_; // 1フレーム前のプレイヤーステート名

    /// ステップ設定テーブル
    static const TutorialStepConfig kConfigs[static_cast<int>(TutorialStep::StepCount)];
};