#include "SceneTransition.h"
#include "Easing.h"
#include "SpriteCommon.h"
#include "TextureManager.h"
#include "algorithm"
#include "myEngine/Frame/Frame.h"
#include <vector>

SceneTransition::SceneTransition() {}

SceneTransition::~SceneTransition() {}

void SceneTransition::Initialize() {
    sprite_ = std::make_unique<Sprite>();
    sprite_->Initialize("debug/black1x1.png", {0, 0}, {1.0f, 1.0f, 1.0f, 1.0f});
    sprite_->SetSize(Vector2(1280, 720)); // 画面全体を覆うサイズ
    sprite_->SetAlpha(0.0f);              // 最初は完全に透明
    duration_ = 1.0f;                     // フェードの持続時間（例: 1秒）
    counter_ = 0.0f;                      // 経過時間カウンターを初期化
    fadeInFinish = false;
    fadeOutFinish = false;
    fadeInStart = false;
    fadeOutStart = false;
    isEnd = false;

    // transition_ 配列の初期化
    int rows = 10;                                   // 縦方向のスプライト数
    int cols = 17;                                   // 横方向のスプライト数
    float size = 0.0f;                               // 初期サイズを 0.0f に設定
    Vector4 defaultColor = {1.0f, 1.0f, 1.0f, 1.0f}; // スプライトの初期色

    // 配列のサイズを設定
    transition_.resize(rows);

    // 各スプライトを初期化
    for (int row = 0; row < rows; ++row) {
        for (int col = 0; col < cols; ++col) {
            auto sprite = std::make_unique<Sprite>();
            sprite->Initialize("debug/black1x1.png",
                               {col * 80.0f, row * 80.0f}, // 位置を設定
                               defaultColor,
                               {0.5f, 0.5f});     // 中心をアンカーポイントに
            sprite->SetSize(Vector2(size, size)); // 初期サイズを 0 に設定
            transition_[row].emplace_back(std::move(sprite));
        }
    }
}

void SceneTransition::Update() {
    FadeUpdate();
}

void SceneTransition::Draw() {
    SpriteCommon::GetInstance()->DrawCommonSetting();
     sprite_->Draw();

    // 各スプライトを描画
   /* for (const auto &row : transition_) {
        for (const auto &sprite : row) {
            sprite->Draw();
        }
    }*/
}

void SceneTransition::Debug() {
    ImGui::Begin("遷移");
    ImGui::DragFloat2("位置", &spPos_.x, 0.1f);
    ImGui::End();
}

void SceneTransition::FadeUpdate() {
    if (fadeInStart) {
        // フェードイン中
        if (!fadeInFinish) {
            FadeIn();
        }
    }
    if (fadeOutStart) {
        // フェードインが終わったら、フェードアウトを開始
        if (fadeInFinish && !fadeOutFinish) {
            FadeOut();
        }
    }

    // トランジションが終了したら、終了フラグを立てる
    if (fadeInFinish && fadeOutFinish) {
        isEnd = true;
        fadeInStart = false;
        fadeOutStart = false;
    }
}

void SceneTransition::FadeIn() {
     DefaultFadeIn();
   // ReverseFadeIn();

    counter_ += 1.0f / 60.0f; // フレームレートを基にカウント（1フレームごとに0.0167秒進む）
    if (counter_ >= duration_) {
        counter_ = duration_; // 終了時間を超えないように制限
        fadeInFinish = true;  // フェードイン終了フラグを立てる
    }
}

void SceneTransition::FadeOut() {
     DefaultFadeOut();
    //ReverseFadeOut();

    // カウンターを減少（フレームレートに基づく）
    counter_ -= 1.0f / 60.0f;
    if (counter_ <= 0.0f) {
        counter_ = 0.0f;      // カウンターが負になるのを防ぐ
        fadeOutFinish = true; // フェードアウト完了フラグを立てる
    }
}

void SceneTransition::DefaultFadeIn() {
    // アルファ値の計算（0.0fから1.0fに増加）
    float alpha = counter_ / duration_;
    sprite_->SetAlpha(alpha);
}

void SceneTransition::DefaultFadeOut() {
    // アルファ値の計算（1.0fから0.0fに減少）
    float alpha = counter_ / duration_; // カウンターが減るほどアルファも減る
    sprite_->SetAlpha(alpha);           // アルファ値を設定
}

void SceneTransition::ReverseFadeIn() {
    int rows = static_cast<int>(transition_.size());
    int cols = static_cast<int>(transition_[0].size());
    // 遅延の最大値（秒）
    const float maxDelay = 0.3f;
    for (int row = 0; row < rows; ++row) {
        for (int col = 0; col < cols; ++col) {
            // 位置に基づいて遅延を計算（左上から右下へ）
            float delay = ((float)(row + col) / (float)(rows + cols - 2)) * maxDelay;
            // カウンターから遅延を引いた値を使用
            float localTime = counter_ - delay;
            if (localTime >= 0.0f && localTime <= duration_) {
                // イージング関数で0 → 80に拡大
                float progress = localTime / duration_;
                float newSize = EaseInSine<float>(0.0f, 80.0f, progress, 0.4f);
                transition_[row][col]->SetSize(Vector2(newSize, newSize));
            } else if (localTime > duration_) {
                // 最大サイズに到達したら固定
                transition_[row][col]->SetSize(Vector2(80.0f, 80.0f));
            } else if (localTime < 0.0f) {
                // 遅延待ち中は初期サイズを維持
                transition_[row][col]->SetSize(Vector2(0.0f, 0.0f));
            }
        }
    }
}

void SceneTransition::ReverseFadeOut() {
    int rows = static_cast<int>(transition_.size());
    int cols = static_cast<int>(transition_[0].size());
    // 遅延の最大値（秒）
    const float maxDelay = 0.3f;
    for (int row = 0; row < rows; ++row) {
        for (int col = 0; col < cols; ++col) {
            // 位置に基づいて遅延を計算（右下から左上へ）
            // (rows-1-row)と(cols-1-col)で座標を反転
            float delay = ((float)((rows - 1 - row) + (cols - 1 - col)) / (float)(rows + cols - 2)) * maxDelay;

            // カウンターから遅延を引いた値を使用
            float localTime = counter_ - delay;
            if (localTime >= 0.0f && localTime <= duration_) {
                // counter_が1→0で変化
                float progress = localTime / duration_;
                float newSize = EaseInSine<float>(0.0f, 80.0f, progress, 0.4f);
                transition_[row][col]->SetSize(Vector2(newSize, newSize));
            } else if (localTime > duration_) {
                // サイズが 0 に到達したら固定
                transition_[row][col]->SetSize(Vector2(0.0f, 0.0f));
            } else if (localTime < 0.0f) {
                // 遅延待ち中も0サイズを維持
                transition_[row][col]->SetSize(Vector2(0.0f, 0.0f));
            }
        }
    }
}

// トランジション状態をリセット
void SceneTransition::Reset() {
    counter_ = 0.0f;
    fadeInFinish = false;
    fadeOutFinish = false;
    fadeInStart = false;
    fadeOutStart = false;
    isEnd = false;
    sprite_->SetAlpha(0.0f); // 最初の透明状態に戻す
}
