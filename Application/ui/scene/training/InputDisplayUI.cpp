#define NOMINMAX
#include "InputDisplayUI.h"
#include "2d/SpriteManager.h"
#include "2d/text/TextRenderer.h"
#include "Application/entity/player/Player.h"
#include "Input.h"
#include <Frame.h>
#include <graphics/texture/TextureManager.h>
#include <algorithm>
#include <cmath>

using namespace Hagine;

void InputDisplayUI::Initialize(Player *player, GamePad *pGamePad)
{
    pPlayer_ = player;
    pGamePad_ = pGamePad;
    history_.clear();
    glyphs_.clear();
    wasMoving_ = false;

    // 文字生成に使うフォントキー（NotoSansJP。日本語も表示可）
    auto keys = TextureManager::GetInstance()->GetAllFontKeys();
    fontKey_ = keys.empty() ? "" : keys[0];

    // ボタンアイコン（既存PNG）
    RegisterButton("btnA", "UI/AButton.png", kGlyphSize);
    RegisterButton("btnB", "UI/BButton.png", kGlyphSize);
    RegisterButton("btnX", "UI/XButton.png", kGlyphSize);
    RegisterButton("btnY", "UI/YButton.png", kGlyphSize);

    // キーキャップ（アイコンの無いキー/ボタンは文字で表現）
    RegisterText("kc_WASD", "WASD", kKeycapHeight);
    RegisterText("kc_LS", "Lstick", kKeycapHeight);
    RegisterText("kc_SPACE", "Space", kKeycapHeight);
    RegisterText("kc_RB", "RB", kKeycapHeight);
    RegisterText("kc_RT", "RT", kKeycapHeight);
    RegisterText("kc_LT", "LT", kKeycapHeight);
    RegisterText("kc_LSHIFT", "LShift", kKeycapHeight);
    RegisterText("kc_RSHIFT", "RShift", kKeycapHeight);
    RegisterText("kc_J", "J", kKeycapHeight);
    RegisterText("kc_K", "K", kKeycapHeight);
    RegisterText("kc_C", "C", kKeycapHeight);
    RegisterText("kc_G", "G", kKeycapHeight);
    RegisterText("kc_H", "H", kKeycapHeight);
    RegisterText("kc_LCTRL", "LCtrl", kKeycapHeight);

    // 操作名ラベル
    RegisterText("lbl_move", "移動", kLabelHeight);
    RegisterText("lbl_jump", "ジャンプ/飛行", kLabelHeight);
    RegisterText("lbl_descend", "下降", kLabelHeight);
    RegisterText("lbl_guard", "ガード", kLabelHeight);
    RegisterText("lbl_shot", "射撃", kLabelHeight);
    RegisterText("lbl_charge", "チャージ", kLabelHeight);
    RegisterText("lbl_chargeshot", "チャージ弾", kLabelHeight);
    RegisterText("lbl_energy", "エネチャ", kLabelHeight);
    RegisterText("lbl_special", "必殺技", kLabelHeight);
    RegisterText("lbl_dash", "ダッシュ", kLabelHeight);
    RegisterText("lbl_rush", "ラッシュ", kLabelHeight);
    RegisterText("lbl_melee", "近接", kLabelHeight);

    // 近接コンボの段名（Player.cpp の punchCombo_.Add(...) と対応）
    const char *atkNames[] = {"Jab", "Hook", "Cross", "Uppercut",
                              "Overhand", "Swing", "Elbow", "Slam"};
    for (const char *n : atkNames)
    {
        RegisterText(std::string("atk_") + n, n, kLabelHeight);
    }

    // エントリ間の矢印
    RegisterText("arrow", "→", kArrowHeight);

    // 描画用スプライトプール（一度だけ生成。テクスチャは毎フレーム再バインドする）
    glyphPool_.clear();
    labelPool_.clear();
    arrowPool_.clear();
    for (int i = 0; i < kMaxHistory; ++i)
    {
        auto g = std::make_unique<Sprite>();
        g->Initialize("UI/AButton.png", {0.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f});
        glyphPool_.push_back(std::move(g));

        auto l = std::make_unique<Sprite>();
        l->Initialize("UI/AButton.png", {0.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f});
        labelPool_.push_back(std::move(l));

        auto a = std::make_unique<Sprite>();
        a->Initialize("UI/AButton.png", {0.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f});
        arrowPool_.push_back(std::move(a));
    }

    // 履歴の背景パネル（暗い半透明）。
    // 白ベースのグラデーション画像を着色して使う。α側に「左(新しい)=濃い→右(古い)=薄い」の
    // 横グラデーションと、古い側の「上から斜め下へ薄くなる」斜めフェードを焼き込んである。
    bgPanel_ = std::make_unique<Sprite>();
    bgPanel_->Initialize("UI/inputHistoryGradient.png", {0.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f});
    bgPanel_->SetColor({0.05f, 0.06f, 0.10f}); // 濃紺寄りの暗色
    panelWidth_ = 0.0f;
    panelAlpha_ = 0.0f;
}

