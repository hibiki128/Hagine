#include "Skin.h"
#include <DirectXCommon.h>
#include"Srv/SrvManager.h"
#include <myMath.h>
#include <cassert>
#include"algorithm"

void Skin::Initialize(const Skeleton& skeleton, const ModelData& modelData)
{
	skinCluster_ = CreateSkinCluster(skeleton,modelData);
}

void Skin::Update(const Skeleton& skeleton)
{
	for (size_t jointIndex = 0; jointIndex < skeleton.joints.size(); ++jointIndex) {
		assert(jointIndex < skinCluster_.inverseBindPoseMatrices.size());
		skinCluster_.mappedPalette[jointIndex].skeletonSpaceMatrix =
			skinCluster_.inverseBindPoseMatrices[jointIndex] * skeleton.joints[jointIndex].skeletonSpaceMatrix;
		skinCluster_.mappedPalette[jointIndex].skeletonSpaceInverseTransposeMatrix =
			Transpose(Inverse(skinCluster_.mappedPalette[jointIndex].skeletonSpaceMatrix));
	}
}

SkinCluster Skin::CreateSkinCluster(const Skeleton& skeleton, const ModelData& modelData)
{
	SkinCluster skinCluster;
	DirectXCommon* dxCommon = DirectXCommon::GetInstance();
	SrvManager* srvManager_ = SrvManager::GetInstance();
	// palette用のResourceを確保
	skinCluster.paletteResource = dxCommon->CreateBufferResource(sizeof(WellForGPU) * skeleton.joints.size());
	WellForGPU* mappedPalette = nullptr;
	skinCluster.paletteResource->Map(0, nullptr, reinterpret_cast<void**>(&mappedPalette));
	skinCluster.mappedPalette = { mappedPalette,skeleton.joints.size() };
	skinClusterSrvIndex_ = srvManager_->Allocate() + 1;
	skinCluster.paletteSrvHandle.first = srvManager_->GetCPUDescriptorHandle(skinClusterSrvIndex_);
	skinCluster.paletteSrvHandle.second = srvManager_->GetGPUDescriptorHandle(skinClusterSrvIndex_);

	// palette用のSRVを作成
	srvManager_->CreateSRVforStructuredBuffer(skinClusterSrvIndex_, skinCluster.paletteResource.Get(), UINT(skeleton.joints.size()), sizeof(WellForGPU));


	// influence用のResourceを確保
	skinCluster.influenceResource = dxCommon->CreateBufferResource(sizeof(VertexInfluence) * modelData.mesh.vertices.size());
	VertexInfluence* mappedInfluence = nullptr;
	skinCluster.influenceResource->Map(0, nullptr, reinterpret_cast<void**>(&mappedInfluence));
	std::memset(mappedInfluence, 0, sizeof(VertexInfluence) * modelData.mesh.vertices.size());
	skinCluster.mappedInfluence = { mappedInfluence,modelData.mesh.vertices.size() };

	// Influence用のVBVを作成
	skinCluster.influenceBufferView.BufferLocation = skinCluster.influenceResource->GetGPUVirtualAddress();
	skinCluster.influenceBufferView.SizeInBytes = UINT(sizeof(VertexInfluence) * modelData.mesh.vertices.size());
	skinCluster.influenceBufferView.StrideInBytes = sizeof(VertexInfluence);

	// InverseBindPoseMatrixの保存領域を作成
	skinCluster.inverseBindPoseMatrices.resize(skeleton.joints.size());
	std::generate(skinCluster.inverseBindPoseMatrices.begin(), skinCluster.inverseBindPoseMatrices.end(), []() { return MakeIdentity4x4(); });

	// ModelDataのSkinCluster情報を解析してInfluenceの中身を埋める
	for (const auto& jointWeight : modelData.skinClusterData) { // ModelのSkinClusterの情報を解析
		auto it = skeleton.jointMap.find(jointWeight.first); // jointWeight.firstはjoint名なので、skeletonに対象となるjointが含まれているか判断
		if (it == skeleton.jointMap.end()) { // そんな名前のJointは存在しない。なので次に回す
			continue;
		}
		// (*it).secondにはjointのindexが入っているので、該当のindexのinverseBindPoseMatrixを代入
		skinCluster.inverseBindPoseMatrices[(*it).second] = jointWeight.second.inverseBindPoseMatrix;
		for (const auto& vertexWeight : jointWeight.second.vertexWeights) {
			auto& currentInfluence = skinCluster.mappedInfluence[vertexWeight.vertexIndex]; // 該当のvertexIndexのinfluence情報を参照しておく
			for (uint32_t index = 0; index < kNumMaxInfluence; ++index) { // 空いてるところに入れる
				if (currentInfluence.weights[index] == 0.0f) {// weight==0が空いている状態なので、その場所にweightとjointのindexを代入
					currentInfluence.weights[index] = vertexWeight.weight;
					currentInfluence.jointIndices[index] = (*it).second;
					break;
				}
			}
		}
	}

	return skinCluster;
}
