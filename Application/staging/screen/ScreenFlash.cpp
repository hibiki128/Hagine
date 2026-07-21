#include "ScreenFlash.h"
#include <OffScreen.h>
#include <offscreen/effect/PostEffectParamsImpl.h> // BloomParams / MonochromeParams / RadialBlurParams
#include <utility/debug/param/GameParamHub.h>
#include <utility/scene/SceneManager.h>

using namespace Hagine;

namespace {
/// メインのオフスクリーン（ステージ0）を取得する
OffScreen *FetchOffScreen()
{
    return SceneManager::GetInstance()->GetOffScreen();
}
} // namespace

void ScreenFlash::Initialize()
{
    if (initialized_)
    {
        return;
    }
    OffScreen *os = FetchOffScreen();
    if (!os)
    {
        return;
    }

    // 既存スロットがあれば再利用（多重確保を防ぐ）。無ければ空きスロットへ追加する。
    // 適用順はスロット番号順なので、Monochrome → Bloom → Blur の順に追加して
    // 「白黒二値化 → 明るい所をブルーム → ラジアルブラー」の順で適用する
    monoSlot_ = os->FindEffectSlotByName(kMonoName);
    if (monoSlot_ < 0)
    {
        monoSlot_ = os->AddEffect(ShaderMode::Monochrome, kMonoName);
    }
    bloomSlot_ = os->FindEffectSlotByName(kBloomName);
    if (bloomSlot_ < 0)
    {
        bloomSlot_ = os->AddEffect(ShaderMode::Bloom, kBloomName);
    }
    blurSlot_ = os->FindEffectSlotByName(kBlurName);
    if (blurSlot_ < 0)
    {
        blurSlot_ = os->AddEffect(ShaderMode::Blur, kBlurName);
    }

    // 各エフェクトのパラメータを反映
    ApplyParams();

    // 初期状態は無効
    SetEffectsEnabled(false);
    initialized_ = true;
}

void ScreenFlash::ApplyParams()
{
    OffScreen *os = FetchOffScreen();
    if (!os)
    {
        return;
    }
    // 白黒二値化の閾値
    if (auto *mono = os->GetEffectParams<MonochromeParams>(monoSlot_))
    {
        mono->GetData()->threshold = monoThreshold_;
    }
    // ブルーム
    if (auto *bloom = os->GetEffectParams<BloomParams>(bloomSlot_))
    {
        bloom->GetData()->threshold = bloomThreshold_;
        bloom->GetData()->intensity = bloomIntensity_;
    }
    // ラジアルブラー（中心固定・ブラー幅のみ調整）
    if (auto *blur = os->GetEffectParams<RadialBlurParams>(blurSlot_))
    {
        blur->GetData()->blurWidth = blurWidth_;
    }
}

void ScreenFlash::Finalize()
{
    if (OffScreen *os = FetchOffScreen())
    {
        os->RemoveAllEffectsByName(kMonoName);
        os->RemoveAllEffectsByName(kBloomName);
        os->RemoveAllEffectsByName(kBlurName);
    }
    monoSlot_ = -1;
    bloomSlot_ = -1;
    blurSlot_ = -1;
    initialized_ = false;
    active_ = false;
}

void ScreenFlash::Trigger()
{
    if (!initialized_)
    {
        Initialize();
    }
    if (!initialized_)
    {
        return;
    }

    // 発動のたびに最新のパラメータを反映する（調整結果を即時反映）
    ApplyParams();

    active_ = true;
    timer_ = 0.0f;
    flashIndex_ = 0;
    currentOn_ = true;
    SetEffectsEnabled(true);
}

void ScreenFlash::Update(float deltaTime)
{
    if (!active_)
    {
        return;
    }

    timer_ += deltaTime;
    const float phaseDuration = currentOn_ ? onDuration_ : gapDuration_;
    if (timer_ < phaseDuration)
    {
        return;
    }

    timer_ -= phaseDuration;
    if (currentOn_)
    {
        // 点灯（白黒）フェーズ終了
        ++flashIndex_;
        if (flashIndex_ >= flashCount_)
        {
            // 全ての明滅が終わったので通常表示へ戻す
            SetEffectsEnabled(false);
            active_ = false;
            return;
        }
        // 通常表示（消灯）フェーズへ
        currentOn_ = false;
        SetEffectsEnabled(false);
    }
    else
    {
        // 通常表示フェーズ終了→次の点灯へ
        currentOn_ = true;
        SetEffectsEnabled(true);
    }
}

void ScreenFlash::SetEffectsEnabled(bool enabled)
{
    OffScreen *os = FetchOffScreen();
    if (!os)
    {
        return;
    }
    if (monoSlot_ >= 0)
    {
        os->SetEffectEnabled(monoSlot_, enabled);
    }
    if (bloomSlot_ >= 0)
    {
        os->SetEffectEnabled(bloomSlot_, enabled);
    }
    if (blurSlot_ >= 0)
    {
        os->SetEffectEnabled(blurSlot_, enabled);
    }
}

void ScreenFlash::RegisterParams(const std::string &owner)
{
    auto *hub = GameParamHub::GetInstance();
    hub->Register(owner, "必殺白黒点灯時間", &onDuration_, {0.01f, 0.0f, 1.0f});
    hub->Register(owner, "必殺白黒間隔時間", &gapDuration_, {0.01f, 0.0f, 1.0f});
    hub->Register(owner, "必殺白黒明滅回数", &flashCount_, {1.0f, 1.0f, 10.0f});
    hub->Register(owner, "必殺白黒二値化閾値", &monoThreshold_, {0.01f, 0.0f, 1.0f});
    hub->Register(owner, "必殺ブルーム閾値", &bloomThreshold_, {0.01f, 0.0f, 1.0f});
    hub->Register(owner, "必殺ブルーム強度", &bloomIntensity_, {0.05f, 0.0f, 5.0f});
    hub->Register(owner, "必殺ブラー幅", &blurWidth_, {0.001f, 0.0f, 0.2f});
}