// テクスチャ情報の登録
void InputDisplayUI::RegisterText(const std::string &key, const std::string &text, float displayHeight)
{
    if (fontKey_.empty())
    {
        return;
    }

    // 文字画像を生成（アウトライン付きで背景に映える）
    TextRenderer::GetInstance()->CreateTextSprite(
        key, text, fontKey_,
        {0.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f},
        true, 4.0f, {0.0f, 0.0f, 0.0f, 1.0f});

    // TextRenderer が SpriteManager に登録するが、ここでは自前プールで描くため解除する
    SpriteManager::GetInstance()->UnregisterSprite(key);

    const std::string path = "Text/" + key + ".png";
    TextureManager::GetInstance()->LoadTexture(path);
    const DirectX::TexMetadata &meta = TextureManager::GetInstance()->GetMetaData(path);

    float scale = (meta.height > 0) ? displayHeight / static_cast<float>(meta.height) : 1.0f;

    GlyphInfo info;
    info.path = path;
    info.size = {static_cast<float>(meta.width) * scale, displayHeight};
    glyphs_[key] = info;
}

void InputDisplayUI::RegisterButton(const std::string &key, const std::string &texturePath, float size)
{
    TextureManager::GetInstance()->LoadTexture(texturePath);
    GlyphInfo info;
    info.path = texturePath;
    info.size = {size, size};
    glyphs_[key] = info;
}

bool InputDisplayUI::IsGamePad() const
{
    return pGamePad_ && pGamePad_->IsConnected();
}

float InputDisplayUI::SlotAlpha(int slot)
{
    switch (slot)
    {
    case 0:
    case 1:
        return 1.0f;
    case 2:
        return 0.55f;
    case 3:
        return 0.28f;
    default:
        return 0.0f;
    }
}

void InputDisplayUI::PollInputs()
{
    Input *pInput = Input::GetInstance();
    const bool pad = IsGamePad();

    // 移動（連続入力なので立ち上がりのみ表示）
    bool moving = false;
    if (pad)
    {
        moving = std::abs(pGamePad_->GetLeftStickX()) > 0.0f ||
                 std::abs(pGamePad_->GetLeftStickY()) > 0.0f;
    }
    else
    {
        moving = pInput->PushKey(DIK_W) || pInput->PushKey(DIK_A) ||
                 pInput->PushKey(DIK_S) || pInput->PushKey(DIK_D);
    }
    if (moving && !wasMoving_)
    {
        PushEntry(pad ? "kc_LS" : "kc_WASD", "lbl_move");
    }
    wasMoving_ = moving;

    // ジャンプ / 飛行
    if (pad ? pGamePad_->IsTrigger(XINPUT_GAMEPAD_RIGHT_SHOULDER) : pInput->TriggerKey(DIK_SPACE))
    {
        PushEntry(pad ? "kc_RB" : "kc_SPACE", "lbl_jump");
    }

    // 下降
    if (pad ? pGamePad_->IsRightTriggerTriggered(0.25f) : pInput->TriggerKey(DIK_LSHIFT))
    {
        PushEntry(pad ? "kc_RT" : "kc_LSHIFT", "lbl_descend");
    }

    // 射撃/チャージ/必殺/ダッシュ/ラッシュ/ガード/エネチャは、入力の有無ではなく
    // Player が「実際に発動した」ときのイベントを表示する（発生条件の推測を避ける）
    if (pPlayer_)
    {
        for (Player::ActionKind kind : pPlayer_->GetActionEvents())
        {
            PushActionEvent(kind);
        }
    }

    // 近接攻撃（実際に発火した段名を使う：先行入力バッファ経由でも取りこぼさない）
    std::string atk;
    if (pPlayer_ && pPlayer_->ConsumeMeleeAttackFired(atk))
    {
        std::string labelKey = "atk_" + atk;
        if (!HasGlyph(labelKey))
        {
            labelKey = "lbl_melee"; // 未知の段名はフォールバック
        }
        PushEntry(pad ? "btnX" : "kc_H", labelKey);
    }
}

