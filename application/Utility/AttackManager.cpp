#include "AttackManager.h"
#include "imgui.h"
#include <Line/DrawLine3D.h>

AttackManager *AttackManager::instance = nullptr;

AttackManager *AttackManager::GetInstance() {
    if (instance == nullptr) {
        instance = new AttackManager;
    }
    return instance;
}

void AttackManager::Finalize() {
    delete instance;
    instance = nullptr;
}

// 登録処理
void AttackManager::Register(BaseObject *object) {
    if (!object)
        return;
    std::string name = object->GetName();
    motions_[name] = AttackMotion();
    motions_[name].target = object;
    motions_[name].objectName = name;
}

// イージング適用ヘルパー関数
float ApplyEasing(EasingType type, float t, float total) {
    switch (type) {
    case EasingType::EaseInSine:
        return EaseInSine(0.0f, 1.0f, t, total);
    case EasingType::EaseOutSine:
        return EaseOutSine(0.0f, 1.0f, t, total);
    case EasingType::EaseInOutSine:
        return EaseInOutSine(0.0f, 1.0f, t, total);
    case EasingType::EaseInBack:
        return EaseInBack(0.0f, 1.0f, t, total);
    case EasingType::EaseOutBack:
        return EaseOutBack(0.0f, 1.0f, t, total);
    case EasingType::EaseInOutBack:
        return EaseInOutBack(0.0f, 1.0f, t, total);
    case EasingType::EaseInQuint:
        return EaseInQuint(0.0f, 1.0f, t, total);
    case EasingType::EaseOutQuint:
        return EaseOutQuint(0.0f, 1.0f, t, total);
    case EasingType::EaseInOutQuint:
        return EaseInOutQuint(0.0f, 1.0f, t, total);
    case EasingType::EaseInCirc:
        return EaseInCirc(0.0f, 1.0f, t, total);
    case EasingType::EaseOutCirc:
        return EaseOutCirc(0.0f, 1.0f, t, total);
    case EasingType::EaseInOutCirc:
        return EaseInOutCirc(0.0f, 1.0f, t, total);
    case EasingType::EaseInExpo:
        return EaseInExpo(0.0f, 1.0f, t, total);
    case EasingType::EaseOutExpo:
        return EaseOutExpo(0.0f, 1.0f, t, total);
    case EasingType::EaseInOutExpo:
        return EaseInOutExpo(0.0f, 1.0f, t, total);
    case EasingType::EaseOutCubic:
        return EaseOutCubic(0.0f, 1.0f, t, total);
    case EasingType::EaseInCubic:
        return EaseInCubic(0.0f, 1.0f, t, total);
    case EasingType::EaseInOutCubic:
        return EaseInOutCubic(0.0f, 1.0f, t, total);
    case EasingType::EaseInQuad:
        return EaseInQuad(0.0f, 1.0f, t, total);
    case EasingType::EaseOutQuad:
        return EaseOutQuad(0.0f, 1.0f, t, total);
    case EasingType::EaseInOutQuad:
        return EaseInOutQuad(0.0f, 1.0f, t, total);
    case EasingType::EaseInQuart:
        return EaseInQuart(0.0f, 1.0f, t, total);
    case EasingType::EaseOutQuart:
        return EaseOutQuart(0.0f, 1.0f, t, total);
    case EasingType::EaseInBounce:
        return EaseInBounce(0.0f, 1.0f, t, total);
    case EasingType::EaseOutBounce:
        return EaseOutBounce(0.0f, 1.0f, t, total);
    case EasingType::EaseInOutBounce:
        return EaseInOutBounce(0.0f, 1.0f, t, total);
    case EasingType::EaseInElastic:
        return EaseInElastic(0.0f, 1.0f, t, total);
    case EasingType::EaseOutElastic:
        return EaseOutElastic(0.0f, 1.0f, t, total);
    case EasingType::EaseInOutElastic:
        return EaseInOutElastic(0.0f, 1.0f, t, total);
    default:
        return t;
    }
}

