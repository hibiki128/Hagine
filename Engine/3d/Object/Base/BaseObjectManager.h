#pragma once
#include "Object/Base/BaseObject.h"
#include "unordered_map"
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

    void RemoveAllObjects();

    void AddObject(std::unique_ptr<BaseObject> baseObject);

    void Update();
    void DrawImGui();
    void Draw(const ViewProjection &viewProjection, Vector3 offSet = {0.0f, 0.0f, 0.0f});

    void UpdateImGui();

    void SaveAll();

    void LoadAll(std::string sceneName);

    BaseObject *GetObjectByName(const std::string &name);

    // メニューからモーダルを開くためのメソッド
    void OpenSceneSaveModal();
    void OpenSceneLoadModal();
    void OpenObjectCreationModal();

    /// ===================================================
    /// 親子付け関連
    /// ===================================================

    void ShowParentChildHierarchy();
    void ShowObjectHierarchy(BaseObject *obj, int depth);
    void SetParentChild(const std::string &childName, const std::string &parentName);
    void RemoveParentChild(const std::string &childName);
    std::vector<std::string> GetObjectNames() const;

    // 親子関係の保存・読み込み
    void SaveAllParentChildRelationships();
    void LoadAllParentChildRelationships();
    void RemoveObject(const std::string &name);
  private:
    // 各機能を個別に描画するメソッド
    void DrawSceneSaveModal();
    void DrawSceneLoadModal();
    void DrawObjectCreationModal();

    void CreateObject(std::string objectName, std::string modelPath, std::string texturePath = "");

  private:
    std::unordered_map<std::string, std::unique_ptr<BaseObject>> baseObjects_;
    std::string sceneName_ = "TitleScene";
    std::string objectName_;
    std::string modelPath_;
    std::string texturePath_;
    // モーダルの状態を管理するフラグ
    bool showSceneSaveModal_ = false;
    bool showSceneLoadModal_ = false;
    bool showObjectCreationModal_ = false;
};
