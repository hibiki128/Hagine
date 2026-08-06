#include "FollowCameraFactory.h"
#include "FollowCamera.h"
#include "parts/CameraFinisher.h"
#include "parts/CameraLockOn.h"
#include "parts/CameraRush.h"
#include "parts/CameraSkillCutscene.h"

std::unique_ptr<FollowCamera> FollowCameraFactory::Create()
{
    // パーツを組み立ててからファサードへ注入する。
    // 差し替え（テスト用のダミーパーツ等）が必要になった場合も、
    // 変更はこのファクトリ内に閉じる
    return std::make_unique<FollowCamera>(
        std::make_unique<CameraLockOn>(),
        std::make_unique<CameraRush>(),
        std::make_unique<CameraSkillCutscene>(),
        std::make_unique<CameraFinisher>());
}
