#include "Bone.h"
#include <myMath.h>
#include "Animator.h"

void Bone::Initialize(ModelData modelData)
{
	skeleton_ = CreateSkeleton(modelData.rootNode);
}

void Bone::Update(const Animation& animation, float animationTime)
{
	ApplyAnimation(animation, animationTime);
	// すべてのJointを更新。親が若いので通常ループで処理可能
	for (Joint& joint : skeleton_.joints) {
		joint.localMatrix = MakeAffineMatrix(joint.transform.scale, joint.transform.rotate, joint.transform.translate);
		if (joint.parent) { // 親がいれば親の行列を掛ける
			joint.skeletonSpaceMatrix = joint.localMatrix * skeleton_.joints[*joint.parent].skeletonSpaceMatrix;
		}
		else { // 親がいないのでlocalMatrixとskeletonSpaceMatrixは一致する
			joint.skeletonSpaceMatrix = joint.localMatrix;
		}
	}
}

int32_t Bone::CreateJoint(const Node& node, const std::optional<int32_t>& parent, std::vector<Joint>& joints)
{
	Joint joint;
	joint.name = node.name;
	joint.localMatrix = node.localMatrix;
	joint.skeletonSpaceMatrix = MakeIdentity4x4();
	joint.transform = node.transform;
	joint.index = static_cast<int32_t>(joints.size());
	joint.parent = parent;
	joints.push_back(joint);
	for (const Node& child : node.children) {
		int32_t childIndex = CreateJoint(child, joint.index, joints);
		joints[joint.index].children.push_back(childIndex);
	}

	return joint.index;
}

Skeleton Bone::CreateSkeleton(const Node& rootNode)
{
	Skeleton skeleton;
	skeleton.root = CreateJoint(rootNode, {}, skeleton.joints);

	for (const Joint& joint : skeleton.joints) {
		skeleton.jointMap.emplace(joint.name, joint.index);
	}

	return skeleton;
}

void Bone::ApplyAnimation(const Animation& animation, float animationTime)
{
	for (Joint& joint : skeleton_.joints) {
		if (auto it = animation.nodeAnimations.find(joint.name); it != animation.nodeAnimations.end()) {
			const NodeAnimation& rootNodeAnimation = (*it).second;
			joint.transform.translate = Animator::CalculateValue(rootNodeAnimation.translate, animationTime);
			joint.transform.rotate = Animator::CalculateValue(rootNodeAnimation.rotate, animationTime);
			joint.transform.scale = Animator::CalculateValue(rootNodeAnimation.scale, animationTime);
		}
	}
}
