#pragma once

#include "ParticleEmitter.h"
#include "ParticleGroup.h"
#include "ParticleGroupManager.h"
#include "Camera/ViewProjection/ViewProjection.h"
#include <filesystem>
#include <map>
#include <memory>
#include <string>
#include <vector>

struct ParticleStats {
    size_t count = 0;
    size_t instanceCount = 0; // 同じ名前のエミッター数
};

class ParticleEditor {
  private:
    // シングルトンインスタンス
    static ParticleEditor *instance;
    // プライベートコンストラクタ
    ParticleEditor() = default;
    // コピー禁止
    ParticleEditor(const ParticleEditor &) = delete;
    ParticleEditor &operator=(const ParticleEditor &) = delete;

    // パーティクルエミッター保持用マップ
    std::unordered_map<std::string, std::unique_ptr<ParticleEmitter>> emitters_;
    int selectedEmitterIndex_ = 0;    // 選択されたエミッターのインデックス
    std::string selectedEmitterName_; // 選択されたエミッターの名前

    // パーティクルグループマネージャーポインタ
    ParticleGroupManager *particleGroupManager_ = nullptr;

    std::unordered_map<std::string, ParticleStats> currentFrameStats_; // 現在フレームの統計
    std::unordered_map<std::string, ParticleStats> displayStats_;      // 表示用の統計（前フレーム確定分）
    uint64_t currentFrameNumber_ = 0;
    uint64_t lastUpdateFrame_ = 0;

    // ローカル変数（UIで使用）
    std::string localName_;                         // パーティクルグループ名
    std::string localFileObj_;                      // .objファイルパス
    std::string localTexturePath_;                  // テクスチャパス
    std::string localEmitterName_;                  // エミッター名
    PrimitiveType localType_ = PrimitiveType::None; // プリミティブタイプ

    // CollapsingHeaderの色を定義
    ImVec4 headerColors_[6];

    // ロード関連変数
    bool isLoad_ = false;
    bool statsCleared_ = false;
    bool statsDisplayedThisFrame_ = false;
    std::string name_;
    std::string fileName_;
    std::string texturePath_;

    // カラーテーマの設定
    void SetupColors();

    // カラー付きCollapsingHeader表示関数
    bool ColoredCollapsingHeader(const char *label, int colorIndex);

    // ファイルセレクタ表示関数
    void ShowFileSelector();

    // JSONファイル一覧取得関数
    std::vector<std::string> GetJsonFiles();

  public:
    // インスタンスの取得
    static ParticleEditor *GetInstance();
    // 終了処理
    static void Finalize();
    // 初期化
    void Initialize();
    // パーティクルエミッター追加（名前指定）
    void AddParticleEmitter(const std::string &name);
    // パーティクルエミッター追加（名前・ファイル・テクスチャ指定）
    void AddParticleEmitter(const std::string &name, const std::string &fileName, const std::string &texturePath);
    // パーティクルグループ追加（OBJモデル使用）
    void AddParticleGroup(const std::string &name, const std::string &fileName, const std::string &texturePath);
    // パーティクルグループ追加（プリミティブ使用）
    void AddPrimitiveParticleGroup(const std::string &name, const std::string &texturePath, PrimitiveType type);
    // エミッターの取得
    // std::unique_ptr<ParticleEmitter> GetEmitter(const std::string &name);

    // 外部パーティクル数をセット（シーン側から呼び出し）
    void SetExternalParticleCount(const std::string &name, size_t count);

    void SceneParticleCount();

    void UpdateFrameStats();

    std::unique_ptr<ParticleEmitter> CreateEmitterFromTemplate(const std::string &name);
    // ImGuiエディターの表示
    void EditorWindow();
    // すべてのエミッターを描画
    void DrawAll(const ViewProjection &vp_);
    // すべてのエミッターのデバッグ情報を表示
    void DebugAll();
    // ImGuiエディターの表示処理
    void ShowImGuiEditor();
    // データのロード
    void Load();
};