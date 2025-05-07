#pragma once
#include "application/Base/BaseObject.h"
#include"unordered_map"
class BaseObjectManager {
  private:
    /// ===================================================
    /// private method
    /// ===================================================
    static BaseObjectManager *instance;
    BaseObjectManager() = default;
    ~BaseObjectManager() = default;
    BaseObjectManager(BaseObjectManager &) = delete;
    BaseObjectManager &operator=(BaseObjectManager &) = delete;

  public:
    /// ===================================================
    /// public method
    /// ===================================================
    static BaseObjectManager *GetInstance();

    void Finalize();

    void DeleteObject();

    void AddObject(std::unique_ptr<BaseObject> baseObject);

    void Update();

    void Draw(const ViewProjection &viewProjection, Vector3 offSet = {0.0f, 0.0f, 0.0f});

    void DrawImGui();

    BaseObject *GetObjectByName(const std::string &name);

  private:
    std::unordered_map<std::string, std::unique_ptr<BaseObject>> baseObjects_;
};
