#include "Animator.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <cassert>
#include <myEngine/Frame/Frame.h>
#include <myMath.h>

std::unordered_map<std::string, Animation> Animator::animationCache;

void Animator::Initialize(const std::string &directorypath, const std::string &filename) {
    haveAnimation = false;
    directorypath_ = directorypath;
    filename_ = filename;

    animation_ = LoadAnimationFile(directorypath_, filename_);
}

void Animator::Update(bool roop) {
    if (isAnimation_) {
        if (roop) {
            // ループアニメーションの場合、アニメーション時間を進めて、超えたら最初に戻る
            animationTime += Frame::DeltaTime();
            animationTime = std::fmod(animationTime, animation_.duration); // 終わったら戻る
        } else {
            // ループしない場合、アニメーションが終了するまで進行
            if (animationTime < animation_.duration) {
                isFinish_ = false;
                animationTime += Frame::DeltaTime();
                // 時間がdurationを超えたら停止（アニメーションの終了時刻を超えないように）
                if (animationTime > animation_.duration) {
                    animationTime = animation_.duration;
                    // アニメーションが終了した時の処理
                    isAnimation_ = false; // ここでアニメーション終了を示す
                    isFinish_ = true;
                }
            }
        }
    }
}

Animation Animator::LoadAnimationFile(const std::string &directoryPath, const std::string &filename) {
    // キャッシュキーの生成
    std::string filePath = directoryPath + "/" + filename;
    Animation animation;
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(filePath.c_str(), 0);
    if (!scene || scene->mNumAnimations == 0) {
        haveAnimation = false;
        return animation; // アニメーションなし
    }

    haveAnimation = true;

    // キャッシュチェック
    auto it = animationCache.find(filePath);
    if (it != animationCache.end()) {
        return it->second; // すでに読み込み済みならキャッシュから返す
    }

    aiAnimation *animationAssimp = scene->mAnimations[0];
    animation.duration = float(animationAssimp->mDuration / animationAssimp->mTicksPerSecond);

    // ノードアニメーションの読み込み
    for (uint32_t channelIndex = 0; channelIndex < animationAssimp->mNumChannels; ++channelIndex) {
        aiNodeAnim *nodeAnimationAssimp = animationAssimp->mChannels[channelIndex];
        NodeAnimation &nodeAnimation = animation.nodeAnimations[nodeAnimationAssimp->mNodeName.C_Str()];

        // Position
        for (uint32_t keyIndex = 0; keyIndex < nodeAnimationAssimp->mNumPositionKeys; ++keyIndex) {
            aiVectorKey &keyAssimp = nodeAnimationAssimp->mPositionKeys[keyIndex];
            KeyframeVector3 keyframe;
            keyframe.time = float(keyAssimp.mTime / animationAssimp->mTicksPerSecond);
            keyframe.value = {-keyAssimp.mValue.x, keyAssimp.mValue.y, keyAssimp.mValue.z};
            nodeAnimation.translate.push_back(keyframe);
        }

        // Rotation
        for (uint32_t keyIndex = 0; keyIndex < nodeAnimationAssimp->mNumRotationKeys; ++keyIndex) {
            aiQuatKey &keyAssimp = nodeAnimationAssimp->mRotationKeys[keyIndex];
            KeyframeQuaternion keyframe;
            keyframe.time = float(keyAssimp.mTime / animationAssimp->mTicksPerSecond);
            keyframe.value = {keyAssimp.mValue.x, -keyAssimp.mValue.y, -keyAssimp.mValue.z, keyAssimp.mValue.w};
            nodeAnimation.rotate.push_back(keyframe);
        }

        // Scale
        for (uint32_t keyIndex = 0; keyIndex < nodeAnimationAssimp->mNumScalingKeys; ++keyIndex) {
            aiVectorKey &keyAssimp = nodeAnimationAssimp->mScalingKeys[keyIndex];
            KeyframeVector3 keyframe;
            keyframe.time = float(keyAssimp.mTime / animationAssimp->mTicksPerSecond);
            keyframe.value = {keyAssimp.mValue.x, keyAssimp.mValue.y, keyAssimp.mValue.z};
            nodeAnimation.scale.push_back(keyframe);
        }
    }

    // キャッシュに登録
    animationCache[filePath] = animation;
    return animation;
}

Vector3 Animator::CalculateValue(const std::vector<KeyframeVector3> &keyframes, float time) {
    assert(!keyframes.empty());                               // キーがないものは返す値がわからないのでダメ
    if (keyframes.size() == 1 || time <= keyframes[0].time) { // キーが一つまたは時刻がキーフレーム前なら最初の値とする
        return keyframes[0].value;
    }

    for (size_t index = 0; index < keyframes.size() - 1; ++index) {
        size_t nextIndex = index + 1;
        // indexとnextIndexの二つのキーフレームを取得して範囲内に時刻があるか確認
        if (keyframes[index].time <= time && time <= keyframes[nextIndex].time) {
            // 範囲内を保管する
            float t = (time - keyframes[index].time) / (keyframes[nextIndex].time - keyframes[index].time);
            return Lerp(keyframes[index].value, keyframes[nextIndex].value, t);
        }
    }
    // ここまできた場合は1番後の時刻よりも後ろなので最後の値を返す
    return (*keyframes.rbegin()).value;
}

Quaternion Animator::CalculateValue(const std::vector<KeyframeQuaternion> &keyframes, float time) {
    assert(!keyframes.empty()); // キーフレームが空でないことを確認
    if (keyframes.size() == 1 || time <= keyframes[0].time) {
        // キーフレームが一つしかないか、時刻が最初のキーフレームより前なら最初の値を返す
        return keyframes[0].value;
    }

    for (size_t index = 0; index < keyframes.size() - 1; ++index) {
        size_t nextIndex = index + 1;
        // indexとnextIndexの二つのキーフレームを取得して範囲内に時刻があるか確認
        if (keyframes[index].time <= time && time <= keyframes[nextIndex].time) {
            // 時刻が範囲内の場合は補間を行う
            float t = (time - keyframes[index].time) / (keyframes[nextIndex].time - keyframes[index].time);
            return Slerp(keyframes[index].value, keyframes[nextIndex].value, t);
        }
    }

    // ここまで来た場合は最後の時刻よりも後ろなので最後の値を返す
    return (*keyframes.rbegin()).value;
}