void AttackManager::Update(float deltaTime) {
    for (auto &[name, motion] : motions_) {
        if (!motion.target) {
            continue;
        }

        // 再生中でない場合の処理
        if (!motion.isPlaying) {
            // 経過時間が0で初期位置が記録されている場合は何もしない（固定しない）
            continue;
        }

        // 初期位置の記録（再生開始時に一度だけ）
        if (!motion.hasInitialTransform) {
            motion.initialPos = motion.target->GetWorldPosition();
            motion.initialRot = motion.target->GetWorldRotation();
            motion.initialScale = motion.target->GetWorldScale();
            motion.hasInitialTransform = true;
        }

        motion.currentTime += deltaTime;

        // 再生終了チェック
        if (motion.currentTime >= motion.totalTime) {
            motion.currentTime = motion.totalTime;
            motion.isPlaying = false;

            // 終了コールバック実行
            if (motion.onFinished) {
                motion.onFinished();
                motion.onFinished = nullptr;
            }
            continue;
        }

        float t = motion.currentTime / motion.totalTime;

        if (motion.useCatmullRom && motion.controlPoints.size() >= 4) {
            // Catmull-Rom曲線による補間（ワールド座標）
            Vector3 localOffset = CatmullRomInterpolation(motion.controlPoints, t);

            // ローカルオフセットをワールド座標に変換
            Vector3 worldOffset = TransformLocalToWorld(localOffset, motion.target->GetWorldTransform().matWorld_);
            motion.target->GetWorldPosition() = motion.basePos + worldOffset;
        } else {
            // 従来のイージング補間（ワールド座標対応）
            float easedT = ApplyEasing(motion.easingType, t, 1.0f);

            // ローカルオフセットをワールド座標に変換
            Vector3 startWorldOffset = TransformLocalToWorld(motion.startPosOffset, motion.target->GetWorldTransform().matWorld_);
            Vector3 endWorldOffset = TransformLocalToWorld(motion.endPosOffset, motion.target->GetWorldTransform().matWorld_);

            Vector3 actualStartPos = motion.basePos + startWorldOffset;
            Vector3 actualEndPos = motion.basePos + endWorldOffset;

            motion.target->GetWorldPosition() = Lerp(actualStartPos, actualEndPos, easedT);
        }

        // 回転とスケールは従来通り（ローカル座標系で適用）
        float easedT = ApplyEasing(motion.easingType, t, 1.0f);
        motion.target->GetWorldRotation() = Lerp(motion.actualStartRot, motion.actualEndRot, easedT);
        motion.target->GetWorldScale() = Lerp(motion.actualStartScale, motion.actualEndScale, easedT);

        // コライダーON/OFF
        bool enable = motion.currentTime >= motion.colliderOnTime && motion.currentTime <= motion.colliderOffTime;
        motion.target->SetCollisionEnabled(enable);
    }

    DrawControlPoints();
    DrawCatmullRomCurve();
}

Vector3 AttackManager::TransformLocalToWorld(const Vector3 &localOffset, const Matrix4x4 &worldMatrix) {
    // 方向ベクトルとして変換（w=0で平行移動成分を無視）
    Vector4 localOffset4 = {localOffset.x, localOffset.y, localOffset.z, 0.0f};

    // 単純な行列・ベクトル積（正規化なし）
    Vector4 worldOffset4;
    worldOffset4.x = localOffset4.x * worldMatrix.m[0][0] + localOffset4.y * worldMatrix.m[1][0] +
                     localOffset4.z * worldMatrix.m[2][0] + localOffset4.w * worldMatrix.m[3][0];
    worldOffset4.y = localOffset4.x * worldMatrix.m[0][1] + localOffset4.y * worldMatrix.m[1][1] +
                     localOffset4.z * worldMatrix.m[2][1] + localOffset4.w * worldMatrix.m[3][1];
    worldOffset4.z = localOffset4.x * worldMatrix.m[0][2] + localOffset4.y * worldMatrix.m[1][2] +
                     localOffset4.z * worldMatrix.m[2][2] + localOffset4.w * worldMatrix.m[3][2];

    return {worldOffset4.x, worldOffset4.y, worldOffset4.z};
}

