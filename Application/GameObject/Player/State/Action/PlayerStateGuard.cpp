#include "PlayerStateGuard.h"
#include "Input.h"
#include "application/GameObject/Player/Player.h"

namespace {
constexpr float kHorizontalDamping = 0.6f;   // 水平速度の減衰係数
constexpr float kGroundPullVelocity = -0.1f;  // 接地維持用の下向き速度
const Vector4 kGuardColor = {0.3f, 0.6f, 1.0f, 1.0f}; // ガード中の青みがかった色
const Vector4 kNormalColor = {1.0f, 1.0f, 1.0f, 1.0f};
} // namespace

void PlayerStateGuard::Enter(Player &player) {
    // ガードに入る直前のステートを覚えておき、解除時に適切な状態へ戻す
    enteredFromFly_ = player.GetPreviewStateName() == "FlyIdle" ||
                      player.GetPreviewStateName() == "FlyMove";

    player.SetGuarding(true);
    player.SetColor(kGuardColor);
}

void PlayerStateGuard::Update(Player &player) {
    // 足を止める（水平速度を減衰）
    Vector3 &vel = player.GetVelocity();
    vel.x *= kHorizontalDamping;
    vel.z *= kHorizontalDamping;

    if (enteredFromFly_) {
        // 飛行ガードはその場でホバリング
        vel.y = 0.0f;
    } else if (player.GetIsGrounded()) {
        player.GetCanJump() = true;
        vel.y = kGroundPullVelocity;
    }

    // 押しっぱ式：ボタンが離されたらガード解除
    bool guardHeld = player.GetGamePad()->IsPress(XINPUT_GAMEPAD_B) ||
                     Input::GetInstance()->PushKey(DIK_RSHIFT);
    if (guardHeld) {
        return;
    }

    if (enteredFromFly_) {
        player.ChangeState("FlyIdle");
    } else if (player.GetIsGrounded()) {
        player.ChangeState(HasMovementInput(player) ? "Move" : "Idle");
    } else {
        player.ChangeState("Air");
    }
}

void PlayerStateGuard::Exit(Player &player) {
    player.SetGuarding(false);
    player.SetColor(kNormalColor);
}

void PlayerStateGuard::DrawParticle(Player &player, const ViewProjection &viewProjection) {
}
