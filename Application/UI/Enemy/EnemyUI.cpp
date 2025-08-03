#include "EnemyUI.h"

void EnemyUI::Init(Enemy *enemy) {
    enemy_ = enemy;
    // HPバーの初期化
    hpBar_ = std::make_unique<Sprite>();
    hpBar_->Initialize("debug/white1x1.png", hpBarPosition_, {1.0f, 0.2f, 0.1f, 1.0f}, {1.0f, 0.0f});
    // バーフレームの初期化
    barFrame_ = std::make_unique<Sprite>();
    barFrame_->Initialize("debug/white1x1.png", barFramePosition_, {0.3f, 0.3f, 0.3f, 0.6f});
    // エネルギーバーの描画
    energyBar_ = std::make_unique<Sprite>();
    energyBar_->Initialize("debug/white1x1.png", energyBarPosition_, {1.0f, 0.5f, 0.0f, 1.0f});
    // エネルギーバーフレームの初期化
    energyBarFrame_ = std::make_unique<Sprite>();
    energyBarFrame_->Initialize("debug/white1x1.png", energyBarFramePosition_, {0.3f, 0.3f, 0.3f, 0.6f});
    // エネミーアイコンの初期化
    enemyIcon_ = std::make_unique<Sprite>();
    enemyIcon_->Initialize("UI/enemyIcon.png", enemyIconPosition_);
}

void EnemyUI::Update() {
    if (enemy_) {
        // HP割合を計算
        float hpRatio = static_cast<float>(enemy_->GetHP()) / static_cast<float>(enemy_->GetMaxHP());

        // HPバーのサイズを割合に応じて調整
        Vector2 currentHPBarSize = {hpBarSize_.x * hpRatio, hpBarSize_.y};

        hpBar_->SetPosition(hpBarPosition_);
        hpBar_->SetSize(currentHPBarSize);
        barFrame_->SetPosition(barFramePosition_);
        barFrame_->SetSize(barSize_);
        energyBar_->SetPosition(energyBarPosition_);
        energyBar_->SetSize(energyBarSize_);
        energyBarFrame_->SetPosition(energyBarFramePosition_);
        energyBarFrame_->SetSize(energyBarFrameSize_);
        enemyIcon_->SetPosition(enemyIconPosition_);
        enemyIcon_->SetSize(iconSize_);
    }
}

void EnemyUI::Draw() {
    if (enemy_) {
        // バーフレームの描画
        if (barFrame_) {
            barFrame_->Draw();
        }
        // エネルギーバーフレームの描画
        if (energyBarFrame_) {
            energyBarFrame_->Draw();
        }
        // エネミーアイコンの描画
        if (enemyIcon_) {
            enemyIcon_->Draw();
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

void EnemyUI::Debug() {
    if (ImGui::CollapsingHeader("エネミーUI")) {
        if (ImGui::TreeNode("HPバー")) {
            ImGui::DragFloat2("位置", &hpBarPosition_.x, 0.1f);
            ImGui::DragFloat2("サイズ", &hpBarSize_.x, 0.1f);
            ImGui::TreePop();
        }
        if (ImGui::TreeNode("エネミーアイコン")) {
            ImGui::DragFloat2("位置", &enemyIconPosition_.x, 0.1f);
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