// 登録済みオブジェクトの再生
void AttackManager::Play(const std::string &objectName, std::function<void()> onFinished) {
    auto it = motions_.find(objectName);
    if (it == motions_.end()) {
        return; // オブジェクトが登録されていない
    }

    AttackMotion &motion = it->second;
    if (!motion.target) {
        return; // ターゲットが無効
    }

    // 初期位置の記録（まだ記録されていない場合のみ）
    if (!motion.hasInitialTransform) {
        motion.initialPos = motion.target->GetWorldPosition();
        motion.initialRot = motion.target->GetWorldRotation();
        motion.initialScale = motion.target->GetWorldScale();
        motion.hasInitialTransform = true;
    }

    // 再生開始時点の現在のTransformを基準として保存
    motion.basePos = motion.target->GetWorldPosition();
    motion.baseRot = motion.target->GetWorldRotation();
    motion.baseScale = motion.target->GetWorldScale();

    // 従来の実際の開始・終了値を計算（ローカルオフセット用）
    motion.actualStartRot = motion.baseRot + motion.startRotOffset;
    motion.actualEndRot = motion.baseRot + motion.endRotOffset;
    motion.actualStartScale = motion.baseScale + motion.startScaleOffset;
    motion.actualEndScale = motion.baseScale + motion.endScaleOffset;

    // 再生開始
    motion.currentTime = 0.0f;
    motion.isPlaying = true;
    motion.onFinished = onFinished;
}

// ファイルから読み込んで任意のオブジェクトで再生
void AttackManager::PlayFromFile(BaseObject *target, const std::string &fileName, std::function<void()> onFinished) {
    if (!target) {
        return;
    }

    // 一時的なモーションデータを作成
    std::string tempName = "temp_" + target->GetName() + "_" + fileName;

    DataHandler data("AttackData", fileName);
    AttackMotion &motion = motions_[tempName];

    motion.target = target;
    motion.objectName = tempName;
    motion.totalTime = data.Load("/totalTime", 1.0f);
    motion.colliderOnTime = data.Load("/colliderOnTime", 0.3f);
    motion.colliderOffTime = data.Load("/colliderOffTime", 0.6f);
    motion.startPosOffset = data.Load<Vector3>("/startPosOffset", {});
    motion.endPosOffset = data.Load<Vector3>("/endPosOffset", {});
    motion.startRotOffset = data.Load<Vector3>("/startRotOffset", {});
    motion.endRotOffset = data.Load<Vector3>("/endRotOffset", {});
    motion.startScaleOffset = data.Load<Vector3>("/startScaleOffset", {0, 0, 0});
    motion.endScaleOffset = data.Load<Vector3>("/endScaleOffset", {0, 0, 0});
    int easingInt = data.Load("/easingType", 0);
    motion.easingType = static_cast<EasingType>(easingInt);

    // 初期位置の記録
    motion.initialPos = target->GetWorldPosition();
    motion.initialRot = target->GetWorldRotation();
    motion.initialScale = target->GetWorldScale();
    motion.hasInitialTransform = true;

    // 再生開始時点の現在のTransformを基準として保存
    motion.basePos = target->GetWorldPosition();
    motion.baseRot = target->GetWorldRotation();
    motion.baseScale = target->GetWorldScale();

    // 従来の実際の開始・終了値を計算（ローカルオフセット用）
    motion.actualStartRot = motion.baseRot + motion.startRotOffset;
    motion.actualEndRot = motion.baseRot + motion.endRotOffset;
    motion.actualStartScale = motion.baseScale + motion.startScaleOffset;
    motion.actualEndScale = motion.baseScale + motion.endScaleOffset;

    // 再生開始
    motion.currentTime = 0.0f;
    motion.isPlaying = true;
    motion.onFinished = [this, tempName, onFinished]() {
        if (onFinished)
            onFinished();
        motions_.erase(tempName); // 一時データを削除
    };
}