void InputDisplayUI::PushActionEvent(Player::ActionKind kind)
{
    const bool pad = IsGamePad();
    switch (kind)
    {
    case Player::ActionKind::NormalShot:
        PushEntry(pad ? "btnY" : "kc_J", "lbl_shot");
        break;
    case Player::ActionKind::ChargeStart:
        PushEntry(pad ? "btnY" : "kc_K", "lbl_charge");
        break;
    case Player::ActionKind::ChargeShot:
        PushEntry(pad ? "btnY" : "kc_K", "lbl_chargeshot");
        break;
    case Player::ActionKind::Special:
        PushEntry(pad ? "btnY" : "kc_G", "lbl_special");
        break;
    case Player::ActionKind::Dash:
        PushEntry(pad ? "btnA" : "kc_LCTRL", "lbl_dash");
        break;
    case Player::ActionKind::Rush:
        PushEntry(pad ? "btnA" : "kc_LCTRL", "lbl_rush");
        break;
    case Player::ActionKind::Guard:
        PushEntry(pad ? "btnB" : "kc_RSHIFT", "lbl_guard");
        break;
    case Player::ActionKind::EnergyCharge:
        PushEntry(pad ? "kc_LT" : "kc_C", "lbl_energy");
        break;
    }
}

void InputDisplayUI::PushEntry(const std::string &glyphKey, const std::string &labelKey)
{
    auto gIt = glyphs_.find(glyphKey);
    auto lIt = glyphs_.find(labelKey);
    if (gIt == glyphs_.end() || lIt == glyphs_.end())
    {
        return;
    }

    // 既存エントリを1つずつ古い側（右）へずらす
    for (HistoryEntry &e : history_)
    {
        e.slot += 1;
    }

    HistoryEntry ne;
    ne.glyphPath = gIt->second.path;
    ne.labelPath = lIt->second.path;
    ne.glyphSize = gIt->second.size;
    ne.labelSize = lIt->second.size;
    ne.slot = 0;
    ne.pos = {kAnchorX, kBaseY + kRiseOffset};
    ne.alpha = 0.0f;
    ne.appearT = 0.0f;
    history_.push_back(ne);

    // 上限（プールサイズ）を超えたら最も古いものを整理する
    while (static_cast<int>(history_.size()) > kMaxHistory)
    {
        size_t maxIdx = 0;
        for (size_t i = 1; i < history_.size(); ++i)
        {
            if (history_[i].slot > history_[maxIdx].slot)
            {
                maxIdx = i;
            }
        }
        history_.erase(history_.begin() + maxIdx);
    }
}

void InputDisplayUI::Update()
{
    PollInputs();

    const float dt = Frame::DeltaTime();
    const float slideT = std::clamp(kSlideLerp * dt, 0.0f, 1.0f);
    const float alphaT = std::clamp(kAlphaLerp * dt, 0.0f, 1.0f);

    for (HistoryEntry &e : history_)
    {
        e.appearT = std::min(1.0f, e.appearT + (kAppearTime > 0.0f ? dt / kAppearTime : 1.0f));
        const float eased = 1.0f - std::pow(1.0f - e.appearT, 3.0f); // OutCubic

        const float targetX = kAnchorX + static_cast<float>(e.slot) * kSlotPitch;
        e.pos.x += (targetX - e.pos.x) * slideT;
        e.pos.y = kBaseY + (1.0f - eased) * kRiseOffset;

        const float targetAlpha = SlotAlpha(e.slot) * eased;
        e.alpha += (targetAlpha - e.alpha) * alphaT;
    }

    // 規定スロットを超え、ほぼ透明になったエントリを削除する
    const int visibleCount = kSlotVisibleCount;
    history_.erase(
        std::remove_if(history_.begin(), history_.end(),
                       [visibleCount](const HistoryEntry &e) {
                           return e.slot >= visibleCount && e.alpha < 0.02f;
                       }),
        history_.end());

    // 背景パネルの幅・アルファを可視エントリに合わせて補間する
    bool anyVisible = false;
    float rightEdge = kAnchorX;
    for (const HistoryEntry &e : history_)
    {
        if (e.alpha > 0.02f)
        {
            anyVisible = true;
            float r = e.pos.x + e.glyphSize.x + kGlyphLabelGap + e.labelSize.x;
            if (r > rightEdge)
            {
                rightEdge = r;
            }
        }
    }
    float targetWidth = anyVisible ? (rightEdge - kAnchorX + kPanelPadX * 2.0f) : 0.0f;
    float targetAlpha = anyVisible ? kPanelAlpha : 0.0f;
    const float panelT = std::clamp(kPanelLerp * dt, 0.0f, 1.0f);
    panelWidth_ += (targetWidth - panelWidth_) * panelT;
    panelAlpha_ += (targetAlpha - panelAlpha_) * panelT;
}

