#include "ModelCommon.h"
using namespace Hagine::Graphics;
ModelCommon *ModelCommon::instance = nullptr;

ModelCommon *ModelCommon::GetInstance() {
    if (instance == nullptr) {
        instance = new ModelCommon();
    }
    return instance;
}

void ModelCommon::Finalize() {
    delete instance;
    instance = nullptr;
}

void ModelCommon::Initialize() {
    // 引数で受け取ってメンバ変数に記録する
    dxCommon_ = Core::DirectXCommon::GetInstance();
}