void AttackManager::Stop(const std::string &objectName) {
    auto it = motions_.find(objectName);
    if (it != motions_.end()) {
        AttackMotion &motion = it->second;
        motion.isPlaying = false;
        motion.currentTime = 0.0f;
        motion.onFinished = nullptr;

        // 初期位置に戻すが固定はしない
        if (motion.hasInitialTransform && motion.target) {
            motion.target->GetWorldPosition() = motion.initialPos;
            motion.target->GetWorldRotation() = motion.initialRot;
            motion.target->GetWorldScale() = motion.initialScale;
        }
    }
}

// 指定オブジェクトの再生停止
void AttackManager::StopAll() {
    for (auto &[name, motion] : motions_) {
        motion.isPlaying = false;
        motion.currentTime = 0.0f;
        motion.onFinished = nullptr;

        // 初期位置に戻すが固定はしない
        if (motion.hasInitialTransform && motion.target) {
            motion.target->GetWorldPosition() = motion.initialPos;
            motion.target->GetWorldRotation() = motion.initialRot;
            motion.target->GetWorldScale() = motion.initialScale;
        }
    }
}

void AttackManager::ResetInitialPosition(const std::string &objectName) {
    auto it = motions_.find(objectName);
    if (it != motions_.end() && it->second.target) {
        AttackMotion &motion = it->second;
        motion.initialPos = motion.target->GetWorldPosition();
        motion.initialRot = motion.target->GetWorldRotation();
        motion.initialScale = motion.target->GetWorldScale();
        motion.hasInitialTransform = true;
    }
}

Vector3 AttackManager::CatmullRomInterpolation(const std::vector<Vector3> &points, float t) {
    if (points.size() < 2)
        return Vector3{0, 0, 0};

    // 2つまたは3つの制御点の場合は線形補間
    if (points.size() == 2) {
        return Lerp(points[0], points[1], t);
    }
    if (points.size() == 3) {
        if (t < 0.5f) {
            return Lerp(points[0], points[1], t * 2.0f);
        } else {
            return Lerp(points[1], points[2], (t - 0.5f) * 2.0f);
        }
    }

    // 4つ以上の制御点がある場合のCatmull-Rom補間
    int numSegments = static_cast<int>(points.size() - 1); // セグメント数を修正
    float segmentT = t * (numSegments - 2);                // 実際に使用するセグメント範囲を調整
    int segment = (int)segmentT;
    float localT = segmentT - segment;

    // 範囲チェック
    if (segment < 0) {
        segment = 0;
        localT = 0.0f;
    }
    if (segment >= numSegments - 2) {
        segment = numSegments - 3;
        localT = 1.0f;
    }

    // インデックス計算（最初と最後の制御点も使用するように修正）
    int i0 = segment;
    int i1 = segment + 1;
    int i2 = segment + 2;
    int i3 = segment + 3;

    // 境界処理
    if (i0 < 0)
        i0 = 0;
    if (i3 >= (int)points.size())
        i3 = static_cast<int>(points.size() - 1);

    Vector3 p0 = points[i0];
    Vector3 p1 = points[i1];
    Vector3 p2 = points[i2];
    Vector3 p3 = points[i3];

    float t2 = localT * localT;
    float t3 = t2 * localT;

    Vector3 result;
    result.x = 0.5f * ((2.0f * p1.x) + (-p0.x + p2.x) * localT +
                       (2.0f * p0.x - 5.0f * p1.x + 4.0f * p2.x - p3.x) * t2 +
                       (-p0.x + 3.0f * p1.x - 3.0f * p2.x + p3.x) * t3);
    result.y = 0.5f * ((2.0f * p1.y) + (-p0.y + p2.y) * localT +
                       (2.0f * p0.y - 5.0f * p1.y + 4.0f * p2.y - p3.y) * t2 +
                       (-p0.y + 3.0f * p1.y - 3.0f * p2.y + p3.y) * t3);
    result.z = 0.5f * ((2.0f * p1.z) + (-p0.z + p2.z) * localT +
                       (2.0f * p0.z - 5.0f * p1.z + 4.0f * p2.z - p3.z) * t2 +
                       (-p0.z + 3.0f * p1.z - 3.0f * p2.z + p3.z) * t3);

    return result;
}