void InputDisplayUI::BindAndDraw(Sprite *pSprite, const std::string &path,
                                 Vector2 size, Vector2 pos, float alpha)
{
    if (!pSprite)
    {
        return;
    }
    pSprite->SetTexturePath(path);
    // テクスチャ切替に伴い、UVを新テクスチャ全体に合わせ直す
    const DirectX::TexMetadata &meta = TextureManager::GetInstance()->GetMetaData(path);
    pSprite->SetTexLeftTop({0.0f, 0.0f});
    pSprite->SetTexSize({static_cast<float>(meta.width), static_cast<float>(meta.height)});
    pSprite->SetSize(size);
    pSprite->SetPosition(pos);
    pSprite->SetAlpha(alpha);
    pSprite->Draw();
}

void InputDisplayUI::Draw()
{
    // 背景パネル（エントリの後ろに1枚。エントリより先に描く）
    if (bgPanel_ && panelAlpha_ > 0.01f && panelWidth_ > 1.0f)
    {
        Vector2 panelPos = {kAnchorX - kPanelPadX, kBaseY - kPanelPadTop};
        Vector2 panelSize = {panelWidth_, kGlyphSize + kPanelPadTop + kPanelPadBottom};
        bgPanel_->SetPosition(panelPos);
        bgPanel_->SetSize(panelSize);
        bgPanel_->SetAlpha(panelAlpha_);
        bgPanel_->Draw();
    }

    for (size_t i = 0; i < history_.size() && i < glyphPool_.size(); ++i)
    {
        HistoryEntry &e = history_[i];
        if (e.alpha < 0.01f)
        {
            continue;
        }

        // グリフ（ボタン/キー）
        BindAndDraw(glyphPool_[i].get(), e.glyphPath, e.glyphSize, e.pos, e.alpha);

        // 操作名（グリフの右、縦中央そろえ）
        Vector2 lpos = {e.pos.x + e.glyphSize.x + kGlyphLabelGap,
                        e.pos.y + (e.glyphSize.y - e.labelSize.y) * 0.5f};
        BindAndDraw(labelPool_[i].get(), e.labelPath, e.labelSize, lpos, e.alpha);

        // 矢印（このエントリより1つ古い(slot+1)が存在すれば間に描く）
        bool hasOlder = false;
        float olderAlpha = 0.0f;
        for (const HistoryEntry &o : history_)
        {
            if (o.slot == e.slot + 1)
            {
                hasOlder = true;
                olderAlpha = o.alpha;
                break;
            }
        }
        if (hasOlder)
        {
            auto arrowIt = glyphs_.find("arrow");
            if (arrowIt != glyphs_.end())
            {
                Vector2 asz = arrowIt->second.size;
                Vector2 apos = {e.pos.x + kArrowInSlotX,
                                kBaseY + (kGlyphSize - asz.y) * 0.5f};
                BindAndDraw(arrowPool_[i].get(), arrowIt->second.path, asz, apos,
                            std::min(e.alpha, olderAlpha));
            }
        }
    }
}

void InputDisplayUI::Finalize()
{
    history_.clear();
    glyphs_.clear();
    glyphPool_.clear();
    labelPool_.clear();
    arrowPool_.clear();
    bgPanel_.reset();
    pPlayer_ = nullptr;
    pGamePad_ = nullptr;
}
