#include "MotionEditor.h"
#ifdef _DEBUG
#include "imgui.h"
#endif // _DEBUG
#include "myMath.h"
#include <Line/DrawLine3D.h>

MotionEditor *MotionEditor::instance = nullptr;
const float MotionEditor::ATTACK_END_INTERVAL = 0.1f;

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


MotionEditor *MotionEditor::GetInstance() {
    if (instance == nullptr) {
        instance = new MotionEditor;
    }
    return instance;
}

void MotionEditor::Finalize() {
    delete instance;
    instance = nullptr;
}

void MotionEditor::Register(BaseObject *object) {
    if (!object)
        return;

    std::string name = object->GetName();

    // 既に登録済みかチェック
    if (motions_.find(name) != motions_.end()) {
        return; 
    }

    // 新しくモーションを登録
    motions_[name] = Motion();
    motions_[name].target = object;
    motions_[name].objectName = name;
}

void MotionEditor::CleanupFinishedTemporaryMotions() {
    auto it = motions_.begin();
    while (it != motions_.end()) {
        if (it->second.isTemporary && it->second.status == MotionStatus::Finished) {
            it = motions_.erase(it);
        } else {
            ++it;
        }
    }
}

Vector3 MotionEditor::TransformLocalToWorld(const Vector3 &localOffset, const Matrix4x4 &worldMatrix) {
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

// ステータス確認関数
MotionStatus MotionEditor::GetMotionStatus(const std::string &objectName) {
    auto it = motions_.find(objectName);
    if (it == motions_.end()) {
        return MotionStatus::Stopped;
    }
    return it->second.status;
}

bool MotionEditor::IsPlaying(const std::string &objectName) {
    return GetMotionStatus(objectName) == MotionStatus::Playing;
}

bool MotionEditor::IsFinished(const std::string &objectName) {
    return GetMotionStatus(objectName) == MotionStatus::Finished;
}

std::string MotionEditor::GetTemporaryMotionName(BaseObject *target, const std::string &fileName) {
    return "temp_" + target->GetName() + "_" + fileName;
}

void MotionEditor::ResetInitialPosition(const std::string &objectName) {
    auto it = motions_.find(objectName);
    if (it != motions_.end() && it->second.target) {
        Motion &motion = it->second;
        motion.initialPos = motion.target->GetLocalPosition();
        motion.initialRot = motion.target->GetLocalRotation().ToEulerAngles();
        motion.initialScale = motion.target->GetLocalScale();
        motion.hasInitialTransform = true;
    }
}

Vector3 MotionEditor::CatmullRomInterpolation(const std::vector<Vector3> &points, float t) {
    if (points.size() < 2)
        return Vector3{0, 0, 0};

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

    int numSegments = static_cast<int>(points.size() - 1); 
    float segmentT = t * (numSegments - 2); 
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

    int i0 = segment;
    int i1 = segment + 1;
    int i2 = segment + 2;
    int i3 = segment + 3;

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

Matrix4x4 MotionEditor::GetParentInverseWorldMatrix(BaseObject *object) {
    if (!object || !object->GetParent()) {
        // 親がない場合は単位行列を返す
        return MakeIdentity4x4();
    }

    // 親のワールド変換行列を取得
    Matrix4x4 parentWorldMatrix = object->GetParent()->GetWorldTransform()->matWorld_;

    // 逆行列を計算して返す（逆行列計算が失敗した場合は単位行列を返す）
    Matrix4x4 inverseMatrix = Inverse(parentWorldMatrix);

    // 単位行列との積で確認
    Matrix4x4 identity = parentWorldMatrix * inverseMatrix;

    return inverseMatrix;
}

Vector3 MotionEditor::GetLocalControlPointPosition(BaseObject *object, const Vector3 &worldPos) {
    if (!object) {
        return worldPos;
    }

    // 親がない場合はそのまま返す
    if (!object->GetParent()) {
        return worldPos;
    }

    // 親のワールド変換行列の逆行列を取得
    Matrix4x4 parentInverseMatrix = GetParentInverseWorldMatrix(object);

    // ワールド座標をローカル座標に変換
    Vector4 worldPos4 = {worldPos.x, worldPos.y, worldPos.z, 1.0f};

    Vector4 localPos4 = Transformation(worldPos4, parentInverseMatrix);

    return {localPos4.x, localPos4.y, localPos4.z};
}

// ローカル座標系での制御点位置をワールド座標に変換するヘルパー関数
Vector3 MotionEditor::TransformLocalControlPointToWorld(BaseObject *object, const Vector3 &localPos) {
    if (!object) {
        return localPos;
    }

    // 親がない場合はそのまま返す
    if (!object->GetParent()) {
        return localPos;
    }

    // 親のワールド変換行列を取得
    Matrix4x4 parentWorldMatrix = object->GetParent()->GetWorldTransform()->matWorld_;

    // ローカル座標をワールド座標に変換
    Vector4 localPos4 = {localPos.x, localPos.y, localPos.z, 1.0f};

    Vector4 worldPos4 = Transformation(localPos4, parentWorldMatrix);

    return {worldPos4.x, worldPos4.y, worldPos4.z};
}

// Updateメソッドの修正：モーション終了時の処理
void MotionEditor::Update(float deltaTime) {
    // インターバルタイマーの更新
    for (auto it = attackEndIntervals_.begin(); it != attackEndIntervals_.end();) {
        it->second -= deltaTime;
        if (it->second <= 0.0f) {
            // インターバル終了、当たり判定を無効化
            if (it->first) {
                it->first->SetCollisionEnabled(false);
            }
            it = attackEndIntervals_.erase(it);
        } else {
            ++it;
        }
    }

    for (auto &[name, motion] : motions_) {
        if (!motion.target) {
            continue;
        }

        // 再生中でない場合の処理
        if (motion.status != MotionStatus::Playing) {
            continue;
        }

        // 初期位置の記録（再生開始時に一度だけ）
        if (!motion.hasInitialTransform) {
            motion.initialPos = motion.target->GetLocalPosition();
            motion.initialRot = motion.target->GetLocalRotation().ToEulerAngles();
            motion.initialScale = motion.target->GetLocalScale();
            motion.hasInitialTransform = true;
        }

        motion.currentTime += deltaTime;

        // 再生終了チェック
        if (motion.currentTime >= motion.totalTime) {
            motion.currentTime = motion.totalTime;
            motion.status = MotionStatus::Finished;

            // 一時的なモーションの場合の処理
            if (motion.isTemporary) {
                // 攻撃終了後のインターバルを設定
                SetAttackEndInterval(motion.target, ATTACK_END_INTERVAL);

                // 元の位置に戻すフラグが立っている場合
                if (motion.returnToOriginal) {
                    ReturnToComboStart(motion.target);
                }
            }

            continue;
        }

        // モーション処理（既存のコード）
        float t = motion.currentTime / motion.totalTime;

        if (motion.useCatmullRom && motion.controlPoints.size() >= 4) {
            Vector3 localOffset = CatmullRomInterpolation(motion.controlPoints, t);
            motion.target->GetLocalPosition() = motion.basePos + localOffset;
        } else {
            float easedT = ApplyEasing(motion.easingType, t, 1.0f);
            Vector3 actualStartPos = motion.basePos + motion.startPosOffset;
            Vector3 actualEndPos = motion.basePos + motion.endPosOffset;
            motion.target->GetLocalPosition() = Lerp(actualStartPos, actualEndPos, easedT);
        }

        float easedT = ApplyEasing(motion.easingType, t, 1.0f);
        Quaternion interpolatedRot = Slerp(motion.actualStartRot, motion.actualEndRot, easedT);
        motion.target->GetWorldTransform()->quateRotation_ = interpolatedRot;

        motion.target->GetLocalScale() = Lerp(motion.actualStartScale, motion.actualEndScale, easedT);

        bool enable = motion.currentTime >= motion.colliderOnTime && motion.currentTime <= motion.colliderOffTime;
        motion.target->SetCollisionEnabled(enable);
    }

    CleanupFinishedTemporaryMotions();
    DrawControlPoints();
    DrawCatmullRomCurve();
}

// 登録済みオブジェクトの再生
void MotionEditor::Play(const std::string &jsonName) {
    auto it = motions_.find(jsonName);
    if (it == motions_.end()) {
        return;
    }

    Motion &motion = it->second;
    if (!motion.target) {
        return; 
    }

    if (!motion.hasInitialTransform) {
        motion.initialPos = motion.target->GetLocalPosition();
        motion.initialRot = motion.target->GetLocalRotation().ToEulerAngles();
        motion.initialScale = motion.target->GetLocalScale();
        motion.hasInitialTransform = true;
    }

    motion.basePos = motion.target->GetLocalPosition();
    motion.baseRot = motion.target->GetLocalRotation().ToEulerAngles();
    motion.baseScale = motion.target->GetLocalScale();

    // 実際の開始・終了値を計算（ローカルオフセット用）
    motion.actualStartRot = Quaternion::FromEulerAngles(motion.baseRot + motion.startRotOffset);
    motion.actualEndRot = Quaternion::FromEulerAngles(motion.baseRot + motion.endRotOffset);
    motion.actualStartScale = motion.baseScale + motion.startScaleOffset;
    motion.actualEndScale = motion.baseScale + motion.endScaleOffset;

    // 再生開始
    motion.currentTime = 0.0f;
    motion.status = MotionStatus::Playing;
}

bool MotionEditor::PlayFromFile(BaseObject *target, const std::string &fileName, bool returnToOriginal) {
    if (!target) {
        return false;
    }

    std::string tempName = GetTemporaryMotionName(target, fileName);

    DataHandler data("AttackData", fileName);
    Motion &motion = motions_[tempName];

    motion.target = target;
    motion.objectName = tempName;
    motion.isTemporary = true;
    motion.returnToOriginal = returnToOriginal;
    motion.totalTime = data.Load<float>("totalTime", 1.0f);
    motion.colliderOnTime = data.Load("colliderOnTime", 0.3f);
    motion.colliderOffTime = data.Load("colliderOffTime", 0.6f);
    motion.startPosOffset = data.Load<Vector3>("startPosOffset", {});
    motion.endPosOffset = data.Load<Vector3>("endPosOffset", {});
    motion.startRotOffset = data.Load<Vector3>("startRotOffset", {});
    motion.endRotOffset = data.Load<Vector3>("endRotOffset", {});
    motion.startScaleOffset = data.Load<Vector3>("startScaleOffset", {0, 0, 0});
    motion.endScaleOffset = data.Load<Vector3>("endScaleOffset", {0, 0, 0});
    int easingInt = data.Load("easingType", 0);
    motion.easingType = static_cast<EasingType>(easingInt);

    motion.useCatmullRom = data.Load<bool>("useCatmullRom", false);
    int pointCount = data.Load<int>("controlPointCount", 0);
    motion.controlPoints.clear();
    for (int i = 0; i < pointCount; ++i) {
        Vector3 point = data.Load<Vector3>("controlPoint" + std::to_string(i), {0, 0, 0});
        motion.controlPoints.push_back(point);
    }

    motion.initialPos = target->GetLocalPosition();
    motion.initialRot = target->GetLocalRotation().ToEulerAngles();
    motion.initialScale = target->GetLocalScale();
    motion.hasInitialTransform = true;

    motion.basePos = target->GetLocalPosition();
    motion.baseRot = target->GetLocalRotation().ToEulerAngles();
    motion.baseScale = target->GetLocalScale();

    motion.actualStartRot = Quaternion::FromEulerAngles(motion.baseRot + motion.startRotOffset);
    motion.actualEndRot = Quaternion::FromEulerAngles(motion.baseRot + motion.endRotOffset);
    motion.actualStartScale = motion.baseScale + motion.startScaleOffset;
    motion.actualEndScale = motion.baseScale + motion.endScaleOffset;

    // 再生開始
    motion.currentTime = 0.0f;
    motion.status = MotionStatus::Playing;

    return true;
}


void MotionEditor::SetComboStartPosition(BaseObject *target) {
    if (!target)
        return;

    // オブジェクトごとにコンボ開始位置を保存
    comboStartPositions_[target] = target->GetLocalPosition();
    comboStartRotations_[target] = target->GetLocalRotation().ToEulerAngles();
    comboStartScales_[target] = target->GetLocalScale();
}

void MotionEditor::ReturnToComboStart(BaseObject *target) {
    if (!target)
        return;

    // 保存されたコンボ開始位置があるかチェック
    auto posIt = comboStartPositions_.find(target);
    auto rotIt = comboStartRotations_.find(target);
    auto scaleIt = comboStartScales_.find(target);

    if (posIt != comboStartPositions_.end() &&
        rotIt != comboStartRotations_.end() &&
        scaleIt != comboStartScales_.end()) {

        target->GetLocalPosition() = posIt->second;
        target->GetLocalRotation() = Quaternion::FromEulerAngles(rotIt->second);
        target->GetLocalScale() = scaleIt->second;
    }
}

void MotionEditor::ClearComboStartPosition(BaseObject *target) {
    if (!target)
        return;

    comboStartPositions_.erase(target);
    comboStartRotations_.erase(target);
    comboStartScales_.erase(target);
}

void MotionEditor::ClearAllComboStartPositions() {
    comboStartPositions_.clear();
    comboStartRotations_.clear();
    comboStartScales_.clear();
}


void MotionEditor::Stop(const std::string &objectName) {
    auto it = motions_.find(objectName);
    if (it != motions_.end()) {
        Motion &motion = it->second;
        motion.status = MotionStatus::Stopped;
        motion.currentTime = 0.0f;

        // 初期位置に戻すが固定はしない
        if (motion.hasInitialTransform && motion.target) {
            motion.target->GetLocalPosition() = motion.initialPos;
            motion.target->GetLocalRotation() = Quaternion::FromEulerAngles(motion.initialRot);
            motion.target->GetLocalScale() = motion.initialScale;
        }
        if (motion.isTemporary) {
            motions_.erase(it);
        }
    }
}

// 全ての再生停止
void MotionEditor::StopAll() {
    auto it = motions_.begin();
    while (it != motions_.end()) {
        Motion &motion = it->second;
        motion.status = MotionStatus::Stopped;
        motion.currentTime = 0.0f;

        if (motion.hasInitialTransform && motion.target) {
            motion.target->GetLocalPosition() = motion.initialPos;
            motion.target->GetLocalRotation() = Quaternion::FromEulerAngles(motion.initialRot);
            motion.target->GetLocalScale() = motion.initialScale;
        }

        if (motion.isTemporary) {
            it = motions_.erase(it);
        } else {
            ++it;
        }
    }
}

bool MotionEditor::IsAttackFinished(BaseObject *target) {
    if (!target)
        return false;

    for (const auto &[name, motion] : motions_) {
        if (motion.target == target && motion.isTemporary) {
            return motion.status == MotionStatus::Finished;
        }
    }

    return true; 
}

// 攻撃が終了してインターバルも過ぎたかどうかをチェック
bool MotionEditor::IsAttackFinishedWithInterval(BaseObject *target) {
    if (!target)
        return false;

    if (!IsAttackFinished(target)) {
        return false;
    }

    auto it = attackEndIntervals_.find(target);
    if (it != attackEndIntervals_.end()) {
        return it->second <= 0.0f;
    }

    return true; // インターバルが設定されていなければtrue
}

// 攻撃終了後のインターバルを設定
void MotionEditor::SetAttackEndInterval(BaseObject *target, float interval) {
    if (!target)
        return;

    attackEndIntervals_[target] = interval;
}

// 攻撃終了後のインターバルをクリア
void MotionEditor::ClearAttackEndInterval(BaseObject *target) {
    if (!target)
        return;

    attackEndIntervals_.erase(target);
}

bool MotionEditor::IsTemporaryMotionFinished(BaseObject *target, const std::string &fileName) {
    if (!target)
        return false;

    std::string tempName = GetTemporaryMotionName(target, fileName);
    auto it = motions_.find(tempName);

    if (it != motions_.end()) {
        return it->second.status == MotionStatus::Finished;
    }

    return true; 
}

void MotionEditor::DrawControlPoints() {
    if (selectedName_.empty())
        return;

    Motion &motion = motions_[selectedName_];
    if (!motion.target || !motion.useCatmullRom || motion.controlPoints.empty())
        return;

    DrawLine3D *drawLine = DrawLine3D::GetInstance();
    const float cubeSize = 0.4f;

    Vector3 basePos;
    if (motion.currentTime == 0.0f) {
        basePos = motion.target->GetLocalPosition();
    } else {
        basePos = motion.basePos;
    }

    for (size_t i = 0; i < motion.controlPoints.size(); ++i) {

        Vector3 localPos = basePos + motion.controlPoints[i];

        Vector3 worldPos = TransformLocalControlPointToWorld(motion.target, localPos);

        Vector4 controlPointColor;
        if (i == 0) {
            controlPointColor = {0.0f, 1.0f, 0.0f, 1.0f}; 
        } else if (i == motion.controlPoints.size() - 1) {
            controlPointColor = {0.0f, 0.0f, 1.0f, 1.0f};
        } else {
            controlPointColor = {1.0f, 0.0f, 0.0f, 1.0f};
        }

        // 選択中の制御点は明るくする
        if ((int)i == selectedControlPoint_) {
            controlPointColor.x = std::min(controlPointColor.x + 0.5f, 1.0f);
            controlPointColor.y = std::min(controlPointColor.y + 0.5f, 1.0f);
            controlPointColor.z = std::min(controlPointColor.z + 0.5f, 1.0f);
        }

        drawLine->DrawSphere(worldPos, controlPointColor, cubeSize, 8);

        // 制御点番号表示用の小さな線（上向き）
        Vector3 numberPos = worldPos;
        numberPos.y += cubeSize + 0.2f;
        drawLine->SetPoints(worldPos, numberPos, controlPointColor);
    }
}

void MotionEditor::DrawCatmullRomCurve() {
    if (selectedName_.empty())
        return;

    Motion &motion = motions_[selectedName_];
    if (!motion.target || !motion.useCatmullRom || motion.controlPoints.size() < 2)
        return;

    DrawLine3D *drawLine = DrawLine3D::GetInstance();
    const Vector4 curveColor = {1.0f, 0.5f, 0.0f, 1.0f}; 
    const int curveResolution = 100;

    Vector3 basePos;
    if (motion.currentTime == 0.0f) {
        basePos = motion.target->GetLocalPosition();
    } else {
        basePos = motion.basePos;
    }

    Vector3 prevLocalOffset = CatmullRomInterpolation(motion.controlPoints, 0.0f);
    Vector3 prevLocalPoint = basePos + prevLocalOffset;
    Vector3 prevWorldPoint = TransformLocalControlPointToWorld(motion.target, prevLocalPoint);

    for (int i = 1; i <= curveResolution; ++i) {
        float t = (float)i / curveResolution;
        Vector3 currentLocalOffset = CatmullRomInterpolation(motion.controlPoints, t);
        Vector3 currentLocalPoint = basePos + currentLocalOffset;
        Vector3 currentWorldPoint = TransformLocalControlPointToWorld(motion.target, currentLocalPoint);

        drawLine->SetPoints(prevWorldPoint, currentWorldPoint, curveColor);
        prevWorldPoint = currentWorldPoint;
    }

    const Vector4 debugLineColor = {0.3f, 0.3f, 0.3f, 1.0f};
    for (size_t i = 0; i < motion.controlPoints.size() - 1; ++i) {
        Vector3 localPoint1 = basePos + motion.controlPoints[i];
        Vector3 localPoint2 = basePos + motion.controlPoints[i + 1];

        Vector3 worldPoint1 = TransformLocalControlPointToWorld(motion.target, localPoint1);
        Vector3 worldPoint2 = TransformLocalControlPointToWorld(motion.target, localPoint2);

        drawLine->SetPoints(worldPoint1, worldPoint2, debugLineColor);
    }

    Vector3 worldBasePos = TransformLocalControlPointToWorld(motion.target, basePos);
    const Vector4 basePosColor = {1.0f, 1.0f, 1.0f, 1.0f}; 
    drawLine->DrawSphere(worldBasePos, basePosColor, 0.2f, 32);

    if (motion.status == MotionStatus::Playing) {
        Vector3 currentLocalPos = motion.target->GetLocalPosition();
        Vector3 currentWorldPos = TransformLocalControlPointToWorld(motion.target, currentLocalPos);
        const Vector4 currentPosColor = {1.0f, 1.0f, 0.0f, 1.0f}; 
        drawLine->DrawSphere(currentWorldPos, currentPosColor, 0.15f, 32);

        // 基準位置と現在位置を線で結ぶ
        const Vector4 connectionColor = {0.5f, 0.5f, 0.5f, 1.0f}; 
        drawLine->SetPoints(worldBasePos, currentWorldPos, connectionColor);
    }
}

void MotionEditor::DrawImGui() {

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
        Motion &m = motions_[selectedName_];

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
                    point = m.target->GetLocalPosition();
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
                Vector3 currentPos = m.target->GetLocalPosition();
                Quaternion currentRot = m.target->GetLocalRotation();
                Vector3 currentScale = m.target->GetLocalScale();
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
}

void MotionEditor::Save(const std::string &fileName) {
    DataHandler data("AttackData", fileName);
    Motion &m = motions_[selectedName_];

    data.Save("totalTime", m.totalTime);
    data.Save("colliderOnTime", m.colliderOnTime);
    data.Save("colliderOffTime", m.colliderOffTime);
    data.Save("startPosOffset", m.startPosOffset);
    data.Save("endPosOffset", m.endPosOffset);
    data.Save("startRotOffset", m.startRotOffset);
    data.Save("endRotOffset", m.endRotOffset);
    data.Save("startScaleOffset", m.startScaleOffset);
    data.Save("endScaleOffset", m.endScaleOffset);
    data.Save("easingType", static_cast<int>(m.easingType));
    data.Save("useCatmullRom", m.useCatmullRom);
    data.Save("controlPointCount", (int)m.controlPoints.size());
    for (int i = 0; i < (int)m.controlPoints.size(); ++i) {
        data.Save("controlPoint" + std::to_string(i), m.controlPoints[i]);
    }
}

Motion MotionEditor::Load(const std::string &fileName) {
    DataHandler data("AttackData", fileName);
    Motion m;
    m.totalTime = data.Load("totalTime", m.totalTime);
    m.colliderOnTime = data.Load("colliderOnTime", m.colliderOnTime);
    m.colliderOffTime = data.Load("colliderOffTime", m.colliderOffTime);
    m.startPosOffset = data.Load("startPosOffset", m.startPosOffset);
    m.endPosOffset = data.Load("endPosOffset", m.endPosOffset);
    m.startRotOffset = data.Load("startRotOffset", m.startRotOffset);
    m.endRotOffset = data.Load("endRotOffset", m.endRotOffset);
    m.startScaleOffset = data.Load("startScaleOffset", m.startScaleOffset);
    m.endScaleOffset = data.Load("endScaleOffset", m.endScaleOffset);
    int easingInt = data.Load("easingType", static_cast<int>(m.easingType));
    m.easingType = static_cast<EasingType>(easingInt);
    m.useCatmullRom = data.Load("useCatmullRom", false);
    int pointCount = data.Load("controlPointCount", 0);
    m.controlPoints.clear();
    for (int i = 0; i < pointCount; ++i) {
        Vector3 point = data.Load<Vector3>("controlPoint" + std::to_string(i), {0, 0, 0});
        m.controlPoints.push_back(point);
    }
    return m;
}