void AttackManager::DrawControlPoints() {
    if (selectedName_.empty())
        return;

    AttackMotion &motion = motions_[selectedName_];
    if (!motion.target || !motion.useCatmullRom || motion.controlPoints.empty())
        return;

    DrawLine3D *drawLine = DrawLine3D::GetInstance();
    const float cubeSize = 0.4f;

    // 基準位置を取得（経過時間が0.0fの時のみ現在位置、それ以外は basePos）
    Vector3 basePos;
    if (motion.currentTime == 0.0f) {
        basePos = motion.target->GetWorldPosition(); // 経過時間0.0fの時は現在位置を使用
    } else {
        basePos = motion.basePos; // 再生中・再生後は再生開始時の位置を固定で使用
    }

    for (size_t i = 0; i < motion.controlPoints.size(); ++i) {
        // ローカルオフセットをワールド座標に変換
        Vector3 localOffset = motion.controlPoints[i];
        Vector3 worldOffset = TransformLocalToWorld(localOffset, motion.target->GetWorldTransform().matWorld_);
        Vector3 worldPos = basePos + worldOffset;

        // 制御点の種類に応じて色を変更
        Vector4 controlPointColor;
        if (i == 0) {
            controlPointColor = {0.0f, 1.0f, 0.0f, 1.0f}; // 最初の点：緑
        } else if (i == motion.controlPoints.size() - 1) {
            controlPointColor = {0.0f, 0.0f, 1.0f, 1.0f}; // 最後の点：青
        } else {
            controlPointColor = {1.0f, 0.0f, 0.0f, 1.0f}; // 中間点：赤
        }

        // 選択中の制御点は明るくする
        if ((int)i == selectedControlPoint_) {
            controlPointColor.x = std::min(controlPointColor.x + 0.5f, 1.0f);
            controlPointColor.y = std::min(controlPointColor.y + 0.5f, 1.0f);
            controlPointColor.z = std::min(controlPointColor.z + 0.5f, 1.0f);
        }

        // 立方体で制御点を描画
        drawLine->DrawSphere(worldPos, controlPointColor, cubeSize, 8);

        // 制御点番号表示用の小さな線（上向き）
        Vector3 numberPos = worldPos;
        numberPos.y += cubeSize + 0.2f;
        drawLine->SetPoints(worldPos, numberPos, controlPointColor);
    }
}

