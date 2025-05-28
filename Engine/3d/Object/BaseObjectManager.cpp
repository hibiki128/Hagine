#include "BaseObjectManager.h"
#include"ImGui/ImGuizmoManager.h"

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
    ImGuizmoManager::GetInstance()->DeleteTarget();
}

void BaseObjectManager::AddObject(std::unique_ptr<BaseObject> baseObject) {
    const std::string &name = baseObject->GetName();
    ImGuizmoManager::GetInstance()->AddTarget(baseObject->GetName(),baseObject.get());
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

void BaseObjectManager::DrawWireframe(const ViewProjection &viewProjection, Vector3 offSet) {
    for (auto &[name, obj] : baseObjects_) {
        obj->DrawWireframe(viewProjection, offSet);
    }
}

void BaseObjectManager::DrawImGui() {
    for (auto &[name, obj] : baseObjects_) {
        obj->ImGui();
    }
}

BaseObject *BaseObjectManager::GetObjectByName(const std::string &name) {
    auto it = baseObjects_.find(name);
    if (it != baseObjects_.end()) {
        return it->second.get();
    }
    return nullptr;
}
