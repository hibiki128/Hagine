#pragma once
#include"Particle/CSParticle/ParticleCSEmitter.h"
class FadeOut {
  public:
    void Initialize();
    void Update();
    void Draw(const ViewProjection& vp);
    void Finalize();
    void ImGui();

  private:

    std::unique_ptr<ParticleCSEmitter> fadeOut_ = nullptr;
    float timer_ = 0.0f;
};