void AttackManager::DrawCatmullRomCurve() {
    if (selectedName_.empty())
        return;

    AttackMotion &motion = motions_[selectedName_];
    if (!motion.target || !motion.useCatmullRom || motion.controlPoints.size() < 2)
        return;

    DrawLine3D *drawLine = DrawLine3D::GetInstance();
    const Vector4 curveColor = {1.0f, 0.5f, 0.0f, 1.0f}; // オレンジ色
    const int curveResolution = 100;                     // 曲線の解像度

    // 基準位置を取得（経過時間が0.0fの時のみ現在位置、それ以外は basePos）
    Vector3 basePos;
    if (motion.currentTime == 0.0f) {
        basePos = motion.target->GetWorldPosition(); // 経過時間0.0fの時は現在位置を使用
    } else {
        basePos = motion.basePos; // 再生中・再生後は再生開始時の位置を固定で使用
    }

    // 曲線の描画
    Vector3 prevLocalOffset = CatmullRomInterpolation(motion.controlPoints, 0.0f);
    Vector3 prevWorldOffset = TransformLocalToWorld(prevLocalOffset, motion.target->GetWorldTransform().matWorld_);
    Vector3 prevWorldPoint = basePos + prevWorldOffset;

    for (int i = 1; i <= curveResolution; ++i) {
        float t = (float)i / curveResolution;
        Vector3 currentLocalOffset = CatmullRomInterpolation(motion.controlPoints, t);
        Vector3 currentWorldOffset = TransformLocalToWorld(currentLocalOffset, motion.target->GetWorldTransform().matWorld_);
        Vector3 currentWorldPoint = basePos + currentWorldOffset;

        drawLine->SetPoints(prevWorldPoint, currentWorldPoint, curveColor);
        prevWorldPoint = currentWorldPoint;
    }

    // デバッグ用：制御点間を直線で結ぶ（薄い色）
    const Vector4 debugLineColor = {0.3f, 0.3f, 0.3f, 1.0f}; // 薄いグレー
    for (size_t i = 0; i < motion.controlPoints.size() - 1; ++i) {
        Vector3 localOffset1 = motion.controlPoints[i];
        Vector3 localOffset2 = motion.controlPoints[i + 1];

        Vector3 worldOffset1 = TransformLocalToWorld(localOffset1, motion.target->GetWorldTransform().matWorld_);
        Vector3 worldOffset2 = TransformLocalToWorld(localOffset2, motion.target->GetWorldTransform().matWorld_);

        Vector3 worldPoint1 = basePos + worldOffset1;
        Vector3 worldPoint2 = basePos + worldOffset2;

        drawLine->SetPoints(worldPoint1, worldPoint2, debugLineColor);
    }

    // 基準位置を球で表示（経過時間0.0fの時は現在位置、それ以外は固定位置）
    const Vector4 basePosColor = {1.0f, 1.0f, 1.0f, 1.0f}; // 白色
    drawLine->DrawSphere(basePos, basePosColor, 0.2f, 32);

    // 再生中の場合、現在のオブジェクト位置も別の色で表示
    if (motion.isPlaying) {
        Vector3 currentPos = motion.target->GetWorldPosition();
        const Vector4 currentPosColor = {1.0f, 1.0f, 0.0f, 1.0f}; // 黄色
        drawLine->DrawSphere(currentPos, currentPosColor, 0.15f, 32);

        // 基準位置と現在位置を線で結ぶ
        const Vector4 connectionColor = {0.5f, 0.5f, 0.5f, 1.0f}; // グレー
        drawLine->SetPoints(basePos, currentPos, connectionColor);
    }
}

