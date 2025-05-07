#include "BaseObjectManager.h"

BaseObjectManager *BaseObjectManager::instance = nullptr;

BaseObjectManager *BaseObjectManager::GetInstance() {
    if (instance == nullptr) {
        instance = new BaseObjectManager();
    }
    return instance;
}

void BaseObjectManager::Finalize() {
    delete instance;
    instance = nullptr;
}

void BaseObjectManager::DeleteObject() {
    baseObjects_.clear();
}

void BaseObjectManager::AddObject(std::unique_ptr<BaseObject> baseObject) {
    const std::string &name = baseObject->GetName();
    baseObjects_.emplace(name, std::move(baseObject));
}

void BaseObjectManager::Update() {
    for (auto &[name, obj] : baseObjects_) {
        obj->Update();
    }
}

void BaseObjectManager::Draw(const ViewProjection &viewProjection, Vector3 offSet) {
    for (auto &[name, obj] : baseObjects_) {
        obj->Draw(viewProjection, offSet);
    }
}

void BaseObjectManager::DrawImGui() {
    for (auto &[name, obj] : baseObjects_) {
        obj->DebugImGui();
    }
}

BaseObject *BaseObjectManager::GetObjectByName(const std::string &name) {
    auto it = baseObjects_.find(name);
    if (it != baseObjects_.end()) {
        return it->second.get();
    }
    return nullptr;
}
