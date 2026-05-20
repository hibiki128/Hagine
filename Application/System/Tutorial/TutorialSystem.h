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
    /// 初期化（TutorialScene::Initialize 内で呼ぶ）
    void Initialize(Player *player);

    /// 毎フレーム更新（TutorialScene::Update 内で呼ぶ）
    void Update(float dt);

    /// 終了処理
    void Finalize();

    // ─────────────── Getter ───────────────

    TutorialStep GetCurrentStep() const { return currentStep_; }

    /// 現ステップの進捗 0.0 〜 1.0（進行度バーに使う）
    float GetProgress() const { return progress_; }

    /// チュートリアル全体が完了したか
    bool IsComplete() const { return currentStep_ == TutorialStep::Complete; }

    /// 操作指示テキストを返す（ダッシュのサブフェーズに応じて切り替わる）
    const char *GetInstructionText() const;

    /// 補足テキストを返す（着地補正中の「空中に戻ろう」など。nullptr = 非表示）
    const char *GetSubText() const;

    /// ─── エネミー出現/消滅リクエスト ───
    /// TutorialScene::Update 内でフラグを確認し、エネミーを表示/非表示にする

    bool ShouldSpawnEnemy() const { return spawnEnemyRequested_; }
    bool ShouldDespawnEnemy() const { return despawnEnemyRequested_; }

    /// TutorialScene 側で出現処理を済ませたら呼ぶ
    void ConsumeSpawnRequest() { spawnEnemyRequested_ = false; }
    /// TutorialScene 側で消滅処理を済ませたら呼ぶ
    void ConsumeDespawnRequest() { despawnEnemyRequested_ = false; }

    /// ステップが切り替わった直後のフレームのみ true（UIアニメーション起動などに使う）
    bool IsStepJustChanged() const { return stepJustChanged_; }

    /// 「着地してしまったので空中に戻れ」補正メッセージを表示すべき状態か
    bool IsShowingReturnToAirMessage() const { return showReturnToAirMessage_; }

    /// ImGui デバッグウィンドウを表示する（TutorialScene::AddSceneSetting などから呼ぶ）
    void DrawImGui();

  private:
    // ─────────────── 進行制御 ───────────────

    void AdvanceStep();               ///< 次のステップへ進む
    void ResetStepState();            ///< ステップ切替時の内部変数初期化
    void UpdateCurrentStep(float dt); ///< 現ステップの進行チェック＆進捗更新

    void RequestSpawnEnemy();   ///< エネミー出現リクエストを発行
    void RequestDespawnEnemy(); ///< エネミー消滅リクエストを発行

    // ─────────────── ステップ個別チェック ───────────────
    // 戻り値: 今フレームに「進行条件を満たしているか」
    //   時間方式  → 条件中ずっと true を返す（蓄積する）
    //   回数方式  → 達成した瞬間のみ true を返す（1回インクリメント）

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
    bool CheckChargeAttack(float dt); ///< J/Y 長押し→離す の計測を内包
    bool CheckEnergyCharge();
    bool CheckSpecialAttack();

    // ─────────────── 入力判定ヘルパー ───────────────

    bool IsMoveInput() const;         ///< WASD / 左スティック傾き
    bool IsJumpTrigger() const;       ///< SPACE(トリガー) / Aボタン
    bool IsAirTransTrigger() const;   ///< SPACE(トリガー) / RBボタン（空中）
    bool IsAscendInput() const;       ///< SPACE保持 / RT
    bool IsDescendInput() const;      ///< LSHIFT保持 / LT（非チャージ時）
    bool IsEnergyChargeInput() const; ///< C保持 / LT保持
    bool IsMeleeInput() const;        ///< K(トリガー) / Bボタン
    bool IsRangedTrigger() const;     ///< J(トリガー) / Yボタン離し

  private:
    Player *player_ = nullptr;
    Input *input_ = nullptr;

    TutorialStep currentStep_ = TutorialStep::Move;
    float progress_ = 0.0f; ///< 現ステップの進捗 0.0〜1.0
    float timer_ = 0.0f;    ///< 時間方式の蓄積秒数
    int count_ = 0;         ///< 回数方式の蓄積回数

    bool stepJustChanged_ = false;
    bool spawnEnemyRequested_ = false;
    bool despawnEnemyRequested_ = false;

    // ── Descend ステップ専用 ──
    bool showReturnToAirMessage_ = false; ///< 「空中に戻れ」補正メッセージ表示中
    bool wasGroundedLastFrame_ = false;

    // ── ChargeAttack ステップ専用 ──
    float chargeHoldTimer_ = 0.0f;   ///< J/Y を押し続けている時間
    bool chargeInputActive_ = false; ///< 現在チャージ入力中か

    // ── フレームをまたぐ状態保持 ──
    std::string prevStateName_; ///< 1フレーム前のプレイヤーステート名

    /// ステップ設定テーブル（TutorialStep::StepCount 分）
    static const TutorialStepConfig kConfigs[static_cast<int>(TutorialStep::StepCount)];
};