void AttackManager::DrawImGui() {
    ImGui::Begin("アタックマネージャ");

    if (!motions_.empty()) {
        std::vector<const char *> names;
        for (auto &[name, _] : motions_)
            names.push_back(name.c_str());
        int index = 0;
        for (size_t i = 0; i < names.size(); ++i) {
            if (names[i] == selectedName_)
                index = (int)i;
        }
        if (ImGui::Combo("対象オブジェクト", &index, names.data(), (int)names.size())) {
            selectedName_ = names[index];
        }
    }

    if (!selectedName_.empty()) {
        AttackMotion &m = motions_[selectedName_];

        ImGui::SliderFloat("現在の経過時間", &m.currentTime, 0.0f, m.totalTime);
        ImGui::DragFloat("トータル時間", &m.totalTime, 0.1f, 0.1f, 30.0f);
        ImGui::DragFloat("コライダーON時間", &m.colliderOnTime, 0.01f);
        ImGui::DragFloat("コライダーOFF時間", &m.colliderOffTime, 0.01f);

        // 再生制御ボタン
        if (ImGui::Button("再生")) {
            Play(selectedName_);
        }
        ImGui::SameLine();
        if (ImGui::Button("停止")) {
            Stop(selectedName_);
        }
        ImGui::SameLine();
        if (ImGui::Button("全停止")) {
            StopAll();
        }
        // モーション方式選択
        ImGui::SeparatorText("モーション方式");
        bool useCatmullRom = m.useCatmullRom;
        if (ImGui::Checkbox("Catmull-Rom曲線を使用", &useCatmullRom)) {
            m.useCatmullRom = useCatmullRom;
        }

        if (m.useCatmullRom) {
            // Catmull-Rom制御点UI
            ImGui::SeparatorText("Catmull-Rom制御点");

            // 制御点追加・削除
            if (ImGui::Button("制御点追加")) {
                Vector3 newPoint = {0, 0, 0};
                m.controlPoints.push_back(newPoint);
            }
            ImGui::SameLine();
            if (ImGui::Button("制御点削除") && selectedControlPoint_ >= 0 &&
                selectedControlPoint_ < (int)m.controlPoints.size()) {
                m.controlPoints.erase(m.controlPoints.begin() + selectedControlPoint_);
                selectedControlPoint_ = -1;
            }
            ImGui::SameLine();
            if (ImGui::Button("全削除")) {
                m.controlPoints.clear();
                selectedControlPoint_ = -1;
            }

            // 制御点リスト表示・選択
            ImGui::Text("制御点数: %d", (int)m.controlPoints.size());
            for (int i = 0; i < (int)m.controlPoints.size(); ++i) {
                bool isSelected = (selectedControlPoint_ == i);
                if (ImGui::Selectable(("制御点 " + std::to_string(i + 1)).c_str(), isSelected)) {
                    selectedControlPoint_ = i;
                }
            }

            // 選択中の制御点編集
            if (selectedControlPoint_ >= 0 && selectedControlPoint_ < (int)m.controlPoints.size()) {
                Vector3 &point = m.controlPoints[selectedControlPoint_];
                ImGui::Text("制御点 %d の編集", selectedControlPoint_ + 1);
                ImGui::DragFloat3("位置", &point.x, 0.1f);

                if (ImGui::Button("現在のオブジェクト位置に設定") && m.target) {
                    point = m.target->GetWorldPosition();
                }
            }

            if (m.controlPoints.size() < 4) {
                ImGui::TextColored(ImVec4(1, 0, 0, 1), "※ Catmull-Rom曲線には最低4つの制御点が必要です");
            }
        } else {

            ImGui::SeparatorText("開始 / 終了のTransform（相対オフセット）");
            ImGui::Text("※ 0,0,0 = 現在の値、-1,0,0 = 現在のX座標-1の位置");
            ImGui::DragFloat3("開始位置オフセット", &m.startPosOffset.x, 0.1f);
            ImGui::DragFloat3("終了位置オフセット", &m.endPosOffset.x, 0.1f);
            ImGui::DragFloat3("開始回転オフセット", &m.startRotOffset.x, 0.1f);
            ImGui::DragFloat3("終了回転オフセット", &m.endRotOffset.x, 0.1f);
            ImGui::DragFloat3("開始スケールオフセット", &m.startScaleOffset.x, 0.1f);
            ImGui::DragFloat3("終了スケールオフセット", &m.endScaleOffset.x, 0.1f);

            // 現在の基準値表示（読み取り専用）
            if (m.target) {
                ImGui::SeparatorText("現在の基準Transform（読み取り専用）");
                Vector3 currentPos = m.target->GetWorldPosition();
                Vector3 currentRot = m.target->GetWorldRotation();
                Vector3 currentScale = m.target->GetWorldScale();
                ImGui::Text("現在位置: %.2f, %.2f, %.2f", currentPos.x, currentPos.y, currentPos.z);
                ImGui::Text("現在回転: %.2f, %.2f, %.2f", currentRot.x, currentRot.y, currentRot.z);
                ImGui::Text("現在スケール: %.2f, %.2f, %.2f", currentScale.x, currentScale.y, currentScale.z);
            }

            static const char *easingNames[] = {
                "Linear", "EaseInSine", "EaseOutSine", "EaseInOutSine",
                "EaseInBack", "EaseOutBack", "EaseInOutBack",
                "EaseInQuint", "EaseOutQuint", "EaseInOutQuint",
                "EaseInCirc", "EaseOutCirc", "EaseInOutCirc",
                "EaseInExpo", "EaseOutExpo", "EaseInOutExpo",
                "EaseOutCubic", "EaseInCubic", "EaseInOutCubic",
                "EaseInQuad", "EaseOutQuad", "EaseInOutQuad",
                "EaseInQuart", "EaseOutQuart",
                "EaseInBounce", "EaseOutBounce", "EaseInOutBounce",
                "EaseInElastic", "EaseOutElastic", "EaseInOutElastic"};
            int easingIdx = static_cast<int>(m.easingType);
            if (ImGui::Combo("イージングタイプ", &easingIdx, easingNames, IM_ARRAYSIZE(easingNames))) {
                m.easingType = static_cast<EasingType>(easingIdx);
            }
        }
        char nameBuffer[256];
        strcpy_s(nameBuffer, sizeof(nameBuffer), jsonName_.c_str());
        ImGui::Text("Jsonファイル名");
        if (ImGui::InputText(" ", nameBuffer, sizeof(nameBuffer))) {
            jsonName_ = std::string(nameBuffer);
        }

        if (!jsonName_.empty()) {
            if (ImGui::Button("セーブ")) {
                Save(jsonName_);
                jsonName_.clear();
            }
        }
    }
    ImGui::End();
}

