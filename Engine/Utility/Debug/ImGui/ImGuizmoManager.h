#pragma once
#ifdef _DEBUG

#include "imgui.h"
#include "ImGuizmo.h"
#include <Object/Base/BaseObject.h>
#include <string>
#include <unordered_map>
#include <vector>

struct Ray {
    Vector3 origin;
    Vector3 direction;
};

class ImGuizmoManager {
  private:
    static ImGuizmoManager *instance;

    ImGuizmoManager() = default;
    ~ImGuizmoManager() = default;
    ImGuizmoManager(const ImGuizmoManager &) = delete;
    ImGuizmoManager &operator=(const ImGuizmoManager &) = delete;

    // 操作対象一覧（名前付き）
    std::unordered_map<std::string, BaseObject *> transformMap;
    // 選択されているオブジェクト名
    std::string selectedName;

    // カメラのビュープロジェクション（1つで十分）
    const ViewProjection *viewProjection = nullptr;

    // 現在の操作モード
    ImGuizmo::OPERATION currentOperation = ImGuizmo::TRANSLATE;
    // 現在の操作空間
    ImGuizmo::MODE currentMode = ImGuizmo::LOCAL;

  public:
    /// <summary>シングルトンインスタンスの取得</summary>
    static ImGuizmoManager *GetInstance();

    /// <summary>終了処理</summary>
    void Finalize();

    /// <summary>ImGuizmoのフレーム開始</summary>
    void BeginFrame();

    /// <summary>ビュープロジェクションの設定</summary>
    void SetViewProjection(ViewProjection *vp);

    /// <summary>操作対象の追加</summary>
    void AddTarget(const std::string &name, BaseObject *transform);

    /// <summary>ImGui更新処理（sceneWindowの位置・サイズが必要）</summary>
    void imgui();

    void Update(const ImVec2 &scenePosition, const ImVec2 &sceneSize);

    /// <summary>現在選択されているWorldTransformを取得</summary>
    BaseObject *GetSelectedTarget();

    void DeleteTarget() { transformMap.clear(); }
  private:

    void ShowSelectedObjectImGui();
    void DeleteSelectedObject();
    void CollectChildrenForDeletion(BaseObject *obj, std::vector<std::string> &deleteList);
    void HandleMouseSelection(const ImVec2 &scenePosition, const ImVec2 &sceneSize);
    void DisplayGizmo(WorldTransform *transform);
    void DecomposeMatrix(const Matrix4x4 &matrix, Vector3 &position, Quaternion &rotation, Vector3 &scale);
    Ray CreateMouseRay(const ImVec2 &mousePos, const ImVec2 &scenePosition, const ImVec2 &sceneSize);
    bool RayIntersectObject(const Ray &ray, BaseObject *obj, float &distance);
    bool RayIntersectSphere(const Ray &ray, const Vector3 &center, float radius, float &distance);
    bool WorldToScreen(const Vector3 &worldPos, Vector3 &screenPos, const ImVec2 &scenePosition, const ImVec2 &sceneSize);
};

#endif // _DEBUG