#pragma once
#include "Animator.h"
#include "Bone.h"
#include "Skin.h"
#include <memory>
class ModelAnimation {
  private:
    std::unique_ptr<Animator> animator_;
    std::unique_ptr<Bone> bone_;
    std::unique_ptr<Skin> skin_;
    std::string directorypath_;
    std::string filename_;

    ModelData modelData_;

  public:
    void Initialize(const std::string &directorypath, const std::string &filename);

    void Update(bool roop);

    void PlayAnimation();

    void SetModelData(ModelData modelData) { modelData_ = modelData; }
    Skeleton GetSkeletonData() { return bone_->GetSkeleton(); }
    Animator *GetAnimator() { return animator_.get(); }
    Bone *GetBone() { return bone_.get(); }
    Skin *GetSkin() { return skin_.get(); }
    bool IsFinish() { return animator_->IsFinish(); }

    void SetIsAnimation(bool anime) { animator_->SetIsAnimation(anime); }
};
