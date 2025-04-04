#include "ModelAnimation.h"

void ModelAnimation::Initialize(const std::string& directorypath, const std::string& filename)
{
	directorypath_ = directorypath;
	filename_ = filename;
	animator_ = std::make_unique<Animator>();
	bone_ = std::make_unique<Bone>();
	skin_ = std::make_unique<Skin>();
	animator_->Initialize(directorypath_, filename_);

	if (animator_->HaveAnimation()) {
		bone_->Initialize(modelData_);
		skin_->Initialize(bone_->GetSkeleton(), modelData_);
	}
}

void ModelAnimation::Update(bool roop)
{
	if (animator_->HaveAnimation()) {
		animator_->Update(roop);
		bone_->Update(animator_->GetAnimation(), animator_->GetAnimationTime());
		skin_->Update(bone_->GetSkeleton());
	}
}

void ModelAnimation::PlayAnimation()
{
	animator_->SetIsAnimation(true);
	animator_->SetAnimationTime(0.0f);
}
