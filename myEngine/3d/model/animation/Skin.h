#pragma once
#include <cstdint>
#include <ModelStructs.h>
class Skin
{
private:
	SkinCluster skinCluster_;
	uint32_t skinClusterSrvIndex_ = 0;
public:
	void Initialize(const Skeleton& skeleton, const ModelData& modelData);
	void Update(const Skeleton& skeleton);
	uint32_t GetSrvIndex() { return skinClusterSrvIndex_; }
	SkinCluster GetSkinCluster() { return skinCluster_; }
private:
	/// <summary>
	/// SkinClusterの生成
	/// </summary>
	/// <param name="device"></param>
	/// <param name="skeleton"></param>
	/// <param name="modelData"></param>
	/// <param name="descriptorHeap"></param>
	/// <param name="descriptorSize"></param>
	/// <returns></returns>
	SkinCluster CreateSkinCluster(const Skeleton& skeleton, const ModelData& modelData);
};

