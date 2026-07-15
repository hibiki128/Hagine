#include "PlayerStateGuard.h"
#include "Input.h"
#include "application/entity/player/Player.h"

using namespace Hagine;
namespace {
constexpr float kHorizontalDamping = 0.6f;            // 水平速度の減衰係数
constexpr float kGroundPullVelocity = -0.1f;          // 接地維持用の下向き速度
const Vector4 kGuardColor = {0.3f, 0.6f, 1.0f, 1.0f}; // ガード中の青みがかった色
} // namespace

void PlayerStateGuard::Enter(Player &player)
{
    // ガードに入る直前のステートを覚えておき、解除時に適切な状態へ戻す
    enteredFromFly_ = player.GetPreviewStateName() == "FlyIdle" ||
                      player.GetPreviewStateName() == "FlyMove";

    // ガード前の本体色を覚えておき、解除時にこの色へ戻す
    // （白固定で戻すと元の青色が失われるため）
    originalColor_ = player.GetColor();

    player.SetGuarding(true);
    player.SetColor(kGuardColor);
}

void PlayerStateGuard::Update(Player &player)
{
    // 足を止める（水平速度を減衰）
    Vector3 &vel = player.GetVelocity();
    vel.x *= kHorizontalDamping;
    vel.z *= kHorizontalDamping;

    if (enteredFromFly_)
    {
        // 飛行ガードはその場でホバリング
        vel.y = 0.0f;
    }
    else if (player.GetIsGrounded())
    {
        player.GetCanJump() = true;
        vel.y = kGroundPullVelocity;
    }

    // 押しっぱ式：ボタンが離されたらガード解除。
    // エネルギーが被弾消費量未満になった場合も維持できずに解除する。
    bool guardHeld = (player.GetGamePad()->IsPress(XINPUT_GAMEPAD_B) ||
                      Input::GetInstance()->PushKey(DIK_RSHIFT)) &&
                     player.CanGuard();
    if (guardHeld)
    {
        return;
    }

    if (enteredFromFly_)
    {
        player.ChangeState("FlyIdle");
    }
    else if (player.GetIsGrounded())
    {
        player.ChangeState(HasMovementInput(player) ? "Move" : "Idle");
    }
    else
    {
        player.ChangeState("Air");
    }
}

void PlayerStateGuard::Exit(Player &player)
{
    player.SetGuarding(false);
    player.SetColor(originalColor_);
}

void PlayerStateGuard::DrawParticle(Player &player, const ViewProjection &viewProjection)
{
}
