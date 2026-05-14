#pragma once
#ifdef _DEBUG

#include "imgui.h"
#include "ImGuizmo.h"
#include <Input.h>
#include <Object/Base/BaseObject.h>
#include <Transform/WorldTransform.h>
#include <algorithm>
#include <cmath>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

// -----------------------------------------------------------------------
// ギズモ操作対象を型に依存せず統一的に扱うためのラッパー構造体
// BaseObject・WorldTransform・Vector3直接参照の3種類に対応する
// -----------------------------------------------------------------------
struct GizmoTarget {
    enum class Type {
        BaseObject,     // BaseObject* を持つオブジェクト
        WorldTransform, // WorldTransform* のみを持つオブジェクト
        FreeTransform,  // Vector3* の直接参照（Sprite や ParticleEmitter など）
    };

    Type type = Type::BaseObject;
    std::string name;
    bool selectable = true;        // ギズモによる選択を許可するか
    bool isScreenSpace = false;    // スクリーン空間座標（ピクセル単位）かどうか（Sprite用）
    float screenHitRadius = 50.0f; // 2Dマウス選択の当たり判定半径（スプライト座標系ピクセル単位）

    // Type::BaseObject 用
    BaseObject *baseObject = nullptr;

    // Type::WorldTransform 用
    WorldTransform *worldTransform = nullptr;

    // Type::FreeTransform 用（各変換成分のメンバ変数への直接ポインタ）
    Vector3 *translate = nullptr; // 平行移動
    Vector3 *rotate = nullptr;    // 回転（オイラー角、ラジアン）
    Vector3 *scale = nullptr;     // スケール

    // ImGui 詳細表示コールバック（nullptr の場合はデフォルト表示）
    std::function<void()> imguiCallback;

    // ワールド行列を構築して返す
    Matrix4x4 GetWorldMatrix() const;
    // ワールド座標（位置成分）を返す
    Vector3 GetWorldPosition() const;
    // 平行移動デルタを各型に応じて適用する
    void ApplyTranslationDelta(const Vector3 &delta);
    // ImGui で変換詳細を表示する
    void ShowImGui();
};

// -----------------------------------------------------------------------

class ImGuizmoManager {
  private:
    ImGuizmoManager() = default;
    ~ImGuizmoManager() = default;
    ImGuizmoManager(const ImGuizmoManager &) = delete;
    ImGuizmoManager &operator=(const ImGuizmoManager &) = delete;

    // 操作対象一覧（名前付き、GizmoTarget で型を統一管理）
    std::unordered_map<std::string, GizmoTarget> transformMap;
    // 選択されているオブジェクト名のセット
    std::unordered_set<std::string> selectedNames;

    std::vector<BaseObject *> copiedObjects; // コピー対象（BaseObject のみ）

    bool isMultiSelecting = false;
    bool isDrawDebug_ = true;

    // カメラのビュープロジェクション
    const ViewProjection *viewProjection = nullptr;

    // 現在の操作モード・座標空間
    ImGuizmo::OPERATION currentOperation = ImGuizmo::TRANSLATE;
    ImGuizmo::MODE currentMode = ImGuizmo::LOCAL;

    bool showDebugRaycast = true;
    bool showDebugAABB = true;
    bool showDebugSphere = true;
    bool showDebugHitPoints = true;
    char searchBuffer_[256] = "";
    std::vector<std::string> filteredNames_;

  public:
    static ImGuizmoManager *GetInstance() {
        static ImGuizmoManager instance;
        return &instance;
    }

    void Finalize();
    void BeginFrame();
    void SetViewProjection(ViewProjection *vp);

    // ---- AddTarget オーバーロード群 ----

    /// BaseObject を登録する（既存の使い方）
    void AddTarget(const std::string &name, BaseObject *object, bool selectable = true);

    /// WorldTransform のみを持つオブジェクトを登録する
    /// imguiCallback を渡すと ImGui 表示をカスタマイズできる
    void AddTarget(const std::string &name, WorldTransform *worldTransform,
                   bool selectable = true,
                   std::function<void()> imguiCallback = nullptr);

    /// Vector3 ポインタを直接指定して登録する（Sprite・ParticleEmitter など）
    /// translate は必須、rotate・scale は nullptr 可（ない場合は非表示）
    /// imguiCallback を渡すと ImGui 表示をカスタマイズできる
    void AddTarget(const std::string &name,
                   Vector3 *translate,
                   Vector3 *rotate = nullptr,
                   Vector3 *scale = nullptr,
                   bool selectable = true,
                   std::function<void()> imguiCallback = nullptr);

    void imgui();
    void Update(const ImVec2 &scenePosition, const ImVec2 &sceneSize);

    /// 現在選択されている最初のオブジェクトを返す（BaseObject のみ対応）
    BaseObject *GetSelectedTarget();
    /// 選択中の全 BaseObject を返す（非 BaseObject エントリは除外）
    std::vector<BaseObject *> GetSelectedTargets();

    void DeleteTarget() { transformMap.clear(); }

    void CopySelectedObjects();
    void PasteObjects();
    void DeleteSelectedObjects();

    void DrawSelectedObjectHighlight();
    void DrawSelectionMarker(const Vector3 &worldPosition);
    void UpdateFilteredNames();

    // ギズモの選択状態をセット
    // selectable が false になった場合は、現在の選択状態からも除外する
    void SetSelectable(const std::string &name, bool selectable) {
        auto it = transformMap.find(name);
        if (it != transformMap.end()) {
            it->second.selectable = selectable;
            if (!selectable) {
                selectedNames.erase(name);
            }
        }
    }

    // スクリーン空間フラグと2Dヒット半径を設定する（Sprite登録後に呼ぶ）
    void SetScreenSpace(const std::string &name, bool isScreenSpace, float hitRadius = 50.0f) {
        auto it = transformMap.find(name);
        if (it != transformMap.end()) {
            it->second.isScreenSpace = isScreenSpace;
            it->second.screenHitRadius = hitRadius;
        }
    }

    // ギズモの選択状態を取得
    bool GetSelectable(const std::string &name) {
        if (transformMap.find(name) != transformMap.end()) {
            return transformMap[name].selectable;
        }
        return false;
    }

    // オブジェクト削除時にギズモからも消すためのメソッド
    void RemoveTarget(const std::string &name) {
        transformMap.erase(name);
    }

  private:
    void ShowSelectedObjectImGui();
    void HandleMouseSelection(const ImVec2 &scenePosition, const ImVec2 &sceneSize);
    // GizmoTarget を受け取ってギズモを表示・操作する
    void DisplayGizmo();
    void DecomposeMatrix(const Matrix4x4 &matrix, Vector3 &position, Quaternion &rotation, Vector3 &scale);
    bool WorldToScreen(const Vector3 &worldPos, Vector3 &screenPos, const ImVec2 &scenePosition, const ImVec2 &sceneSize);

    std::string GenerateUniqueName(const std::string &baseName);

    void DrawDebugRaycast();
    void DrawAABBWireframe(const Matrix4x4 &worldMatrix, const Vector4 &color);
    void DrawSphereWireframe(const Matrix4x4 &worldMatrix, const Vector4 &color);
    // 行列を直接受け取るレイヒット描画（GizmoTarget が BaseObject 以外の場合にも対応）
    void TestAndDrawRayHit(const Ray &ray, const GizmoTarget &target);
    // sceneSize を追加（スクリーン空間ギズモの描画に必要）
    void DisplayGizmo(const ImVec2 &scenePosition, const ImVec2 &sceneSize);

    RayHitInfo hitInfo;
};

#endif // _DEBUG