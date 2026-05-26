#pragma once
#include "Object/Base/BaseObject.h"
#include "unordered_map"
class BaseObjectManager {
  private:
    /// ===================================================
    /// private method
    /// ===================================================
    BaseObjectManager() = default;
    ~BaseObjectManager() = default;
    BaseObjectManager(BaseObjectManager &) = delete;
    BaseObjectManager &operator=(BaseObjectManager &) = delete;

  public:
    /// ===================================================
    /// public method
    /// ===================================================
      static BaseObjectManager* GetInstance() {
        static BaseObjectManager instance;
        return &instance;
    }

    void Finalize();

    void RemoveAllObjects();

    void RemoveObjectByName(const std::string &name);

    // 所有権を渡して追加（LoadAll/CreateObject 用）
    void AddObject(std::unique_ptr<BaseObject> baseObject);

    // 非所有登録（シーンが unique_ptr を保持したまま登録する）
    void RegisterExternal(BaseObject* obj);
    void UnregisterExternal(BaseObject* obj);

    void Update();
    void DrawHierarchyEditor();
    void Draw(const ViewProjection &viewProjection);

    void UpdateImGui();

    void SaveAll();

    void LoadAll(std::string sceneName);

    BaseObject *GetObjectByName(const std::string &name);

    // メニューからモーダルを開くためのメソッド
    void OpenSceneSaveModal();
    void OpenSceneLoadModal();
    void OpenObjectCreationModal();
    void OpenObjectLoadModal();

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
    void ShowSaveTargetManager();
  private:
    // 各機能を個別に描画するメソッド
    void DrawSceneSaveModel();
    void DrawSceneLoadModel();
    void DrawObjectCreationModel();
    void DrawObjectLoadModel();
    void LoadObjectFromJson(const std::string &startPath, const std::string &objectName);
    void AddToSaveTargets(const std::string &objectName);
    void RemoveFromSaveTargets(const std::string &objectName);
    void RestoreParentChildRelationshipForObject(BaseObject *object);

    void CreateObject(std::string objectName, std::string modelPath, std::string texturePath = "");

  private:
    // LoadAll/CreateObject が所有するオブジェクト
    std::unordered_map<std::string, std::unique_ptr<BaseObject>> ownedObjects_;
    // Draw/Update/GetObjectByName で使う統合ビュー（所有・外部両方）
    std::unordered_map<std::string, BaseObject*> objects_;

    std::string sceneName_ = "TitleScene";
    std::string objectName_;
    std::string modelPath_;
    std::string texturePath_;
    // モーダルの状態を管理するフラグ
    bool showSceneSaveModal_ = false;
    bool showSceneLoadModal_ = false;
    bool showObjectCreationModal_ = false;
    bool showObjectLoadModal_ = false;
    std::string selectedJsonPath_;
};
