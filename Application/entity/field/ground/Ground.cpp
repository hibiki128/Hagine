#include "Ground.h"
#include <model/material/Material.h>
#include "3d/object/base/BaseObjectManager.h"

using namespace Hagine;

Ground::~Ground()
{
    // 自身がアクティブ地形として登録されている場合のみ解除する
    // （シーン切替時に新旧の Ground が同時に存在しても壊れないようにする）
    if (activeTerrain_ == terrainCollider_)
    {
        activeTerrain_ = nullptr;
    }
}

void Ground::Init(const std::string className)
{
    BaseObject::Init(className);
    BaseObject::CreateModel("Field/Ground.obj");
    BaseObject::SetTexture("Field/ground.png");

    // 地面の初期トランスフォーム設定
    transform_->translation_.y = -1.0f; // 少し下に配置

    // 隆起した地形モデルから三角形メッシュコライダーを構築する
    // （プレイヤー・敵の接地、弾の消滅、カメラの遮蔽判定に使う）
    // 保存済みJSONから既に復元されている場合はそれを再利用する
    // （終了時の自動保存→起動時の復元で毎回コライダーが増殖するのを防ぐ）
    for (auto &collider : GetColliders())
    {
        if (collider && collider->GetType() == ColliderType::Mesh)
        {
            terrainCollider_ = static_cast<MeshCollider *>(collider.get());
            break;
        }
    }
    if (!terrainCollider_)
    {
        terrainCollider_ = AddMeshCollider("Ground_Mesh");
    }
    terrainCollider_->SetTag("Ground");
    activeTerrain_ = terrainCollider_;

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

float Ground::GetSurfaceY(float x, float z)
{
    if (!activeTerrain_)
    {
        return kFallbackSurfaceY;
    }

    // 上空から真下へレイを飛ばし、地形表面の高さを求める
    float distance = 0.0f;
    Vector3 normal;
    if (activeTerrain_->Raycast({x, kProbeHeight, z}, {0.0f, -1.0f, 0.0f}, kProbeLength, distance, normal))
    {
        return kProbeHeight - distance;
    }
    return kFallbackSurfaceY;
}

float Ground::GetStandingY(float x, float z)
{
    return GetSurfaceY(x, z) + kStandOffset;
}
