#pragma once
#ifdef _DEBUG

#include "imgui.h"
#include "ImGuizmo.h"
#include <Input.h>
#include <Object/Base/BaseObject.h>
#include <algorithm> // std::max用
#include <cmath>     // std::sqrt用
#include <string>
#include <unordered_map>
#include <vector>

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
    std::unordered_set<std::string> selectedNames;
    BaseObject *copiedObject = nullptr;
    std::vector<BaseObject *> copiedObjects; // 複数コピー対応

     bool isMultiSelecting = false;

     bool isDrawDebug_ = true;

    // カメラのビュープロジェクション（1つで十分）
    const ViewProjection *viewProjection = nullptr;

    // 現在の操作モード
    ImGuizmo::OPERATION currentOperation = ImGuizmo::TRANSLATE;
    // 現在の操作空間
    ImGuizmo::MODE currentMode = ImGuizmo::LOCAL;

    bool showDebugRaycast = true;
    bool showDebugAABB = true;      // AABB表示ON/OFF
    bool showDebugSphere = true;    // Sphere表示ON/OFF
    bool showDebugHitPoints = true; // ヒット点表示ON/OFF
    char searchBuffer_[256] = "";   // 検索用バッファ
    std::vector<std::string> filteredNames_; // フィルタされた名前リスト

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

    std::vector<BaseObject *> GetSelectedTargets();

    /// <summary>選択中のオブジェクトをコピー</summary>
  //  void CopySelectedObject();

    /// <summary>コピーしたオブジェクトをペースト</summary>
  //  void PasteObject();

    void DeleteTarget() { transformMap.clear(); }

    //void DeleteSelectedObject();

    void CopySelectedObjects();

    void PasteObjects();

    void DeleteSelectedObjects();

    void DrawSelectedObjectHighlight();

    void DrawSelectionMarker(const Vector3 &worldPosition);

    void UpdateFilteredNames();

  private:
    void ShowSelectedObjectImGui();
    void HandleMouseSelection(const ImVec2 &scenePosition, const ImVec2 &sceneSize);
    void DisplayGizmo(WorldTransform *transform);
    void DecomposeMatrix(const Matrix4x4 &matrix, Vector3 &position, Quaternion &rotation, Vector3 &scale);
    bool WorldToScreen(const Vector3 &worldPos, Vector3 &screenPos, const ImVec2 &scenePosition, const ImVec2 &sceneSize);

    /// <summary>ユニークな名前を生成</summary>
    std::string GenerateUniqueName(const std::string &baseName);

    void DrawDebugRaycast();
    void DrawAABBWireframe(const Matrix4x4 &worldMatrix, const Vector4 &color);
    void DrawSphereWireframe(const Matrix4x4 &worldMatrix, const Vector4 &color);
    void TestAndDrawRayHit(const Ray &ray, BaseObject *targetObject, const std::string &objectName);

    RayHitInfo hitInfo;
};

#endif // _DEBUG