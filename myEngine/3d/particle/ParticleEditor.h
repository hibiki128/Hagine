#pragma once
#include "ParticleEmitter.h"
#include "unordered_map"
#include <memory>
class ParticleEditor {
  private:
    static ParticleEditor *instance;

    ParticleEditor() = default;
    ~ParticleEditor() = default;
    ParticleEditor(ParticleEditor &) = delete;
    ParticleEditor &operator=(ParticleEditor &) = delete;

  public:
    static ParticleEditor *GetInstance();

    void Finalize();

    void Initialize();
    void Load();
    void AddParticleEmitter(const std::string &name);
    void EditorWindow();
    void DrawAll(const ViewProjection &vp_);
    void DebugAll();

    std::unique_ptr<ParticleEmitter> GetEmitter(const std::string &name);

  private:
    void ShowImGuiEditor();
    void ShowFileSelector();
    void AddParticleEmitter(const std::string &name, const std::string &fileName, const std::string &texturePath);
    std::vector<std::string> GetJsonFiles();

  private:
    std::string localFileObj_;
    std::string localTexturePath_;
    std::string localFileTexture_;
    std::string localName_;
    std::string name_;
    std::string fileName_;
    std::string texturePath_;
    std::unordered_map<std::string, std::unique_ptr<ParticleEmitter>> emitters_;
    bool isLoad_ = false;
};