void AttackManager::Save(const std::string &fileName) {
    DataHandler data("AttackData", fileName);
    for (const auto &[name, m] : motions_) {
        data.Save(name + "/totalTime", m.totalTime);
        data.Save(name + "/colliderOnTime", m.colliderOnTime);
        data.Save(name + "/colliderOffTime", m.colliderOffTime);
        data.Save(name + "/startPosOffset", m.startPosOffset);
        data.Save(name + "/endPosOffset", m.endPosOffset);
        data.Save(name + "/startRotOffset", m.startRotOffset);
        data.Save(name + "/endRotOffset", m.endRotOffset);
        data.Save(name + "/startScaleOffset", m.startScaleOffset);
        data.Save(name + "/endScaleOffset", m.endScaleOffset);
        data.Save(name + "/easingType", static_cast<int>(m.easingType));
        data.Save(name + "/useCatmullRom", m.useCatmullRom);
        data.Save(name + "/controlPointCount", (int)m.controlPoints.size());
        for (int i = 0; i < (int)m.controlPoints.size(); ++i) {
            data.Save(name + "/controlPoint" + std::to_string(i), m.controlPoints[i]);
        }
    }
}

void AttackManager::Load(const std::string &fileName) {
    DataHandler data("AttackData", fileName);
    for (auto &[name, m] : motions_) {
        m.totalTime = data.Load(name + "/totalTime", m.totalTime);
        m.colliderOnTime = data.Load(name + "/colliderOnTime", m.colliderOnTime);
        m.colliderOffTime = data.Load(name + "/colliderOffTime", m.colliderOffTime);
        m.startPosOffset = data.Load(name + "/startPosOffset", m.startPosOffset);
        m.endPosOffset = data.Load(name + "/endPosOffset", m.endPosOffset);
        m.startRotOffset = data.Load(name + "/startRotOffset", m.startRotOffset);
        m.endRotOffset = data.Load(name + "/endRotOffset", m.endRotOffset);
        m.startScaleOffset = data.Load(name + "/startScaleOffset", m.startScaleOffset);
        m.endScaleOffset = data.Load(name + "/endScaleOffset", m.endScaleOffset);
        int easingInt = data.Load(name + "/easingType", static_cast<int>(m.easingType));
        m.easingType = static_cast<EasingType>(easingInt);
        m.useCatmullRom = data.Load(name + "/useCatmullRom", false);
        int pointCount = data.Load(name + "/controlPointCount", 0);
        m.controlPoints.clear();
        for (int i = 0; i < pointCount; ++i) {
            Vector3 point = data.Load<Vector3>(name + "/controlPoint" + std::to_string(i), {0, 0, 0});
            m.controlPoints.push_back(point);
        }
    }
}
