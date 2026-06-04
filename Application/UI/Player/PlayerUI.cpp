#include "PlayerUI.h"
#include "SpriteCommon.h"

using namespace Hagine;
void PlayerUI::Init(Player *player) {
    player_ = player;
    // HPバーの初期化
    hpBar_ = std::make_unique<Sprite>();
    hpBar_->Initialize("debug/white1x1.png", hpBarPosition_, {kHPBarColorR, kHPBarColorG, kHPBarColorB, kHPBarColorA});
    // バーフレームの初期化
    barFrame_ = std::make_unique<Sprite>();
    barFrame_->Initialize("debug/white1x1.png", barFramePosition_, {kFrameColorR, kFrameColorG, kFrameColorB, kFrameColorA});
    // エネルギーバーの描画
    energyBar_ = std::make_unique<Sprite>();
    energyBar_->Initialize("debug/white1x1.png", energyBarPosition_, {kEnergyBarColorR, kEnergyBarColorG, kEnergyBarColorB, kEnergyBarColorA});
    // エネルギーバーフレームの初期化
    energyBarFrame_ = std::make_unique<Sprite>();
    energyBarFrame_->Initialize("debug/white1x1.png", energyBarFramePosition_, {kFrameColorR, kFrameColorG, kFrameColorB, kFrameColorA});
    // プレイヤーアイコンの初期化
    playerIcon_ = std::make_unique<Sprite>();
    playerIcon_->Initialize("UI/playerIcon.png", playerIconPosition_);
}

void PlayerUI::Update() {
    if (player_) {

        float hpRatio = static_cast<float>(player_->GetHP()) / static_cast<float>(player_->GetMaxHP());
        float energyRatio = static_cast<float>(player_->GetEnergy()) / static_cast<float>(player_->GetMaxEnergy());

        Vector2 currentHPBarSize = {hpBarSize_.x * hpRatio, hpBarSize_.y};
        Vector2 currentEnergyBarSize = {energyBarSize_.x * energyRatio, energyBarSize_.y};

        hpBar_->SetPosition(hpBarPosition_);
        hpBar_->SetSize(currentHPBarSize);
        barFrame_->SetPosition(barFramePosition_);
        barFrame_->SetSize(barSize_);
        energyBar_->SetPosition(energyBarPosition_);
        energyBar_->SetSize(currentEnergyBarSize);
        energyBarFrame_->SetPosition(energyBarFramePosition_);
        energyBarFrame_->SetSize(energyBarFrameSize_);
        playerIcon_->SetPosition(playerIconPosition_);
        playerIcon_->SetSize(iconSize_);
    }
}

void PlayerUI::Draw() {
    SpriteCommon::GetInstance()->DrawCommonSetting();
    if (player_) {
        // バーフレームの描画
        if (barFrame_) {
            barFrame_->Draw();
        }
        // エネルギーバーフレームの描画
        if (energyBarFrame_) {
            energyBarFrame_->Draw();
        }
        // プレイヤーアイコンの描画
        if (playerIcon_) {
            playerIcon_->Draw();
        }
        // エネルギーバーの描画
        if (energyBar_) {
            energyBar_->Draw();
        }
        // HPバーの描画
        if (hpBar_) {
            hpBar_->Draw();
        }
    }
}

void PlayerUI::SetFadeAlpha(float alpha) {
    if (hpBar_) hpBar_->SetAlpha(kHPBarColorA * alpha);
    if (barFrame_) barFrame_->SetAlpha(kFrameColorA * alpha);
    if (energyBar_) energyBar_->SetAlpha(kEnergyBarColorA * alpha);
    if (energyBarFrame_) energyBarFrame_->SetAlpha(kFrameColorA * alpha);
    if (playerIcon_) playerIcon_->SetAlpha(alpha);
}

void PlayerUI::Debug() {
#ifdef USE_IMGUI
    if (ImGui::CollapsingHeader("プレイヤーUI")) {
        if (ImGui::TreeNode("HPバー")) {
            ImGui::DragFloat2("位置", &hpBarPosition_.x, 0.1f);
            ImGui::DragFloat2("サイズ", &hpBarSize_.x, 0.1f);
            ImGui::TreePop();
        }
        if (ImGui::TreeNode("プレイヤーアイコン")) {
            ImGui::DragFloat2("位置", &playerIconPosition_.x, 0.1f);
            ImGui::DragFloat2("サイズ", &iconSize_.x, 0.1f);
            ImGui::TreePop();
        }
        if (ImGui::TreeNode("エネルギーバー")) {
            ImGui::DragFloat2("位置", &energyBarPosition_.x, 0.1f);
            ImGui::DragFloat2("サイズ", &energyBarSize_.x, 0.1f);
            ImGui::TreePop();
        }
        if (ImGui::TreeNode("バーのフレーム")) {
            ImGui::DragFloat2("位置", &barFramePosition_.x, 0.1f);
            ImGui::DragFloat2("サイズ", &barSize_.x, 0.1f);
            ImGui::TreePop();
        }
        if (ImGui::TreeNode("エネルギーバーのフレーム")) {
            ImGui::DragFloat2("位置", &energyBarFramePosition_.x, 0.1f);
            ImGui::DragFloat2("サイズ", &energyBarFrameSize_.x, 0.1f);
            ImGui::TreePop();
        }
    }
#endif // USE_IMGUI
}