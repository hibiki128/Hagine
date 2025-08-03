#include "PlayerUI.h"

void PlayerUI::Init(Player *player) {
    player_ = player;
    // HPバーの初期化
    hpBar_ = std::make_unique<Sprite>();
    hpBar_->Initialize("debug/white1x1.png", hpBarPosition_, {0.1f, 0.2f, 1.0f, 1.0f});
    // バーフレームの初期化
    barFrame_ = std::make_unique<Sprite>();
    barFrame_->Initialize("debug/white1x1.png", barFramePosition_, {0.3f, 0.3f, 0.3f, 0.6f});
    // エネルギーバーの描画
    energyBar_ = std::make_unique<Sprite>();
    energyBar_->Initialize("debug/white1x1.png", energyBarPosition_, {1.0f, 1.0f, 0.0f, 1.0f});
    // エネルギーバーフレームの初期化
    energyBarFrame_ = std::make_unique<Sprite>();
    energyBarFrame_->Initialize("debug/white1x1.png", energyBarFramePosition_, {0.3f, 0.3f, 0.3f, 0.6f});
    // プレイヤーアイコンの初期化
    playerIcon_ = std::make_unique<Sprite>();
    playerIcon_->Initialize("UI/playerIcon.png", playerIconPosition_);
}


void PlayerUI::Update() {
    if (player_) {
        hpBar_->SetPosition(hpBarPosition_);
        hpBar_->SetSize(hpBarSize_);
        barFrame_->SetPosition(barFramePosition_);
        barFrame_->SetSize(barSize_);
        energyBar_->SetPosition(energyBarPosition_);
        energyBar_->SetSize(energyBarSize_);
        energyBarFrame_->SetPosition(energyBarFramePosition_);
        energyBarFrame_->SetSize(energyBarFrameSize_);
        playerIcon_->SetPosition(playerIconPosition_);
        playerIcon_->SetSize(iconSize_);
    }
}

void PlayerUI::Draw() {
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

void PlayerUI::Debug() {
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
}