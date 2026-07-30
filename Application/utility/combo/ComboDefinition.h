#pragma once
#include <string>
#include <vector>

/// <summary>
/// コンボ1段分の定義データ
/// </summary>
struct ComboStepData
{
    std::string attackName;               ///< 段の識別名（"Jab" 等。モーションファイル名としても使う）
    std::string animationPath;            ///< 段ごとに再生する本体アニメーションのパス（空なら再生しない）
    float damage = 10.0f;                 ///< ダメージ量
    float knockbackPower = 3.0f;          ///< ノックバックの強さ
    float colliderActiveDuration = 0.25f; ///< 判定が有効な時間（秒）
    float colliderActivateDelay = 0.08f;  ///< 攻撃開始から判定が有効になるまでの遅延（秒）
};

/// <summary>
/// コンボ全体の定義データ
/// 段の並び・攻撃パラメータ・アニメーションをコードから切り離し、
/// jsons/ComboDefinition/(ファイル名).json で管理する
/// </summary>
class ComboDefinition
{
  public:
    /// ===================================================
    /// public method
    /// ===================================================

    /// <summary>
    /// 定義JSONを読み込む
    /// </summary>
    /// <param name="fileName">拡張子を除いたファイル名（"PunchCombo" 等）</param>
    /// <returns>bool: 1段以上読み込めれば true</returns>
    bool Load(const std::string &fileName);

    /// <summary>
    /// 現在の内容を定義JSONへ書き出す（ImGuiでの調整結果の保存に使う）
    /// </summary>
    /// <returns>bool: 書き出しに成功すれば true</returns>
    bool Save() const;

    /// ===================================================
    /// Getter
    /// ===================================================

    /// <summary>読み込み元のファイル名（拡張子なし）を取得</summary>
    const std::string &GetFileName() const { return fileName_; }

    /// <summary>コンボ段の一覧を取得</summary>
    const std::vector<ComboStepData> &GetSteps() const { return steps_; }

    /// <summary>コンボ段の一覧を取得（書き換え用）</summary>
    std::vector<ComboStepData> &GetSteps() { return steps_; }

    /// <summary>出し切り後の硬直時間（秒）を取得</summary>
    float GetFinishRecovery() const { return finishRecovery_; }

    /// <summary>段が1つも無いかを取得</summary>
    bool IsEmpty() const { return steps_.empty(); }

    /// ===================================================
    /// Setter
    /// ===================================================

    /// <summary>出し切り後の硬直時間（秒）を設定</summary>
    /// <param name="recovery">硬直時間（秒）</param>
    void SetFinishRecovery(float recovery) { finishRecovery_ = recovery; }

  private:
    /// ===================================================
    /// private method
    /// ===================================================

    /// <summary>
    /// ファイル名から定義JSONのフルパスを組み立てる
    /// </summary>
    /// <param name="fileName">拡張子を除いたファイル名</param>
    /// <returns>std::string: JSONファイルのパス</returns>
    static std::string BuildFilePath(const std::string &fileName);

    /// ===================================================
    /// private variables
    /// ===================================================

    static constexpr const char *kFolderName = "ComboDefinition"; ///< jsons 以下の格納フォルダ名
    static constexpr float kDefaultFinishRecovery = 0.5f;         ///< 硬直時間の既定値

    std::string fileName_;                     ///< 読み込み元のファイル名（拡張子なし）
    float finishRecovery_ = kDefaultFinishRecovery; ///< 出し切り後の硬直時間（秒）
    std::vector<ComboStepData> steps_;         ///< コンボ段の一覧
};
