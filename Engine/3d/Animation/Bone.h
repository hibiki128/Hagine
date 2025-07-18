#pragma once
#include "Model/ModelStructs.h"
class Bone {
  public:
  private:
    Skeleton skeleton_;

  public:
    void Initialize(ModelData modelData);

    void Update(const Animation &animation, float animtaionTime);

    std::optional<Vector3> GetJointWorldPosition(const std::string &jointName, const Matrix4x4 &worldMatrix) const;

    std::optional<Matrix4x4> GetJointSkeletonSpaceMatrix(const std::string &jointName) const;

    std::optional<Matrix4x4> GetJointWorldMatrix(const std::string &jointName, const Matrix4x4 &worldMatrix) const;
    Skeleton GetSkeleton() { return skeleton_; }
    void SetSkeleton(Skeleton &skeleton) { skeleton_ = skeleton; }

  private:
    /// <summary>
    /// Joint作成
    /// </summary>
    /// <param name="node"></param>
    /// <param name="parent"></param>
    /// <param name="joints"></param>
    /// <returns></returns>
    int32_t CreateJoint(const Node &node, const std::optional<int32_t> &parent, std::vector<Joint> &joints);

    /// <summary>
    /// 骨作成
    /// </summary>
    /// <param name="rootNode"></param>
    /// <returns></returns>
    Skeleton CreateSkeleton(const Node &rootNode);

    /// <summary>
    /// アニメーションの適応
    /// </summary>
    /// <param name="skeleton"></param>
    /// <param name="animation"></param>
    /// <param name="animtionTime"></param>
    void ApplyAnimation(const Animation &animation, float animationTime);
};
