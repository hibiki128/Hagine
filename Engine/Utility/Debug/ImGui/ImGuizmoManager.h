#pragma once
#ifdef _DEBUG


#include "imgui.h"
#include "ImGuizmo.h"
#include "ViewProjection/ViewProjection.h"
#include "WorldTransform.h"
#include <string>
#include <unordered_map>
#include <vector>
#include"application/Base/BaseObject.h"
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
    void Update(const ImVec2 &scenePosition, const ImVec2 &sceneSize);

    /// <summary>現在選択されているWorldTransformを取得</summary>
    BaseObject *GetSelectedTarget();

    void DeleteTarget() { transformMap.clear(); }

  private:
    void ConvertMatrix4x4ToFloat16(const Matrix4x4 &matrix, float *outMatrix);
    void ConvertFloat16ToMatrix4x4(const float *inMatrix, Matrix4x4 &outMatrix);
    void DecomposeMatrix(WorldTransform *transform);
};

#endif // _DEBUG