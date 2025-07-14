#pragma once
#include "Object/Base/BaseObject.h"

class PlayerHand : public BaseObject {
    /// ==================================
    /// public methods
    /// ==================================
  public:
    void Init(const std::string objectName) override;
    void Update() override;
    void Draw(const ViewProjection &viewProjection, Vector3 offSet = {0.0f, 0.0f, 0.0f}) override;

    /// ==================================
    /// private methods
    /// ==================================
  private:

    /// ==================================
    /// public variables
    /// ==================================
  public:

    /// ==================================
    /// private variables
    /// ==================================
  private:
};
