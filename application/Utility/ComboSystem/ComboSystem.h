#pragma once
#include <string>
#include <vector>

class BaseObject;   
class MotionEditor; 

class ComboSystem {
  private:
    struct ComboData {
        BaseObject *target;
        std::string attackData;

        ComboData(BaseObject *obj, const std::string &attack)
            : target(obj), attackData(attack) {}
    };

    std::vector<ComboData> comboData_;

    // コンボの状態管理
    int comboIndex_;
    float comboCooldown_;
    bool comboStarted_;
    bool waitingForReturn_;
    float returnDelay_;
    float comboTimeout_;
    bool inputBuffered_;
    float inputBufferTime_;

    static const float COMBO_INTERVAL;
    static const float INPUT_BUFFER_DURATION;
    static const float FINAL_RETURN_DELAY;
    static const float COMBO_TIMEOUT_DURATION;

    // 初期位置を保存するためのマップ
    std::vector<BaseObject *> comboStartObjects_;

    void ExecuteComboAttack();
    void ResetCombo();
    void SaveComboStartPositions();

  public:
    ComboSystem();
    ~ComboSystem();

    // チェーン形式でコンボを追加
    ComboSystem &Add(BaseObject *target, const std::string &attackData);

    // コンボをクリア
    void Clear();

    // コンボを実行（入力時に呼び出し）
    bool TryExecuteCombo();

    // 毎フレーム呼び出し（時間管理用）
    void Update(float deltaTime);

    // 現在のコンボ状態を取得
    bool IsComboActive() const { return comboStarted_; }
    int GetCurrentComboIndex() const { return comboIndex_; }
    int GetComboLength() const { return static_cast<int>(comboData_.size()); }
    // 特定のオブジェクトの攻撃が完全に終了したかチェック
    bool IsObjectAttackCompleted(BaseObject *target) const;

    // 現在のコンボの攻撃が完全に終了したかチェック
    bool IsCurrentAttackCompleted() const;
};
