#include "Ground.h"
#include <model/material/Material.h>
#include "3d/object/base/BaseObjectManager.h"

using namespace Hagine;
void Ground::Init(const std::string className)
{
    BaseObject::Init(className);
    BaseObject::CreateModel("Field/Ground.obj");
    BaseObject::SetTexture("Field/ground.png");

    // 地面の初期トランスフォーム設定
    transform_->translation_.y = -1.0f; // 少し下に配置
    // transform_->scale_ = {200.0f, 200.0f, 200.0f}; // 広大なサイズに設定

    // 平面モデルを地面として使うためにX軸を回転
    // transform_->quateRotation_ = Quaternion::FromEulerAngles(Vector3(degreesToRadians(-90.0f), 0.0f, 0.0f));

    // 用意した法線マップを適用して凹凸感を出す。
    // 巨大平面(スケール200)なので UV をタイリングしないと法線マップが引き伸ばされてしまう。
    // uvSize でタイル数を調整（アルベドも一緒にタイリングされる）。強さは normalStrength で調整可。
    /* if (Material *mat = GetMaterial(0)) {
         mat->SetNormalMap("NormalMap/ground.png");
         mat->SetNormalStrength(0.1f);
         mat->SetUVSize({20.0f, 20.0f});
     }*/

    BaseObjectManager::GetInstance()->RegisterExternal(this);
}

void Ground::Update()
{
    // 基底クラスの更新処理
    BaseObject::Update();
}

void Ground::Draw(const ViewProjection &viewProjection)
{
    // 基底クラスの描画処理
    BaseObject::Draw(viewProjection);
}
