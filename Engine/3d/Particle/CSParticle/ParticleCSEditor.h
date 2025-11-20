#pragma once
#include "Particle/ParticleStruct.h"
#include "ParticleCSEmitter.h"
#include "ParticleCSGroupManager.h"
#include "map"
#include <unordered_map>
class ParticleCSEditor {
  private:
    /// ===================================
    /// private methods
    /// ===================================

    // シングルトンインスタンス
    static ParticleCSEditor *instance;
    // プライベートコンストラクタ
    ParticleCSEditor() = default;
    // コピー禁止
    ParticleCSEditor(const ParticleCSEditor &) = delete;
    ParticleCSEditor &operator=(const ParticleCSEditor &) = delete;

  private:
    /// ===================================
    /// private variaus
    /// ===================================

    std::unordered_map<std::string, std::unique_ptr<ParticleCSEmitter>> emitters_;
    int selectedEmitterIndex_ = 0;
    std::string selectedEmitterName_;

    ParticleCSGroupManager *particleGroupManager_ = nullptr;

    std::string localName_;
    std::string localFileObj_;
    std::string localTexturePath_;
    std::string localEmitterName_;
    int localMaxParticleCount_ = 1000;
    PrimitiveType localType_ = PrimitiveType::None;

#ifdef USE_IMGUI
    // CollapsingHeaderの色を定義
    ImVec4 headerColors_[6];
#endif // USE_IMGUI

    bool isLoad_ = false;
    std::string name_;
    std::string fileName_;
    std::string texturePath_;
    int maxParticleCount_ = 1000;

  private:
    void SetupColors();

    bool ColoredCollapsingHeader(const char *label, int colorIndex);

    void ShowFileSelector();

    std::vector<std::string> GetJsonFiles();
    std::string localEmitterModelPath_;
    PrimitiveType localEmitterType_ = PrimitiveType::None;

  public:
    // インスタンスの取得
    static ParticleCSEditor *GetInstance();
    // 終了処理
    static void Finalize();
    // 初期化
    void Initialize();
    // パーティクルエミッター追加（名前指定）
    void AddParticleEmitter(const std::string &name);
    void AddParticleEmitter(const std::string &name, const std::string &modelPath);
    void AddParticleEmitter(const std::string &name, PrimitiveType primitiveType);
    // パーティクルグループ追加（OBJモデル使用）
    void AddParticleGroup(const std::string &name, const std::string &fileName, uint32_t maxParticleCount, const std::string &texturePath);
    // パーティクルグループ追加（プリミティブ使用）
    void AddPrimitiveParticleGroup(const std::string &name, PrimitiveType type, uint32_t maxParticleCount, const std::string &texturePath);
    std::unique_ptr<ParticleCSEmitter> CreateEmitterFromTemplate(const std::string &name);
    void ShowGPUParticleStatistics();
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
